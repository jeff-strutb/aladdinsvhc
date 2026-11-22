#!/bin/sh
# fill_shots.sh APP OUTDIR VIEW [JOBS]: pictures of every level at its start, at its right end, and at the rooms the
# camera locks (levels 3, $0D, $10, $11's arena), at the view VIEW (WxH), for looking at the fill past a level's edges
# (video/level_fill.c). APP is a window build's executable (macOS: aladdin.app/Contents/MacOS/aladdin); a window build
# draws in real time, so JOBS runs go at once (default 8), each about 70 seconds. Run from the repository root.
# A picture is OUTDIR/VIEW-NAME-uNNNNNN.png, the picture only (the window's black bars cut off).
set -u
app=$1 out=$2 view=$3 jobs=${4:-8}
root=$(pwd)
res=$(cd "$(dirname "$app")/../Resources" && pwd)
mkdir -p "$out"
tmp=$(mktemp -d)
# name part-stage x,y [updates first-shot every]
cat > "$tmp/list.txt" <<'LIST'
L00-start 0-0 - 4070 4060 1000
L00-end 0-0 3264,430 4070 4060 1000
L01-start 0-1 - 4070 4060 1000
L01-end 0-1 2496,680 4070 4060 1000
L02-start 0-2 - 4070 4060 1000
L02-end 0-2 4288,648 4070 4060 1000
L03-start 0-3 - 4070 4060 1000
L03-end 0-3 1728,428 4070 4060 1000
L03-room 0-3 1264,432,1024,256 4260 4100 80
L04-start 1-0 - 4070 4060 1000
L04-end 1-0 4032,392 4070 4060 1000
L05-start 1-1 - 4070 4060 1000
L05-end 1-1 3776,376 4070 4060 1000
L06-start 2-0 - 4070 4060 1000
L06-end 2-0 4544,128 4070 4060 1000
L07-start 2-1 - 4070 4060 1000
L07-end 2-1 8128,128 4070 4060 1000
L08-start 3-0 - 4070 4060 1000
L08-end 3-0 4032,136 4070 4060 1000
L09-start 3-1 - 4070 4060 1000
L09-end 3-1 4032,136 4070 4060 1000
L0A-start 3-2 - 4070 4060 1000
L0A-end 3-2 3264,1880 4070 4060 1000
L0B-start 4-0 - 4070 4060 1000
L0B-end 4-0 2496,424 4070 4060 1000
L0C-start 4-1 - 4070 4060 1000
L0C-end 4-1 3008,680 4070 4060 1000
L0D-start 4-2 - 4070 4060 1000
L0D-end 4-2 3008,168 4070 4060 1000
L0D-room 4-2 2544,120 4260 4100 80
L0E-start 5-0 - 4070 4060 1000
L0E-end 5-0 4032,360 4070 4060 1000
L0F-start 5-1 - 4070 4060 1000
L0F-end 5-1 3008,104 4070 4060 1000
L10-start 5-2 - 4070 4060 1000
L10-end 5-2 4800,168 4070 4060 1000
L10-room 5-2 4496,168 4260 4100 80
L11-start 5-3 - 4070 4060 1000
L11-end 5-3 704,152 4070 4060 1000
L11-arena 5-3 490,152 4300 4100 100
LIST
n=0
while read -r name ps at last from every; do
  p=${ps%-*} s=${ps#*-}
  python3 native/checks/plays/level_select_input.py "$p" "$s" "$tmp/$name.in"
  (
    d="$tmp/$name.d"; mkdir -p "$d"
    [ "$at" = - ] && unset ALADDIN_DEV_AT || export ALADDIN_DEV_AT=$at
    PORT_NOACTIVATE=1 PORT_RESDIR=$res ALADDIN_VIEW=$view ALADDIN_DEV_SHOTS=$d ALADDIN_DEV_SHOT_EVERY=$every ALADDIN_DEV_SHOT_FROM=$from \
      "$app" --updates "$last" --input "$tmp/$name.in" > "$d/log.txt" 2>&1
    for f in "$d"/u*.ppm; do
      [ -f "$f" ] || continue
      b=$(basename "$f" .ppm)
      python3 - "$f" "$out/$view-$name-$b.ppm" <<'PY'
import sys
f = open(sys.argv[1], 'rb'); f.readline(); w, h = map(int, f.readline().split()); f.readline(); d = f.read()
rows = [y for y in range(h) if any(d[y * w * 3 + i] for i in range(0, w * 3, 7))]
cols = [x for x in range(w) if any(d[(y * w + x) * 3] or d[(y * w + x) * 3 + 1] or d[(y * w + x) * 3 + 2] for y in range(rows[0], rows[-1] + 1, 5))]
y0, y1, x0, x1 = rows[0], rows[-1], cols[0], cols[-1]
o = open(sys.argv[2], 'wb'); o.write(b'P6\n%d %d\n255\n' % (x1 - x0 + 1, y1 - y0 + 1))
for y in range(y0, y1 + 1):
    o.write(d[(y * w + x0) * 3:(y * w + x1 + 1) * 3])
PY
      sips -s format png "$out/$view-$name-$b.ppm" --out "$out/$view-$name-$b.png" > /dev/null && rm -f "$out/$view-$name-$b.ppm"
    done
  ) &
  n=$((n + 1))
  [ $((n % jobs)) -eq 0 ] && wait
done < "$tmp/list.txt"
wait
rm -rf "$tmp"
echo "done: $(ls "$out" | grep -c "^$view-") pictures in $out"
