#pragma once

// =============================================================================
// game_types.hpp — данные боя без логики и без консоли.
// Зачем отдельный файл: типы можно включать в тесты и в модули правил без зависимости
// от ввода/вывода; границы «что есть в мире игры» видны в одном месте.
// =============================================================================

#include <string>
#include <vector>

namespace game {

// Состояние брони по ступеням: целая → повреждённая → сломанная.
enum class ArmorState { Intact, Damaged, Broken };

// Часть тела шаблона юнита: характеристики для Lust и (опционально) агр./контр.
// Агрессия и контроль у союзника не задают цель удара — только Lust и порог срыва;
// у врага агр./контр. участвуют в AI.
struct Part {
  std::string name;
  int aggression_bonus{};
  int control_bonus{};
  int lust_passive_per_turn{};
  int lust_on_aggressive_action{};
  int lust_extra_armor_damaged{};
  int lust_extra_armor_broken{};
  int lust_per_battle_intensity{};
};

// Боевой юнит (союзник или враг): HP, Lust, собственная броня, части.
struct Unit {
  std::string name;
  int hp{};
  int lust{};
  ArmorState armor{ArmorState::Intact};
  std::vector<Part> parts;
  // Если true — входящий урон по этому юниту делится пополам (см. deal_damage).
  // У союзника в текущей модели игроком не выставляется; поле для симметрии API.
  bool guarding{};

  // Порог срыва: при Lust >= значения автоматическая атака союзника идёт на броню ГГЖ.
  int lust_redirect_threshold{70};
};

// Главная героиня: игрок выбирает действия; броня снижается при срыве союзника.
struct GgHeroine {
  std::string name;
  ArmorState armor{ArmorState::Intact};
  // Стойка «прикрыть союзника»: ослабляет следующий удар врага по союзнику.
  bool guarding{};
};

// Результат разрешения автоматической атаки союзника (без текста для консоли).
enum class AlliedStrikeKind {
  DamagedEnemy,           // урон нанесён врагу
  StruckBrokenGgArmor,    // срыв, но броня ГГЖ уже сломана — эффекта на броню нет
  DegradedGgArmor         // срыв, броня ГГЖ ухудшилась на одну ступень
};

struct AlliedStrikeResolution {
  AlliedStrikeKind kind{AlliedStrikeKind::DamagedEnemy};
  // Фактический урон по врагу (если kind == DamagedEnemy).
  int damage_to_enemy = 0;
  // Состояние брони ГГЖ до удара — для сообщения о переходе (если DegradedGgArmor).
  ArmorState gg_armor_before_strike{ArmorState::Intact};
};

}  // namespace game
