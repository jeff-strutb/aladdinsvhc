#!/bin/sh
# actor_check_kinds.sh DIR FIRST LAST KIND...: the actor check (aladdin_actor_check, built by actors.sh's steps
# into $BUILD) on DIR's per-frame dumps FIRST..LAST, one run for each kind: prints, per kind, the fields compared,
# the fields that differ and the first frame, and the first differences (each run keeps its output in
# $OUT/kind_KK.txt). FIRST must be a frame at which the level is loaded and no record is live yet (see
# play_dump.py). Run from the repository root with ALADDIN_CPU_RUN set; BUILD and OUT default under
# build/aladdin/native/ in the main checkout.
set -eu
[ $# -ge 4 ] || { echo "usage: $0 DIR FIRST LAST KIND..." >&2; exit 2; }
dir="$1"; first="$2"; last="$3"; shift 3
run="${ALADDIN_CPU_RUN:-}"
[ -n "$run" ] || { echo "actor_check_kinds.sh: ALADDIN_CPU_RUN names no CPU queue" >&2; exit 2; }
main="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)"
build="${BUILD:-$main/build/aladdin/native/checks/actors}"
out="${OUT:-$main/build/aladdin/native/actor-check-out}"
mkdir -p "$out"
for k in "$@"; do
    (cd "$build" && timeout 1500 "$run" -j 1 -m 1G -- ./aladdin_actor_check --headless --updates 1 -- "$dir" "$first" "$last" "$k" > "$out/kind_$k.txt" 2>&1) || true
    line=$(grep "^actor check: kind \\\$$k:" "$out/kind_$k.txt" || true)
    total=$(grep '^actor check: frames' "$out/kind_$k.txt" || echo "no summary (crashed?)")
    echo "kind \$$k: ${line:-0 fields differ} | $total"
done
