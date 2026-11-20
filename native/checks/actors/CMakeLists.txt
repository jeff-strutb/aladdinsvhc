cmake_minimum_required(VERSION 3.20)
project(aladdin_actor_check C)
add_subdirectory(${NSHVC_ROOT} nshvc)
include(${NSHVC_ROOT}/cmake/NshvcGame.cmake)
set(game ${CMAKE_CURRENT_SOURCE_DIR}/../../aladdin)
# the whole game but its entry (system/main.c): the kinds' code reaches every area
file(GLOB_RECURSE game_sources CONFIGURE_DEPENDS ${game}/src/*.c)
list(FILTER game_sources EXCLUDE REGEX "/src/system/main\\.c$")
nshvc_game(aladdin_actor_check SOURCES check.c ${game_sources} ASSETS ../../aladdin/assets TITLE "Aladdin actor check")
target_include_directories(aladdin_actor_check PRIVATE ${game}/src)
target_compile_options(aladdin_actor_check PRIVATE -std=gnu11 -Wall -Wextra)
# the calls the check answers from the dump (check.c's __wrap_ functions)
target_link_options(aladdin_actor_check PRIVATE -Wl,--wrap=SoundQueueAdd -Wl,--wrap=PaletteLoad -Wl,--wrap=TileSlotFill
                    -Wl,--wrap=TaskCreate -Wl,--wrap=task_busy -Wl,--wrap=TileSlotClaim)
