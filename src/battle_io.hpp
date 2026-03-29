#pragma once

// =============================================================================
// battle_io.hpp — консольный ввод/вывод боя.
// Здесь нет изменения правил игры: только текст, запросы и отображение состояния.
// =============================================================================

#include "game_types.hpp"

#include <string>
#include <vector>

namespace battle_io {

// Кодировка UTF-8 в консоли Windows (если доступно).
void setup_console_utf8();

// Человекочитаемая подпись брони для вывода.
const char* armor_state_label(game::ArmorState s);

// Приветствие и правила в начале сессии.
void print_game_intro(int lust_redirect_threshold);

// Разделитель и шапка раунда.
void print_round_banner(int round);
void print_round_state_preamble();

// Участники в начале раунда.
void print_battle_header(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy);

// Снимок и сводки.
void print_state_after_partial_turn(const char* turn_name, const game::GgHeroine& gg,
                                    const game::Unit& ally, const game::Unit& enemy,
                                    const std::string& what_happened);

void print_round_summary(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy,
                         const std::string& last_events);

void print_lust_change(const std::string& who, int before, int after,
                       const std::vector<std::string>& reasons);

void print_no_lust_change_note();

// Ход ГГЖ: запрос и реакции.
void print_gg_turn_header(const std::string& gg_name);
void print_gg_menu_prompt();
void print_gg_menu_invalid();
void print_gg_exit_message();

void print_gg_action_attack(const std::string& enemy_name, int dmg);
void print_gg_action_guard();
void print_gg_action_wait();

// Ход союзника (авто).
void print_allied_turn_header(const std::string& ally_name);
void print_allied_turn_hint();

// Результат resolve_allied_strike — текст как в прежнем main.
void print_allied_strike_result(int ally_lust, int threshold,
                                const game::AlliedStrikeResolution& res, const game::Unit& enemy,
                                const game::GgHeroine& gg);

// Ход врага.
void print_enemy_turn_header(const std::string& enemy_name);
void print_enemy_attack_ally(int dmg, bool gg_was_defending);
void print_enemy_wait();

// Исход боя.
void print_victory();
void print_defeat_ally();

// Чтение меню ГГЖ: 0 выход, 1–3 действия, -1 повтор.
int read_gg_menu_choice();

}  // namespace battle_io
