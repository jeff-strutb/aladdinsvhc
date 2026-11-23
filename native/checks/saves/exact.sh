#!/bin/sh
# exact.sh BIN A_PLAY U LEN B_PLAY V [fault]: a save is the exact moment (docs/aladdin/native/SAVES.md).
#   A: A_PLAY run with ALADDIN_DEV_SAVE_AT=U (slot 0 saved at the start of update U), on to U+LEN, developer traces kept.
#   (EXACT_A_ENV="NAME=VALUE" sets one more variable for A only, e.g. ALADDIN_DEV_SCREEN=genie.)
#   B: another run (B_PLAY, or "-" for the title with no keys) that loads the slot at the start of update V and goes on with A's
#      keys from there (A's pad state at U at update V+1, A's later lines shifted by V+1-U), to V+1+LEN.
#   The traces (the player, every live actor with its own bytes, the video fields) from A's update U and B's update V+1 are
#   compared line for line apart from the update number. With "fault" the load leaves the player's x as it was: they must differ.
#   EXACT_UI=1: B is a play of keys that opens SAVED GAMES on the title and loads slot 1 through the screen (its file's last line, at V, is the
#   press that loads); no ALADDIN_DEV_LOAD_AT.
#   EXACT_MENU_SAVE=1: after A, the slot is saved again through the pause menu (Escape, Down, Z, Z: SAVE MENU, slot 1 empty or used) by keys
#   (m.txt: A's keys before U, then those) and B loads that file.
# Run from the repository root with BIN a build (headless or macOS) whose resources hold cart.dat. Exit 0: as asked.
set -u
bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
a_play=$2; u=$3; len=$4; b_play=$5; v=$6; fault=${7:-}
out=${EXACT_OUT:-build/aladdin/native/exact}/$(basename "$a_play" .txt)-$(basename "$b_play" .txt)${fault:+-fault}
rm -rf "$out"; mkdir -p "$out/save"; out=$(cd "$out" && pwd)
res=$(dirname "$bin")/../Resources
[ -f "$res/cart.dat" ] || res=$(dirname "$bin")
python3 -I - "$a_play" "$u" "$b_play" "$v" "$out/b.txt" "${EXACT_UI:-}" <<'PY'
import sys
a, u, b, v, outp, ui = sys.argv[1], int(sys.argv[2]), sys.argv[3], int(sys.argv[4]), sys.argv[5], sys.argv[6]
def lines(p):
    r = []
    if p == '-': return r
    for l in open(p):
        t = l.split()
        if t and t[0].isdigit(): r.append((int(t[0]), t[1:]))
    return r
A, B = lines(a), lines(b)
state = []
for n, t in A:
    if n <= u: state = t
shift = v + 1 - u
o = [(n, t) for n, t in B if (n <= v if ui else n < v)] + [(v + 1, state)] + [(n + shift, t) for n, t in A if n > u]
open(outp, 'w').write(''.join(' '.join([str(n)] + t) + '\n' for n, t in o))
PY
python3 -I - "$a_play" "$u" "$out/m.txt" <<'PY'
import sys
a, u, outp = sys.argv[1], int(sys.argv[2]), sys.argv[3]
o = []
for l in open(a):
    t = l.split()
    if t and t[0].isdigit() and int(t[0]) < u: o.append(l.rstrip('\n'))
# the pause menu by keys at U: Escape opens it, Down to SAVE MENU, Z opens the slots, Z on slot 1 saves
o += [f"{u} key=1B", f"{u+1}", f"{u+10} key=28", f"{u+13}", f"{u+20} key=5A", f"{u+23}", f"{u+40} key=5A", f"{u+43}"]
open(outp, 'w').write('\n'.join(o) + '\n')
PY
norm() {   # file base: the trace lines of updates from base on, update numbers made relative
  awk -v b="$2" '/^u=/ { split($1, p, "="); n = p[2] + 0; keep = (n >= b); if (keep) { sub(/^u=[0-9]+/, "u=" (n - b)); print }; next } /^ / { if (keep) print }' "$1"
}
run() {   # name input updates env...
  name=$1; input=$2; ups=$3; shift 3
  inarg=""; [ "$input" != "-" ] && inarg="--input $input"
  (cd "$(dirname "$bin")" && env PORT_RESDIR="$res" PORT_SAVEDIR="$out/save" ALADDIN_DEV_CHEAT=1 ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 ALADDIN_RUNAHEAD=0 "$@" \
     "$bin" --headless --updates "$ups" $inarg > "$out/$name.out" 2> "$out/$name.err") &
  pid=$!; t=0
  while kill -0 $pid 2>/dev/null; do sleep 2; t=$((t + 2)); [ $t -gt 900 ] && { kill $pid; echo "$name: timed out"; return 1; }; done
  wait $pid
}
a_in=$a_play; [ "$a_play" = "-" ] || { case $a_play in /*) ;; *) a_in=$(pwd)/$a_play;; esac; }
run a "$a_in" $((u + len)) ALADDIN_DEV_SAVE_AT=$u ${EXACT_A_ENV:-} || exit 1
grep -q "saved to slot 0: written" "$out/a.err" || { echo "A did not save"; exit 1; }
if [ -n "${EXACT_MENU_SAVE:-}" ]; then   # the slot is written again through the pause menu, with the keys of m.txt, over A's
  rm -f "$out/save/save_00"   # A's own save of the slot: the menu saves into an empty one
  run m "$out/m.txt" $((u + 100)) || exit 1
  grep -q "save screen opened from the game" "$out/m.err" || { echo "the pause menu's save screen did not open"; exit 1; }
fi
if [ -n "$fault" ]; then run b "$out/b.txt" $((v + 1 + len)) ALADDIN_DEV_LOAD_AT=$v ALADDIN_DEV_FAULT=1 || exit 1
elif [ -n "${EXACT_UI:-}" ]; then run b "$out/b.txt" $((v + 1 + len)) || exit 1
else run b "$out/b.txt" $((v + 1 + len)) ALADDIN_DEV_LOAD_AT=$v || exit 1; fi
if [ -z "${EXACT_UI:-}" ]; then grep -q "loaded slot 0: loaded" "$out/b.err" || { echo "B did not load"; head -5 "$out/b.err"; exit 1; }; fi
norm "$out/a.err" "$u" > "$out/a.trace"
norm "$out/b.err" $((v + 1)) > "$out/b.trace"
na=$(grep -c '^u=' "$out/a.trace"); nb=$(grep -c '^u=' "$out/b.trace")
if cmp -s "$out/a.trace" "$out/b.trace"; then
  echo "$(basename "$a_play") save at $u, load at $v of $(basename "$b_play"): $na updates, the traces are the same"; [ -z "$fault" ] && exit 0 || { echo "FAULT NOT CAUGHT"; exit 1; }
else
  first=$(diff "$out/a.trace" "$out/b.trace" | head -3 | tr '\n' '|' | cut -c1-200)
  echo "$(basename "$a_play") save at $u, load at $v of $(basename "$b_play"): $na and $nb updates, the traces DIFFER: $first"; [ -n "$fault" ] && exit 0 || exit 1
fi
