#!/bin/sh
# make.sh PART STAGE LAST_UPDATE [DEPTH]: plays level PART STAGE (the level select by keys, then the developer's player ALADDIN_CPU_RUN names the CPU queue command.
# dev_bot.c from update 4100) with the headless build and writes native/checks/plays/pPART-STAGE.txt: an --input file
# (the select's keys, then the player's) that run.sh replays. Run from the repository root with the headless build in
# build/aladdin/native/headless. ALADDIN_DEV_BOT_SEG and _GOAL (dev_bot.c) pass through.
set -e
part=$1 stage=$2 last=$3 depth=${4:-1}
root=$(pwd)
plays=$root/native/checks/plays
tmp=$(mktemp -d)
python3 "$plays/level_select_input.py" "$part" "$stage" "$tmp/pre.txt"
cd "$root/build/aladdin/native/headless"
ALADDIN_DEV_BOT=4100 ALADDIN_DEV_BOT_DEPTH=$depth ALADDIN_DEV_BOT_OUT="$tmp/bot.txt" ALADDIN_DEV_TRACE=${TRACE:-400} \
  "${ALADDIN_CPU_RUN:?set ALADDIN_CPU_RUN to the CPU queue command}" -j 2 -m 2G -- ./aladdin --headless --updates "$last" --input "$tmp/pre.txt" 2>&1 | grep -v '^WARNING\|^Dropped\|^  actor\|^u=[0-3]'
cat "$tmp/pre.txt" "$tmp/bot.txt" > "$plays/p$part-$stage.txt"
rm -rf "$tmp"
echo "wrote $plays/p$part-$stage.txt"
