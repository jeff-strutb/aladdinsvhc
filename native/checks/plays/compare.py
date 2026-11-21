#!/usr/bin/env python3
"""compare.py ORIGDIR NATIVE_ACTORS.txt FROM TO [TICK0] : the original's dumps (to_box.py, one a frame, d%06d; original frame u+1 = native update u) against the native trace
(ALADDIN_DEV_ACTORS=1 ALADDIN_DEV_TRACE=1 lines): the first update where the player (x y health) differs and the first where the actor list (kind, x, y) differs."""
import re,struct,sys,os
ALIAS={int(x.split(':')[0],16):int(x.split(':')[1],16) for x in os.environ.get('KIND_ALIAS','').split(',') if x}   # KIND_ALIAS=97:53: the native kind $97 counts as $53 (DEPARTURES.md departure 3)
d,nf,a,b=sys.argv[1],sys.argv[2],int(sys.argv[3]),int(sys.argv[4])
nat={};cur=None;pl={}
for l in open(nf):
    m=re.match(r'u=(\d+) level=\d+ x=(\d+) y=(\d+) health=(\d+)',l)
    if m: cur=int(m.group(1)); nat[cur]={}; pl[cur]=tuple(map(int,m.groups()[1:])); continue
    m=re.match(r'  actor (\d+) kind=(\w+) state=(\w+) x=(\d+) y=(\d+)',l)
    if m and cur is not None: nat[cur][int(m.group(1))]=(ALIAS.get(int(m.group(2),16),int(m.group(2),16)),int(m.group(4)),int(m.group(5)))
# the original runs its logic at half rate where it is slowed down (the play-time tick $147C/$147D steps every other frame): the native update u is the
# original frame at which its tick has stepped u-a times since frame a+1 (a logic frame, not a video frame)
def tick(f):
    w=open('%s/d%06d.wram'%(d,f),'rb').read(0x1500); return w[0x147D]*60+w[0x147C]
import os
frame_of={}
if len(sys.argv)>5:
    # TICK0 (a recording made with lbox.sh/to_box.py LH_FROM: 3936 is the level's second logic frame, tick 2): update u is the first original frame
    # at which the play-time tick reads TICK0 + u - a
    # (the frames from the level's first logic frame on: the first with tick 1 after the pre-level dumps, which are full size)
    t0=int(sys.argv[5]); seen={}; fs=sorted(int(fn[1:7]) for fn in os.listdir(d) if fn.endswith('.wram'))
    big=[f for f in fs if os.path.getsize('%s/d%06d.wram'%(d,f))>20000]
    f1=next(f for f in fs if f>big[-1] and tick(f)==1)
    for f in fs:
        if f>=f1: seen.setdefault(tick(f),f)
    for u in range(a,b):
        if t0+u-a in seen: frame_of[u]=seen[t0+u-a]
else:
    # the older recording, one video frame a native update: the original's logic frames are the frames at which its tick has stepped
    steps=0; t0=tick(a+1); frame_of[a]=a+1; prev=t0
    f=a+2
    while os.path.exists('%s/d%06d.wram'%(d,f)):
        t=tick(f)
        if t!=prev: steps+=1; frame_of[a+steps]=f; prev=t
        f+=1
fp=fa=None
for u in range(a,b):
    if u not in frame_of: break
    w=open('%s/d%06d.wram'%(d,frame_of[u]),'rb').read(0x1500)
    r=0x8DB
    o=(struct.unpack('<H',w[r+0x0C:r+0x0E])[0],struct.unpack('<H',w[r+0x0F:r+0x11])[0],w[0x367])
    if fp is None and u in pl and o!=pl[u]: fp=u; print('player first differs at update %d: original (x,y,health) %s native %s'%(u,o,pl[u]))
    oa={}
    for n in range(1,36):
        q=0x8DB+0x50*n
        if w[q]: oa[n]=(ALIAS.get(w[q+6],w[q+6]),struct.unpack('<H',w[q+0x0C:q+0x0E])[0],struct.unpack('<H',w[q+0x0F:q+0x11])[0])
    na={k:v for k,v in nat.get(u,{}).items() if k}
    if fa is None and u in nat and oa!=na:
        fa=u; dd=[(k,oa.get(k),na.get(k)) for k in sorted(set(oa)|set(na)) if oa.get(k)!=na.get(k)]
        print('actors first differ at update %d: (record, original, native) %s'%(u,dd[:5]))
    if fp is not None and fa is not None: break
print('player',fp,'actors',fa)
