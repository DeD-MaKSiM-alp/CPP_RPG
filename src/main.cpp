// =============================================================================
// Учебный прототип: пошаговый бой в консоли.
//
// Участники и роли:
//   • ГГЖ — главная героиня; ею управляет игрок (меню 0/1/2/3).
//   • Союзник ГГ — отдельный автономный юнит на стороне игрока: каждый раунд
//     сам совершает атаку; цель удара определяется Lust и порогом срыва, не вводом.
//   • Враг — одна вражеская сущность; полуход по правилу агрессия/контроль (только враг).
//
// Механика срыва союзника (агрессия/контроль союзника цель НЕ выбирают):
//   Если Lust союзника >= lust_redirect_threshold → удар не по врагу, а по броне ГГЖ
//   (включительно на пороге). Если броня ГГЖ уже сломана — безопасная ветка и текст.
// =============================================================================

#include "game_rules.hpp"

#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32) && __has_include(<windows.h>)
#include <windows.h>
#define GAME_HAS_WINDOWS_API 1
#endif

// -----------------------------------------------------------------------------
// setup_console_utf8
// Зачем: на русской Windows консоль часто не в UTF-8; без этого кириллица в
//        выводе превращается в «кракозябры».
// Принимает: ничего.
// Делает: включает UTF-8 для ввода/вывода консоли (только если есть windows.h).
// Меняет: глобальное состояние консоли процесса.
// Возвращает: ничего.
// Когда вызывается: один раз в начале main.
// -----------------------------------------------------------------------------
static void setup_console_utf8() {
#ifdef GAME_HAS_WINDOWS_API
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
#endif
}

// Состояние брони по ступеням: целая → повреждённая → сломанная.
enum class ArmorState { Intact, Damaged, Broken };

// -----------------------------------------------------------------------------
// armor_state_label
// Зачем: единая текстовая подпись состояния брони для консоли.
// Принимает: s — текущее перечисление ArmorState.
// Делает: сопоставляет enum человекочитаемой строке на русском.
// Меняет: ничего (только чтение).
// Возвращает: указатель на строковый литерал (не выделяет память).
// Когда: при любой печати состояния брони ГГЖ, союзника или врага.
// -----------------------------------------------------------------------------
static const char* armor_state_label(ArmorState s) {
  switch (s) {
    case ArmorState::Intact:
      return "броня цела";
    case ArmorState::Damaged:
      return "броня повреждена";
    case ArmorState::Broken:
      return "броня сломана";
  }
  return "?";
}

// Часть тела шаблона юнита: характеристики для Lust и (опционально) агр./контр.
// Агрессия и контроль у союзника НЕ задают цель удара — только Lust и порог срыва;
// агр./контр. у врага используются в AI врага.
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
// У союзника guarding в этом прототипе не используется для меню — защиту выбирает ГГЖ.
struct Unit {
  std::string name;
  int hp{};
  int lust{};
  ArmorState armor{ArmorState::Intact};
  std::vector<Part> parts;
  // Если true — входящий урон по этому юниту делится пополам (см. deal_damage).
  // У союзника в текущей модели не выставляется игроком; оставлено для симметрии API.
  bool guarding{};

  // Порог срыва: если Lust >= этого значения, автоматическая атака союзника
  // идёт не по врагу, а по броне ГГЖ (см. resolve_allied_automatic_attack).
  int lust_redirect_threshold{70};
};

// Главная героиня: игрок выбирает действия; броня страдает при срыве союзника.
// guarding — стойка «защитить союзника»: ослабляет следующий удар врага по союзнику.
struct GgHeroine {
  std::string name;
  ArmorState armor{ArmorState::Intact};
  bool guarding{};
};

// -----------------------------------------------------------------------------
// Суммы по частям юнита (агрессия/контроль — в основном для врага и отчётов).
// -----------------------------------------------------------------------------

static int total_aggression(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.aggression_bonus;
  }
  return s;
}

static int total_control(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.control_bonus;
  }
  return s;
}

static int sum_passive(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_passive_per_turn;
  }
  return s;
}

static int sum_on_aggressive_action(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_on_aggressive_action;
  }
  return s;
}

static int sum_armor_damaged(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_extra_armor_damaged;
  }
  return s;
}

static int sum_armor_broken(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_extra_armor_broken;
  }
  return s;
}

static int sum_battle_scaling(const Unit& u) {
  int s = 0;
  for (const Part& p : u.parts) {
    s += p.lust_per_battle_intensity;
  }
  return s;
}

// -----------------------------------------------------------------------------
// apply_lust_for_turn
// Зачем: единое место, где за полуход юнита пересчитывается Lust по частям.
// Принимает:
//   u — юнит, у которого растёт поле lust (изменяется по ссылке);
//   battle_intensity — глобальный множитель напряжения боя (здесь константа);
//   performed_attack — было ли агрессивное действие «атака» в этом полуходе
//       (у союзника: автоматическая атака по врагу или по броне ГГЖ при срыве);
//   reasons — сюда дописываются строки для консоли (почему вырос Lust).
// Меняет: u.lust, заполняет reasons.
// Возвращает: сколько единиц Lust добавлено за этот вызов.
// -----------------------------------------------------------------------------
static int apply_lust_for_turn(Unit& u, int battle_intensity, bool performed_attack,
                               std::vector<std::string>& reasons) {
  reasons.clear();
  int gain = 0;

  // Пассивные источники с частей тела — стабильный фон роста Lust за полуход.
  const int passive = sum_passive(u);
  if (passive != 0) {
    gain += passive;
    reasons.push_back("пассивный рост за ход (части): +" + std::to_string(passive));
  }

  // Если юнит атаковал, добавляется вклад «агрессивного действия» с частей.
  if (performed_attack) {
    const int on_act = sum_on_aggressive_action(u);
    if (on_act != 0) {
      gain += on_act;
      reasons.push_back("агрессивное действие (атака): +" + std::to_string(on_act));
    }
  }

  // Масштаб от «напряжения боя» — в MVP константа, но формула остаётся явной.
  if (battle_intensity > 0) {
    const int scale = sum_battle_scaling(u);
    const int from_battle = scale * battle_intensity;
    if (from_battle != 0) {
      gain += from_battle;
      reasons.push_back("напряжение боя (уровень " + std::to_string(battle_intensity) +
                        "): +" + std::to_string(from_battle));
    }
  }

  // Модификаторы от СОБСТВЕННОЙ брони юнита (не путать с бронёй ГГЖ).
  switch (u.armor) {
    case ArmorState::Intact:
      break;
    case ArmorState::Damaged: {
      const int d = sum_armor_damaged(u);
      if (d != 0) {
        gain += d;
        reasons.push_back("броня повреждена: +" + std::to_string(d));
      }
      break;
    }
    case ArmorState::Broken: {
      const int b = sum_armor_broken(u);
      if (b != 0) {
        gain += b;
        reasons.push_back("броня сломана: +" + std::to_string(b));
      }
      break;
    }
  }

  u.lust += gain;
  return gain;
}

// -----------------------------------------------------------------------------
// degrade_armor_on_hit
// Зачем: когда по юниту прошёл ненулевой урон HP, его личная броня ухудшается на ступень.
// -----------------------------------------------------------------------------
static void degrade_armor_on_hit(Unit& u) {
  if (u.armor == ArmorState::Intact) {
    u.armor = ArmorState::Damaged;
  } else if (u.armor == ArmorState::Damaged) {
    u.armor = ArmorState::Broken;
  }
}

// -----------------------------------------------------------------------------
// downgrade_gg_armor_one_tier
// Зачем: удар союзника по броне ГГЖ при срыве — одна ступень за удар.
// Если уже Broken — возвращаем false; вызывающий код печатает «без эффекта», HP не трогаем.
// -----------------------------------------------------------------------------
static bool downgrade_gg_armor_one_tier(GgHeroine& gg) {
  if (gg.armor == ArmorState::Intact) {
    gg.armor = ArmorState::Damaged;
    return true;
  }
  if (gg.armor == ArmorState::Damaged) {
    gg.armor = ArmorState::Broken;
    return true;
  }
  return false;
}

// -----------------------------------------------------------------------------
// deal_damage
// Зачем: урон по юниту-цели (враг, когда его бьют ГГЖ или союзник без срыва).
// Учитывает guarding цели (округление вверх для малых чисел).
// -----------------------------------------------------------------------------
static int deal_damage(Unit& target, int raw_damage) {
  // Локальная копия: дальше можем уменьшить из-за стойки защиты на самой цели.
  int dmg = raw_damage;
  if (target.guarding) {
    // Целое деление с округлением вверх для малых чисел: (dmg+1)/2.
    dmg = (dmg + 1) / 2;
    // Стойка одноразовая: после получения удара флаг снимается.
    target.guarding = false;
  }
  if (dmg > 0) {
    // Сначала HP, затем побочный эффект на броню юнита (не путать с бронёй ГГЖ).
    target.hp -= dmg;
    degrade_armor_on_hit(target);
  }
  return dmg;
}

// -----------------------------------------------------------------------------
// deal_damage_from_enemy_to_ally
// Зачем: отдельный путь урона врага по союзнику — защита выбрана у ГГЖ (gg_is_defending),
//        а не у поля ally.guarding (игрок не управляет союзником в меню).
// Принимает: ally — кого бьёт враг; raw_damage — базовый урон; gg_is_defending — стойка ГГЖ.
// Меняет: ally.hp и ally.armor при ненулевом уроне; флаг guarding союзника не используется.
// -----------------------------------------------------------------------------
static int deal_damage_from_enemy_to_ally(Unit& ally, int raw_damage, bool gg_is_defending) {
  int dmg = raw_damage;
  if (gg_is_defending) {
    // ГГЖ прикрыла союзника: тот же приём половинного урона, что и у self-guard у Unit.
    dmg = (dmg + 1) / 2;
  }
  if (dmg > 0) {
    ally.hp -= dmg;
    degrade_armor_on_hit(ally);
  }
  return dmg;
}

// -----------------------------------------------------------------------------
// resolve_allied_automatic_attack
// Зачем: единственное место выбора цели автоматической атаки союзника — только Lust и порог.
// Сравнение порога — до прироста Lust за этот полуход (как и раньше по смыслу ТЗ).
// Принимает: ally, gg, enemy, attack_damage — см. вызов из main после хода ГГЖ.
// Меняет: enemy или gg.armor; печатает в консоль (граница «логика / вывод» здесь намеренно
//         оставлена в одном месте ради простоты MVP; правило срыва дублируется в game_rules.hpp).
// -----------------------------------------------------------------------------
static void resolve_allied_automatic_attack(Unit& ally, GgHeroine& gg, Unit& enemy, int attack_damage) {
  const int threshold = ally.lust_redirect_threshold;

  // Ветка «обычная атака по врагу»: Lust ещё не перешёл порог срыва.
  if (!game::allied_attack_redirects_to_gg_armor(ally.lust, threshold)) {
    const int dmg = deal_damage(enemy, attack_damage);
    std::cout << "  Союзник атакует врага: урон по «" << enemy.name << "»: " << dmg << "\n";
    return;
  }

  // Ветка срыва: агрессия/контроль союзника цель не переопределяют — только высокий Lust.
  std::cout << "  !!! Срыв: Lust союзника (" << ally.lust << ") >= порога срыва (" << threshold
            << ").\n";
  std::cout << "  Союзник сорвался и бьёт по броне ГГЖ («" << gg.name << "»), а не по врагу.\n";

  if (gg.armor == ArmorState::Broken) {
    std::cout << "  Удар приходится по уже сломанной броне: дополнительного эффекта нет.\n";
    return;
  }

  const ArmorState before = gg.armor;
  const bool changed = downgrade_gg_armor_one_tier(gg);
  if (changed) {
    std::cout << "  Броня ГГЖ: «" << armor_state_label(before) << "» → «" << armor_state_label(gg.armor)
              << "»\n";
  }
}

// -----------------------------------------------------------------------------
// print_unit_line — строка юнита с HP/Lust/бронёй.
// -----------------------------------------------------------------------------
static void print_unit_line(const Unit& u) {
  std::cout << "  " << u.name << " | HP: " << u.hp << " | Lust: " << u.lust << " | "
            << armor_state_label(u.armor);
  if (u.guarding) {
    std::cout << " | стойка защиты";
  }
  std::cout << "\n";
}

// -----------------------------------------------------------------------------
// print_gg_line — ГГЖ без HP в MVP: броня и флаг готовности защищать союзника от врага.
// -----------------------------------------------------------------------------
static void print_gg_line(const GgHeroine& gg) {
  std::cout << "  " << gg.name << " | " << armor_state_label(gg.armor);
  if (gg.guarding) {
    std::cout << " | защита союзника (следующий удар врага по союзнику ослаблен)";
  }
  std::cout << "\n";
}

// -----------------------------------------------------------------------------
// print_battle_header — шапка: ГГЖ, союзник, враг.
// -----------------------------------------------------------------------------
static void print_battle_header(const GgHeroine& gg, const Unit& ally, const Unit& enemy) {
  print_gg_line(gg);
  print_unit_line(ally);
  print_unit_line(enemy);
}

// -----------------------------------------------------------------------------
// print_core_combat_snapshot — ключевые числа для сводок после полуходов и раунда.
// -----------------------------------------------------------------------------
static void print_core_combat_snapshot(const GgHeroine& gg, const Unit& ally, const Unit& enemy) {
  std::cout << "  HP врага («" << enemy.name << "»): " << enemy.hp << "\n";
  std::cout << "  HP союзника («" << ally.name << "»): " << ally.hp << "\n";
  std::cout << "  Lust союзника: " << ally.lust << " (порог срыва: " << ally.lust_redirect_threshold
            << ")\n";
  std::cout << "  Броня ГГЖ («" << gg.name << "»): " << armor_state_label(gg.armor) << "\n";
}

// -----------------------------------------------------------------------------
// print_state_after_partial_turn — снимок после одного полухода (ГГЖ / союзник / враг).
// -----------------------------------------------------------------------------
static void print_state_after_partial_turn(const char* turn_name, const GgHeroine& gg,
                                           const Unit& ally, const Unit& enemy,
                                           const std::string& what_happened) {
  std::cout << "\n--- Состояние после хода " << turn_name << " ---\n";
  print_core_combat_snapshot(gg, ally, enemy);
  std::cout << "  Что произошло на этом ходу: " << what_happened << "\n";
}

// -----------------------------------------------------------------------------
// print_round_summary — итог раунда после всех трёх полуходов (если дошли до конца).
// -----------------------------------------------------------------------------
static void print_round_summary(const GgHeroine& gg, const Unit& ally, const Unit& enemy,
                                const std::string& last_events) {
  std::cout << "\n--- Сводка после раунда (ГГЖ → союзник → враг) ---\n";
  print_core_combat_snapshot(gg, ally, enemy);
  std::cout << "  Что было за раунд: " << last_events << "\n";
}

// -----------------------------------------------------------------------------
// print_lust_change — визуализация прироста Lust и причин.
// -----------------------------------------------------------------------------
static void print_lust_change(const std::string& who, int before, int after,
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

// -----------------------------------------------------------------------------
// read_gg_menu_choice
// Зачем: единственная точка чтения меню ГГЖ из консоли — отделена от игровой логики раунда.
// Принимает: ничего (читает std::cin построчно).
// Возвращает:
//   1 — атака по врагу; 2 — защита союзника; 3 — ожидание;
//   0 — штатный выход или конец потока ввода (EOF);
//   -1 — пустая строка или неподходящий символ (нужен повтор запроса в вызывающем цикле).
// Меняет: только позицию в потоке ввода.
// Когда: в начале каждого раунда в main, пока игрок не введёт допустимую команду или не выйдет.
// -----------------------------------------------------------------------------
static int read_gg_menu_choice() {
  std::string line;
  if (!std::getline(std::cin, line)) {
    // EOF или ошибка потока — трактуем как запрос выхода, чтобы не зациклиться.
    return 0;
  }
  if (line.empty()) {
    // Пустая строка — не меняем состояние боя, просим ввод снова.
    return -1;
  }
  const char c = line[0];
  if (c == '0') {
    return 0;
  }
  if (c == '1' || c == '2' || c == '3') {
    return c - '0';
  }
  // Любой другой первый символ — повтор запроса (не считаем это выходом).
  return -1;
}

// -----------------------------------------------------------------------------
// enemy_decides_to_attack — AI врага: агрессия и контроль только у врага.
// -----------------------------------------------------------------------------
static bool enemy_decides_to_attack(const Unit& enemy) {
  return total_aggression(enemy) > total_control(enemy);
}

// =============================================================================
// main: инициализация и цикл боя.
//
// Внешний while — раунды, пока живы союзник и враг (поражение — падение союзника).
// Порядок полуходов в раунде (явно зафиксирован):
//   1) Ход ГГЖ — ввод игрока (атака по врагу / защита союзника / ожидание / выход).
//   2) Ход союзника — всегда автоматическая атака; цель от Lust и порога (срыв → броня ГГЖ).
//   3) Ход врага — AI; урон по союзнику, при защите ГГЖ урон снижается.
// Затем сброс стойки защиты ГГЖ и сводка раунда.
//
// Завершение: победа (враг 0 HP), поражение (союзник 0 HP), выход по «0» или EOF.
// После выхода из while — return 0; процесс завершается без ожидания ввода.
// =============================================================================
int main() {
  setup_console_utf8();

  const int battle_intensity = 1;
  const int attack_damage = 12;

  GgHeroine gg;
  gg.name = "ГГЖ — Лира";
  gg.armor = ArmorState::Intact;
  gg.guarding = false;

  Unit ally;
  ally.name = "Союзник — Кейн";
  ally.hp = 80;
  ally.lust = 55;
  ally.armor = ArmorState::Intact;
  ally.lust_redirect_threshold = 70;
  ally.parts.push_back({"Железная голова", 15, 5, 1, 2, 1, 4, 1});
  ally.parts.push_back({"Искажённая рука", 25, 0, 2, 4, 3, 8, 2});
  ally.parts.push_back({"Сдерживающий пояс", 0, 30, 0, 0, 0, 0, 0});

  Unit enemy;
  enemy.name = "Враг — Сломленный страж";
  enemy.hp = 55;
  enemy.lust = 5;
  enemy.armor = ArmorState::Intact;
  enemy.lust_redirect_threshold = 0;
  enemy.parts.push_back({"Костяной кулак", 22, 4, 1, 3, 1, 3, 1});
  enemy.parts.push_back({"Ржавый нагрудник", 8, 2, 1, 2, 0, 2, 0});

  std::cout << "=== Пошаговый бой: ГГЖ (вы), союзник (авто), враг ===\n";
  std::cout << "Управление ГГЖ: 0 — выход, 1 — атака по врагу, 2 — защита союзника "
               "(ослабляет удар врага по союзнику в этом раунде), 3 — ждать\n";
  std::cout << "Союзник каждый раунд атакует сам; при Lust >= " << ally.lust_redirect_threshold
            << " — срыв на броню ГГЖ.\n\n";

  int round = 0;

  // ---------------------------------------------------------------------------
  // Главный цикл боя: одна итерация = один полный раунд из трёх полуходов.
  // Условие продолжения: враг и союзник живы (ГГЖ в MVP не имеет собственного HP).
  // ---------------------------------------------------------------------------
  while (ally.hp > 0 && enemy.hp > 0) {
    ++round;
    std::cout << "────────── Раунд " << round << " ──────────\n";
    std::cout << "Состояние в начале раунда:\n";
    print_battle_header(gg, ally, enemy);
    std::cout << "\n";

    // round_log накапливает краткое текстовое описание для итоговой сводки раунда.
    std::string round_log;

    // ========== Полуход 1: ГГЖ (игрок) ==========
    std::cout << ">>> Ход ГГЖ: «" << gg.name << "»\n";

    int choice = -1;
    while (choice == -1) {
      std::cout << "  Действие ГГЖ? [0 выход] [1 атака] [2 защита союзника] [3 ждать]: ";
      std::cout.flush();
      choice = read_gg_menu_choice();
      if (choice == -1) {
        std::cout << "  (неверный ввод — введите 0, 1, 2 или 3 и Enter)\n";
      }
    }

    if (choice == 0) {
      std::cout << "\n*** Выход: игра завершена по выбору игрока (или ввод недоступен). ***\n";
      break;
    }

    // По умолчанию считаем, что героиня не готовила защиту — перезапишем при выборе «2».
    gg.guarding = false;

    if (choice == 1) {
      // Прямой урон по врагу от ГГЖ — отдельно от автоматической атаки союзника.
      const int dmg = deal_damage(enemy, attack_damage);
      std::cout << "  ГГЖ атакует врага: урон по «" << enemy.name << "»: " << dmg << "\n";
      round_log += "ГГЖ атаковала врага; ";
    } else if (choice == 2) {
      // Стойка действует до конца полухода врага: снижает урон по союзнику, не по ГГЖ.
      gg.guarding = true;
      std::cout << "  ГГЖ прикрывает союзника: удар врага по союзнику в этом раунде будет слабее.\n";
      round_log += "ГГЖ защищала союзника; ";
    } else {
      std::cout << "  ГГЖ ждёт.\n";
      round_log += "ГГЖ ждала; ";
    }

    print_state_after_partial_turn("ГГЖ", gg, ally, enemy, round_log);

    if (enemy.hp <= 0) {
      round_log += "враг повержен.";
      print_round_summary(gg, ally, enemy, round_log);
      std::cout << "\n*** Победа: враг уничтожен. ***\n";
      break;
    }

    // ========== Полуход 2: союзник (автоматически атакует) ==========
    std::cout << "\n>>> Ход союзника (авто): «" << ally.name << "»\n";
    std::cout << "  Союзник по умолчанию атакует; цель — по Lust и порогу (не по агр./контр.).\n";

    const bool will_redirect =
        game::allied_attack_redirects_to_gg_armor(ally.lust, ally.lust_redirect_threshold);
    const bool gg_armor_already_broken = (gg.armor == ArmorState::Broken);

    resolve_allied_automatic_attack(ally, gg, enemy, attack_damage);

    if (!will_redirect) {
      round_log += "союзник ударил по врагу; ";
    } else if (gg_armor_already_broken) {
      round_log += "срыв: удар по уже сломанной броне ГГЖ; ";
    } else {
      round_log += "срыв: удар по броне ГГЖ; ";
    }

    const bool ally_attacked = true;
    {
      const int lust_before = ally.lust;
      std::vector<std::string> reasons;
      const int gain = apply_lust_for_turn(ally, battle_intensity, ally_attacked, reasons);
      print_lust_change(ally.name, lust_before, ally.lust, reasons);
      if (gain == 0 && reasons.empty()) {
        std::cout << "    (изменений Lust нет при текущих частях и действии)\n";
      }
    }

    print_state_after_partial_turn("союзника", gg, ally, enemy, round_log);

    if (enemy.hp <= 0) {
      round_log += "враг повержен.";
      print_round_summary(gg, ally, enemy, round_log);
      std::cout << "\n*** Победа: враг уничтожен. ***\n";
      break;
    }

    // ========== Полуход 3: враг (AI) ==========
    std::cout << "\n>>> Ход врага: «" << enemy.name << "»\n";
    const bool enemy_attacks = enemy_decides_to_attack(enemy);
    bool enemy_did_attack = false;
    std::string enemy_turn_desc;

    if (enemy_attacks) {
      enemy_did_attack = true;
      const int dmg = deal_damage_from_enemy_to_ally(ally, attack_damage, gg.guarding);
      std::cout << "  Враг (агрессия > контроль): атака по союзнику, урон: " << dmg;
      if (gg.guarding) {
        std::cout << " (учтена защита ГГЖ)";
      }
      std::cout << "\n";
      round_log += "враг атаковал союзника; ";
      enemy_turn_desc =
          "враг атаковал союзника (урон может быть снижен защитой ГГЖ).";
    } else {
      std::cout << "  Враг ждёт (агрессия не выше контроля).\n";
      round_log += "враг ждал; ";
      enemy_turn_desc = "враг ждал (AI: агрессия не выше контроля).";
    }

    {
      const int lust_before = enemy.lust;
      std::vector<std::string> reasons;
      const int gain = apply_lust_for_turn(enemy, battle_intensity, enemy_did_attack, reasons);
      print_lust_change(enemy.name, lust_before, enemy.lust, reasons);
      if (gain == 0 && reasons.empty()) {
        std::cout << "    (изменений Lust нет при текущих частях и действии)\n";
      }
    }

    print_state_after_partial_turn("врага", gg, ally, enemy, enemy_turn_desc);

    // Стойка защиты ГГЖ действует только один раунд — сбрасываем после ответа врага.
    gg.guarding = false;

    if (ally.hp <= 0) {
      round_log += "союзник погиб.";
      print_round_summary(gg, ally, enemy, round_log);
      std::cout << "\n*** Поражение: союзник погиб. ***\n";
      break;
    }

    round_log += "раунд завершён.";
    print_round_summary(gg, ally, enemy, round_log);
    std::cout << "\n";
  }

  return 0;
}
