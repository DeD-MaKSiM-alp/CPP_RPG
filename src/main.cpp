// =============================================================================
// main.cpp — точка входа: консоль, демо-состояние, главный цикл боя.
//
// Ответственность:
//   • вызов battle_io::setup_console_utf8 и приветствие;
//   • создание начального состояния через battle_round::make_demo_battle();
//   • цикл while (союзник и враг живы): номер раунда, battle_round::run_single_round(...);
//   • выход из цикла, если раунд вернул не ContinueBattle (победа, поражение, выход игрока).
//
// Сценарий одного раунда (порядок полуходов, меню, round_log) не дублируется здесь —
// он инкапсулирован в battle_round::run_single_round (см. battle_round.cpp).
// =============================================================================

#include "battle_io.hpp"
#include "battle_round.hpp"

int main() {
  battle_io::setup_console_utf8();

  const int battle_intensity = 1;
  const int attack_damage = 12;

  // Одна структура вместо трёх отдельных переменных — то же состояние, что раньше в main.
  battle_round::DemoBattleState state = battle_round::make_demo_battle();

  battle_io::print_game_intro(state.ally.lust_redirect_threshold);

  int round = 0;

  // Пока оба «якоря» живы — враг и союзник. Исходы «победа/поражение/выход» обрабатываются
  // внутри run_single_round (печать сообщений и возврат kind).
  while (state.ally.hp > 0 && state.enemy.hp > 0) {
    ++round;
    const battle_round::SingleRoundResult rr = battle_round::run_single_round(
        state.gg, state.ally, state.enemy, battle_intensity, attack_damage, round);

    if (rr.kind != battle_round::RoundOutcomeKind::ContinueBattle) {
      break;
    }
  }

  return 0;
}
