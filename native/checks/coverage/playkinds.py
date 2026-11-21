import sys,os,subprocess,re
# playkinds.py DIR FIRST LAST : one check run for each kind (but 00) met in DIR's dumps FIRST..LAST; per kind: live frames, fields compared/differ,
# the first differing frame and the mass-free frames (3 or more records gone in one frame: a restart or a level's end) up to it
d,a,b=sys.argv[1],int(sys.argv[2]),int(sys.argv[3]); seen={}; prev=None; mass=[]
for f in range(a,b+1):
    fn='%s/f%05d.wram'%(d,f)
    if not os.path.exists(fn): continue
    w=open(fn,'rb').read(); live=set()
    for n in range(1,36):
        r=0x8DB+0x50*n
        if w[r]: live.add(n); seen.setdefault(w[r+6],[f,f])[1]=f
    if prev is not None and len(prev-live)>=3: mass.append(f)
    prev=live
# the first dump must precede the first record (and be the level's): the frame before any record is live
for f in range(a,b+1):
    fn='%s/f%05d.wram'%(d,f)
    if not os.path.exists(fn): continue
    w=open(fn,'rb').read(0x8DB+0x50*36)
    if any(w[0x8DB+0x50*n] for n in range(36)):
        a=max(a,f-1); break
print('%s %d-%d mass-free frames: %s'%(os.path.basename(d),a,b,' '.join(map(str,mass))))
env=dict(os.environ)
for k in sorted(seen):
    if k==0: continue
    kk='%02X'%k
    subprocess.run([os.environ['WTREE']+'/native/checks/actor_check_kinds.sh',d,str(a),str(b),kk],capture_output=True,text=True,env=env,cwd=os.environ['WTREE'])
    t=open('%s/kind_%s.txt'%(os.environ['OUT'],kk)).read()
    m=re.search(r'actor check: frames (\d+)-(\d+), (\d+) fields compared, (\d+) differ',t)
    x=re.search(r'actor check: kind \$%s: (\d+) fields differ, the first at frame (\d+)'%kk,t)
    first=int(x.group(2)) if x else None
    rest=[mf for mf in mass if first and mf<=first+2]
    # with the player's own turn listed too (00): kinds that read him see his x/y as his turn left them, not the dump's end of frame
    w00=''
    if kk!='00':
        subprocess.run([os.environ['WTREE']+'/native/checks/actor_check_kinds.sh',d,str(a),str(b),'00,'+kk],capture_output=True,text=True,env=env,cwd=os.environ['WTREE'])
        t2=open('%s/kind_00,%s.txt'%(os.environ['OUT'],kk)).read()
        x2=re.search(r'actor check: kind \$%s: (\d+) fields differ, the first at frame (\d+)'%kk,t2)
        w00=' | with 00: %s'%(('%s differ, first %s'%x2.groups()) if x2 else '0 differ')
    print('  $%s live %d-%d: %s fields, %s differ%s%s'%(kk,seen[k][0],seen[k][1],m.group(3) if m else '?',m.group(4) if m else '?',', first %d'%first if first else '', (' (restart/free at %s)'%rest[-1]) if rest else (' (NO mass-free before)' if first else '')) + w00)
