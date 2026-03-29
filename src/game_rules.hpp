#pragma once

// =============================================================================
// game_rules.hpp — объявления чистой игровой логики (реализация в game_rules.cpp).
// Ни cin, ни cout: только состояние участников и возвращаемые/исходные структуры.
// =============================================================================

#include "game_types.hpp"

#include <string>
#include <vector>

namespace game {

// Правило срыва: удар союзника перенаправляется на броню ГГЖ (включая ровно порог).
bool allied_attack_redirects_to_gg_armor(int ally_lust, int lust_redirect_threshold);

// Пересчёт Lust за полуход; reasons — человекочитаемые причины для слоя вывода.
int apply_lust_for_turn(Unit& u, int battle_intensity, bool performed_attack,
                        std::vector<std::string>& reasons);

// Ухудшение личной брони юнита после ненулевого урона по HP.
void degrade_armor_on_hit(Unit& u);

// Одна ступень брони ГГЖ при ударе союзника при срыве; false, если уже Broken.
bool downgrade_gg_armor_one_tier(GgHeroine& gg);

// Урон по юниту с учётом self-guarding цели.
int deal_damage(Unit& target, int raw_damage);

// Урон врага по союзнику; при gg_is_defending урон снижается (защита ГГЖ).
int deal_damage_from_enemy_to_ally(Unit& ally, int raw_damage, bool gg_is_defending);

// AI врага: атаковать или ждать.
bool enemy_decides_to_attack(const Unit& enemy);

// Автоматическая атака союзника: враг или броня ГГЖ по правилам Lust/порога.
AlliedStrikeResolution resolve_allied_strike(Unit& ally, GgHeroine& gg, Unit& enemy,
                                             int attack_damage);

}  // namespace game
