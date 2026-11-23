#!/bin/bash
# latency.sh BIN KEYLINES [BETWEEN]: how many updates after the press the first picture differs (FRAMERATE.md 9.2, 9.8). BIN is
# the Metal build, run headless over the level 0 play's start (native/checks/plays/p0-0.txt up to the start of play, update
# 3903) then KEYLINES (for example "4100 key=27\n4200": Right pressed at update 4100, let go at 4200), against the same run without
# the keys. A picture written at update N shows what was drawn after update N - 1. Prints the first update at which
# actor 0's state differs from the run without the keys and the first picture file that differs, for ALADDIN_RUNAHEAD 0 and 1.
set -u
bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1"); keys=$2; at=${3:-0}
root=$(cd "$(dirname "$0")/../../.." && pwd)
res=$(dirname "$bin")/../Resources
out=${LATENCY_OUT:-/tmp/latency-ra}
rm -rf "$out"; mkdir -p "$out"
awk '$1 <= 3903' "$root/native/checks/plays/p0-0.txt" > "$out/pre.txt"
cp "$out/pre.txt" "$out/with.txt"; printf "$keys\n" >> "$out/with.txt"
for m in 0 1; do
  for r in pre with; do
    d=$out/$m-$r; mkdir -p "$d"
    (cd "$(dirname "$bin")" && PORT_RESDIR=$res ALADDIN_RUNAHEAD=$m ALADDIN_DEV_BETWEEN_AT=$at ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=1 \
      ALADDIN_DEV_SHOTS=$d ALADDIN_DEV_SHOT_EVERY=1 ALADDIN_DEV_SHOT_FROM=4090 "$bin" --headless --updates 4240 --input "$out/$r.txt" \
      > /dev/null 2> "$out/$m-$r.err") || { echo "run failed"; exit 1; }
    (cd "$d" && for f in u*.ppm; do echo "$f $(md5 -q "$f")"; done) > "$out/$m-$r.md5"
    rm -rf "$d"
    awk '/^u=/ { split($1, a, "="); u = a[2] } /^  actor 0 / { $1 = ""; $2 = ""; print u, $0 }' "$out/$m-$r.err" > "$out/$m-$r.a0"
  done
  state=$(paste -d'|' "$out/$m-pre.a0" "$out/$m-with.a0" | awk -F'|' '{ x = $1; y = $2; sub(/^[0-9]+ /, "", x); sub(/^[0-9]+ /, "", y); if (x != y) { split($1, a, " "); print a[1]; exit } }')
  pic=$(paste "$out/$m-pre.md5" "$out/$m-with.md5" | awk '$2 != $4 { print $1; exit }')
  echo "ALADDIN_RUNAHEAD=$m: first change in actor 0's state at update ${state:-none}; first picture that differs: ${pic:-none}"
done
