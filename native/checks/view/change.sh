#!/bin/sh
# change.sh APP OUTDIR PLAY LAST FROM TO U [K]: the check of one change of the view in play (VIEW.md 4.8, CHECK.md). APP is a macOS build's
# executable bundle (aladdin.app; the pictures need the Metal renderer); PLAY a file of native/checks/plays (p0-3.txt, ...) or - for the no-input
# boot; the play is run to LAST updates three times, each with the developer's trace and a picture every update from U to U + K (default 12):
# chg started at view FROM wide and changed to TO at update U (ALADDIN_DEV_VIEW_AT), same at FROM unchanged, fresh started at TO. Then
# change_report.py prints the traces and the pictures compared. CHANGE_ENV="NAME=VALUE ..." adds environment to all three runs (a developer's
# start at a screen: ALADDIN_DEV_SCREEN=genie). Run from the repository root.
set -u
app=$1; out=$2; play=$3; last=$4; from=$5; to=$6; u=$7; k=${8:-12}
root=$(pwd)
d=$out/$(basename "$play" .txt)-$from-$to-$u; rm -rf "$d"; mkdir -p "$d/chg" "$d/same" "$d/fresh"
res=$(cd "$(dirname "$app")" && pwd)/$(basename "$app")/Contents/Resources
bin=$(cd "$(dirname "$app")" && pwd)/$(basename "$app")/Contents/MacOS/aladdin
if [ "$play" = - ]; then in=""; cheat=0; else in="--input $root/native/checks/plays/$play"; cheat=1; fi
run() {   # name view [extra env]
  n=$1; v=$2; shift 2
  (cd "$(dirname "$bin")" && env PORT_RESDIR=$res ALADDIN_DEV_CHEAT=$cheat ALADDIN_VIEW=${v}x224 ALADDIN_DEV_TRACE=1 ALADDIN_DEV_ACTORS=2 ALADDIN_DEV_VIDEO=1 \
    ALADDIN_DEV_SHOTS=$d/$n ALADDIN_DEV_SHOT_EVERY=1 ALADDIN_DEV_SHOT_FROM=$u ${CHANGE_ENV:-} "$@" \
    perl -e 'alarm 900; exec @ARGV' "$bin" --headless --updates "$last" $in > "$d/$n.out" 2> "$d/$n.err"; echo $? > "$d/$n.exit")
}
run chg $from ALADDIN_DEV_VIEW_AT=$u:${to}x224 &
run same $from &
run fresh $to &
wait
echo "== $play $from -> $to at update $u, to $last: exits chg $(cat "$d/chg.exit") same $(cat "$d/same.exit") fresh $(cat "$d/fresh.exit")"
python3 "$root/native/checks/view/change_report.py" "$d" $from $to $u $k
