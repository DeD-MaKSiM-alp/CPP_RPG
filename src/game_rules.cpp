// =============================================================================
// Файл: game_rules.cpp
// Назначение: реализация правил боя без консоли — суммы по частям тела, Lust, урон, ИИ врага.
// Все формулы сосредоточены здесь, чтобы main и battle_io не дублировали вычисления.
// =============================================================================

#include "game_rules.hpp" // объявления функций и типов из game + vector/string

namespace game { // начало пространства имён игровой логики

namespace { // анонимное пространство: вспомогательные функции видны только в этом .cpp файле

/**
 * total_aggression — суммирует бонусы агрессии со всех частей тела юнита.
 * Зачем: одно число для сравнения с контролем в AI врага.
 * Где используется: только enemy_decides_to_attack в этом файле.
 * Параметры: u — юнит с заполненным вектором parts.
 * Возвращает: сумму полей aggression_bonus по всем частям.
 */
int total_aggression(const Unit& u) {
  int s = 0; // накопитель суммы, начинаем с нуля
  for (const Part& p : u.parts) { // перебираем каждую часть тела в векторе юнита
    s += p.aggression_bonus; // добавляем вклад этой части в общую агрессию
  }
  return s; // отдаём итоговую сумму вызывающему коду
}

/**
 * total_control — суммирует бонусы контроля со всех частей тела юнита.
 * Зачем: парное число к агрессии для решения «атаковать или ждать».
 * Где используется: enemy_decides_to_attack.
 * Параметры: u — юнит с частями.
 * Возвращает: сумму полей control_bonus.
 */
int total_control(const Unit& u) {
  int s = 0; // накопитель суммы контроля
  for (const Part& p : u.parts) { // обходим все части
    s += p.control_bonus; // суммируем контроль с каждой части
  }
  return s; // возвращаем полную сумму контроля
}

/**
 * sum_passive — сумма пассивного прироста Lust за ход по частям.
 * Зачем: вклад в apply_lust_for_turn независимо от атаки.
 * Параметры: u — юнит.
 * Возвращает: сумму lust_passive_per_turn.
 */
int sum_passive(const Unit& u) {
  int s = 0; // сумма пассивных слагаемых
  for (const Part& p : u.parts) { // перебор частей
    s += p.lust_passive_per_turn; // каждая часть может добавлять пассив за ход
  }
  return s; // итог пассивного роста
}

/**
 * sum_on_aggressive_action — сумма бонусов Lust при агрессивном действии (атака).
 * Зачем: отдельная добавка в apply_lust_for_turn, если performed_attack true.
 * Параметры: u — юнит.
 * Возвращает: сумму lust_on_aggressive_action.
 */
int sum_on_aggressive_action(const Unit& u) {
  int s = 0; // сумма за агрессивное действие
  for (const Part& p : u.parts) { // обход частей
    s += p.lust_on_aggressive_action; // накапливаем вклад при атаке
  }
  return s; // возвращаем сумму
}

/**
 * sum_armor_damaged — дополнительный Lust, пока броня юнита в состоянии «повреждена».
 * Зачем: ветка switch для ArmorState::Damaged в apply_lust_for_turn.
 * Параметры: u — юнит.
 * Возвращает: сумму lust_extra_armor_damaged.
 */
int sum_armor_damaged(const Unit& u) {
  int s = 0; // сумма штрафов/бонусов за повреждённую броню
  for (const Part& p : u.parts) { // цикл по частям
    s += p.lust_extra_armor_damaged; // вклад части при damaged-броне
  }
  return s; // итог
}

/**
 * sum_armor_broken — дополнительный Lust, пока броня юнита «сломана».
 * Зачем: ветка ArmorState::Broken в apply_lust_for_turn.
 * Параметры: u — юнит.
 * Возвращает: сумму lust_extra_armor_broken.
 */
int sum_armor_broken(const Unit& u) {
  int s = 0; // сумма за сломанную броню
  for (const Part& p : u.parts) { // перебор частей
    s += p.lust_extra_armor_broken; // вклад при broken-броне
  }
  return s; // возвращаем сумму
}

/**
 * sum_battle_scaling — сумма множителей «на порог боя» по частям (на юнита).
 * Зачем: умножается на battle_intensity в apply_lust_for_turn.
 * Параметры: u — юнит.
 * Возвращает: сумму lust_per_battle_intensity.
 */
int sum_battle_scaling(const Unit& u) {
  int s = 0; // накопитель масштаба от напряжения боя
  for (const Part& p : u.parts) { // обход частей
    s += p.lust_per_battle_intensity; // каждая часть даёт вклад на уровень интенсивности
  }
  return s; // суммарный коэффициент до умножения на intensity
}

}  // namespace // конец анонимного namespace внутри game

/**
 * allied_attack_redirects_to_gg_armor — срыв: удар союзника идёт не во врага, а в броню ГГЖ.
 * Порог включительно: lust == threshold уже срыв.
 * Где используется: resolve_allied_strike, append логов в battle_round, тесты.
 */
bool allied_attack_redirects_to_gg_armor(int ally_lust, int lust_redirect_threshold) {
  return ally_lust >= lust_redirect_threshold; // сравниваем текущий Lust с порогом: true если достигли или превысили
}

/**
 * apply_lust_for_turn — пересчитывает Lust за полуход и заполняет reasons для консоли.
 * Меняет u.lust; reasons сначала очищается.
 */
int apply_lust_for_turn(Unit& u, int battle_intensity, bool performed_attack,
                        std::vector<std::string>& reasons) {
  reasons.clear(); // убираем старые строки причин с прошлого вызова
  int gain = 0; // сколько всего прибавим к u.lust за этот вызов

  const int passive = sum_passive(u); // считаем пассив с частей
  if (passive != 0) { // если пассив ненулевой
    gain += passive; // добавляем к суммарному приросту
    reasons.push_back("пассивный рост за ход (части): +" + std::to_string(passive)); // строка для игрока с числом
  }

  if (performed_attack) { // если в этом полуходе считается, что персонаж атаковал
    const int on_act = sum_on_aggressive_action(u); // сумма бонусов за агрессию
    if (on_act != 0) { // только если есть что добавлять
      gain += on_act; // увеличиваем общий прирост
      reasons.push_back("агрессивное действие (атака): +" + std::to_string(on_act)); // пояснение в список
    }
  }

  if (battle_intensity > 0) { // уровень напряжения боя положителен
    const int scale = sum_battle_scaling(u); // сумма коэффициентов с частей
    const int from_battle = scale * battle_intensity; // умножаем на интенсивность сценария
    if (from_battle != 0) { // если произведение не ноль
      gain += from_battle; // добавляем к приросту Lust
      reasons.push_back("напряжение боя (уровень " + std::to_string(battle_intensity) +
                        "): +" + std::to_string(from_battle)); // текст с обоими числами
    }
  }

  switch (u.armor) { // в зависимости от состояния брони юнита
    case ArmorState::Intact: // броня целая
      break; // дополнительных слагаемых от повреждения брони нет
    case ArmorState::Damaged: { // броня уже повреждена
      const int d = sum_armor_damaged(u); // считаем дополнительный Lust за это состояние
      if (d != 0) { // если части что-то дают
        gain += d; // плюсуем к gain
        reasons.push_back("броня повреждена: +" + std::to_string(d)); // объясняем игроку
      }
      break; // выходим из ветки Damaged
    }
    case ArmorState::Broken: { // броня сломана
      const int b = sum_armor_broken(u); // сумма за сломанную броню
      if (b != 0) { // если ненулевая
        gain += b; // добавляем к приросту
        reasons.push_back("броня сломана: +" + std::to_string(b)); // строка причины
      }
      break; // конец ветки Broken
    }
  }

  u.lust += gain; // применяем весь накопленный прирост к полю lust персонажа
  return gain; // возвращаем величину прироста за этот вызов (может быть 0)
}

/**
 * degrade_armor_on_hit — после ненулевого урона по HP сдвигает броню Unit на ступень вниз.
 * Только для брони юнита Unit, не для GgHeroine.
 */
void degrade_armor_on_hit(Unit& u) {
  if (u.armor == ArmorState::Intact) { // если броня была целой
    u.armor = ArmorState::Damaged; // переходим к повреждённой
  } else if (u.armor == ArmorState::Damaged) { // если уже была повреждена
    u.armor = ArmorState::Broken; // делаем сломанной
  } // если уже Broken — не меняем (нет else)
}

/**
 * downgrade_gg_armor_one_tier — одна ступень вниз для брони главной героини при срыве союзника.
 */
bool downgrade_gg_armor_one_tier(GgHeroine& gg) {
  if (gg.armor == ArmorState::Intact) { // первая ступень срыва
    gg.armor = ArmorState::Damaged; // опускаем до повреждённой
    return true; // изменение произошло
  }
  if (gg.armor == ArmorState::Damaged) { // вторая ступень
    gg.armor = ArmorState::Broken; // делаем сломанной
    return true; // снова успех
  }
  return false; // уже Broken — деградировать некуда
}

/**
 * deal_damage — урон по юниту; если цель в стойке guarding, урон режется пополам (с округлением вверх для половины).
 */
int deal_damage(Unit& target, int raw_damage) {
  int dmg = raw_damage; // рабочая величина урона, может уменьшиться
  if (target.guarding) { // если у цели включена защита самой цели
    dmg = (dmg + 1) / 2; // целочисленное «половинение» с округлением вверх для нечётных
    target.guarding = false; // стойка снимается после одного применения
  }
  if (dmg > 0) { // наносим урон только если он остался положительным
    target.hp -= dmg; // уменьшаем здоровье на величину урона
    degrade_armor_on_hit(target); // ухудшаем броню при попадании
  }
  return dmg; // возвращаем фактически применённый урон (после деления)
}

/**
 * deal_damage_from_enemy_to_ally — враг бьёт союзника; защита героини (флаг) снижает урон.
 */
int deal_damage_from_enemy_to_ally(Unit& ally, int raw_damage, bool gg_is_defending) {
  int dmg = raw_damage; // копия базового урона для модификации
  if (gg_is_defending) { // если героиня в этом раунде выбрала защиту союзника
    dmg = (dmg + 1) / 2; // урон режется так же, как в deal_damage для guarding
  }
  if (dmg > 0) { // при положительном итоге
    ally.hp -= dmg; // снимаем HP у союзника
    degrade_armor_on_hit(ally); // броня союзника страдает от попадания
  }
  return dmg; // фактический урон
}

/**
 * enemy_decides_to_attack — true, если суммарная агрессия врага строго больше суммарного контроля.
 */
bool enemy_decides_to_attack(const Unit& enemy) {
  return total_aggression(enemy) > total_control(enemy); // сравниваем два накопленных по частям числа
}

/**
 * resolve_allied_strike — решает, куда бьёт союзник: по врагу или по броне ГГЖ при срыве.
 * Lust сравнивается до прироста за этот полуход (вызывающий код потом вызывает apply_lust).
 */
AlliedStrikeResolution resolve_allied_strike(Unit& ally, GgHeroine& gg, Unit& enemy,
                                             int attack_damage) {
  AlliedStrikeResolution r; // пустая структура с полями по умолчанию

  if (!allied_attack_redirects_to_gg_armor(ally.lust, ally.lust_redirect_threshold)) { // если срыва нет
    r.kind = AlliedStrikeKind::DamagedEnemy; // исход — удар по врагу
    r.damage_to_enemy = deal_damage(enemy, attack_damage); // наносим урон врагу и запоминаем число
    return r; // ранний выход с результатом
  }

  if (gg.armor == ArmorState::Broken) { // срыв есть, но броня героини уже на дне
    r.kind = AlliedStrikeKind::StruckBrokenGgArmor; // особый вид «удара в пустоту»
    return r; // HP врага не трогаем, броню не меняем
  }

  r.gg_armor_before_strike = gg.armor; // запоминаем, какой была броня до удара, для сообщения
  const bool changed = downgrade_gg_armor_one_tier(gg); // пытаемся опустить броню на ступень
  if (changed) { // если ступень реально изменилась
    r.kind = AlliedStrikeKind::DegradedGgArmor; // исход — деградация брони
  } else {
    r.kind = AlliedStrikeKind::StruckBrokenGgArmor; // предположительно уже был Broken (двойная защита логики)
  }
  return r; // возвращаем заполненную структуру
}

}  // namespace game // конец пространства имён game
