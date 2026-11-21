import sys,os,subprocess
# sect.py SCRIPT KINDS : check each section between `load`s of the replayed script separately
import os; M=os.environ.get('M','/home/strutb/projects/strutb/aladdinsvhc'); W=M+'/build/aladdin/native/coverage-work'; WT=os.environ.get('WTREE',M)
s,kinds=sys.argv[1],sys.argv[2]
loads=[];f=0
for l in open('%s/sd_%s.box'%(W,s)):
    l=l.strip()
    if l.startswith('dump f'): f=int(l[6:])
    elif l.startswith('load'): loads.append(f+1)   # the load takes effect before frame f+1
last=len([x for x in os.listdir('%s/covrec_%s'%(W,s)) if x.endswith('.wram')])
bounds=[1]+loads[1:]+[last+1]
env=dict(os.environ,ALADDIN_CPU_RUN=os.environ['ALADDIN_CPU_RUN'],BUILD=M+'/build/aladdin/native/checks/actors',OUT=W+'/sect_out')
for a,b in zip(bounds,bounds[1:]):
    r=subprocess.run([WT+'/native/checks/actor_check_kinds.sh','%s/covrec_%s'%(W,s),str(a),str(b-1),kinds],capture_output=True,text=True,env=env,cwd=WT)
    print('%s section %d-%d: %s'%(s,a,b-1,r.stdout.strip()[:230]))
