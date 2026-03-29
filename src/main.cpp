#include "raylib.h"

int main() {
  constexpr int screen_width = 800;
  constexpr int screen_height = 450;

  InitWindow(screen_width, screen_height, "raylib starter");
  SetTargetFPS(60);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Hello, raylib", 190, 200, 20, LIGHTGRAY);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
