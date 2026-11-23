#!/bin/bash
# future.sh MACOS_BIN INPUT FROM TO BETWEEN: the ahead picture is the true future (FRAMERATE.md 9.8). The Metal build is run
# headless twice over INPUT, ALADDIN_RUNAHEAD=0 and =1, a picture every update from FROM to TO, each drawn at between BETWEEN
# (ALADDIN_DEV_BETWEEN_AT). Picture uK written with runahead is the world K - 1 eased towards the ahead world K; without it
# picture u(K+1) is world K - 1 eased towards the real world K. They are the same, byte for byte, at every K where the input
# did not change at update K (a line of INPUT with that update number), and may differ where it did. Prints the counts.
set -u
bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1"); in=$2; from=$3; to=$4; at=$5
res=$(dirname "$bin")/../Resources
out=${FUTURE_OUT:-/tmp/future-ra}
for m in 0 1; do
  d=$out/$m; rm -rf "$d"; mkdir -p "$d"
  (cd "$(dirname "$bin")" && PORT_RESDIR=$res ALADDIN_RUNAHEAD=$m ALADDIN_DEV_BETWEEN_AT=$at ALADDIN_DEV_SHOTS=$d ALADDIN_DEV_SHOT_EVERY=1 \
    ALADDIN_DEV_SHOT_FROM=$from "$bin" --headless --updates "$to" --input "$in" > /dev/null 2>&1) || { echo "run $m failed"; exit 1; }
  (cd "$d" && for f in u*.ppm; do echo "${f#u} $(md5 -q "$f")"; done) | sed 's/\.ppm//' > "$out/$m.md5"
  rm -rf "$d"
done
python3 - "$in" "$out" <<'PY'
import sys
chg = {int(l.split()[0]) for l in open(sys.argv[1]) if l.split() and l.split()[0].isdigit()}
off = dict(l.split() for l in open(sys.argv[2] + '/0.md5')); on = dict(l.split() for l in open(sys.argv[2] + '/1.md5'))
held = [0, 0]; changed = [0, 0]
for k in sorted(int(x) for x in on):
    if '%06d' % (k + 1) not in off:
        continue
    same = on['%06d' % k] == off['%06d' % (k + 1)]
    (changed if k in chg else held)[0 if same else 1] += 1
    if not same and k not in chg:
        print('DIFFERS at update', k)
print('input held: %d equal, %d differ; input changed at the update: %d equal, %d differ' % (held[0], held[1], changed[0], changed[1]))
sys.exit(1 if held[1] else 0)
PY
