#!/bin/sh
# covrun.sh SCRIPT KIND[,KIND..]: record the decomp's script with a dump after every frame, check the kinds over it (first dump = frame 1), keep the result lines in covres/SCRIPT.txt
M=${M:-$(git rev-parse --show-toplevel)}; CPU_RUN=${ALADDIN_CPU_RUN:?}; W=$M/build/aladdin/native/coverage-work; WT=${WTREE:-$M}
mkdir -p $W/covres; R=$W/covrec_$1; rm -rf $R; mkdir -p $R
cd $M
timeout 1700 $CPU_RUN -j 1 -m 2G -- build/aladdin/box/snesbox --rom build/aladdin/baserom.sfc --script $W/sd_$1.box --out $R > $W/covres/$1.log 2>&1
last=$(ls $R | grep -c wram)
export ALADDIN_CPU_RUN=$CPU_RUN BUILD=$M/build/aladdin/native/checks/actors OUT=$W/covres/out_$1
python3 - $R > $W/covres/$1.kinds <<'PY'
import sys
R=sys.argv[1]; seen={}
import os
for fn in sorted(os.listdir(R)):
    if not fn.endswith('.wram'): continue
    w=open(R+'/'+fn,'rb').read(); f=int(fn[1:6])
    for n in range(36):
        r=0x8DB+0x50*n
        if w[r]: seen.setdefault(w[r+6],[f,f])[1]=f
print(' '.join('%02X:%d-%d'%(k,v[0],v[1]) for k,v in sorted(seen.items())))
PY
(cd $WT && native/checks/actor_check_kinds.sh $R 1 $last $2 > $W/covres/$1.txt 2>&1)
