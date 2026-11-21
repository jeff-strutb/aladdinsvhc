#!/bin/sh
# make.sh PART STAGE LAST_UPDATE [DEPTH]: plays level PART STAGE (the level select by keys, then the developer's player, dev_bot.c,
# from update 4100) with the headless build and writes native/checks/plays/pPART-STAGE.txt: an --input file (the select's keys,
# then the player's) that run.sh replays. ALADDIN_CPU_RUN names the CPU queue command. Run from the repository root with the
# headless build in build/aladdin/native/headless. The player's settings pass through the environment (ALADDIN_DEV_BOT_SEG, _GOAL,
# _WAY, _GE, ...: system/dev_bot.c). With ALADDIN_DEV_BOT_GE set (the archive search) the player finds the whole way to the level's
# exit first, so the file is cut 120 updates after the exit (the fade) and LAST_UPDATE need only be later than that; J is the
# CPU slots to claim (default 2; the archive search needs its copies + 1). With END set the file is cut at that update instead.
set -e
part=$1 stage=$2 last=$3 depth=${4:-1}
root=$(pwd)
plays=$root/native/checks/plays
tmp=$(mktemp -d)
python3 "$plays/level_select_input.py" "$part" "$stage" "$tmp/pre.txt"
cd "$root/build/aladdin/native/headless"
ALADDIN_DEV_BOT=4100 ALADDIN_DEV_BOT_DEPTH=$depth ALADDIN_DEV_BOT_OUT="$tmp/bot.txt" ALADDIN_DEV_TRACE=${TRACE:-400} \
  "${ALADDIN_CPU_RUN:?set ALADDIN_CPU_RUN to the CPU queue command}" -j ${J:-2} -m ${M:-3G} -- ./aladdin --headless --updates "$last" --input "$tmp/pre.txt" 2>&1 | grep -v '^WARNING\|^Dropped\|^  actor\|^u=[0-3]' | tee "$tmp/log.txt"
if [ -z "$END" ]; then
  n=$(sed -n 's/.*found at update [0-9]*: \([0-9]*\) segments.*/\1/p' "$tmp/log.txt" | tail -1)
  seg=${ALADDIN_DEV_BOT_SEG:-16}
  [ -n "$n" ] && END=$((4100 + n * seg + 120))
fi
awk -v e="${END:-999999999}" '$1<=e' "$tmp/bot.txt" > "$tmp/cut.txt"
cat "$tmp/pre.txt" "$tmp/cut.txt" > "$plays/p$part-$stage.txt"
rm -rf "$tmp"
echo "wrote $plays/p$part-$stage.txt (cut at ${END:-the end})"
