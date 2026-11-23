#!/bin/sh
# prove_convert.sh BIN OLD_DIR NEW_DIR: the proof that the plays converted by convert_prefix.py (NEW_DIR) play as the originals (OLD_DIR) did, on
# one build that still has the options screen (the build before the title menu changed). Each play (p*.txt, boss16-2hits.txt, and
# end-*.txt, run with ALADDIN_DEV_AT from the list in README.txt) is run to its last key + 600 twice, the original as it
# was and the converted one with ALADDIN_DEV_CHEAT=1, both with the developer traces (the player's line, every live actor with its own
# bytes, the video fields), and the traces compared from update 3900 (the title's Start) on: the same level, the same play to its exit and
# after it. Exit 0: every pair the same. Run from the repository root.
set -u
bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
old=$(cd "$2" && pwd); new=$(cd "$3" && pwd)
out=${PROVE_OUT:-$(pwd)/build/aladdin/native/prove}
rm -rf "$out"; mkdir -p "$out"
res=$(dirname "$bin")/../Resources
[ -f "$res/cart.dat" ] || res=$(dirname "$bin")
one() {   # file
  f=$1; n=$(basename "$f" .txt)
  case $n in
    end-0-1) at=2404,200;; end-0-2) at=4196,392;; end-1-0) at=3940,300;; end-1-1) at=3684,328;; end-2-0) at=4452,104;;
    end-2-1) at=7028,128;; end-3-0) at=3940,136;; end-3-1) at=3940,136;; end-3-2) at=3172,136;; end-4-0) at=2404,392;;
    end-4-1) at=2916,136;; end-4-2) at=2916,168;; end-5-0) at=3940,232;; end-5-2) at=4708,168;; end-5-3) at=2604,351;;
    *) at=;;
  esac
  atenv=${at:+ALADDIN_DEV_AT=$at}
  last=$(awk '$1 ~ /^[0-9]+$/ { n = $1 } END { print n + 600 }' "$old/$f")
  for side in old new; do
    if [ $side = old ]; then dir=$old; cheat=0; else dir=$new; cheat=1; fi
    (cd "$(dirname "$bin")" && env PORT_RESDIR=$res ALADDIN_DEV_CHEAT=$cheat $atenv ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 \
      perl -e 'alarm 900; exec @ARGV' "$bin" --headless --updates "$last" --input "$dir/$f" > "$out/$n.$side.out" 2> "$out/$n.$side.raw")
    echo $? > "$out/$n.$side.exit"
    awk '/^u=3900 /{on=1} on' "$out/$n.$side.raw" > "$out/$n.$side.err"; rm -f "$out/$n.$side.raw"
  done
  if [ "$(cat "$out/$n.old.exit")" != 0 ] || [ "$(cat "$out/$n.new.exit")" != 0 ]; then
    echo "DIFFERS $n: a run failed (exit $(cat "$out/$n.old.exit") / $(cat "$out/$n.new.exit"))"
  elif cmp -s "$out/$n.old.err" "$out/$n.new.err"; then
    echo "same    $n ($last updates, $(grep -c '^u=' "$out/$n.new.err") traced from 3900)"
  else
    echo "DIFFERS $n: first difference at line $(cmp "$out/$n.old.err" "$out/$n.new.err" | sed -n 's/.* line \([0-9]*\).*/\1/p'), update $(cmp "$out/$n.old.err" "$out/$n.new.err" >/dev/null; awk -v l="$(cmp "$out/$n.old.err" "$out/$n.new.err" | sed -n 's/.* line \([0-9]*\).*/\1/p')" 'NR<=l && /^u=/{u=$1} END{print u}' "$out/$n.old.err")"
  fi
}
files=$(cd "$old" && ls p*.txt boss16-2hits.txt end-*.txt)
for f in $files; do
  one "$f" > "$out/$f.res" &
  while [ "$(jobs -r | wc -l)" -ge "${PROVE_JOBS:-4}" ]; do sleep 1; done
done
wait
cat "$out"/*.res | sort
cat "$out"/*.res | grep -q DIFFERS && exit 1
exit 0
