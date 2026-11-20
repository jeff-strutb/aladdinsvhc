#!/bin/sh
# record.sh SCRIPT...: records the original on each input-only script for the
# native game's checks (native/checks/record/record.c says what is written).
# SCRIPT is a name in tools/aladdin/scripts (attract) or a path to a .box;
# `--all` records every script native/aladdin/tools/check_inputs.py --classify
# finds replayable by input. Each recording is build/aladdin/native/recordings/NAME.
# Recordings come from the ROM and are never committed.
#
# Each recording is one job on the CPU and memory queue ALADDIN_CPU_RUN names,
# with a timeout; JOBS (default 1) recordings run at once. Run from the
# repository root. Settings (environment): ROM (default: the main checkout's
# reference/aladdin), SHOT_EVERY (default 60: a bsnes picture every 60 frames),
# SHOTS (frames, comma-separated, pictured too), VOICES=1 (voices.bin as well,
# 160 bytes a sample), JOBS (recordings at once, default 1), LIMIT (seconds
# for one recording).
set -eu
[ $# -ge 1 ] || { sed -n '2,15p' "$0" >&2; exit 2; }
top="$(git rev-parse --show-toplevel)"
main="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)"
run="${ALADDIN_CPU_RUN:-}"
[ -n "$run" ] || { echo "record.sh: ALADDIN_CPU_RUN names no CPU queue" >&2; exit 2; }
rom="${ROM:-$main/reference/aladdin/Aladdin (USA).sfc}"
sum="$(sha1sum "$rom" | cut -c1-40)"
[ "$sum" = bd5fea18ce7456cc0231a39473927d8ea9c6589b ] || { echo "record.sh: $rom is not Aladdin (USA) (SHA-1 $sum)" >&2; exit 2; }
bin="$top/build/aladdin/native/checks"
mkdir -p "$bin"

# The core with the PPU write hook, and the recorder.
core="$(sh "$top/native/checks/core/build.sh")"
if [ ! -x "$bin/record" ] || [ "$top/native/checks/record/record.c" -nt "$bin/record" ]; then
    timeout 300 "$run" -j 1 -m 1G -- cc -O2 -Wall -Wextra -o "$bin/record.new" "$top/native/checks/record/record.c" -ldl
    mv "$bin/record.new" "$bin/record"
fi

if [ "$1" = "--all" ]; then
    set -- $(python3 "$top/native/aladdin/tools/check_inputs.py" --classify --root "$top" | awk '/^    /{print $1}')
fi

one() {
    case "$1" in
        */*|*.box) script="$1" ;;
        *) script="$top/tools/aladdin/scripts/$1.box" ;;
    esac
    name="$(basename "$script" .box)"
    out="$top/build/aladdin/native/recordings/$name"
    rm -rf "$out"
    mkdir -p "$out"
    frames="$(python3 "$top/native/aladdin/tools/check_inputs.py" "$script" --root "$top" --out "$out/inputs.txt")"
    # LIMIT seconds (default: 60 plus a second per 50 frames, a quarter of the
    # oracle's measured speed, CHECK.md).
    limit="${LIMIT:-$((60 + frames / 50))}"
    start=$(date +%s)
    if timeout "$limit" "$run" -j 1 -m 256M -- "$bin/record" --core "$core" --rom "$rom" --inputs "$out/inputs.txt" \
        --frames "$frames" --out "$out" --shot-every "${SHOT_EVERY:-60}" ${SHOTS:+--shots "$SHOTS"} \
        ${VOICES:+--voices} > "$out/record.txt" 2>&1; then
        echo "$name: $(cat "$out/record.txt" | tail -1), $(( $(date +%s) - start )) s, $(du -sh "$out" | cut -f1)"
    else
        echo "$name: FAILED ($?): $(tail -3 "$out/record.txt")"
        return 1
    fi
}

# JOBS groups, each recorded in turn, the groups at once.
jobs="${JOBS:-1}"
status=0
k=0
pids=""
while [ "$k" -lt "$jobs" ]; do
    (
        i=0; st=0
        for s in "$@"; do
            [ $((i % jobs)) -eq "$k" ] && { one "$s" || st=1; }
            i=$((i + 1))
        done
        exit $st
    ) &
    pids="$pids $!"
    k=$((k + 1))
done
for p in $pids; do wait "$p" || status=1; done
exit $status
