#!/bin/sh
# hold.sh BIN [PLAY UPDATE HOLD]: the menu leaves the game as it found it. A play (default native/checks/plays/p0-0.txt)
# is run twice for 300 updates past UPDATE (an update where the play holds no key; default 6433): once as it is, once with
# Escape pressed at UPDATE (the menu opens) and again HOLD updates later (it shuts), the play's later updates put off by
# HOLD + 1. The two runs' developer traces (the player, every live actor with its own bytes, the video fields) must then
# be the same line for line from the update the game goes on at, apart from the update number. Run from the repository
# root. Exit 0: the same, and the menu held the game (no trace lines while it was open).
set -u
bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
play=${2:-native/checks/plays/p0-0.txt}
at=${3:-6433}
hold=${4:-50}
out=${HOLD_OUT:-build/aladdin/native/hold}
res=$(dirname "$bin")/../Resources
[ -f "$res/cart.dat" ] || res=$(dirname "$bin")
rm -rf "$out"; mkdir -p "$out"
shift_by=$((hold + 1))
awk -v at="$at" -v hold="$hold" -v sh="$shift_by" '
  $1 + 0 < at { print; next }
  $1 + 0 == at { if (!done) { print at " key=1B"; print at + 1; print at + hold " key=1B"; print at + hold + 1; done = 1 } ; next }
  { n = $1 + 0; $1 = n + sh; print }
' "$play" > "$out/held.txt"
cp "$play" "$out/plain.txt"
last=$((at + 300))
run() {   # name input updates
  (cd "$(dirname "$bin")" && PORT_RESDIR=$res ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 \
    "$bin" --headless --updates "$3" --input "$2" > "$out/$1.out" 2> "$out/$1.err")
}
case $play in /*) ;; *) here=$(pwd); out=$here/$out; run() { (cd "$(dirname "$bin")" && PORT_RESDIR=$res ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 "$bin" --headless --updates "$3" --input "$out/$2" > "$out/$1.out" 2> "$out/$1.err"); }; ;; esac
run plain plain.txt "$last"
run held held.txt $((last + shift_by))
python3 -I - "$out/plain.err" "$out/held.err" "$at" "$hold" "$last" <<'PY'
import re, sys
plain, held, at, hold, last = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
def read(path):
    d, cur = {}, None
    for line in open(path):
        m = re.match(r'u=(\d+) (.*)', line)
        if m:
            cur = int(m.group(1)); d[cur] = [m.group(2)]
        elif cur is not None and line.startswith('  '):
            d[cur].append(line.rstrip('\n'))
    return d
a, b = read(plain), read(held)
shift = hold + 1
bad = 0
inside = [u for u in b if at <= u < at + shift]
if inside:
    print('FAIL: the game stepped while the menu was open, at updates', inside[:5]); bad = 1
n = 0
for u in range(at + 1, last + 1):
    if u not in a:
        continue
    if b.get(u + shift) != a[u]:
        print('FAIL: update %d of the plain run differs from update %d of the held run' % (u, u + shift)); bad = 1
        break
    n += 1
print('%d updates after the menu compared; the game was held for %d updates' % (n, shift - len(inside)))
sys.exit(bad)
PY
