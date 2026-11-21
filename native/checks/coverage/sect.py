import sys,os,subprocess,re
# sect.py SCRIPT KINDS : check each section between `load`s of the replayed script separately, KINDS (comma list, all listed
# together: the native runs them all, the rest follows the dump); prints per section the fields compared and, per kind, the
# fields that differ and the first frame (a kind not named has none)
M=os.environ.get('M','/home/strutb/projects/strutb/aladdinsvhc'); W=os.environ.get('W',M+'/build/aladdin/native/coverage-work'); WT=os.environ.get('WTREE',M)
s,kinds=sys.argv[1],sys.argv[2]
loads=[];f=0
for l in open('%s/sd_%s.box'%(W,s)):
    l=l.strip()
    if l.startswith('dump f'): f=int(l[6:])
    elif l.startswith('load'): loads.append(f+1)   # the load takes effect before frame f+1
fr=sorted(int(x[1:6]) for x in os.listdir('%s/covrec_%s'%(W,s)) if x.endswith('.wram'))
first,last=fr[0],fr[-1]
if os.environ.get('LEVELSPLIT'):
    # also a section at each frame the level number ($035E) changes: the game's own level load, a first dump with the new level's tables
    prevl=None
    for f in range(first,last+1):
        w=open('%s/covrec_%s/f%05d.wram'%(W,s,f),'rb').read(0x360)
        lv=w[0x35E]|w[0x35F]<<8
        if prevl is not None and lv!=prevl: loads.append(f)
        prevl=lv
    # and where the records first appear after none (a level's start): a window from just before them
    liv=None
    for f in range(first,last+1):
        w=open('%s/covrec_%s/f%05d.wram'%(W,s,f),'rb').read(0x8DB+0x50*36)
        n=sum(1 for k in range(36) if w[0x8DB+0x50*k])
        if liv==0 and n>0: loads.append(max(first,f-2))
        liv=n
    loads=([loads[0]] if loads else [first])+sorted(set(loads[1:]))
bounds=[first]+[x for x in loads[1:] if x>first]+[last+1]
out=os.environ.get('OUT',W+'/sect_out')
env=dict(os.environ,ALADDIN_CPU_RUN=os.environ['ALADDIN_CPU_RUN'],BUILD=os.environ.get('BUILD',M+'/build/aladdin/native/checks/actors'),OUT=out)
for a,b in zip(bounds,bounds[1:]):
    # KINDS "+KK,KK": all listed together; else one run for each kind (the others follow the dump)
    groups=[kinds[1:]] if kinds.startswith('+') else kinds.split(',')
    print('%s section %d-%d:'%(s,a,b-1))
    for g in groups:
        subprocess.run([WT+'/native/checks/actor_check_kinds.sh','%s/covrec_%s'%(W,s),str(a),str(b-1),g],capture_output=True,text=True,env=env,cwd=WT)
        f='%s/kind_%s.txt'%(out,g)
        t=open(f).read() if os.path.exists(f) else ''
        m=re.search(r'actor check: frames (\d+)-(\d+), (\d+) fields compared, (\d+) differ',t)
        d=re.search(r'actor check: kind \$%s: (\d+) fields differ, the first at frame (\d+)'%g,t)
        ev=re.findall(r'frame (\d+): ActorsFreeAll',t)
        if ev: print('  free-all at %s'%','.join(ev[:8]))
        if not m: print('kind $%s: NO SUMMARY'%g)
        else: print('kind $%s: %s | actor check: frames %s-%s, %s fields compared, %s differ'%(g,('actor check: kind $%s: %s fields differ, the first at frame %s'%(g,d.group(1),d.group(2))) if d else '0 fields differ',m.group(1),m.group(2),m.group(3),m.group(4)))
