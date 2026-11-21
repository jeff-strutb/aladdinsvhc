import sys,os,subprocess
import os; M=os.environ.get('M','/home/strutb/projects/strutb/aladdinsvhc'); WT=os.environ.get('WTREE',M); W=os.environ.get('W',M+'/build/aladdin/native/coverage-work')
st=sys.argv[1]; play=sys.argv[2]; maxlen=int(sys.argv[3]) if len(sys.argv)>3 else 2400; ST=M+'/tools/aladdin/states/03_stages.box'; PL=(play if '/' in play else M+'/tools/aladdin/scripts/%s.box'%play)
run=os.environ['ALADDIN_CPU_RUN']
def snes(box,out):
    os.makedirs(out,exist_ok=True)
    subprocess.run([run,'-j','1','-m','2G','--','timeout','1500',M+'/build/aladdin/box/snesbox','--rom',M+'/build/aladdin/baserom.sfc','--script',box,'--out',out],cwd=M,stdout=open(box+'.log','w'),stderr=subprocess.STDOUT,check=True)
def sd(out,a,b,env=None):
    e=dict(os.environ); 
    if env: e.update(env)
    subprocess.run(['python3',WT+'/native/checks/stage_dump.py',out,str(a),str(b),ST,st,PL],check=True,env=e)
sd(W+'/c.box',1,0,{'STEP':'20'})
subprocess.run(['rm','-rf',W+'/c']); snes(W+'/c.box',W+'/c')
rows=[]
for fn in sorted(os.listdir(W+'/c')):
    w=open(W+'/c/'+fn,'rb').read(); f=int(fn[1:6])
    rows.append((f,w[0x35E]|w[0x35F]<<8,sum(1 for k in range(36) if w[0x8DB+0x50*k])))
# gameplay level: first level (not 0,44,48,37?) with records>=2 after being 0
start=None
for i in range(1,len(rows)):
    if rows[i][2]>=1 and rows[i-1][2]==0 and rows[i][1] not in (0,44,48,57):
        start=rows[i-1][0]; lvl=rows[i][1]; break
print('level',lvl,'loaded near',start)
subprocess.run(['rm','-rf',W+'/c'])
a=start-19; b=a+maxlen
sd(W+'/r.box',a,b)
subprocess.run(['rm','-rf',W+'/rec']); snes(W+'/r.box',W+'/rec')
# first dump with no records just before the first record
first=None
for f in range(a,b+1):
    if not os.path.exists(W+'/rec/f%05d.wram'%f): break
    w=open(W+'/rec/f%05d.wram'%f,'rb').read()
    live=sum(1 for k in range(36) if w[0x8DB+0x50*k])
    if live>0 and f>a: first=f-1; break
kinds=set(); end=b
for f in range(first,b+1):
    if not os.path.exists(W+'/rec/f%05d.wram'%f): end=f-1; break
    w=open(W+'/rec/f%05d.wram'%f,'rb').read()
    if (w[0x35E]|w[0x35F]<<8)!=lvl: end=f-1; break
    kinds|=set(w[0x8DB+0x50*k+6] for k in range(36) if w[0x8DB+0x50*k])
print('first',first,'last',end,'kinds',' '.join('%02X'%k for k in sorted(kinds)))
