#!/bin/sh
# fd.sh SCRIPT FIRST LAST KIND[,KIND]: the recording of SCRIPT (kept in W/covrec_SCRIPT; delete it when done) and the first differing fields of one check run
[ -d $W/covrec_$1 ] || { sh $WTREE/native/checks/coverage/covrun.sh $1 00 >/dev/null 2>&1; }
OUT=$W/fd native/checks/actor_check_kinds.sh $W/covrec_$1 $2 $3 $4 | cut -c1-200
grep -v '^anim\|already running\|made new' $W/fd/kind_$4.txt | head -${N:-8}
