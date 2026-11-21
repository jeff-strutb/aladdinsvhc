import sys,os,subprocess,re
# windows.py REC FIRST LAST OUTDIR: split FIRST..LAST at record-free frames (restarts); per window check every kind met (not 00); print one line per kind: window, kind, differ, compared
import os; M=os.environ.get('M','/home/strutb/projects/strutb/aladdinsvhc'); WT=os.environ.get('WTREE',M)
rec,first,last,outd=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]),sys.argv[4]
live={}
kinds={}
lvl={}
for f in range(first,last+1):
    p='%s/f%05d.wram'%(rec,f)
    if not os.path.exists(p): continue
    w=open(p,'rb').read()
    ks=set(w[0x8DB+0x50*n+6] for n in range(36) if w[0x8DB+0x50*n]); live[f]=len(ks)>0; kinds[f]=ks; lvl[f]=w[0x35E]
wins=[];start=None
fs=sorted(live)
for i,f in enumerate(fs):
    if not live[f]:
        if start is not None and f-1>start+30: wins.append((start,f-1))
        start=f   # last record-free frame so far = the candidate first dump
        continue
# windows: from a free frame s to the frame before the next free frame
wins=[];cur=None
for f in fs:
    if not live[f]:
        if cur and cur[1]>cur[0]+30: wins.append(cur)
        cur=[f,f]
    elif cur: cur[1]=f
if cur and cur[1]>cur[0]+30: wins.append(cur)
env=dict(os.environ,ALADDIN_CPU_RUN=os.environ['ALADDIN_CPU_RUN'],BUILD=M+'/build/aladdin/native/checks/actors',OUT=outd)
for a,b in wins:
    ks=set()
    for f in range(a,b+1): ks|=kinds[f]
    ks.discard(0)
    if not ks: continue
    r=subprocess.run([WT+'/native/checks/actor_check_kinds.sh',rec,str(a),str(b)]+['%02X'%k for k in sorted(ks)],capture_output=True,text=True,env=env,cwd=WT)
    for line in r.stdout.strip().split('\n'):
        print('window %d-%d level %d: %s'%(a,b,lvl[a+1] if a+1 in lvl else -1,line[:200]))
