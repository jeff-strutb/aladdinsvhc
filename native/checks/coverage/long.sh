#!/bin/sh
# long.sh STAGE SEED FRAMES : random play from a stage state, dense dumps, every window checked
M=${M:-$(git rev-parse --show-toplevel)}; CPU_RUN=${ALADDIN_CPU_RUN:?}; W=$M/build/aladdin/native/coverage-work; WT=${WTREE:-$M}
python3 $M/tools/aladdin/randplay.py play build/aladdin/states/stage_$1.state $2 $3 $W/gen_long_$1_$2.box
python3 $W/levelst.py stage_$1 $W/gen_long_$1_$2.box $3 > $W/lvl_$1_$2.txt 2>&1
first=$(sed -n 's/^first \([0-9]*\) last \([0-9]*\) kinds.*/\1/p' $W/lvl_$1_$2.txt); last=$(sed -n 's/^first \([0-9]*\) last \([0-9]*\) kinds.*/\2/p' $W/lvl_$1_$2.txt)
python3 $W/windows.py $W/rec $first $last $W/outl-$1-$2 > $W/win_$1_$2.txt 2>&1
rm -rf $W/rec.long$1_$2; mv $W/rec $W/rec.long$1_$2
