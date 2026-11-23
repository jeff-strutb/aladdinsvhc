#!/bin/bash
# cross.sh BIN PLAY S K [BASE_PLAY L]: the snapshot across two processes (docs/aladdin/native/FRAMERATE.md 10.5).
# BIN is the Metal build (headless runs with pictures). Run A plays PLAY and writes a snapshot at the start of update S (the
# world after update S - 1). Run B is a fresh process: it plays BASE_PLAY (a play's input file, or none: the boot and the title)
# up to update L, loads the snapshot there (no step in that update), and is then given PLAY's keys from update S on (the keys of
# PLAY after S, shifted to B's update numbers). B's update L + 1 + j is A's update S + j. Compares K updates: the developer's
# trace and the state hash line for line, and the picture of every update (B's file u(n + D) against A's u(n), D = L + 1 - S,
# from n = S + 2: a picture shows the update before's world). Exit 0 when all the same.
# SNAP_SAME=1: B is the one-process round trip: BASE_PLAY is PLAY, B saves at S itself, runs on to L, loads its own snapshot there.
# Env: SNAP_SAVE_ENV, SNAP_LOAD_ENV: extra environment for A, for B (a planted omission, ALADDIN_RUNAHEAD).
set -u
bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1"); play=$2; S=$3; K=$4; base=${5:--}; L=${6:-300}
res=$(dirname "$bin")/../Resources
out=${SNAP_OUT:-/tmp/snapshot-cross}; rm -rf "$out"; mkdir -p "$out/a" "$out/b"
D=$((L + 1 - S))
python3 - "$play" "$base" "$S" "$L" "$out" <<'PY'
import sys
play, base, S, L, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
def lines(f):
    r = []
    for l in open(f):
        w = l.split()
        if w and w[0].isdigit():
            r.append((int(w[0]), w[1:]))
    return r
A = lines(play)
state = []                                   # the keys in effect at S: the last line at or before S
for n, k in A:
    if n <= S: state = k
B = []
if base != '-':
    B = [(n, k) for n, k in lines(base) if n <= L]
D = L + 1 - S
B.append((L + 1, state))
B += [(n + D, k) for n, k in A if n > S]
open(out + '/a.txt', 'w').write(''.join('%d %s\n' % (n, ' '.join(k)) for n, k in A))
open(out + '/b.txt', 'w').write(''.join('%d %s\n' % (n, ' '.join(k)) for n, k in B))
PY
env_common="PORT_RESDIR=$res ALADDIN_DEV_CHEAT=1 ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 ALADDIN_DEV_STATE_HASH=2 ALADDIN_DEV_SHOT_EVERY=1"
(cd "$(dirname "$bin")" && env $env_common ALADDIN_DEV_SNAP_SAVE=$out/snap@$S ALADDIN_DEV_SHOTS=$out/a ALADDIN_DEV_SHOT_FROM=$S ${SNAP_SAVE_ENV:-} \
  perl -e 'alarm 600; exec @ARGV' "$bin" --headless --updates $((S + K + 3)) --input "$out/a.txt" > /dev/null 2> "$out/a.err") || { echo "run A failed"; tail -3 "$out/a.err"; exit 1; }
(cd "$(dirname "$bin")" && env $env_common $([ "${SNAP_SAME:-0}" = 1 ] && echo "ALADDIN_DEV_SNAP_SAVE=$out/snap2@$S ALADDIN_DEV_SNAP_LOAD=$out/snap2@$L" || echo "ALADDIN_DEV_SNAP_LOAD=$out/snap@$L") ALADDIN_DEV_SHOTS=$out/b ALADDIN_DEV_SHOT_FROM=$((L + 1)) ${SNAP_LOAD_ENV:-} \
  perl -e 'alarm 600; exec @ARGV' "$bin" --headless --updates $((L + 1 + K + 3)) --input "$out/b.txt" > /dev/null 2> "$out/b.err") || { echo "run B failed"; tail -3 "$out/b.err"; exit 1; }
python3 - "$out" "$S" "$K" "$D" "$L" <<'PY'
import sys, re, hashlib, os
out, S, K, D, L = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
def trace(f, lo, hi):
    r = {}
    for l in open(f, errors='replace'):
        m = re.match(r'(?:u=|hash u=)(\d+)', l)
        if re.match(r'u=\d+ ', l) or l.startswith('hash u='):
            cur = int(re.search(r'u=(\d+)', l).group(1)); r.setdefault(cur, []).append(re.sub(r'u=\d+', 'u=', l.rstrip()))
        elif l.startswith('  '):
            r.setdefault(cur, []).append(l.rstrip())
    return {k: v for k, v in r.items() if lo <= k < hi}
a = trace(out + '/a.err', S, S + K); b = trace(out + '/b.err', L + 1, L + 1 + K)
def same(x, y, j):
    if x is None or y is None: return False
    if j > 0: return x == y
    # the first update: B's driver is given the state in the audio advance after it (music.h), so its sound part and the
    # whole hash that holds it are compared from the next update on
    strip = lambda t: [re.sub(r' sound=\w+', '', l)[:-17] if l.startswith('hash u=') else l for l in t]
    return strip(x) == strip(y)
bad = 0; first = None
for j in range(K):
    if not same(a.get(S + j), b.get(L + 1 + j), j):
        bad += 1; first = first or j
print('trace and state hash: %d of %d updates differ%s' % (bad, K, '' if not bad else ', first at A update %d' % (S + first)))
def md5(p): return hashlib.md5(open(p, 'rb').read()).hexdigest() if os.path.exists(p) else None
pb = 0; n = 0
for j in range(2, K):
    n += 1
    if md5('%s/a/u%06d.ppm' % (out, S + j)) != md5('%s/b/u%06d.ppm' % (out, S + j + D)) or md5('%s/a/u%06d.ppm' % (out, S + j)) is None:
        pb += 1
print('pictures: %d of %d differ' % (pb, n))
sys.exit(1 if bad or pb else 0)
PY
