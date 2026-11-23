#!/bin/sh
# crop_check.sh MACBIN [U]: the crop a save through the pause menu keeps is the picture of the saved state, with runahead on and off.
# MACBIN is the Metal build run headless (the null renderer reads no picture), PORT_RESDIR its Resources. A play (p1-1) runs to U
# (default 4720), where Escape opens the pause menu and keys choose SAVE MENU and slot 1; the slot's crop is compared pixel for pixel
# with the crop of the picture a plain run shows for the world after update U-1 (the plain run's shot at update U+1: pictures are
# one update behind), cut as saves_crop cuts it around the player. Prints the pixels that differ of 6,912 for runahead 0 and 1.
set -u
bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1"); u=${2:-4720}
S=${CROP_OUT:-$(pwd)/build/aladdin/native/crop}; mkdir -p "$S"; S=$(cd "$S" && pwd)
res=$(dirname "$bin")/../Resources
python3 -I - "$u" "$S/m.txt" <<'PY'
import sys
u = int(sys.argv[1])
o = [l.rstrip('\n') for l in open('native/checks/plays/p1-1.txt') if l.split() and l.split()[0].isdigit() and int(l.split()[0]) < u]
o += [f"{u} key=1B", f"{u+1}", f"{u+10} key=28", f"{u+13}", f"{u+20} key=5A", f"{u+23}", f"{u+40} key=5A", f"{u+43}"]
open(sys.argv[2], 'w').write('\n'.join(o) + '\n')
PY
run() {   # name updates input env...
  n=$1; ups=$2; in=$3; shift 3; mkdir -p "$S/$n/save.$$"   # a folder of its own each run: an empty slot (a used one asks first)
  (cd "$(dirname "$bin")" && env PORT_RESDIR="$res" PORT_SAVEDIR="$S/$n/save.$$" ALADDIN_DEV_CHEAT=1 "$@" "$bin" --headless --updates "$ups" --input "$in" > /dev/null 2> "$S/$n/err") &
  p=$!; t=0; while kill -0 $p 2>/dev/null; do sleep 2; t=$((t + 2)); [ $t -gt 300 ] && kill $p; done
}
run plain $((u + 5)) "$(pwd)/native/checks/plays/p1-1.txt" ALADDIN_RUNAHEAD=0 ALADDIN_DEV_TRACE=1 ALADDIN_DEV_SHOTS="$S/plain" ALADDIN_DEV_SHOT_EVERY=1 ALADDIN_DEV_SHOT_FROM=$((u + 1))
run r0 $((u + 80)) "$S/m.txt" ALADDIN_RUNAHEAD=0
run r1 $((u + 80)) "$S/m.txt" ALADDIN_RUNAHEAD=1
python3 -I - "$S" "$u" <<'PY'
import sys, re, glob, os
S, u = sys.argv[1], int(sys.argv[2])
def rd(p):
    d = open(p, 'rb').read(); i = 0; t = []
    while len(t) < 4:
        while d[i:i+1].isspace(): i += 1
        j = i
        while not d[j:j+1].isspace(): j += 1
        t.append(d[i:j]); i = j
    return int(t[1]), int(t[2]), d[i+1:]
tr = {}
for l in open(S + '/plain/err'):
    m = re.match(r'u=(\d+) level=\d+ x=(\d+) y=(\d+) .*scroll=(\d+) scrolly=(\d+)', l)
    if m: tr[int(m[1])] = tuple(map(int, m.groups()[1:]))
x, y, sx, sy = tr[u - 1]
cx, cy = x - sx, y - sy - 24
w, h, b = rd('%s/plain/u%06d.ppm' % (S, u + 1))
x0 = min(max(cx - 48, 0), w - 96); y0 = min(max(cy - 36, 0), h - 72)
ref = [0xFF000000 | b[((y0+j)*w+x0+i)*3] << 16 | b[((y0+j)*w+x0+i)*3+1] << 8 | b[((y0+j)*w+x0+i)*3+2] for j in range(72) for i in range(96)]
for r in (0, 1):
    f = open(max(glob.glob('%s/r%d/save.*/save_00' % (S, r)), key=os.path.getmtime), 'rb').read()
    s = [int.from_bytes(f[36+4*i:40+4*i], 'little') for i in range(96*72)]
    print('runahead %d (picture taken at update %d): %d of %d pixels of the crop differ from the saved state\'s picture' % (r, int.from_bytes(f[28:36], 'little'), sum(a != c for a, c in zip(s, ref)), 96*72))
PY
