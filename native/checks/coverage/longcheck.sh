#!/bin/sh
# longcheck.sh SCRIPT SPLIT: longrec.sh, then the kinds met in the recorded part checked per section (sect.py, LEVELSPLIT) into covresH/SCRIPT.sect (replacing the 9999-frame one)
set -e
s=$1; n=$2
sh $WTREE/native/checks/coverage/longrec.sh $s $n > /dev/null
python3 - $W/covrec_$s > $W/covresH/$s.kinds.hi <<'P'
import sys,os
R=sys.argv[1]; seen={}
for fn in sorted(os.listdir(R)):
    if not fn.endswith('.wram'): continue
    w=open(R+'/'+fn,'rb').read(); f=int(fn[1:6])
    for k in range(36):
        r=0x8DB+0x50*k
        if w[r]: seen.setdefault(w[r+6],[f,f])[1]=f
print(' '.join('%02X:%d-%d'%(k,v[0],v[1]) for k,v in sorted(seen.items())))
P
ks=$(tr ' ' '\n' < $W/covresH/$s.kinds.hi | cut -d: -f1 | grep -v '^00$' | tr '\n' ',' | sed 's/,$//')
python3 $WTREE/native/checks/coverage/mass.py $W/covrec_$s > $W/covresH/$s.mass.hi
LEVELSPLIT=1 OUT=$W/soutH python3 $WTREE/native/checks/coverage/sect.py $s $ks > $W/covresH/$s.sect.hi 2>&1
rm -rf $W/covrec_$s $W/long_$s/a
