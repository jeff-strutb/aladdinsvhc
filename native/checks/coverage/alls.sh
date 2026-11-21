#!/bin/sh
# all.sh N PLAY: record level of password N, check every kind met
M=${M:-$(git rev-parse --show-toplevel)}; CPU_RUN=${ALADDIN_CPU_RUN:?}; W=$M/build/aladdin/native/coverage-work; WT=${WTREE:-$M}
export ALADDIN_CPU_RUN=$CPU_RUN BUILD=$M/build/aladdin/native/checks/actors OUT=$W/outs-$1
python3 $W/levelst.py $1 $2 > $W/lvs$1.txt 2>&1
first=$(sed -n 's/^first \([0-9]*\) last \([0-9]*\) kinds.*/\1/p' $W/lvs$1.txt); last=$(sed -n 's/^first \([0-9]*\) last \([0-9]*\) kinds.*/\2/p' $W/lvs$1.txt)
kinds=$(sed -n 's/.*kinds //p' $W/lvs$1.txt | tr ' ' '\n' | grep -v '^00$' | tr '\n' ' ')
cd $WT && native/checks/actor_check_kinds.sh $W/rec $first $last $kinds >> $W/lvs$1.txt 2>&1
rm -rf $W/rec.keeps$1; mv $W/rec $W/rec.keeps$1
