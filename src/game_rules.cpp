// =============================================================================
// game_rules.cpp — реализация правил боя без консоли.
// Все суммы по частям и расчёты сосредоточены здесь, чтобы main и battle_io
// не дублировали формулы.
// =============================================================================

#include "game_rules.hpp"

namespace game {

namespace {

// -----------------------------------------------------------------------------
// Суммы по частям юнита (вспомогательные; снаружи не видны — нужны только правилам).
// -----------------------------------------------------------------------------

int total_aggression(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.aggression_bonus;
  }
  return s;
}

int total_control(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.control_bonus;
  }
  return s;
}

int sum_passive(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_passive_per_turn;
  }
  return s;
}

int sum_on_aggressive_action(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_on_aggressive_action;
  }
  return s;
}

int sum_armor_damaged(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_extra_armor_damaged;
  }
  return s;
}

int sum_armor_broken(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_extra_armor_broken;
  }
  return s;
}

int sum_battle_scaling(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_per_battle_intensity;
  }
  return s;
}

}  // namespace

// -----------------------------------------------------------------------------
// allied_attack_redirects_to_gg_armor
// Порог включительно: lust == threshold уже срыв.
// -----------------------------------------------------------------------------
bool allied_attack_redirects_to_gg_armor(int ally_lust, int lust_redirect_threshold) {
  return ally_lust >= lust_redirect_threshold;
}

// -----------------------------------------------------------------------------
// apply_lust_for_turn
// Меняет u.lust; reasons очищается и заполняется строками для отображения игроку.
// -----------------------------------------------------------------------------
int apply_lust_for_turn(Unit& u, int battle_intensity, bool performed_attack,
                        std::vector<std::string>& reasons) {
  reasons.clear();
  int gain = 0;

  const int passive = sum_passive(u);
  if (passive != 0) {
    gain += passive;
    reasons.push_back("пассивный рост за ход (части): +" + std::to_string(passive));
  }

  if (performed_attack) {
    const int on_act = sum_on_aggressive_action(u);
    if (on_act != 0) {
      gain += on_act;
      reasons.push_back("агрессивное действие (атака): +" + std::to_string(on_act));
    }
  }

  if (battle_intensity > 0) {
    const int scale = sum_battle_scaling(u);
    const int from_battle = scale * battle_intensity;
    if (from_battle != 0) {
      gain += from_battle;
      reasons.push_back("напряжение боя (уровень " + std::to_string(battle_intensity) +
                        "): +" + std::to_string(from_battle));
    }
  }

  switch (u.armor) {
    case ArmorState::Intact:
      break;
    case ArmorState::Damaged: {
      const int d = sum_armor_damaged(u);
      if (d != 0) {
        gain += d;
        reasons.push_back("броня повреждена: +" + std::to_string(d));
      }
      break;
    }
    case ArmorState::Broken: {
      const int b = sum_armor_broken(u);
      if (b != 0) {
        gain += b;
        reasons.push_back("броня сломана: +" + std::to_string(b));
      }
      break;
    }
  }

  u.lust += gain;
  return gain;
}

// -----------------------------------------------------------------------------
// degrade_armor_on_hit — только для брони юнита Unit (не ГГЖ).
// -----------------------------------------------------------------------------
void degrade_armor_on_hit(Unit& u) {
  if (u.armor == ArmorState::Intact) {
    u.armor = ArmorState::Damaged;
  } else if (u.armor == ArmorState::Damaged) {
    u.armor = ArmorState::Broken;
  }
}

// -----------------------------------------------------------------------------
// downgrade_gg_armor_one_tier — броня ГГЖ при срыве союзника.
// -----------------------------------------------------------------------------
bool downgrade_gg_armor_one_tier(GgHeroine& gg) {
  if (gg.armor == ArmorState::Intact) {
    gg.armor = ArmorState::Damaged;
    return true;
  }
  if (gg.armor == ArmorState::Damaged) {
    gg.armor = ArmorState::Broken;
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// deal_damage — урон по юниту; учитывается self-guarding цели.
// -----------------------------------------------------------------------------
int deal_damage(Unit& target, int raw_damage) {
  int dmg = raw_damage;
  if (target.guarding) {
    dmg = (dmg + 1) / 2;
    target.guarding = false;
  }
  if (dmg > 0) {
    target.hp -= dmg;
    degrade_armor_on_hit(target);
  }
  return dmg;
}

// -----------------------------------------------------------------------------
// deal_damage_from_enemy_to_ally — защита ГГЖ, не поле ally.guarding.
// -----------------------------------------------------------------------------
int deal_damage_from_enemy_to_ally(Unit& ally, int raw_damage, bool gg_is_defending) {
  int dmg = raw_damage;
  if (gg_is_defending) {
    dmg = (dmg + 1) / 2;
  }
  if (dmg > 0) {
    ally.hp -= dmg;
    degrade_armor_on_hit(ally);
  }
  return dmg;
}

// -----------------------------------------------------------------------------
// enemy_decides_to_attack
// -----------------------------------------------------------------------------
bool enemy_decides_to_attack(const Unit& enemy) {
  return total_aggression(enemy) > total_control(enemy);
}

// -----------------------------------------------------------------------------
// resolve_allied_strike — единая точка применения автоматической атаки союзника.
// Lust сравнивается до прироста за этот полуход (вызывающий код затем вызывает apply_lust).
// -----------------------------------------------------------------------------
AlliedStrikeResolution resolve_allied_strike(Unit& ally, GgHeroine& gg, Unit& enemy,
                                             int attack_damage) {
  AlliedStrikeResolution r;

  if (!allied_attack_redirects_to_gg_armor(ally.lust, ally.lust_redirect_threshold)) {
    r.kind = AlliedStrikeKind::DamagedEnemy;
    r.damage_to_enemy = deal_damage(enemy, attack_damage);
    return r;
  }

  if (gg.armor == ArmorState::Broken) {
    r.kind = AlliedStrikeKind::StruckBrokenGgArmor;
    return r;
  }

  r.gg_armor_before_strike = gg.armor;
  const bool changed = downgrade_gg_armor_one_tier(gg);
  if (changed) {
    r.kind = AlliedStrikeKind::DegradedGgArmor;
  } else {
    r.kind = AlliedStrikeKind::StruckBrokenGgArmor;
  }
  return r;
}

}  // namespace game
