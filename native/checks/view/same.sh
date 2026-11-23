#!/bin/sh
# same.sh BASE_BIN NEW_BIN [VIEW]: the view's check (docs/aladdin/native/VIEW.md section 8). Runs the no-input boot
# (11,000 updates) and every play in native/checks/plays (p*.txt, boss16-2hits.txt; each to its last key + 600) on two
# headless builds, the new one with ALADDIN_VIEW=VIEW (default 256x224), and compares the developer traces update by
# update: the player's line, every live actor with its own bytes, the video fields. SAME_NEW_ENV sets environment variables
# for the new build alone, SAME_BASE_ENV for the base alone (the runahead check, FRAMERATE.md 9.8: the same build twice,
# SAME_BASE_ENV="ALADDIN_RUNAHEAD=0 ALADDIN_DEV_STATE_HASH=1" and SAME_NEW_ENV="ALADDIN_RUNAHEAD=1 ALADDIN_DEV_STATE_HASH=1"). At 256x224 every run must be the
# same; the first differing line of each run that differs is printed. Run from the repository root.
# Exit 0: all the same and every run exited 0. Exit 1: some differ or failed.
set -u
base=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
new=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
view=${3:-256x224}
root=$(pwd)
out=${SAME_OUT:-$root/build/aladdin/native/same}
rm -rf "$out"; mkdir -p "$out"
runs=$out/runs.txt
: > "$runs"
echo "boot 11000 -" >> "$runs"
for f in native/checks/plays/p*.txt native/checks/plays/boss16-2hits.txt; do
  last=$(awk '$1 ~ /^[0-9]+$/ { n = $1 } END { print n + 600 }' "$f")
  echo "$(basename "$f" .txt) $last $root/$f" >> "$runs"
done
one() {   # name updates input
  for side in base new; do
    if [ $side = base ]; then bin=$base; v=256x224; else bin=$new; v=$view; fi
    if [ "$3" = - ]; then in=""; else in="--input $3"; fi
    res=$(dirname "$bin")/../Resources   # a macOS build keeps cart.dat in the bundle; the headless host reads PORT_RESDIR
    [ -f "$res/cart.dat" ] || res=$(dirname "$bin")
    if [ $side = base ]; then extra=${SAME_BASE_ENV:-}; else extra=${SAME_NEW_ENV:-}; fi   # SAME_BASE_ENV, SAME_NEW_ENV="NAME=VALUE ...": environment for one side only (the runahead check)
    (cd "$(dirname "$bin")" && env PORT_RESDIR=$res ALADDIN_VIEW=$v ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 $extra \
      "$bin" --headless --updates "$2" $in > "$out/$1.$side.out" 2> "$out/$1.$side.err")
    echo $? > "$out/$1.$side.exit"
  done
  if [ "$(cat "$out/$1.base.exit")" != 0 ] || [ "$(cat "$out/$1.new.exit")" != 0 ]; then
    echo "DIFFERS $1: a run failed (exit $(cat "$out/$1.base.exit") / $(cat "$out/$1.new.exit")): $(head -c 200 "$out/$1.new.err" | tr '\n' ' ')"
  elif cmp -s "$out/$1.base.err" "$out/$1.new.err" && cmp -s "$out/$1.base.out" "$out/$1.new.out"; then
    echo "same    $1"
  else
    echo "DIFFERS $1 (exit $(cat "$out/$1.base.exit") / $(cat "$out/$1.new.exit")): first difference at line $(cmp "$out/$1.base.err" "$out/$1.new.err" 2>/dev/null | sed -n 's/.* line \([0-9]*\).*/\1/p')"
  fi
}
export -f one 2>/dev/null
jobs=${SAME_JOBS:-4}
fail=0
while read -r name last input; do
  one "$name" "$last" "$input" &
  while [ "$(jobs -r | wc -l)" -ge "$jobs" ]; do sleep 1; done
done < "$runs" > "$out/result.txt"
wait
sort "$out/result.txt"
grep -q DIFFERS "$out/result.txt" && fail=1
echo "runs $(wc -l < "$out/result.txt"), differing $(grep -c DIFFERS "$out/result.txt"); traces in $out"
exit $fail
