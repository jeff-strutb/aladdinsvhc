#!/bin/sh
# join.sh BASE_BIN NEW_BIN: the proof for the level end's removal (DEPARTURES.md section 8, CHECK.md "The level end's removal"). The plays that pass a
# level's end (p0-3, p6-0, end-1-1, end-2-1, end-3-0, end-3-1, end-3-2, end-4-2, boss16-2hits) are run twice with the developer traces: the
# build with the level-end screen (base-b36b712) with the play as it was (old-flow/), the new build with the play moved back by
# convert_join.py (this folder), and join_compare.py says, for each, that before the screen the traces are identical and after it the next
# screen's traces are identical with the update numbers moved back by the screen's length, or which fields differ. boss16-2hits is
# run to 10500 with the boss's patch (boss-flow.sh 16). The other plays that pass no level end (every other file of this folder) are unchanged and checked by
# native/checks/view/same.sh. Run from the repository root. Exit 0: every comparison identical.
set -u
base=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
new=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
root=$(pwd)
here=$root/native/checks/plays
out=${JOIN_OUT:-$root/build/aladdin/native/join}
rm -rf "$out"; mkdir -p "$out"
one() {   # name
  n=$1
  case $n in
    end-1-1) at=3684,328;; end-2-1) at=7028,128;; end-3-0|end-3-1) at=3940,136;; end-3-2) at=3172,136;; end-4-2) at=2916,168;; *) at=;;
  esac
  own=; last=
  if [ $n = boss16-2hits ]; then own=8000:98:1:1; last=10500; fi
  [ -z "$last" ] && last=$(awk '$1 ~ /^[0-9]+$/ { m = $1 } END { print m + 600 }' "$here/old-flow/$n.txt")
  for side in base new; do
    if [ $side = base ]; then bin=$base; file=$here/old-flow/$n.txt; else bin=$new; file=$here/$n.txt; fi
    res=$(dirname "$bin")/../Resources; [ -f "$res/cart.dat" ] || res=$(dirname "$bin")
    (cd "$(dirname "$bin")" && env PORT_SAVEDIR=$out/save.$n.$side PORT_RESDIR=$res ALADDIN_DEV_CHEAT=1 ${at:+ALADDIN_DEV_AT=$at} ${own:+ALADDIN_DEV_OWN=$own} \
      ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 perl -e 'alarm 900; exec @ARGV' "$bin" --headless --updates "$last" --input "$file" \
      > "$out/$n.$side.out" 2> "$out/$n.$side.err"); echo $? > "$out/$n.$side.exit"
  done
  echo "== $n (base $last updates; exit $(cat "$out/$n.base.exit") / $(cat "$out/$n.new.exit"))"
  python3 -I "$here/join_compare.py" "$out/$n.base.err" "$out/$n.new.err"
}
fail=0
for n in p0-3 p6-0 end-1-1 end-2-1 end-3-0 end-3-1 end-3-2 end-4-2 boss16-2hits; do
  one $n > "$out/$n.result" 2>&1 &
  while [ "$(jobs -r | wc -l)" -ge "${JOIN_JOBS:-4}" ]; do sleep 1; done
done
wait
for n in p0-3 p6-0 end-1-1 end-2-1 end-3-0 end-3-1 end-3-2 end-4-2 boss16-2hits; do cat "$out/$n.result"; done
grep -q "DIFFER\|difference\|exit [1-9]" "$out"/*.result && fail=1
echo "traces in $out"
exit $fail
