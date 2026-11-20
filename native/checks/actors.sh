#!/bin/sh
# actors.sh: the native actor code against the original on the attract play
# (native/checks/actors/check.c). Records the original's work RAM after
# each frame of three windows of the attract play (the opening story's kind
# $36 and $85 actors, its kind $95 and $96 actors, the title's $82 and $4A)
# with snesbox into build/aladdin/native/actor-rec (once; about 230 MB),
# builds the check headless and runs it on each window. Builds and runs go
# through the CPU and memory queue ALADDIN_CPU_RUN names, with timeouts.
# Run from the repository root.
set -eu
top="$(git rev-parse --show-toplevel)"
build="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)/build"   # the main checkout's, from a worktree too
run="${ALADDIN_CPU_RUN:-}"
[ -n "$run" ] || { echo "actors.sh: ALADDIN_CPU_RUN names no CPU queue" >&2; exit 2; }
nshvc="${NSHVC_ROOT:-/home/strutb/projects/strutb/nativeshvc}"
rec="$build/aladdin/native/actor-rec"
bdir="$build/aladdin/native/checks/actors"
windows="3480:4400 7600:8100 9200:9600"
if [ ! -f "$rec/f09600.wram" ]; then
    mkdir -p "$rec"
    python3 - "$rec/rec.box" $windows <<'PY'
import sys
lines, at = [], 0
for w in sys.argv[2:]:
    a, b = map(int, w.split(':'))
    lines += ['wait %d' % (a - at), 'dump f%05d' % a]
    for f in range(a + 1, b + 1):
        lines += ['wait 1', 'dump f%05d' % f]
    at = b
open(sys.argv[1], 'w').write('\n'.join(lines) + '\n')
PY
    timeout 1200 "$run" -j 1 -m 2G -- "$build/aladdin/box/snesbox" --rom "$build/aladdin/baserom.sfc" \
        --script "$rec/rec.box" --out "$rec" > "$rec/rec.log"
fi
timeout 900 "$run" -j 4 -m 3G -- sh -c 'cmake -S "$1" -B "$2" -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT="$3" >/dev/null \
    && cmake --build "$2" -j "$CPU_RUN_JOBS" >/dev/null' sh "$top/native/checks/actors" "$bdir" "$nshvc"
status=0
for w in $windows; do
    a=${w%:*}; b=${w#*:}
    (cd "$bdir" && timeout 300 "$run" -j 1 -m 1G -- ./aladdin_actor_check --headless --updates 1 -- "$rec" "$a" "$b") || status=1
done
exit $status
