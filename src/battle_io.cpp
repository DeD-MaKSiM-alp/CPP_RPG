// =============================================================================
// Файл: battle_io.cpp
// Назначение: реализация консольного интерфейса — печать состояния боя и чтение меню игрока.
// Зависимости: iostream, string; типы из game_types; логику боя не пересчитываем (только показ).
// =============================================================================

#include "battle_io.hpp" // объявления функций этого модуля

#include <iostream> // std::cin, std::cout, std::flush
#include <string>   // std::string, std::getline

#if defined(_WIN32) && __has_include(<windows.h>) // если Windows и заголовок windows.h доступен компилятору
#include <windows.h> // API SetConsoleCP / SetConsoleOutputCP
#define BATTLE_IO_HAS_WINDOWS_API 1 // макрос: ниже можно вызывать WinAPI
#endif // конец условной компиляции для Windows

namespace battle_io { // всё тело файла — реализации из battle_io.hpp

/**
 * setup_console_utf8 — переключает кодовые страницы консоли на UTF-8 на Windows.
 */
void setup_console_utf8() {
#ifdef BATTLE_IO_HAS_WINDOWS_API // этот блок компилируется только при успешном include windows.h
  SetConsoleOutputCP(CP_UTF8); // задаём кодовую страницу вывода консоли (UTF-8)
  SetConsoleCP(CP_UTF8);       // задаём кодовую страницу ввода консоли (UTF-8)
#endif // на не-Windows или без заголовка блок пустой — ничего не делаем
}

/**
 * armor_state_label — русская строка для enum ArmorState.
 */
const char* armor_state_label(game::ArmorState s) {
  switch (s) { // выбираем ветку по значению перечисления
    case game::ArmorState::Intact: // броня целая
      return "броня цела"; // указатель на статический литерал
    case game::ArmorState::Damaged: // броня повреждена
      return "броня повреждена"; // строка для вывода
    case game::ArmorState::Broken: // броня сломана
      return "броня сломана"; // последний осмысленный case
  }
  return "?"; // теоретический запас: если добавят значение enum и забудут case — не упасть молча (NOTE: для текущего enum недостижимо)
}

namespace { // внутренние функции файла — не экспортируются из battle_io

/**
 * print_unit_line — одна строка описания боевого юнита (союзник или враг).
 */
void print_unit_line(const game::Unit& u) {
  std::cout << "  " << u.name << " | HP: " << u.hp << " | Lust: " << u.lust << " | " // начинаем строку с отступа и полей
            << armor_state_label(u.armor); // дописываем текст брони через общую функцию
  if (u.guarding) { // если у юнита включена стойка защиты (редко для союзника в этой игре)
    std::cout << " | стойка защиты"; // поясняем пометку в той же строке
  }
  std::cout << "\n"; // перевод строки после описания юнита
}

/**
 * print_gg_line — одна строка описания главной героини (без HP/Lust как у юнита).
 */
void print_gg_line(const game::GgHeroine& gg) {
  std::cout << "  " << gg.name << " | " << armor_state_label(gg.armor); // имя и состояние брони ГГЖ
  if (gg.guarding) { // если игрок выбрал защиту союзника в этом раунде
    std::cout << " | защита союзника (следующий удар врага по союзнику ослаблен)"; // пояснение эффекта
  }
  std::cout << "\n"; // конец строки героини
}

}  // namespace // конец анонимного namespace

/**
 * print_game_intro — правила и заголовок в начале игры.
 */
void print_game_intro(int lust_redirect_threshold) {
  std::cout << "=== Пошаговый бой: ГГЖ (вы), союзник (авто), враг ===\n"; // первая строка заголовка
  std::cout << "Управление ГГЖ: 0 — выход, 1 — атака по врагу, 2 — защита союзника "
               "(ослабляет удар врага по союзнику в этом раунде), 3 — ждать\n"; // вторая строка: меню
  std::cout << "Союзник каждый раунд атакует сам; при Lust >= " << lust_redirect_threshold
            << " — срыв на броню ГГЖ.\n\n"; // третья строка: порог срыва и двойной перевод строки
}

/**
 * print_round_banner — разделитель с номером раунда.
 */
void print_round_banner(int round) {
  std::cout << "────────── Раунд " << round << " ──────────\n"; // декоративная строка с числом раунда
}

/**
 * print_round_state_preamble — подпись перед блоком «кто в бою».
 */
void print_round_state_preamble() {
  std::cout << "Состояние в начале раунда:\n"; // фиксированная фраза
}

/**
 * print_battle_header — три строки участников через внутренние помощники.
 */
void print_battle_header(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy) {
  print_gg_line(gg);     // сначала героиня
  print_unit_line(ally); // затем союзник
  print_unit_line(enemy); // затем враг
}

/**
 * print_core_combat_snapshot — числовой срез: враг, союзник, Lust, броня ГГЖ.
 */
void print_core_combat_snapshot(const game::GgHeroine& gg, const game::Unit& ally,
                                const game::Unit& enemy) {
  std::cout << "  HP врага («" << enemy.name << "»): " << enemy.hp << "\n"; // HP противника
  std::cout << "  HP союзника («" << ally.name << "»): " << ally.hp << "\n"; // HP союзника
  std::cout << "  Lust союзника: " << ally.lust << " (порог срыва: " << ally.lust_redirect_threshold
            << ")\n"; // Lust и порог в одной строке (разбита на два оператора <<)
  std::cout << "  Броня ГГЖ («" << gg.name << "»): " << armor_state_label(gg.armor) << "\n"; // броня героини текстом
}

/**
 * print_state_after_partial_turn — снимок после части раунда и пояснение события.
 */
void print_state_after_partial_turn(const char* turn_name, const game::GgHeroine& gg,
                                    const game::Unit& ally, const game::Unit& enemy,
                                    const std::string& what_happened) {
  std::cout << "\n--- Состояние после хода " << turn_name << " ---\n"; // пустая строка и заголовок с именем хода
  print_core_combat_snapshot(gg, ally, enemy); // повторяем числовой блок
  std::cout << "  Что произошло на этом ходу: " << what_happened << "\n"; // человекочитаемое описание
}

/**
 * print_round_summary — итог раунда с накопленной строкой событий.
 */
void print_round_summary(const game::GgHeroine& gg, const game::Unit& ally, const game::Unit& enemy,
                         const std::string& last_events) {
  std::cout << "\n--- Сводка после раунда (ГГЖ → союзник → враг) ---\n"; // заголовок сводки
  print_core_combat_snapshot(gg, ally, enemy); // повтор снимка
  std::cout << "  Что было за раунд: " << last_events << "\n"; // длинная строка лога
}

/**
 * print_lust_change — строка изменения Lust и список причин.
 */
void print_lust_change(const std::string& who, int before, int after,
                       const std::vector<std::string>& reasons) {
  std::cout << "  Lust («" << who << "»): " << before << " → " << after; // начало строки со стрелкой
  if (after > before) { // если Lust вырос
    std::cout << " (+" << (after - before) << ")"; // показываем прирост в скобках
  }
  std::cout << "\n"; // перевод строки после основной части
  if (!reasons.empty()) { // если есть список причин
    for (const std::string& r : reasons) { // перебираем каждую причину
      std::cout << "    • " << r << "\n"; // маркер списка и текст
    }
  }
}

/**
 * print_no_lust_change_note — пометка при нулевом изменении без причин.
 */
void print_no_lust_change_note() {
  std::cout << "    (изменений Lust нет при текущих частях и действии)\n"; // фиксированное пояснение
}

/**
 * print_gg_turn_header — заголовок хода игрока.
 */
void print_gg_turn_header(const std::string& gg_name) {
  std::cout << ">>> Ход ГГЖ: «" << gg_name << "»\n"; // строка с именем в кавычках
}

/**
 * print_gg_menu_prompt — приглашение и сброс буфера вывода.
 */
void print_gg_menu_prompt() {
  std::cout << "  Действие ГГЖ? [0 выход] [1 атака] [2 защита союзника] [3 ждать]: "; // текст без перевода строки
  std::cout.flush(); // принудительно отправляем буфер в консоль до ожидания ввода
}

/**
 * print_gg_menu_invalid — сообщение об ошибке ввода.
 */
void print_gg_menu_invalid() {
  std::cout << "  (неверный ввод — введите 0, 1, 2 или 3 и Enter)\n"; // подсказка повторить
}

/**
 * print_gg_exit_message — выход из игры по меню или EOF.
 */
void print_gg_exit_message() {
  std::cout << "\n*** Выход: игра завершена по выбору игрока (или ввод недоступен). ***\n"; // пустая строка и финальное сообщение
}

/**
 * print_gg_action_attack — сообщение об атаке героини.
 */
void print_gg_action_attack(const std::string& enemy_name, int dmg) {
  std::cout << "  ГГЖ атакует врага: урон по «" << enemy_name << "»: " << dmg << "\n"; // имя цели и число
}

/**
 * print_gg_action_guard — текст защиты союзника.
 */
void print_gg_action_guard() {
  std::cout << "  ГГЖ прикрывает союзника: удар врага по союзнику в этом раунде будет слабее.\n"; // фиксированная фраза
}

/**
 * print_gg_action_wait — ожидание героини.
 */
void print_gg_action_wait() {
  std::cout << "  ГГЖ ждёт.\n"; // короткое сообщение
}

/**
 * print_allied_turn_header — заголовок автоматического хода союзника.
 */
void print_allied_turn_header(const std::string& ally_name) {
  std::cout << "\n>>> Ход союзника (авто): «" << ally_name << "»\n"; // пустая строка перед блоком
}

/**
 * print_allied_turn_hint — напоминание про правила цели удара.
 */
void print_allied_turn_hint() {
  std::cout << "  Союзник по умолчанию атакует; цель — по Lust и порогу (не по агр./контр.).\n"; // пояснение механики
}

/**
 * print_allied_strike_result — разбор структуры AlliedStrikeResolution для консоли.
 */
void print_allied_strike_result(int ally_lust, int threshold, const game::AlliedStrikeResolution& res,
                                const game::Unit& enemy, const game::GgHeroine& gg) {
  using game::AlliedStrikeKind; // локальный alias для краткости имён вариантов

  if (res.kind == AlliedStrikeKind::DamagedEnemy) { // обычный удар по врагу
    std::cout << "  Союзник атакует врага: урон по «" << enemy.name << "»: " << res.damage_to_enemy
              << "\n"; // имя врага и урон из структуры
    return; // остальные ветки не нужны
  }

  std::cout << "  !!! Срыв: Lust союзника (" << ally_lust << ") >= порога срыва (" << threshold
            << ").\n"; // предупреждающая строка про срыв
  std::cout << "  Союзник сорвался и бьёт по броне ГГЖ («" << gg.name << "»), а не по врагу.\n"; // объяснение редиректа

  if (res.kind == AlliedStrikeKind::StruckBrokenGgArmor) { // удар по уже сломанной броне
    std::cout << "  Удар приходится по уже сломанной броне: дополнительного эффекта нет.\n"; // нет деградации
    return; // выходим из функции
  }

  if (res.kind == AlliedStrikeKind::DegradedGgArmor) { // броня реально ухудшилась
    std::cout << "  Броня ГГЖ: «" << armor_state_label(res.gg_armor_before_strike) << "» → «"
              << armor_state_label(gg.armor) << "»\n"; // показываем переход до/после
  }
}

/**
 * print_enemy_turn_header — заголовок хода врага.
 */
void print_enemy_turn_header(const std::string& enemy_name) {
  std::cout << "\n>>> Ход врага: «" << enemy_name << "»\n"; // новая секция с именем врага
}

/**
 * print_enemy_attack_ally — атака врага по союзнику.
 */
void print_enemy_attack_ally(int dmg, bool gg_was_defending) {
  std::cout << "  Враг (агрессия > контроль): атака по союзнику, урон: " << dmg; // базовое сообщение с уроном
  if (gg_was_defending) { // если героиня в начале хода врага ещё защищала
    std::cout << " (учтена защита ГГЖ)"; // пометка в той же строке
  }
  std::cout << "\n"; // завершаем строку
}

/**
 * print_enemy_wait — враг не атакует.
 */
void print_enemy_wait() {
  std::cout << "  Враг ждёт (агрессия не выше контроля).\n"; // фиксированная фраза для AI «ждать»
}

/**
 * print_victory — сообщение о победе.
 */
void print_victory() {
  std::cout << "\n*** Победа: враг уничтожен. ***\n"; // пустая строка и финал
}

/**
 * print_defeat_ally — сообщение о поражении.
 */
void print_defeat_ally() {
  std::cout << "\n*** Поражение: союзник погиб. ***\n"; // пустая строка и финал
}

/**
 * read_gg_menu_choice — читает строку с stdin, возвращает код 0–3 или -1 или 0 при EOF.
 */
int read_gg_menu_choice() {
  std::string line; // пустая строка для приёма ввода
  if (!std::getline(std::cin, line)) { // если чтение не удалось (EOF, ошибка потока)
    return 0; // трактуем как выход (как пункт 0) — см. сообщение print_gg_exit_message
  }
  if (line.empty()) { // пользователь нажал только Enter без символов
    return -1; // неверный ввод — повторить меню
  }
  const char c = line[0]; // берём только первый символ строки как команду
  if (c == '0') { // явный выход
    return 0; // код выхода
  }
  if (c == '1' || c == '2' || c == '3') { // одна из игровых команд
    return c - '0'; // перевод символа цифры в число 1..3
  }
  return -1; // любой другой первый символ — снова запросить
}

}  // namespace battle_io // конец реализаций модуля battle_io
