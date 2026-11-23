#!/bin/sh
# behind.sh BASE_BIN NEW_BIN [EVERY]: the pictures-between-updates check, stage R1 (docs/aladdin/native/FRAMERATE.md 6).
# Headless pictures are at between 0, the update before's whole picture, so NEW's picture written after update u+1
# must equal BASE's after update u, byte for byte. Runs the no-input boot (11,000 updates) and every play (to its last
# key + 600) on two Metal builds, a picture every EVERY updates (default 25; BASE's at updates 25k, NEW's at 25k + 1,
# by ALADDIN_DEV_SHOT_PHASE), compares each pair by its hash, and keeps no pictures. Both builds must have the phase
# (system/dev_bot.c); a BASE without it is given the phase 0 it has anyway. A NEW on engine 6.10 passes its places'
# fractions, which a picture at between 0 shows as well (the update before's places between pixels): run it with
# ALADDIN_DEV_WHOLE_PIXELS=1 in the environment, which this script passes on. Run from the repository root. Exit 0: every pair equal, every run exit 0.
set -u
base=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
new=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
every=${3:-25}
shift_by=${BEHIND_SHIFT:-1}       # NEW's picture u + shift_by against BASE's u: 1 for pictures between updates, 0 for two builds that must be the same
root=$(pwd)
out=${BEHIND_OUT:-$root/build/aladdin/native/behind}
rm -rf "$out"; mkdir -p "$out"
fail=0
run() {   # side bin from name updates input
  d=$out/$4.$1
  mkdir -p "$d"
  if [ "$6" = - ]; then in=""; cheat=0; else in="--input $6"; cheat=1; fi   # the plays select their level with the cheat set from the start
  (cd "$(dirname "$2")" && PORT_RESDIR=$(dirname "$2")/../Resources ALADDIN_DEV_CHEAT=$cheat ALADDIN_DEV_SHOTS=$d ALADDIN_DEV_SHOT_EVERY=$every \
    ALADDIN_DEV_SHOT_PHASE=$3 "$2" --headless --updates "$5" $in > "$d.out" 2>&1)
  echo $? > "$d.exit"
  (cd "$d" && for f in u*.ppm; do [ -f "$f" ] && echo "$(md5 -q "$f") $f"; done) > "$d.md5"
  rm -rf "$d"
}
{ echo "boot 11000 -"
  for f in native/checks/plays/p*.txt native/checks/plays/boss16-2hits.txt; do
    echo "$(basename "$f" .txt) $(awk '$1 ~ /^[0-9]+$/ { n = $1 } END { print n + 600 }' "$f") $root/$f"
  done; } > "$out/runs.txt"
while read -r name last input; do
  run base "$base" 0 "$name" "$last" "$input" &
  run new "$new" "$shift_by" "$name" "$last" "$input" &
  wait
  # BASE's picture uN against NEW's u(N+1), joined by number
  awk 'NR == FNR { sub(/^u/, "", $2); sub(/\.ppm$/, "", $2); b[$2 + s] = $1; next }
       { n = $2; sub(/^u/, "", n); sub(/\.ppm$/, "", n); n += 0; if (n in b) print b[n], "u" (n - s), $1, $2 }' s="$shift_by" \
    "$out/$name.base.md5" "$out/$name.new.md5" > "$out/$name.pairs"
  pairs=$(wc -l < "$out/$name.pairs" | tr -d ' ')
  differ=$(awk '$1 != $3 { n++ } END { print n + 0 }' "$out/$name.pairs")
  first=$(awk '$1 != $3 { print $2 " / " $4; exit }' "$out/$name.pairs")
  ex="$(cat "$out/$name.base.exit")/$(cat "$out/$name.new.exit")"
  if [ "$differ" = 0 ] && [ "$ex" = 0/0 ] && [ "$pairs" -gt 0 ]; then
    echo "same    $name: $pairs pairs"
  else
    echo "DIFFERS $name: $differ of $pairs pairs differ (exit $ex), first $first"; fail=1
  fi
done < "$out/runs.txt"
exit $fail
