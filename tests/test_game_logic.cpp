// =============================================================================
// test_game_logic.cpp — проверки чистых правил без консольного сценария.
// Код возврата 0 — все проверки пройдены; иначе 1.
// =============================================================================

#include "game_rules.hpp"
#include "game_types.hpp"

#include <string>
#include <vector>

namespace {

int test_redirect_rule() {
  using game::allied_attack_redirects_to_gg_armor;
  if (allied_attack_redirects_to_gg_armor(69, 70)) {
    return 1;
  }
  if (!allied_attack_redirects_to_gg_armor(70, 70)) {
    return 1;
  }
  if (!allied_attack_redirects_to_gg_armor(100, 70)) {
    return 1;
  }
  return 0;
}

int test_gg_armor_transitions() {
  game::GgHeroine gg;
  gg.name = "Тест";
  gg.armor = game::ArmorState::Intact;
  if (!game::downgrade_gg_armor_one_tier(gg)) {
    return 1;
  }
  if (gg.armor != game::ArmorState::Damaged) {
    return 1;
  }
  if (!game::downgrade_gg_armor_one_tier(gg)) {
    return 1;
  }
  if (gg.armor != game::ArmorState::Broken) {
    return 1;
  }
  if (game::downgrade_gg_armor_one_tier(gg)) {
    return 1;
  }
  if (gg.armor != game::ArmorState::Broken) {
    return 1;
  }
  return 0;
}

int test_deal_damage_guard_halves() {
  game::Unit target;
  target.name = "Цель";
  target.hp = 100;
  target.guarding = true;
  const int applied = game::deal_damage(target, 12);
  if (applied != 6) {
    return 1;
  }
  if (target.hp != 94) {
    return 1;
  }
  if (target.guarding) {
    return 1;
  }
  return 0;
}

int test_deal_damage_from_enemy_with_gg_defense() {
  game::Unit ally;
  ally.name = "Союзник";
  ally.hp = 80;
  const int applied = game::deal_damage_from_enemy_to_ally(ally, 12, true);
  if (applied != 6) {
    return 1;
  }
  if (ally.hp != 74) {
    return 1;
  }
  return 0;
}

int test_resolve_allied_strike_enemy() {
  game::Unit ally;
  ally.lust = 50;
  ally.lust_redirect_threshold = 70;
  game::GgHeroine gg;
  game::Unit enemy;
  enemy.hp = 100;
  enemy.guarding = false;

  const game::AlliedStrikeResolution r = game::resolve_allied_strike(ally, gg, enemy, 12);
  if (r.kind != game::AlliedStrikeKind::DamagedEnemy) {
    return 1;
  }
  if (r.damage_to_enemy != 12) {
    return 1;
  }
  if (enemy.hp != 88) {
    return 1;
  }
  return 0;
}

int test_resolve_allied_strike_redirect_degrades_gg() {
  game::Unit ally;
  ally.lust = 70;
  ally.lust_redirect_threshold = 70;
  game::GgHeroine gg;
  gg.armor = game::ArmorState::Intact;
  game::Unit enemy;
  enemy.hp = 100;

  const game::AlliedStrikeResolution r = game::resolve_allied_strike(ally, gg, enemy, 12);
  if (r.kind != game::AlliedStrikeKind::DegradedGgArmor) {
    return 1;
  }
  if (enemy.hp != 100) {
    return 1;
  }
  if (gg.armor != game::ArmorState::Damaged) {
    return 1;
  }
  return 0;
}

int test_resolve_allied_strike_broken_armor_safe() {
  game::Unit ally;
  ally.lust = 80;
  ally.lust_redirect_threshold = 70;
  game::GgHeroine gg;
  gg.armor = game::ArmorState::Broken;
  game::Unit enemy;
  enemy.hp = 100;

  const game::AlliedStrikeResolution r = game::resolve_allied_strike(ally, gg, enemy, 12);
  if (r.kind != game::AlliedStrikeKind::StruckBrokenGgArmor) {
    return 1;
  }
  if (enemy.hp != 100) {
    return 1;
  }
  if (gg.armor != game::ArmorState::Broken) {
    return 1;
  }
  return 0;
}

int test_apply_lust_minimal_unit() {
  game::Unit u;
  u.lust = 10;
  std::vector<std::string> reasons;
  const int gain = game::apply_lust_for_turn(u, 1, false, reasons);
  if (gain != 0) {
    return 1;
  }
  if (u.lust != 10) {
    return 1;
  }
  return 0;
}

}  // namespace

int main() {
  if (test_redirect_rule() != 0) {
    return 1;
  }
  if (test_gg_armor_transitions() != 0) {
    return 1;
  }
  if (test_deal_damage_guard_halves() != 0) {
    return 1;
  }
  if (test_deal_damage_from_enemy_with_gg_defense() != 0) {
    return 1;
  }
  if (test_resolve_allied_strike_enemy() != 0) {
    return 1;
  }
  if (test_resolve_allied_strike_redirect_degrades_gg() != 0) {
    return 1;
  }
  if (test_resolve_allied_strike_broken_armor_safe() != 0) {
    return 1;
  }
  if (test_apply_lust_minimal_unit() != 0) {
    return 1;
  }
  return 0;
}
