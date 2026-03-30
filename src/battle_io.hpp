#pragma once // этот файл включают из нескольких единиц компиляции — гвард от повторного разбора

// =============================================================================
// Файл: battle_io.hpp
// Назначение: объявления консольного ввода/вывода боя — текст, меню, отображение чисел.
// Правила игры здесь не пересчитываются: только показ состояния и чтение выбора игрока.
// =============================================================================

#include "game_types.hpp" // AlliedStrikeResolution, ArmorState, GgHeroine, Unit

#include <string>   // имена и строки сообщений
#include <vector>   // список причин изменения Lust для печати

namespace battle_io { // всё, что связано с консолью, в одном пространстве имён

/**
 * setup_console_utf8 — настраивает кодовые страницы консоли Windows на UTF-8 (если доступно API).
 * Зачем: кириллица и спецсимволы выводились бы иначе без смены CP.
 * Где используется: в начале main до любого вывода.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void setup_console_utf8();

/**
 * armor_state_label — возвращает короткую русскую подпись для состояния брони.
 * Зачем: не дублировать строки в каждом месте вывода.
 * Где используется: печать строк юнитов и героини, сводки боя.
 * Параметры: s — одно из значений ArmorState.
 * Возвращает: указатель на строковый литерал (статический текст программы).
 */
const char* armor_state_label(game::ArmorState s);

/**
 * print_game_intro — печатает заголовок игры и краткие правила в начале сессии.
 * Зачем: игрок видит управление и порог срыва Lust до первого раунда.
 * Где используется: main после make_demo_battle.
 * Параметры: lust_redirect_threshold — число для строки про срыв союзника.
 * Возвращает: ничего.
 */
void print_game_intro(int lust_redirect_threshold);

/**
 * print_round_banner — печатает визуальный разделитель с номером раунда.
 * Зачем: отделять раунды в длинной сессии.
 * Где используется: в начале run_single_round.
 * Параметры: round — номер раунда (1, 2, …).
 * Возвращает: ничего.
 */
void print_round_banner(int round);

/**
 * print_round_state_preamble — короткая строка перед списком участников.
 * Зачем: пояснить, что ниже — состояние на начало раунда.
 * Где используется: run_single_round после баннера.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_round_state_preamble();

/**
 * print_battle_header — выводит три строки: героиня, союзник, враг.
 * Зачем: сводка участников в начале раунда.
 * Где используется: run_single_round.
 * Параметры: gg, ally, enemy — текущие объекты состояния.
 * Возвращает: ничего.
 */
void print_battle_header(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy);

/**
 * print_state_after_partial_turn — снимок HP/Lust/брони после частичного хода и пояснение строкой.
 * Зачем: игрок видит последствия каждого полухода.
 * Где используется: после хода ГГЖ, союзника, врага.
 * Параметры: turn_name — кто ходил; gg, ally, enemy; what_happened — краткое описание события.
 * Возвращает: ничего.
 */
void print_state_after_partial_turn(const char* turn_name, const game::GgHeroine& gg,
                                    const game::Unit& ally, const game::Unit& enemy,
                                    const std::string& what_happened);

/**
 * print_round_summary — итоговая сводка после полного раунда с накопленной строкой событий.
 * Зачем: одна строка «что было за раунд» плюс числа.
 * Где используется: перед выходом из раунда или при победе/поражении.
 * Параметры: gg, ally, enemy; last_events — склеенное текстовое описание.
 * Возвращает: ничего.
 */
void print_round_summary(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy,
                         const std::string& last_events);

/**
 * print_lust_change — печатает изменение Lust и опционально список причин с новой строки.
 * Зачем: связать число с правилами из game_rules (строки уже собраны).
 * Где используется: после apply_lust_for_turn для союзника и врага.
 * Параметры: who — имя; before, after — числа; reasons — список пояснений.
 * Возвращает: ничего.
 */
void print_lust_change(const std::string& who, int before, int after,
                       const std::vector<std::string>& reasons);

/**
 * print_no_lust_change_note — пометка, что прироста Lust не было при пустых причинах.
 * Зачем: игрок не думает, что вывод «сломался».
 * Где используется: когда gain == 0 и reasons пусты после apply_lust_for_turn.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_no_lust_change_note();

/**
 * print_gg_turn_header — заголовок секции хода главной героини.
 * Зачем: визуально отделить меню игрока от автоматических ходов.
 * Где используется: перед циклом ввода меню в run_single_round.
 * Параметры: gg_name — имя героини для кавычек в тексте.
 * Возвращает: ничего.
 */
void print_gg_turn_header(const std::string& gg_name);

/**
 * print_gg_menu_prompt — строка-приглашение ввести цифру действия и сброс буфера вывода.
 * Зачем: игрок видит подсказку до блокировки на вводе.
 * Где используется: внутри цикла до valid choice.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_gg_menu_prompt();

/**
 * print_gg_menu_invalid — сообщение при неверной первой цифре в строке.
 * Зачем: повторный запрос без молчания.
 * Где используется: когда read_gg_menu_choice вернул -1.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_gg_menu_invalid();

/**
 * print_gg_exit_message — текст при выходе по пункту 0 или при недоступном вводе (EOF).
 * Зачем: явное завершение сценария для игрока.
 * Где используется: при choice == 0 после меню.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_gg_exit_message();

/**
 * print_gg_action_attack — сообщение об атаке героини по врагу с числом урона.
 * Зачем: связать действие с результатом deal_damage.
 * Где используется: ветка choice == 1 в run_single_round.
 * Параметры: enemy_name — имя цели; dmg — применённый урон.
 * Возвращает: ничего.
 */
void print_gg_action_attack(const std::string& enemy_name, int dmg);

/**
 * print_gg_action_guard — текст про стойку защиты союзника.
 * Зачем: объяснить, что изменится при ударе врага.
 * Где используется: ветка choice == 2.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_gg_action_guard();

/**
 * print_gg_action_wait — короткое сообщение о том, что героиня ждёт.
 * Зачем: ветка choice == 3 не молчит.
 * Где используется: в else последней ветки действий ГГЖ.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_gg_action_wait();

/**
 * print_allied_turn_header — заголовок автоматического хода союзника.
 * Зачем: отделить блок союзника от блока героини.
 * Где используется: перед resolve_allied_strike.
 * Параметры: ally_name — имя союзника.
 * Возвращает: ничего.
 */
void print_allied_turn_header(const std::string& ally_name);

/**
 * print_allied_turn_hint — напоминание, что цель удара определяется Lust и порогом.
 * Зачем: игрок понимает автоматику без чтения кода.
 * Где используется: сразу после заголовка союзника.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_allied_turn_hint();

/**
 * print_allied_strike_result — развёрнутый текст по результату resolve_allied_strike.
 * Зачем: визуализировать срыв, урон по врагу или по броне ГГЖ.
 * Где используется: после вычисления strike_res в run_single_round.
 * Параметры: ally_lust, threshold — числа для строки срыва; res — структура исхода; enemy, gg — для имён и брони.
 * Возвращает: ничего.
 */
void print_allied_strike_result(int ally_lust, int threshold,
                                const game::AlliedStrikeResolution& res, const game::Unit& enemy,
                                const game::GgHeroine& gg);

/**
 * print_enemy_turn_header — заголовок хода врага.
 * Зачем: структура раунда «три блока».
 * Где используется: перед AI врага.
 * Параметры: enemy_name — имя противника.
 * Возвращает: ничего.
 */
void print_enemy_turn_header(const std::string& enemy_name);

/**
 * print_enemy_attack_ally — текст атаки врага по союзнику с учётом защиты ГГЖ.
 * Зачем: показать урон и пометку про ослабление.
 * Где используется: если enemy_attacks истинно.
 * Параметры: dmg — применённый урон; gg_was_defending — была ли защита до удара.
 * Возвращает: ничего.
 */
void print_enemy_attack_ally(int dmg, bool gg_was_defending);

/**
 * print_enemy_wait — текст, если враг не атакует.
 * Зачем: ветка AI «ждать» не пустая.
 * Где используется: else от enemy_attacks.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_enemy_wait();

/**
 * print_victory — финальное сообщение о победе.
 * Зачем: явный исход при hp врага <= 0.
 * Где используется: ветки Victory в run_single_round.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_victory();

/**
 * print_defeat_ally — финальное сообщение о поражении (погиб союзник).
 * Зачем: явный исход при hp союзника <= 0 после врага.
 * Где используется: ветка DefeatAlly.
 * Параметры: нет.
 * Возвращает: ничего.
 */
void print_defeat_ally();

/**
 * read_gg_menu_choice — читает одну строку с stdin и возвращает код действия.
 * Зачем: отделить разбор ввода от игровой логики; первая значащая цифра — команда.
 * Где используется: цикл меню в run_single_round.
 * Параметры: нет (читает глобальный std::cin).
 * Возвращает: 0 — выход или ошибка чтения; 1–3 — действия; -1 — неверный ввод, повторить.
 */
int read_gg_menu_choice();

}  // namespace battle_io
