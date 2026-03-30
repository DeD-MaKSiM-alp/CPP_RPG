// =============================================================================
// Файл: test_game_logic.cpp
// Назначение: автономные проверки функций из game_rules без консоли и без battle_round.
// Запускается отдельным exe; код возврата 0 — все тесты прошли, 1 — хотя бы один провален.
// =============================================================================

#include "game_rules.hpp" // тестируемые функции и типы правил
#include "game_types.hpp" // Unit, GgHeroine, перечисления

#include <string>   // std::string в векторах причин (не в каждом тесте)
#include <vector>   // std::vector для apply_lust_for_turn

namespace { // тестовые функции спрятаны, чтобы имена не конфликтовали с глобальным main

/**
 * test_redirect_rule — проверяет порог срыва: строго ниже / ровно / выше порога.
 * Возвращает: 0 если все проверки ок, иначе 1.
 */
int test_redirect_rule() {
  using game::allied_attack_redirects_to_gg_armor; // импортируем имя функции в область видимости
  if (allied_attack_redirects_to_gg_armor(69, 70)) { // 69 < 70 — срыва быть не должно
    return 1; // код ошибки теста
  }
  if (!allied_attack_redirects_to_gg_armor(70, 70)) { // ровно порог — срыв должен быть
    return 1; // ошибка: ожидали true
  }
  if (!allied_attack_redirects_to_gg_armor(100, 70)) { // выше порога — тоже срыв
    return 1; // ошибка
  }
  return 0; // все утверждения прошли
}

/**
 * test_gg_armor_transitions — цепочка Intact→Damaged→Broken и отказ дальше.
 */
int test_gg_armor_transitions() {
  game::GgHeroine gg; // пустая героиня с броней по умолчанию Intact
  gg.name = "Тест"; // задаём имя для отладки (в assert не используется)
  gg.armor = game::ArmorState::Intact; // явно целая броня
  if (!game::downgrade_gg_armor_one_tier(gg)) { // первый сдвиг должен удаться
    return 1; // ошибка
  }
  if (gg.armor != game::ArmorState::Damaged) { // ожидаем повреждённую
    return 1; // ошибка
  }
  if (!game::downgrade_gg_armor_one_tier(gg)) { // второй сдвиг
    return 1; // ошибка
  }
  if (gg.armor != game::ArmorState::Broken) { // ожидаем сломанную
    return 1; // ошибка
  }
  if (game::downgrade_gg_armor_one_tier(gg)) { // третий вызов не должен менять состояние
    return 1; // ошибка: вернули true при Broken
  }
  if (gg.armor != game::ArmorState::Broken) { // броня остаётся сломанной
    return 1; // ошибка
  }
  return 0; // успех
}

/**
 * test_deal_damage_guard_halves — guarding цели уменьшает урон и снимает флаг.
 */
int test_deal_damage_guard_halves() {
  game::Unit target; // тестовая цель
  target.name = "Цель"; // имя для ясности
  target.hp = 100; // стартовое HP
  target.guarding = true; // включаем стойку — урон должен поделиться
  const int applied = game::deal_damage(target, 12); // сырой урон 12
  if (applied != 6) { // ожидаем половину 12
    return 1; // ошибка
  }
  if (target.hp != 94) { // 100 - 6 = 94
    return 1; // ошибка
  }
  if (target.guarding) { // флаг должен сброситься после удара
    return 1; // ошибка
  }
  return 0; // успех
}

/**
 * test_deal_damage_from_enemy_with_gg_defense — защита героини режет урон по союзнику.
 */
int test_deal_damage_from_enemy_with_gg_defense() {
  game::Unit ally; // союзник
  ally.name = "Союзник"; // имя
  ally.hp = 80; // стартовое HP
  const int applied = game::deal_damage_from_enemy_to_ally(ally, 12, true); // gg_is_defending = true
  if (applied != 6) { // половина от 12
    return 1; // ошибка
  }
  if (ally.hp != 74) { // 80 - 6
    return 1; // ошибка
  }
  return 0; // успех
}

/**
 * test_resolve_allied_strike_enemy — при Lust ниже порога удар идёт во врага.
 */
int test_resolve_allied_strike_enemy() {
  game::Unit ally; // союзник
  ally.lust = 50; // ниже порога 70
  ally.lust_redirect_threshold = 70; // порог срыва
  game::GgHeroine gg; // героиня
  game::Unit enemy; // враг
  enemy.hp = 100; // достаточно HP для проверки
  enemy.guarding = false; // без стойки

  const game::AlliedStrikeResolution r = game::resolve_allied_strike(ally, gg, enemy, 12); // урон 12
  if (r.kind != game::AlliedStrikeKind::DamagedEnemy) { // ожидаем удар по врагу
    return 1; // ошибка
  }
  if (r.damage_to_enemy != 12) { // полный урон без guarding врага
    return 1; // ошибка
  }
  if (enemy.hp != 88) { // 100 - 12
    return 1; // ошибка
  }
  return 0; // успех
}

/**
 * test_resolve_allied_strike_redirect_degrades_gg — на пороге Lust удар редиректится на броню ГГЖ.
 */
int test_resolve_allied_strike_redirect_degrades_gg() {
  game::Unit ally; // союзник
  ally.lust = 70; // ровно порог
  ally.lust_redirect_threshold = 70; // тот же порог
  game::GgHeroine gg; // героиня
  gg.armor = game::ArmorState::Intact; // броня готова к первому срыву
  game::Unit enemy; // враг
  enemy.hp = 100; // не должен потерять HP при редиректе

  const game::AlliedStrikeResolution r = game::resolve_allied_strike(ally, gg, enemy, 12); // удар
  if (r.kind != game::AlliedStrikeKind::DegradedGgArmor) { // броня должна ухудшиться
    return 1; // ошибка
  }
  if (enemy.hp != 100) { // по врагу не били
    return 1; // ошибка
  }
  if (gg.armor != game::ArmorState::Damaged) { // одна ступень вниз
    return 1; // ошибка
  }
  return 0; // успех
}

/**
 * test_resolve_allied_strike_broken_armor_safe — при сломанной броне срыв не ломает её сильнее.
 */
int test_resolve_allied_strike_broken_armor_safe() {
  game::Unit ally; // союзник
  ally.lust = 80; // выше порога
  ally.lust_redirect_threshold = 70; // порог
  game::GgHeroine gg; // героиня
  gg.armor = game::ArmorState::Broken; // уже минимум
  game::Unit enemy; // враг
  enemy.hp = 100; // HP неизменны

  const game::AlliedStrikeResolution r = game::resolve_allied_strike(ally, gg, enemy, 12); // срыв
  if (r.kind != game::AlliedStrikeKind::StruckBrokenGgArmor) { // особый вид исхода
    return 1; // ошибка
  }
  if (enemy.hp != 100) { // враг цел
    return 1; // ошибка
  }
  if (gg.armor != game::ArmorState::Broken) { // броня осталась сломанной
    return 1; // ошибка
  }
  return 0; // успех
}

/**
 * test_apply_lust_minimal_unit — юнит без частей не меняет Lust при нулевой интенсивности сценария.
 */
int test_apply_lust_minimal_unit() {
  game::Unit u; // пустой юнит: parts пустой, броня Intact по умолчанию
  u.lust = 10; // стартовое значение
  std::vector<std::string> reasons; // сюда apply_lust положит строки
  const int gain = game::apply_lust_for_turn(u, 1, false, reasons); // battle_intensity 1, без атаки
  if (gain != 0) { // без частей прирост должен быть 0
    return 1; // ошибка
  }
  if (u.lust != 10) { // Lust не должен измениться
    return 1; // ошибка
  }
  return 0; // успех
}

}  // namespace // конец анонимного namespace с тестами

/**
 * main — запускает все тесты подряд; при первом провале возвращает 1.
 */
int main() {
  if (test_redirect_rule() != 0) { // проверка порога срыва
    return 1; // провал всей программы тестов
  }
  if (test_gg_armor_transitions() != 0) { // проверка брони ГГЖ
    return 1; // провал
  }
  if (test_deal_damage_guard_halves() != 0) { // проверка deal_damage
    return 1; // провал
  }
  if (test_deal_damage_from_enemy_with_gg_defense() != 0) { // проверка защиты от врага
    return 1; // провал
  }
  if (test_resolve_allied_strike_enemy() != 0) { // удар по врагу
    return 1; // провал
  }
  if (test_resolve_allied_strike_redirect_degrades_gg() != 0) { // редирект на броню
    return 1; // провал
  }
  if (test_resolve_allied_strike_broken_armor_safe() != 0) { // сломанная броня
    return 1; // провал
  }
  if (test_apply_lust_minimal_unit() != 0) { // пустой юнит и Lust
    return 1; // провал
  }
  return 0; // все тесты прошли — успешный код для CI/скриптов
}
