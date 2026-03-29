cmake_minimum_required(VERSION 3.24)

project(raylib_starter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_executable(test_game_logic tests/test_game_logic.cpp)
target_include_directories(test_game_logic PRIVATE ${CMAKE_SOURCE_DIR}/src)

enable_testing()
add_test(NAME game_logic COMMAND test_game_logic)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
