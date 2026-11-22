#!/bin/sh
# wide.sh NEW_BIN VIEW: the check at a view wider or taller than 256x224 (docs/aladdin/native/VIEW.md section 8). The game's
# camera and triggers are the original's, but the spawn, despawn and in-view tests cover the whole view, so the game traces differ
# from the original's there. This runs the no-input boot and every play (as same.sh does) headless at VIEW with
# ALADDIN_DEV_TRIGGERS=1 and requires: exit 0 for every run, no abort or trap text on stderr, and that every trigger test that
# turned true (TRIGGER lines: player-to-actor or screen-edge distance, with the table's limits) was inside its original limits
# (trigger_check.py). Run from the repository root. Exit 0: all runs passed.
set -u
new=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
view=$2
root=$(pwd)
out=${WIDE_OUT:-$root/build/aladdin/native/wide-$view}
rm -rf "$out"; mkdir -p "$out"
runs=$out/runs.txt
echo "boot 11000 -" > "$runs"
for f in native/checks/plays/p*.txt native/checks/plays/boss16-2hits.txt; do
  last=$(awk '$1 ~ /^[0-9]+$/ { n = $1 } END { print n + 600 }' "$f")
  echo "$(basename "$f" .txt) $last $root/$f" >> "$runs"
done
one() {   # name updates input
  if [ "$3" = - ]; then in=""; else in="--input $3"; fi
  res=$(dirname "$new")/../Resources
  [ -f "$res/cart.dat" ] || res=$(dirname "$new")
  (cd "$(dirname "$new")" && PORT_RESDIR=$res ALADDIN_VIEW=$view ALADDIN_DEV_TRIGGERS=1 \
    "$new" --headless --updates "$2" $in > "$out/$1.out" 2> "$out/$1.err")
  code=$?
  if [ $code != 0 ]; then echo "FAILED  $1: exit $code: $(grep -v '^TRIGGER' "$out/$1.err" | head -c 200 | tr '\n' ' ')"
  elif grep -v '^TRIGGER' "$out/$1.err" | grep -qiE 'trap|abort|assert|overrun|stack|fault'; then echo "FAILED  $1: $(grep -v '^TRIGGER' "$out/$1.err" | grep -iE 'trap|abort|assert|overrun|stack|fault' | head -1)"
  else echo "ok      $1"; fi
}
jobs=${WIDE_JOBS:-3}
while read -r name last input; do
  one "$name" "$last" "$input" &
  while [ "$(jobs -r | wc -l)" -ge "$jobs" ]; do sleep 1; done
done < "$runs" > "$out/result.txt"
wait
sort "$out/result.txt"
echo "runs $(wc -l < "$out/result.txt"), failed $(grep -c FAILED "$out/result.txt"); logs in $out"
python3 "$root/native/checks/view/trigger_check.py" "$out" || exit 1
grep -q FAILED "$out/result.txt" && exit 1
exit 0
