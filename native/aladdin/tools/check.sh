#!/bin/sh
# check.sh: one decomp input script replayed on the original (bsnes, through
# nativeshvc's oracle) and on the native game (headless), the sound compared
# sample by sample by nativeshvc's tools/nshvc-check. docs/aladdin/native/CHECK.md.
#
#   native/aladdin/tools/check.sh SCRIPT              the full check
#   native/aladdin/tools/check.sh --oracle-only SCRIPT the original alone
#
# SCRIPT is a name in tools/aladdin/scripts (attract) or a path to a .box.
# Writes build/aladdin/native/check/NAME/ (inputs.txt, keys.txt, script.txt,
# original.raw, game.raw, result.txt). Every compile and run goes through the
# machine's CPU and memory queue, the command ALADDIN_CPU_RUN names (called as
# CMD -j CORES -m MEMORY -- COMMAND, as tools/aladdin/runner.py calls it), with a
# timeout; check.sh will not start without it. Run from the repository root.
#
# Settings (environment):
#   NSHVC_ROOT  nativeshvc checkout (default /home/strutb/projects/strutb/nativeshvc)
#   CORE        the project's bsnes core (default: the main checkout's
#               tools/toolchains/bsnes/bsnes_libretro.so, as setup.sh builds it)
#   ROM         the cartridge (default: the main checkout's reference/aladdin)
#   GAME        the native game (default build/aladdin/native/headless/aladdin)
#   KEYS        nshvc-check --keys, the key the game's controls give each button
#               (default: nshvc-check's own)
#   FRAMES      play only this many frames (default: the whole script)
#   LIMIT       seconds allowed for the original's run (default: 60 + frames / 50)
set -eu

oracle_only=0
if [ "${1:-}" = "--oracle-only" ]; then oracle_only=1; shift; fi
[ $# -eq 1 ] || { sed -n "2,25p" "$0" >&2; exit 2; }

top="$(git rev-parse --show-toplevel)"
main="$(cd "$(git rev-parse --git-common-dir)/.." && pwd)"
nshvc="${NSHVC_ROOT:-/home/strutb/projects/strutb/nativeshvc}"
core="${CORE:-$main/tools/toolchains/bsnes/bsnes_libretro.so}"
rom="${ROM:-$main/reference/aladdin/Aladdin (USA).sfc}"
game="${GAME:-$top/build/aladdin/native/headless/aladdin}"
run="${ALADDIN_CPU_RUN:-}"
[ -n "$run" ] || { echo "check.sh: ALADDIN_CPU_RUN names no CPU queue" >&2; exit 2; }

case "$1" in
    */*|*.box) script="$1" ;;
    *) script="$top/tools/aladdin/scripts/$1.box" ;;
esac
[ -f "$script" ] || { echo "check.sh: no script $script" >&2; exit 2; }
name="$(basename "$script" .box)"

[ -f "$rom" ] || { echo "check.sh: no ROM at $rom" >&2; exit 2; }
sum="$(sha1sum "$rom" | cut -c1-40)"
[ "$sum" = bd5fea18ce7456cc0231a39473927d8ea9c6589b ] || { echo "check.sh: $rom is not Aladdin (USA) (SHA-1 $sum)" >&2; exit 2; }
[ -f "$core" ] || { echo "check.sh: no core at $core (./setup.sh bsnes builds it)" >&2; exit 2; }

base="$top/build/aladdin/native/check"
bin="$base/bin"
out="$base/$name"
mkdir -p "$bin" "$out"

# The oracle, compiled out of tree from the nativeshvc checkout (its CMake
# target is one file linked with libdl).
src="$nshvc/tests/oracle/oracle.c"
if [ ! -x "$bin/oracle" ] || [ "$src" -nt "$bin/oracle" ]; then
    timeout 300 "$run" -j 1 -m 1G -- cc -O2 -o "$bin/oracle.new" "$src" -ldl
    mv "$bin/oracle.new" "$bin/oracle"
fi
# A private copy of the core (a rebuild of the shared one never lands under a
# run), and the wrapper that gives the oracle the core's DSP hook by the name it
# looks for (check_core_shim.c); the oracle loads the wrapper as its --core.
if ! cmp -s "$core" "$bin/bsnes_libretro.so"; then
    cp "$core" "$bin/bsnes_libretro.so.new"
    mv "$bin/bsnes_libretro.so.new" "$bin/bsnes_libretro.so"
    rm -f "$bin/libcheckcore.so"
fi
shim="$top/native/aladdin/tools/check_core_shim.c"
if [ ! -f "$bin/libcheckcore.so" ] || [ "$shim" -nt "$bin/libcheckcore.so" ]; then
    timeout 300 "$run" -j 1 -m 1G -- cc -O2 -shared -fPIC -o "$bin/libcheckcore.so.new" "$shim" \
        -L"$bin" -l:bsnes_libretro.so -Wl,-rpath,'$ORIGIN'
    mv "$bin/libcheckcore.so.new" "$bin/libcheckcore.so"
fi

# The script's buttons as the engine's inputs.txt, the oracle's script for the
# same buttons; its length in frames.
inputs="$out/inputs.txt"
frames="$(python3 "$top/native/aladdin/tools/check_inputs.py" "$script" --root "$top" --out "$inputs" \
    --oracle-script "$out/script.txt" --nshvc "$nshvc" ${FRAMES:+--frames "$FRAMES"})"
# LIMIT: seconds for the original's run (the full check gets twice that). The
# oracle ran level1_walk's 4,704 frames in 24 s here (about 196 frames a
# second); the default allows 50 a second, plus a minute to start.
limit="${LIMIT:-$((60 + frames / 50))}"
# The core links OpenMP; its threads are kept to one inside each job (set in the
# job's own environment: GNU nproc reads OMP_NUM_THREADS, so exporting it here
# would make the queue count one core).

if [ "$oracle_only" = 1 ]; then
    rm -f "$out/original.raw"
    start=$(date +%s)
    timeout "$limit" "$run" -j 1 -m 1G -- env OMP_NUM_THREADS=1 "$bin/oracle" --core "$bin/libcheckcore.so" --rom "$rom" \
        --script "$out/script.txt" --out "$out" --sound "$out/original.raw"
    end=$(date +%s)
    python3 -c "
import sys, array
a = array.array('h', open(sys.argv[1], 'rb').read())
n = len(a) // 2
loud = sum(1 for i in range(n) if a[2 * i] or a[2 * i + 1])
first = next((i for i in range(n) if a[2 * i] or a[2 * i + 1]), None)
print(f'original: {sys.argv[2]} frames, {n} samples ({loud} not silent, the first at {first}), {sys.argv[3]} s; files in {sys.argv[4]}')" \
        "$out/original.raw" "$frames" "$((end - start))" "$out" | tee "$out/result.txt"
    exit 0
fi

[ -x "$game" ] || { echo "check.sh: no native game at $game (docs/aladdin/native/BUILD.md builds it)" >&2; exit 2; }
# The game reads cart.dat beside itself, so it runs from its own folder.
set +e
(cd "$(dirname "$game")" && timeout $((limit * 2)) "$run" -j 2 -m 2G -- env OMP_NUM_THREADS=1 python3 "$nshvc/tools/nshvc-check" \
    --oracle "$bin/oracle" --core "$bin/libcheckcore.so" --rom "$rom" --game "$game" \
    --frames "$frames" --input "$inputs" ${KEYS:+--keys "$KEYS"} --out "$out") > "$out/result.txt" 2>&1
status=$?
set -e
cat "$out/result.txt"
exit $status
