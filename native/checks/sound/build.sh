#!/bin/sh
# build.sh: the sound checks into build/aladdin/sound-check/. Run from the
# repository root after the headless game's build (docs/aladdin/native/BUILD.md),
# whose engine library sound_play and dsp_replay link.
#   sound_check    the native driver against the original's, tick by tick
#   sound_play     the native driver on the engine, to the original's schedule
#   dsp_replay     the original's own DSP writes on the engine
#   sound_compare  two 32,040 Hz sounds compared
# ALADDIN_CPU_RUN: the machine's CPU queue (every compile goes through it).
# NSHVC_ROOT: the engine's checkout (API 6.6 or later); ENGINE_LIB: its
# headless library.
set -e
NSHVC_ROOT=${NSHVC_ROOT:-/home/strutb/projects/strutb/nativeshvc}
OUT=${OUT:-build/aladdin/sound-check}
ENGINE_LIB=${ENGINE_LIB:-build/aladdin/native/headless/nshvc/libnshvc.a}
N=native/aladdin/src
GAME="$N/audio/driver.c $N/audio/music.c $N/audio/sound.c $N/data/data.c $N/data/sound_lists.c $N/system/state.c"
CC="gcc -std=gnu11 -O2 -Wall -Wextra -I$N -I$NSHVC_ROOT/include"
mkdir -p "$OUT"
[ -n "$ALADDIN_CPU_RUN" ] || { echo "build.sh: set ALADDIN_CPU_RUN to the machine's CPU queue"; exit 2; }
R="$ALADDIN_CPU_RUN -j 1 -m 1G -- timeout 300"
$R $CC -o "$OUT/sound_check" native/checks/sound/sound_check.c $GAME
$R $CC -o "$OUT/sound_play" native/checks/sound/sound_play.c $GAME "$ENGINE_LIB" -lm -lpthread -ldl
$R $CC -o "$OUT/dsp_replay" native/checks/sound/dsp_replay.c $GAME "$ENGINE_LIB" -lm -lpthread -ldl
$R $CC -o "$OUT/sound_compare" native/checks/sound/sound_compare.c -lm
