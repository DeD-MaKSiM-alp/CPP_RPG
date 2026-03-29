// =============================================================================
// battle_io.cpp — реализация консольного интерфейса боя.
// Зависимости: iostream, game_types; логика боя здесь не пересчитывается.
// =============================================================================

#include "battle_io.hpp"

#include <iostream>
#include <string>

#if defined(_WIN32) && __has_include(<windows.h>)
#include <windows.h>
#define BATTLE_IO_HAS_WINDOWS_API 1
#endif

namespace battle_io {

void setup_console_utf8() {
#ifdef BATTLE_IO_HAS_WINDOWS_API
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif
}

const char* armor_state_label(game::ArmorState s) {
  switch (s) {
    case game::ArmorState::Intact:
      return "броня цела";
    case game::ArmorState::Damaged:
      return "броня повреждена";
    case game::ArmorState::Broken:
      return "броня сломана";
  }
  return "?";
}

namespace {

void print_unit_line(const game::Unit& u) {
  std::cout << "  " << u.name << " | HP: " << u.hp << " | Lust: " << u.lust << " | "
            << armor_state_label(u.armor);
  if (u.guarding) {
    std::cout << " | стойка защиты";
  }
  std::cout << "\n";
}

void print_gg_line(const game::GgHeroine& gg) {
  std::cout << "  " << gg.name << " | " << armor_state_label(gg.armor);
  if (gg.guarding) {
    std::cout << " | защита союзника (следующий удар врага по союзнику ослаблен)";
  }
  std::cout << "\n";
}

}  // namespace

void print_game_intro(int lust_redirect_threshold) {
  std::cout << "=== Пошаговый бой: ГГЖ (вы), союзник (авто), враг ===\n";
  std::cout << "Управление ГГЖ: 0 — выход, 1 — атака по врагу, 2 — защита союзника "
               "(ослабляет удар врага по союзнику в этом раунде), 3 — ждать\n";
  std::cout << "Союзник каждый раунд атакует сам; при Lust >= " << lust_redirect_threshold
            << " — срыв на броню ГГЖ.\n\n";
}

void print_round_banner(int round) {
  std::cout << "────────── Раунд " << round << " ──────────\n";
}

void print_round_state_preamble() {
  std::cout << "Состояние в начале раунда:\n";
}

void print_battle_header(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy) {
  print_gg_line(gg);
  print_unit_line(ally);
  print_unit_line(enemy);
}

void print_core_combat_snapshot(const game::GgHeroine& gg, const game::Unit& ally,
                                const game::Unit& enemy) {
  std::cout << "  HP врага («" << enemy.name << "»): " << enemy.hp << "\n";
  std::cout << "  HP союзника («" << ally.name << "»): " << ally.hp << "\n";
  std::cout << "  Lust союзника: " << ally.lust << " (порог срыва: " << ally.lust_redirect_threshold
            << ")\n";
  std::cout << "  Броня ГГЖ («" << gg.name << "»): " << armor_state_label(gg.armor) << "\n";
}

void print_state_after_partial_turn(const char* turn_name, const game::GgHeroine& gg,
                                    const game::Unit& ally, const game::Unit& enemy,
                                    const std::string& what_happened) {
  std::cout << "\n--- Состояние после хода " << turn_name << " ---\n";
  print_core_combat_snapshot(gg, ally, enemy);
  std::cout << "  Что произошло на этом ходу: " << what_happened << "\n";
}

void print_round_summary(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy,
                         const std::string& last_events) {
  std::cout << "\n--- Сводка после раунда (ГГЖ → союзник → враг) ---\n";
  print_core_combat_snapshot(gg, ally, enemy);
  std::cout << "  Что было за раунд: " << last_events << "\n";
}

void print_lust_change(const std::string& who, int before, int after,
                       const std::vector<std::string>& reasons) {
  std::cout << "  Lust («" << who << "»): " << before << " → " << after;
  if (after > before) {
    std::cout << " (+" << (after - before) << ")";
  }
  std::cout << "\n";
  if (!reasons.empty()) {
    for (const std::string& r : reasons) {
      std::cout << "    • " << r << "\n";
    }
  }
}

void print_no_lust_change_note() {
  std::cout << "    (изменений Lust нет при текущих частях и действии)\n";
}

void print_gg_turn_header(const std::string& gg_name) {
  std::cout << ">>> Ход ГГЖ: «" << gg_name << "»\n";
}

void print_gg_menu_prompt() {
  std::cout << "  Действие ГГЖ? [0 выход] [1 атака] [2 защита союзника] [3 ждать]: ";
  std::cout.flush();
}

void print_gg_menu_invalid() {
  std::cout << "  (неверный ввод — введите 0, 1, 2 или 3 и Enter)\n";
}

void print_gg_exit_message() {
  std::cout << "\n*** Выход: игра завершена по выбору игрока (или ввод недоступен). ***\n";
}

void print_gg_action_attack(const std::string& enemy_name, int dmg) {
  std::cout << "  ГГЖ атакует врага: урон по «" << enemy_name << "»: " << dmg << "\n";
}

void print_gg_action_guard() {
  std::cout << "  ГГЖ прикрывает союзника: удар врага по союзнику в этом раунде будет слабее.\n";
}

void print_gg_action_wait() {
  std::cout << "  ГГЖ ждёт.\n";
}

void print_allied_turn_header(const std::string& ally_name) {
  std::cout << "\n>>> Ход союзника (авто): «" << ally_name << "»\n";
}

void print_allied_turn_hint() {
  std::cout << "  Союзник по умолчанию атакует; цель — по Lust и порогу (не по агр./контр.).\n";
}

void print_allied_strike_result(int ally_lust, int threshold, const game::AlliedStrikeResolution& res,
                                const game::Unit& enemy, const game::GgHeroine& gg) {
  using game::AlliedStrikeKind;

  if (res.kind == AlliedStrikeKind::DamagedEnemy) {
    std::cout << "  Союзник атакует врага: урон по «" << enemy.name << "»: " << res.damage_to_enemy
              << "\n";
    return;
  }

  std::cout << "  !!! Срыв: Lust союзника (" << ally_lust << ") >= порога срыва (" << threshold
            << ").\n";
  std::cout << "  Союзник сорвался и бьёт по броне ГГЖ («" << gg.name << "»), а не по врагу.\n";

  if (res.kind == AlliedStrikeKind::StruckBrokenGgArmor) {
    std::cout << "  Удар приходится по уже сломанной броне: дополнительного эффекта нет.\n";
    return;
  }

  if (res.kind == AlliedStrikeKind::DegradedGgArmor) {
    std::cout << "  Броня ГГЖ: «" << armor_state_label(res.gg_armor_before_strike) << "» → «"
              << armor_state_label(gg.armor) << "»\n";
  }
}

void print_enemy_turn_header(const std::string& enemy_name) {
  std::cout << "\n>>> Ход врага: «" << enemy_name << "»\n";
}

void print_enemy_attack_ally(int dmg, bool gg_was_defending) {
  std::cout << "  Враг (агрессия > контроль): атака по союзнику, урон: " << dmg;
  if (gg_was_defending) {
    std::cout << " (учтена защита ГГЖ)";
  }
  std::cout << "\n";
}

void print_enemy_wait() {
  std::cout << "  Враг ждёт (агрессия не выше контроля).\n";
}

void print_victory() {
  std::cout << "\n*** Победа: враг уничтожен. ***\n";
}

void print_defeat_ally() {
  std::cout << "\n*** Поражение: союзник погиб. ***\n";
}

int read_gg_menu_choice() {
  std::string line;
  if (!std::getline(std::cin, line)) {
    return 0;
  }
  if (line.empty()) {
    return -1;
  }
  const char c = line[0];
  if (c == '0') {
    return 0;
  }
  if (c == '1' || c == '2' || c == '3') {
    return c - '0';
  }
  return -1;
}

}  // namespace battle_io
