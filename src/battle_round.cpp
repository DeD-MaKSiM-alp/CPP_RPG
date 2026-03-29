// =============================================================================
// battle_round.cpp — реализация одного раунда и демо-инициализации.
// =============================================================================

#include "battle_round.hpp"

#include "battle_io.hpp"
#include "game_rules.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace battle_round {

namespace {

// -----------------------------------------------------------------------------
// append_allied_strike_to_log
// Зачем: единое место склейки фразы про союзника в round_log по тем же условиям,
//        что и раньше в main (редирект / сломанная броня / удар по броне).
// Принимает: will_redirect — результат allied_attack_redirects_to_gg_armor до удара;
//            gg_armor_already_broken — броня ГГЖ была Broken до resolve_allied_strike.
// Меняет: round_log дописывается в конец.
// -----------------------------------------------------------------------------
void append_allied_strike_to_log(std::string& round_log, bool will_redirect,
                                 bool gg_armor_already_broken) {
  if (!will_redirect) {
    round_log += "союзник ударил по врагу; ";
  } else if (gg_armor_already_broken) {
    round_log += "срыв: удар по уже сломанной броне ГГЖ; ";
  } else {
    round_log += "срыв: удар по броне ГГЖ; ";
  }
}

}  // namespace

// -----------------------------------------------------------------------------
// make_demo_battle
// Зачем: убрать из main длинную инициализацию юнитов; поведение и числа те же.
// -----------------------------------------------------------------------------
DemoBattleState make_demo_battle() {
  DemoBattleState s;

  s.gg.name = "ГГЖ — Лира";
  s.gg.armor = game::ArmorState::Intact;
  s.gg.guarding = false;

  s.ally.name = "Союзник — Кейн";
  s.ally.hp = 80;
  s.ally.lust = 55;
  s.ally.armor = game::ArmorState::Intact;
  s.ally.lust_redirect_threshold = 70;
  s.ally.parts.push_back({"Железная голова", 15, 5, 1, 2, 1, 4, 1});
  s.ally.parts.push_back({"Искажённая рука", 25, 0, 2, 4, 3, 8, 2});
  s.ally.parts.push_back({"Сдерживающий пояс", 0, 30, 0, 0, 0, 0, 0});

  s.enemy.name = "Враг — Сломленный страж";
  s.enemy.hp = 55;
  s.enemy.lust = 5;
  s.enemy.armor = game::ArmorState::Intact;
  s.enemy.lust_redirect_threshold = 0;
  s.enemy.parts.push_back({"Костяной кулак", 22, 4, 1, 3, 1, 3, 1});
  s.enemy.parts.push_back({"Ржавый нагрудник", 8, 2, 1, 2, 0, 2, 0});

  return s;
}

// -----------------------------------------------------------------------------
// run_single_round
//
// Этапы (одна итерация внешнего цикла в main):
//   1) Печать баннера раунда и снимка «начало раунда».
//   2) Полуход ГГЖ: меню до валидного ввода; при 0 — выход из боя (результат PlayerExit).
//      Иначе атака / защита / ожидание; снимок после ГГЖ; при hp врага <= 0 — Victory.
//   3) Полуход союзника: resolve_allied_strike, Lust союзника, снимок; при hp врага <= 0 — Victory.
//   4) Полуход врага: AI, урон по союзнику, Lust врага, снимок; сброс gg.guarding;
//      при hp союзника <= 0 — DefeatAlly.
//   5) Иначе — дописать «раунд завершён», сводка раунда, перевод строки → ContinueBattle.
//
// Завершение боя определяется здесь только в смысле «этот раунд уже напечатал итог
// (победа/поражение/выход)»; главный while в main останавливается по kind != ContinueBattle.
//
// round_log накапливается только внутри этой функции — main больше не склеивает фразы вручную.
// -----------------------------------------------------------------------------
SingleRoundResult run_single_round(game::GgHeroine& gg, game::Unit& ally, game::Unit& enemy,
                                   int battle_intensity, int attack_damage, int round_index) {
  SingleRoundResult out;

  battle_io::print_round_banner(round_index);
  battle_io::print_round_state_preamble();
  battle_io::print_battle_header(gg, ally, enemy);
  std::cout << "\n";

  // Накопление краткого текстового лога для строки «что было за раунд» (как раньше в main).
  std::string round_log;

  // ---------- Полуход 1: ГГЖ (игрок) ----------
  battle_io::print_gg_turn_header(gg.name);
  int choice = -1;
  while (choice == -1) {
    battle_io::print_gg_menu_prompt();
    choice = battle_io::read_gg_menu_choice();
    if (choice == -1) {
      battle_io::print_gg_menu_invalid();
    }
  }

  if (choice == 0) {
    battle_io::print_gg_exit_message();
    out.kind = RoundOutcomeKind::PlayerExit;
    return out;
  }

  gg.guarding = false;

  if (choice == 1) {
    const int dmg = game::deal_damage(enemy, attack_damage);
    battle_io::print_gg_action_attack(enemy.name, dmg);
    round_log += "ГГЖ атаковала врага; ";
  } else if (choice == 2) {
    gg.guarding = true;
    battle_io::print_gg_action_guard();
    round_log += "ГГЖ защищала союзника; ";
  } else {
    battle_io::print_gg_action_wait();
    round_log += "ГГЖ ждала; ";
  }

  battle_io::print_state_after_partial_turn("ГГЖ", gg, ally, enemy, round_log);

  if (enemy.hp <= 0) {
    round_log += "враг повержен.";
    battle_io::print_round_summary(gg, ally, enemy, round_log);
    battle_io::print_victory();
    out.kind = RoundOutcomeKind::Victory;
    return out;
  }

  // ---------- Полуход 2: союзник (авто) ----------
  battle_io::print_allied_turn_header(ally.name);
  battle_io::print_allied_turn_hint();

  const bool will_redirect =
      game::allied_attack_redirects_to_gg_armor(ally.lust, ally.lust_redirect_threshold);
  const bool gg_armor_already_broken = (gg.armor == game::ArmorState::Broken);

  const int ally_lust_before_strike = ally.lust;
  const game::AlliedStrikeResolution strike_res =
      game::resolve_allied_strike(ally, gg, enemy, attack_damage);
  battle_io::print_allied_strike_result(ally_lust_before_strike, ally.lust_redirect_threshold,
                                        strike_res, enemy, gg);

  append_allied_strike_to_log(round_log, will_redirect, gg_armor_already_broken);

  const bool ally_attacked = true;
  {
    const int lust_before = ally.lust;
    std::vector<std::string> reasons;
    const int gain = game::apply_lust_for_turn(ally, battle_intensity, ally_attacked, reasons);
    battle_io::print_lust_change(ally.name, lust_before, ally.lust, reasons);
    if (gain == 0 && reasons.empty()) {
      battle_io::print_no_lust_change_note();
    }
  }

  battle_io::print_state_after_partial_turn("союзника", gg, ally, enemy, round_log);

  if (enemy.hp <= 0) {
    round_log += "враг повержен.";
    battle_io::print_round_summary(gg, ally, enemy, round_log);
    battle_io::print_victory();
    out.kind = RoundOutcomeKind::Victory;
    return out;
  }

  // ---------- Полуход 3: враг ----------
  battle_io::print_enemy_turn_header(enemy.name);
  const bool enemy_attacks = game::enemy_decides_to_attack(enemy);
  bool enemy_did_attack = false;
  std::string enemy_turn_desc;

  const bool gg_defending_before_enemy = gg.guarding;
  if (enemy_attacks) {
    enemy_did_attack = true;
    const int dmg = game::deal_damage_from_enemy_to_ally(ally, attack_damage, gg.guarding);
    battle_io::print_enemy_attack_ally(dmg, gg_defending_before_enemy);
    round_log += "враг атаковал союзника; ";
    enemy_turn_desc =
        "враг атаковал союзника (урон может быть снижен защитой ГГЖ).";
  } else {
    battle_io::print_enemy_wait();
    round_log += "враг ждал; ";
    enemy_turn_desc = "враг ждал (AI: агрессия не выше контроля).";
  }

  {
    const int lust_before = enemy.lust;
    std::vector<std::string> reasons;
    const int gain = game::apply_lust_for_turn(enemy, battle_intensity, enemy_did_attack, reasons);
    battle_io::print_lust_change(enemy.name, lust_before, enemy.lust, reasons);
    if (gain == 0 && reasons.empty()) {
      battle_io::print_no_lust_change_note();
    }
  }

  battle_io::print_state_after_partial_turn("врага", gg, ally, enemy, enemy_turn_desc);

  gg.guarding = false;

  if (ally.hp <= 0) {
    round_log += "союзник погиб.";
    battle_io::print_round_summary(gg, ally, enemy, round_log);
    battle_io::print_defeat_ally();
    out.kind = RoundOutcomeKind::DefeatAlly;
    return out;
  }

  round_log += "раунд завершён.";
  battle_io::print_round_summary(gg, ally, enemy, round_log);
  std::cout << "\n";

  out.kind = RoundOutcomeKind::ContinueBattle;
  return out;
}

}  // namespace battle_round
