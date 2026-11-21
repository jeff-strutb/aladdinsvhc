#!/usr/bin/env python3
"""to_box.py PLAY.txt OUT.box LAST STEP FROM: an --input play (UPDATE keys lines) as a snesbox script from power-on, one frame a native update,
with a work-RAM dump every STEP frames from FROM (dNNNNNN). Keys: 26 up 28 down 25 left 27 right 0D start 5A B 41 Y 58 A 57 L; controller=1,BITS pad 2
(ignored: the options code is replaced by a poke of $0009 at update 3180, as ALADDIN_DEV_CHEAT=1)."""
import sys
inp,out,last,step,frm=sys.argv[1],sys.argv[2],int(sys.argv[3]),int(sys.argv[4]),int(sys.argv[5])
K={'26':'UP','28':'DOWN','25':'LEFT','27':'RIGHT','0D':'START','5A':'B','41':'Y','58':'A','57':'L'}
P2={0x10:'L',0x20:'R',0x80:'START',0x40:'SELECT',0x08:'X',0x04:'Y',0x02:'A',0x01:'B'}
import re
m=re.search(r'p(\d)-(\d)',inp); PART,STAGE=int(m.group(1)),int(m.group(2))
import os
SH=int(os.environ.get('START_EARLIER','0'))    # the original's level load takes about 59 frames longer than native's: START that much earlier lines the level's first frame up
ev={}
for l in open(inp):
    t=l.split()
    if not t: continue
    u=int(t[0]); p1=[];p2=[]
    if 3890<=u<=3910: u-=SH
    for w in t[1:]:
        if w.startswith('key='): p1.append(K.get(w[4:].upper(),'?'+w))
        elif w.startswith('controller=1,'):
            b=int(w[13:],16); p2=[n for m,n in P2.items() if b&m]
    ev[u]=(p1,p2)
cur=([],[]); lines=[]
for u in range(0,last):
    if u in ev: cur=ev[u]
    keys=cur[0]    # pad 2 is not driven: snesbox's 2.X 2.Y 2.A 2.B reach pad 1; the cheat is poked below
    if u==3180: lines.append('poke $000009 01')
    if u==3880-SH: lines.append('poke $00035C %02X' % PART); lines.append('poke $00035D %02X' % STAGE)
    lines.append(('hold %s 1'%'+'.join(keys)) if keys else 'wait 1')
    if u+1>=frm and (u+1-frm)%step==0: lines.append('dump d%06d'%(u+1))
open(out,'w').write('\n'.join(lines)+'\n')
