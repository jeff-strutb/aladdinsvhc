#!/bin/sh
# run.sh FILE UPDATES [SHOTS_DIR [EVERY [FROM]]]: replays a play (an --input file) with the Linux build (pictures every
# EVERY updates, default 120, from FROM, default 4000, as DIR/uNNNNNN.ppm) or, with no SHOTS_DIR, the headless build;
# prints the player's line every 200 updates (ALADDIN_DEV_TRACE) and whatever the game prints. Run from the repository root.
set -e
file=$(readlink -f "$1") last=$2 shots=$3
root=$(pwd)
if [ -n "$shots" ]; then
  mkdir -p "$shots"
  cd "$root/build/aladdin/native/linux"
  export ALADDIN_DEV_SHOTS=$shots ALADDIN_DEV_SHOT_EVERY=${4:-120} ALADDIN_DEV_SHOT_FROM=${5:-4000}
else
  cd "$root/build/aladdin/native/headless"
fi
ALADDIN_DEV_CHEAT=1 ALADDIN_DEV_TRACE=${TRACE:-200} "${ALADDIN_CPU_RUN:?set ALADDIN_CPU_RUN to the CPU queue command}" -j 2 -m 3G -- ./aladdin --headless --updates "$last" --input "$file" 2>&1 | grep -v "^WARNING\|^Dropped"
