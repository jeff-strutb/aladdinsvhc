#!/bin/sh
# shape_load.sh BIN A_PLAY U LEN B_PLAY V A_VIEW B_VIEW: a save made at one shape and loaded at another (VIEW.md 4.8, SAVES.md).
# saves/exact.sh with A run at ALADDIN_VIEW=A_VIEW (WxH, 288x224 ...) and B at B_VIEW: B loads the save (its view is the saved one) and then follows
# its window, which is B_VIEW (a developer's window, ALADDIN_VIEW). The traces of A from U and of B from its load are then compared by kind: the
# player's line, the video line, the actors. With the same shape they must be the same (exit 0); with another, the player's and the video lines
# must be the same until a figure the wider view makes live touches them, and the first update that differs is printed. Run from the repository root.
set -u
export EXACT_A_ENV="ALADDIN_VIEW=$7" EXACT_B_ENV="ALADDIN_VIEW=$8" EXACT_OUT=${EXACT_OUT:-build/aladdin/native/shape-load}
sh native/checks/saves/exact.sh "$1" "$2" "$3" "$4" "$5" "$6" > /dev/null 2>&1
out=$EXACT_OUT/$(basename "$2" .txt)-$(basename "$5" .txt)
python3 -I - "$out/a.trace" "$out/b.trace" "$7" "$8" "$2" "$3" "$5" "$6" <<'PY'
import sys, re
def load(p):
    d = {}; cur = None
    for l in open(p, errors='replace'):
        m = re.match(r'u=(\d+) ', l)
        if m: cur = int(m.group(1)); d[cur] = {'p': l.rstrip(), 'v': '', 'a': []}
        elif cur is not None and l.startswith('  video'): d[cur]['v'] = l.rstrip()
        elif cur is not None and l.startswith('  actor'): d[cur]['a'].append(re.sub(r'^  actor \d+ ', '', l.rstrip()))
    return d
# the update numbers are relative to the save and the load; the player's line is compared without its update number
a, b = load(sys.argv[1]), load(sys.argv[2])
n = min(len(a), len(b)); res = []
strip = lambda s: re.sub(r'^u=\d+ ', '', s)
for k, name in (('p', 'player'), ('v', 'video'), ('a', 'actors')):
    bad = [i for i in range(n) if (strip(a[i][k]) != strip(b[i][k]) if k != 'a' else sorted(a[i][k]) != sorted(b[i][k]))]
    res.append(f"{name} {len(bad)} differ" + (f" (first +{bad[0]})" if bad else ""))
print(f"{sys.argv[5]} saved at {sys.argv[6]} at {sys.argv[3]}, loaded at {sys.argv[8]} of {sys.argv[7]} at {sys.argv[4]}: {n} updates; " + ', '.join(res))
PY
