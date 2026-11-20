cmake_minimum_required(VERSION 3.20)
project(aladdin_actor_check C)
add_subdirectory(${NSHVC_ROOT} nshvc)
include(${NSHVC_ROOT}/cmake/NshvcGame.cmake)
set(game ${CMAKE_CURRENT_SOURCE_DIR}/../../aladdin)
# the whole game but its entry (system/main.c): the kinds' code reaches every area
file(GLOB_RECURSE game_sources CONFIGURE_DEPENDS ${game}/src/*.c)
list(FILTER game_sources EXCLUDE REGEX "/src/system/main\\.c$")
# the game's state outside the actors, as C initialisers, from state.h (make_state_csv.py's CSV, made afresh)
find_program(PYTHON3 python3 REQUIRED)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/world_fields.inc
  COMMAND ${PYTHON3} ${CMAKE_CURRENT_SOURCE_DIR}/gen_world.py ${game} ${CMAKE_CURRENT_BINARY_DIR}/world_fields.inc
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gen_world.py ${game}/tools/make_state_csv.py ${game}/src/system/state.h VERBATIM)
nshvc_game(aladdin_actor_check SOURCES check.c ${CMAKE_CURRENT_BINARY_DIR}/world_fields.inc ${game_sources} ASSETS ../../aladdin/assets TITLE "Aladdin actor check")
target_include_directories(aladdin_actor_check PRIVATE ${game}/src ${CMAKE_CURRENT_BINARY_DIR})
target_compile_options(aladdin_actor_check PRIVATE -std=gnu11 -Wall -Wextra)
# the calls the check answers from the dump (check.c's __wrap_ functions)
target_link_options(aladdin_actor_check PRIVATE -Wl,--wrap=SoundQueueAdd -Wl,--wrap=PaletteLoad -Wl,--wrap=TileSlotFill
                    -Wl,--wrap=TaskCreate -Wl,--wrap=task_busy -Wl,--wrap=TileSlotClaim)
