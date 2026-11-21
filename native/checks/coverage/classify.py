import os,sys,subprocess,json,collections
# classify.py WORKDIR : for each unexplained difference of agg2.json (script, section, first frame), re-records the script (kept until all its items are done) and tells
# what the original did around that frame: the loop's cut ($032F/$0330 nonzero: a lag frame), records gone/new, the player's state; one line each
W=sys.argv[1]; WT=os.environ['WTREE']
d=json.load(open(W+'/agg2.json'))
items=collections.defaultdict(set)
for k,e in d['ev'].items():
    for u in e['unexpl']: items[u[0]].add(u[2])
for s in sorted(items):
    R='%s/covrec_%s'%(W,s)
    made=False
    if not os.path.isdir(R):
        sd=open('%s/sd_%s.box'%(W,s)).read().count('\ndump')
        if sd>=9990: print(s,'longer than 9999 frames: not classified'); continue
        subprocess.run(['sh',WT+'/native/checks/coverage/covrun.sh',s,'00'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL); made=True
    for fr in sorted(items[s]):
        row=[]
        for f in (fr-1,fr):
            fn='%s/f%05d.wram'%(R,f)
            if not os.path.exists(fn): row.append('%d missing'%f); continue
            w=open(fn,'rb').read()
            live=sum(1 for n in range(1,36) if w[0x8DB+0x50*n])
            row.append('f%d: L=%d live=%d player %02X tick %d'%(f,w[0x32F],live,w[0x8DB],w[0x147D]*60+w[0x147C]))
        print(s,fr,'|',' | '.join(row))
    if made: subprocess.run(['rm','-rf',R])
