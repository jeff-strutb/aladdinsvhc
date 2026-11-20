#!/bin/sh
# actors.sh [DIR FIRST LAST KINDS]: the native actor code against the
# original (native/checks/actors/check.c). With no arguments: records the
# original's work RAM after each frame of three windows of the attract play
# (the opening story's kind $36 and $85 actors, its kind $95 and $96 actors,
# the title's $82 and $4A) with snesbox into build/aladdin/native/actor-rec
# (once; about 230 MB), builds the check headless and runs it on each window
# for the core's kinds. With arguments: builds it and runs it on the dumps
# DIR/fNNNNN.wram from FIRST to LAST (one a frame, snesbox `dump` after each
# `wait 1`), comparing KINDS (hex, by commas: 2E,35). Builds and runs go
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
kinds=""
if [ $# -ge 4 ]; then
    rec="$(cd "$1" && pwd)"; windows="$2:$3"; kinds="$4"
elif [ ! -f "$rec/f09600.wram" ]; then
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
# the build's output is kept (a failed build says why and stops the run: a stale binary is never run)
rm -f "$bdir/aladdin_actor_check"
if ! timeout 900 "$run" -j 4 -m 3G -- sh -c 'cmake -S "$1" -B "$2" -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT="$3" \
    && cmake --build "$2" -j "$CPU_RUN_JOBS"' sh "$top/native/checks/actors" "$bdir" "$nshvc" > "$build/aladdin/native/actor-check-build.log" 2>&1; then
    grep -i "error\|undefined" "$build/aladdin/native/actor-check-build.log" | head -20 >&2
    echo "actors.sh: the check does not build (log: $build/aladdin/native/actor-check-build.log)" >&2
    exit 1
fi
[ -x "$bdir/aladdin_actor_check" ] || { echo "actors.sh: the build made no aladdin_actor_check" >&2; exit 1; }
status=0
for w in $windows; do
    a=${w%:*}; b=${w#*:}
    (cd "$bdir" && timeout 300 "$run" -j 1 -m 1G -- ./aladdin_actor_check --headless --updates 1 -- "$rec" "$a" "$b" $kinds) || status=1
done
exit $status
