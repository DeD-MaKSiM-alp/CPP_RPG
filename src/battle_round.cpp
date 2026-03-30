// =============================================================================
// Файл: battle_round.cpp
// Назначение: сборка демо-боя и выполнение одного полного раунда (три полухода подряд).
// Подключает правила игры и консольный вывод; main только вызывает make_demo_battle и цикл.
// =============================================================================

#include "battle_round.hpp" // SingleRoundResult, DemoBattleState, объявления функций

#include "battle_io.hpp"  // печать и ввод меню
#include "game_rules.hpp" // deal_damage, resolve_allied_strike, apply_lust, AI врага

#include <iostream> // std::cout для пустой строки между блоками
#include <string>   // std::string для round_log
#include <vector>   // std::vector для причин Lust

namespace battle_round { // реализация сценария раунда

namespace { // внутренние вспомогательные функции файла

/**
 * append_allied_strike_to_log — дописывает в round_log фразу про исход удара союзника.
 * Зачем: единое место склейки текста по редиректу / сломанной броне / удару по броне.
 * Параметры: round_log — накапливаемая строка; will_redirect — был ли срыв по Lust до удара;
 *            gg_armor_already_broken — была ли броня ГГЖ Broken до resolve_allied_strike.
 * Возвращает: ничего (меняет round_log по ссылке).
 */
void append_allied_strike_to_log(std::string& round_log, bool will_redirect,
                                 bool gg_armor_already_broken) {
  if (!will_redirect) { // если срыва не было — удар по врагу
    round_log += "союзник ударил по врагу; "; // дописываем короткую фразу в лог
  } else if (gg_armor_already_broken) { // срыв был и броня уже была сломана до удара
    round_log += "срыв: удар по уже сломанной броне ГГЖ; "; // уточняем вариант срыва
  } else { // срыв и броня ещё не была сломана до удара
    round_log += "срыв: удар по броне ГГЖ; "; // обычный срыв с деградацией или первым ударом по броне
  }
}

}  // namespace // конец анонимного namespace

/**
 * make_demo_battle — создаёт фиксированное демо-состояние трёх участников с частями тел.
 */
DemoBattleState make_demo_battle() {
  DemoBattleState s; // пустая структура с полями по умолчанию

  s.gg.name = "ГГЖ — Лира"; // имя главной героини
  s.gg.armor = game::ArmorState::Intact; // броня героини начинается с целой
  s.gg.guarding = false; // стойка защиты в начале не активна

  s.ally.name = "Союзник — Кейн"; // имя союзника
  s.ally.hp = 80; // стартовые очки здоровья союзника
  s.ally.lust = 55; // стартовый Lust союзника (ниже порога срыва 70 в демо)
  s.ally.armor = game::ArmorState::Intact; // броня союзника целая
  s.ally.lust_redirect_threshold = 70; // порог срыва для автоматической атаки
  s.ally.parts.push_back({"Железная голова", 15, 5, 1, 2, 1, 4, 1}); // первая часть тела: имя и числовые поля Part
  s.ally.parts.push_back({"Искажённая рука", 25, 0, 2, 4, 3, 8, 2}); // вторая часть
  s.ally.parts.push_back({"Сдерживающий пояс", 0, 30, 0, 0, 0, 0, 0}); // третья часть

  s.enemy.name = "Враг — Сломленный страж"; // имя противника
  s.enemy.hp = 55; // здоровье врага
  s.enemy.lust = 5; // стартовый Lust врага
  s.enemy.armor = game::ArmorState::Intact; // броня врага
  s.enemy.lust_redirect_threshold = 0; // у врага порог срыва не используется в логике союзника
  s.enemy.parts.push_back({"Костяной кулак", 22, 4, 1, 3, 1, 3, 1}); // части врага для AI и Lust
  s.enemy.parts.push_back({"Ржавый нагрудник", 8, 2, 1, 2, 0, 2, 0}); // вторая часть врага

  return s; // возвращаем полностью заполненное состояние
}

/**
 * run_single_round — один полный раунд: баннер, три полухода, проверки победы/поражения/выхода.
 */
SingleRoundResult run_single_round(game::GgHeroine& gg, game::Unit& ally, game::Unit& enemy,
                                   int battle_intensity, int attack_damage, int round_index) {
  SingleRoundResult out; // результат по умолчанию с kind = ContinueBattle

  battle_io::print_round_banner(round_index); // печатаем номер раунда
  battle_io::print_round_state_preamble(); // подпись «состояние в начале»
  battle_io::print_battle_header(gg, ally, enemy); // три строки участников
  std::cout << "\n"; // пустая строка для читаемости

  // Накопление краткого текстового лога для строки «что было за раунд» (как раньше в main).
  std::string round_log; // пока пустая строка, будем дописывать фрагменты

  // ---------- Полуход 1: ГГЖ (игрок) ----------
  battle_io::print_gg_turn_header(gg.name); // заголовок хода игрока
  int choice = -1; // код меню: -1 означает «ещё нет валидного выбора»
  while (choice == -1) { // крутимся, пока игрок не введёт допустимую команду
    battle_io::print_gg_menu_prompt(); // показываем приглашение и flush
    choice = battle_io::read_gg_menu_choice(); // читаем строку из stdin
    if (choice == -1) { // неверный ввод
      battle_io::print_gg_menu_invalid(); // сообщаем об ошибке
    }
  }

  if (choice == 0) { // игрок выбрал выход
    battle_io::print_gg_exit_message(); // текст завершения
    out.kind = RoundOutcomeKind::PlayerExit; // помечаем исход для main
    return out; // выходим из функции без дальнейших полуходов
  }

  gg.guarding = false; // сбрасываем защиту перед новым выбором; выставим true только в ветке 2

  if (choice == 1) { // атака по врагу
    const int dmg = game::deal_damage(enemy, attack_damage); // наносим урон врагу и получаем фактический урон
    battle_io::print_gg_action_attack(enemy.name, dmg); // сообщаем игроку число
    round_log += "ГГЖ атаковала врага; "; // дописываем в лог раунда
  } else if (choice == 2) { // защита союзника
    gg.guarding = true; // включаем флаг для ослабления удара врага
    battle_io::print_gg_action_guard(); // текст про защиту
    round_log += "ГГЖ защищала союзника; "; // фрагмент лога
  } else { // остаётся choice == 3 — ждать
    battle_io::print_gg_action_wait(); // короткое сообщение
    round_log += "ГГЖ ждала; "; // фрагмент лога
  }

  battle_io::print_state_after_partial_turn("ГГЖ", gg, ally, enemy, round_log); // снимок после хода героини

  if (enemy.hp <= 0) { // враг убит в ход героини
    round_log += "враг повержен."; // дописываем финальную фразу в лог
    battle_io::print_round_summary(gg, ally, enemy, round_log); // полная сводка
    battle_io::print_victory(); // сообщение победы
    out.kind = RoundOutcomeKind::Victory; // исход для main
    return out; // выходим из раунда
  }

  // ---------- Полуход 2: союзник (авто) ----------
  battle_io::print_allied_turn_header(ally.name); // заголовок блока союзника
  battle_io::print_allied_turn_hint(); // напоминание про правила

  const bool will_redirect =
      game::allied_attack_redirects_to_gg_armor(ally.lust, ally.lust_redirect_threshold); // запоминаем, будет ли срыв до удара (для лога)
  const bool gg_armor_already_broken = (gg.armor == game::ArmorState::Broken); // броня сломана до удара или нет

  const int ally_lust_before_strike = ally.lust; // сохраняем Lust для текста «срыв» до прироста за ход
  const game::AlliedStrikeResolution strike_res =
      game::resolve_allied_strike(ally, gg, enemy, attack_damage); // выполняем автоматический удар
  battle_io::print_allied_strike_result(ally_lust_before_strike, ally.lust_redirect_threshold,
                                        strike_res, enemy, gg); // расшифровываем результат в консоль

  append_allied_strike_to_log(round_log, will_redirect, gg_armor_already_broken); // дописываем текст про союзника

  const bool ally_attacked = true; // в модели союзник в своём полуходе всегда «агрессивен» для Lust
  {
    const int lust_before = ally.lust; // Lust до пересчёта за полуход
    std::vector<std::string> reasons; // сюда apply_lust положит строки причин
    const int gain = game::apply_lust_for_turn(ally, battle_intensity, ally_attacked, reasons); // пересчёт Lust союзника
    battle_io::print_lust_change(ally.name, lust_before, ally.lust, reasons); // печать изменения
    if (gain == 0 && reasons.empty()) { // если ничего не произошло и список пуст
      battle_io::print_no_lust_change_note(); // пометка для игрока
    }
  }

  battle_io::print_state_after_partial_turn("союзника", gg, ally, enemy, round_log); // снимок после союзника

  if (enemy.hp <= 0) { // враг мог погибнуть от удара союзника по нему
    round_log += "враг повержен."; // фраза в лог
    battle_io::print_round_summary(gg, ally, enemy, round_log); // сводка
    battle_io::print_victory(); // победа
    out.kind = RoundOutcomeKind::Victory; // исход
    return out; // конец раунда
  }

  // ---------- Полуход 3: враг ----------
  battle_io::print_enemy_turn_header(enemy.name); // заголовок хода врага
  const bool enemy_attacks = game::enemy_decides_to_attack(enemy); // решаем по агрессии/контролю
  bool enemy_did_attack = false; // флаг для формулы Lust врага
  std::string enemy_turn_desc; // человекочитаемое описание для снимка после врага

  const bool gg_defending_before_enemy = gg.guarding; // запоминаем защиту до возможного сброса в deal_damage
  if (enemy_attacks) { // враг атакует
    enemy_did_attack = true; // для apply_lust врага считается агрессивное действие
    const int dmg = game::deal_damage_from_enemy_to_ally(ally, attack_damage, gg.guarding); // урон по союзнику с учётом защиты ГГЖ
    battle_io::print_enemy_attack_ally(dmg, gg_defending_before_enemy); // текст с уроном
    round_log += "враг атаковал союзника; "; // фрагмент лога
    enemy_turn_desc =
        "враг атаковал союзника (урон может быть снижен защитой ГГЖ)."; // описание для снимка
  } else { // враг ждёт
    battle_io::print_enemy_wait(); // текст ожидания
    round_log += "враг ждал; "; // лог
    enemy_turn_desc = "враг ждал (AI: агрессия не выше контроля)."; // описание для снимка
  }

  {
    const int lust_before = enemy.lust; // Lust врага до прироста
    std::vector<std::string> reasons; // причины для консоли
    const int gain = game::apply_lust_for_turn(enemy, battle_intensity, enemy_did_attack, reasons); // пересчёт Lust врага
    battle_io::print_lust_change(enemy.name, lust_before, enemy.lust, reasons); // вывод строки
    if (gain == 0 && reasons.empty()) { // нет изменений и пустой список
      battle_io::print_no_lust_change_note(); // пометка
    }
  }

  battle_io::print_state_after_partial_turn("врага", gg, ally, enemy, enemy_turn_desc); // снимок после врага

  gg.guarding = false; // сбрасываем стойку защиты на конец раунда (не переносим на следующий)

  if (ally.hp <= 0) { // союзник погиб от удара врага
    round_log += "союзник погиб."; // финальная фраза лога
    battle_io::print_round_summary(gg, ally, enemy, round_log); // сводка
    battle_io::print_defeat_ally(); // поражение
    out.kind = RoundOutcomeKind::DefeatAlly; // исход
    return out; // конец раунда
  }

  round_log += "раунд завершён."; // бой в этом раунде не закончен — помечаем завершение раунда
  battle_io::print_round_summary(gg, ally, enemy, round_log); // итоговая сводка
  std::cout << "\n"; // пустая строка перед следующим раундом в main

  out.kind = RoundOutcomeKind::ContinueBattle; // main должен продолжить цикл
  return out; // нормальный конец раунда без победы/поражения/выхода
}

}  // namespace battle_round // конец модуля
