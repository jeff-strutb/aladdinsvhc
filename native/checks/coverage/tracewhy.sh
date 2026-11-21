#!/bin/sh
# tracewhy.sh SCRIPT FRAME: the control transfers (instruction addresses) of the original's frame FRAME that frame FRAME-1 did not have
# (snesbox --trace from a state saved after frame FRAME-2); the new edges are printed. 81:825F is LevelRamSetup (a restart of the level).
# Needs M, W, ALADDIN_CPU_RUN set; uses W/tr.
set -e
s=$1; f=$2; t=$W/tr/$s.$f; rm -rf $t; mkdir -p $t/o0 $t/t1 $t/t2
n=$(printf 'f%05d' $((f-2)))
awk -v n="dump $n" '{print} $0==n{print "save st"; exit}' $W/sd_$s.box > $t/a.box
cd $M
$ALADDIN_CPU_RUN -j 1 -m 2G -- build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script $t/a.box --out $t/o0 > /dev/null 2>&1
for k in 1 2; do
  printf 'load %s/o0/st.state\nwait %d\n' $t $k > $t/b$k.box
  $ALADDIN_CPU_RUN -j 1 -m 2G -- build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script $t/b$k.box --out $t/t$k --trace > /dev/null 2>&1
done
python3 - $t <<'P'
import sys
t=sys.argv[1]
def rd(p):
    d={}
    for l in open(p).read().split('\n')[1:]:
        x=l.split(',')
        if len(x)==5: d[tuple(x[:4])]=int(x[4])
    return d
a=rd(t+'/t1/edges.csv'); b=rd(t+'/t2/edges.csv')
new=sorted(k for k in b if k not in a and k[0]=='cpu')
print(' '.join('%s>%s'%(k[2],k[3]) for k in new if k[1] in ('020','022','06b','040')) )
print('RESTART' if any(k[3]=='819655' or k[2]=='81825f' for k in new) else 'no LevelRamSetup')
P
rm -rf $t/o0
