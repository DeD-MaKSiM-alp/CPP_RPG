#pragma once // защита от повторного включения заголовка в один translation unit

// =============================================================================
// Файл: game_rules.hpp
// Назначение: объявления «чистой» игровой логики — функции меняют состояние участников
// по правилам, без чтения с клавиатуры и без печати в консоль (реализация в game_rules.cpp).
// =============================================================================

#include "game_types.hpp" // Unit, GgHeroine, AlliedStrikeResolution и перечисления

#include <string>   // для строк в векторе причин изменения Lust
#include <vector>   // список пояснений для вывода

namespace game { // логика боя в одном пространстве имён

/**
 * allied_attack_redirects_to_gg_armor — нужно ли перенаправить удар союзника на броню ГГЖ.
 * Зачем: одно место для правила «порог Lust включительно = срыв».
 * Где используется: resolve_allied_strike, battle_round (текст лога), тесты.
 * Параметры: ally_lust — текущий Lust союзника; lust_redirect_threshold — порог из юнита.
 * Возвращает: true, если удар должен идти в броню героини, а не во врага.
 */
bool allied_attack_redirects_to_gg_armor(int ally_lust, int lust_redirect_threshold);

/**
 * apply_lust_for_turn — пересчитывает Lust персонажа за один полуход по частям тела и броне.
 * Зачем: централизовать формулы прироста и собрать человекочитаемые причины для консоли.
 * Где используется: полуходы союзника и врага в battle_round.cpp.
 * Параметры: u — кого меняем; battle_intensity — множитель боя; performed_attack — был ли удар;
 *            reasons — сюда дописываются строки объяснений (сначала очищается).
 * Возвращает: суммарный прирост Lust за этот вызов (может быть 0).
 */
int apply_lust_for_turn(Unit& u, int battle_intensity, bool performed_attack,
                        std::vector<std::string>& reasons);

/**
 * degrade_armor_on_hit — ухудшает броню обычного юнита после получения урона по HP.
 * Зачем: модель «броня страдает вместе с здоровьем» для союзника/врага.
 * Где используется: внутри deal_damage и deal_damage_from_enemy_to_ally при ненулевом уроне.
 * Параметры: u — юнит, чью броню сдвигаем на одну ступень вниз (если возможно).
 * Возвращает: ничего (void).
 */
void degrade_armor_on_hit(Unit& u);

/**
 * downgrade_gg_armor_one_tier — опускает броню главной героини на одну ступень при срыве союзника.
 * Зачем: отдельная цепочка Intact→Damaged→Broken для ГГЖ, не смешивая с бронёй Unit.
 * Где используется: resolve_allied_strike при редиректе удара на героиню.
 * Параметры: gg — героиня, чью броню меняем.
 * Возвращает: true, если ступень изменилась; false, если броня уже была Broken.
 */
bool downgrade_gg_armor_one_tier(GgHeroine& gg);

/**
 * deal_damage — наносит урон юниту с учётом его собственной стойки guarding (половина урона).
 * Зачем: один вход для атаки ГГЖ по врагу и для удара союзника по врагу.
 * Где используется: battle_round, resolve_allied_strike.
 * Параметры: target — цель; raw_damage — урон до уменьшения от guarding.
 * Возвращает: фактически применённый урон (после деления, может быть 0).
 */
int deal_damage(Unit& target, int raw_damage);

/**
 * deal_damage_from_enemy_to_ally — враг бьёт союзника; защита героини ослабляет урон.
 * Зачем: защита ГГЖ не хранится в ally.guarding, а передаётся отдельным флагом.
 * Где используется: полуход врага в battle_round.cpp.
 * Параметры: ally — союзник; raw_damage — базовый урон; gg_is_defending — прикрыла ли ГГЖ.
 * Возвращает: фактический урон по союзнику.
 */
int deal_damage_from_enemy_to_ally(Unit& ally, int raw_damage, bool gg_is_defending);

/**
 * enemy_decides_to_attack — решает, атакует ли враг в этом полуходе или ждёт.
 * Зачем: простой ИИ «агрессия против контроля» по суммам с частей тела.
 * Где используется: полуход врага в battle_round.cpp.
 * Параметры: enemy — враг с заполненным вектором parts.
 * Возвращает: true — атаковать союзника; false — пропустить удар.
 */
bool enemy_decides_to_attack(const Unit& enemy);

/**
 * resolve_allied_strike — применяет автоматический удар союзника: по врагу или по броне ГГЖ.
 * Зачем: единая точка правил Lust/порога и переходов брони героини.
 * Где используется: полуход союзника в battle_round.cpp; тесты.
 * Параметры: ally, gg, enemy — состояние; attack_damage — сила удара.
 * Возвращает: структура с видом исхода и числами для вывода.
 */
AlliedStrikeResolution resolve_allied_strike(Unit& ally, GgHeroine& gg, Unit& enemy,
                                             int attack_damage);

}  // namespace game
