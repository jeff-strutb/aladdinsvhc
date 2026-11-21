import os,re,sys,collections,json
# agg2.py WORKDIR : per registered kind, the evidence of the decomp's coverage scripts (WORKDIR/covresF: per script sections, per kind
# the check's result, the mass-free frames, the free-all events) and of the plays (WORKDIR/plays*.txt, lv/plays_*.txt): sections where it
# is exact, sections where it differs and why (restart / free-all / poke), and the unexplained differences
W=sys.argv[1]; M=os.environ.get('M','/home/strutb/projects/strutb/aladdinsvhc'); WT=os.environ.get('WTREE',M)
owner={}
for f in 'a b c d e game player'.split():
    for l in open('%s/native/aladdin/src/actor/kinds_%s.c'%(WT,f)):
        m=re.match(r'\s*\{\s*0x([0-9A-Fa-f]{2})\s*,',l)
        if m: owner.setdefault(m.group(1).upper(),f)
def pokes(s):
    out=[];f=0
    for l in open('%s/sd_%s.box'%(W,s)):
        t=l.split()
        if not t: continue
        if t[0]=='dump': f=int(t[1][1:])
        elif t[0]=='poke':
            a=int(t[1].lstrip('$'),16)&0xFFFFFF
            if a>>16==0x7E: a&=0xFFFF
            c='rec' if 0x92B<=a<0x1500 else 'player' if 0x8DB<=a<0x92B else 'other'
            out.append((f+1,c,a))
    return out
ev=collections.defaultdict(lambda:{'ok':[],'restart':[],'freeall':[],'poke':[],'unexpl':[],'live':{}})
D0=W+'/covresH'
def process(s,sectfile,massfile,kindsfile):
    mass=[int(x) for x in open(massfile).read().split()] if os.path.exists(massfile) else []
    live={}
    if os.path.exists(kindsfile):
        for t in open(kindsfile).read().split():
            k,r=t.split(':'); a,b=r.split('-'); live[k]=(int(a),int(b))
    ps=pokes(s) if os.path.exists('%s/sd_%s.box'%(W,s)) else []
    sec=None; fa=[]
    for l in open(sectfile):
        m=re.match(r'%s section (\d+)-(\d+):'%re.escape(s),l)
        if m: sec=(int(m.group(1)),int(m.group(2))); fa=[]; continue
        m=re.match(r'\s*free-all at (.*)',l)
        if m: fa=[int(x) for x in m.group(1).split(',')]; continue
        m=re.match(r'kind \$([0-9A-F]{2}): (.*)\| actor check: frames [\d-]+, (\d+) fields compared, (\d+) differ',l)
        if not (m and sec): continue
        k=m.group(1); n=int(m.group(3))
        if k=='00' or n==0: continue
        lr=live.get(k)
        if not lr or lr[1]<sec[0] or lr[0]>sec[1]: continue      # not live in this section: nothing was compared of it
        e=ev[k]; e['live'][s]=live.get(k)
        d=re.search(r'(\d+) fields differ, the first at frame (\d+)',m.group(2))
        if not d: e['ok'].append((s,sec,n,max(0,min(sec[1],lr[1])-max(sec[0],lr[0])+1))); continue
        nd,ff=int(d.group(1)),int(d.group(2))
        rs=[x for x in mass if sec[0]<=x<=ff+2]
        pk=[p for p in ps if sec[0]<=p[0]<=ff]
        if rs and abs(ff-rs[-1])<=3: e['restart'].append((s,sec,ff,rs[-1],nd))
        elif [x for x in fa if x<=ff and ff-x<=3]: e['freeall'].append((s,sec,ff,nd))
        elif any(p[1]=='rec' for p in pk): e['poke'].append((s,sec,ff,'record',nd))
        elif any(p[1]=='player' for p in pk): e['poke'].append((s,sec,ff,'player',nd))
        else: e['unexpl'].append((s,sec,ff,nd))
for fn in sorted(os.listdir(D0)):
    if not fn.endswith('.sect'): continue
    s=fn[:-5]
    process(s,D0+'/'+fn,D0+'/'+s+'.mass',D0+'/'+s+'.kinds')
    if os.path.exists(D0+'/'+s+'.sect.hi'):        # frames beyond snesbox's 9999 (longcheck.sh): its sections after the first 9000 frames
        process(s,D0+'/'+s+'.sect.hi',D0+'/'+s+'.mass.hi',D0+'/'+s+'.kinds.hi')
json.dump({'owner':owner,'ev':ev},open(W+'/agg2.json','w'),default=list)
for k in sorted(owner):
    e=ev.get(k)
    if not e: print('$%s [%s] no script section meets it'%(k,owner[k])); continue
    print('$%s [%s] ok %d, restart %d, free-all %d, poke %d, UNEXPLAINED %d'%(k,owner[k],len(e['ok']),len(e['restart']),len(e['freeall']),len(e['poke']),len(e['unexpl'])))
    for u in e['unexpl'][:6]: print('      unexpl %s %d-%d first %d (%d differ)'%(u[0],u[1][0],u[1][1],u[2],u[3]))
