#!/bin/sh
# replay.sh NAME [--plant]: plays the recording build/aladdin/native/recordings/NAME
# through the engine (native/checks/replay, headless on the Linux Vulkan build,
# every picture written) and puts bsnes's pictures beside the engine's
# (native/checks/compare.py). --plant plants a change the comparison must
# find (BG1's horizontal scroll one pixel off). Writes
# build/aladdin/native/replays/NAME[-plant]/: shots/ (the engine's pictures),
# beside/ (bsnes left, the engine right, the differing pixels below),
# compare.txt. Builds and runs go through the CPU and memory queue
# ALADDIN_CPU_RUN names, with timeouts. Run from the repository root.
set -eu
[ $# -ge 1 ] || { sed -n '2,11p' "$0" >&2; exit 2; }
top="$(git rev-parse --show-toplevel)"
run="${ALADDIN_CPU_RUN:-}"
[ -n "$run" ] || { echo "replay.sh: ALADDIN_CPU_RUN names no CPU queue" >&2; exit 2; }
nshvc="${NSHVC_ROOT:-/home/strutb/projects/strutb/nativeshvc}"
rec="$top/build/aladdin/native/recordings/$1"
[ -f "$rec/state.bin" ] || { echo "replay.sh: no recording at $rec (native/checks/record.sh $1)" >&2; exit 2; }
plant=0
suffix=""
if [ "${2:-}" = "--plant" ]; then plant=1; suffix="-plant"; fi
out="$top/build/aladdin/native/replays/$1$suffix"
bdir="$top/build/aladdin/native/checks/replay-linux"

timeout 900 "$run" -j 4 -m 4G -- sh -c 'cmake -S "$1" -B "$2" -G Ninja -DCMAKE_BUILD_TYPE=Release -DNSHVC_ROOT="$3" \
    -DNSHVC_HOST=linux -DNSHVC_RENDER=vulkan >/dev/null && cmake --build "$2" -j "$CPU_RUN_JOBS" >/dev/null' \
    sh "$top/native/checks/replay" "$bdir" "$nshvc"

pictures=$(wc -l < "$rec/pictures.txt")
rm -rf "$out"
mkdir -p "$out/shots"
# one update per picture; LIMIT seconds (default: a minute plus a second per 20 pictures)
limit="${LIMIT:-$((60 + pictures / 20))}"
(cd "$bdir" && ALADDIN_REPLAY="$rec" ALADDIN_REPLAY_PLANT=$plant timeout "$limit" "$run" -j 2 -m 2G -- \
    ./aladdin_replay --headless --updates "$pictures" --shots "$out/shots") > "$out/replay.txt" 2>&1 || {
    echo "replay.sh: the replay failed:"; tail -5 "$out/replay.txt"; exit 1; }
set +e
timeout 1800 "$run" -j 1 -m 512M -- python3 "$top/native/checks/compare.py" "$rec" "$out/shots" --beside "$out/beside" > "$out/compare.txt"
status=$?
set -e
# Keep only the engine's pictures that have a bsnes picture beside them (one a
# picture is about 170 KB).
python3 -c "
import os, sys
rec, shots = sys.argv[1], sys.argv[2]
keep = {int(n[:-4]) - 1 for n in os.listdir(os.path.join(rec, 'shots')) if n.endswith('.ppm')}
for n in os.listdir(shots):
    if n.endswith('.ppm') and int(n[:-4]) not in keep:
        os.remove(os.path.join(shots, n))" "$rec" "$out/shots"
tail -1 "$out/compare.txt"
exit $status
