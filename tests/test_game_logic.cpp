// Минимальные проверки правил срыва союзника (Lust vs порог). Код возврата 0 — успех.

#include "game_rules.hpp"

#include <cstdlib>

int main() {
  using game::allied_attack_redirects_to_gg_armor;

  // Ниже порога — бьём врага, срыва нет.
  if (allied_attack_redirects_to_gg_armor(69, 70)) {
    return 1;
  }
  // Ровно на пороге — уже срыв (сравнение включительно).
  if (!allied_attack_redirects_to_gg_armor(70, 70)) {
    return 1;
  }
  // Выше порога — срыв.
  if (!allied_attack_redirects_to_gg_armor(100, 70)) {
    return 1;
  }

  return 0;
}
