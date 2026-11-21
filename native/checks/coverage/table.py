import os,re,sys,json,collections
# table.py WORKDIR NOTES.json : the "EVERY KIND" lines from agg2.json (the scripts), the plays files and the notes (kind -> text)
W=sys.argv[1]; notes=json.load(open(sys.argv[2])) if len(sys.argv)>2 else {}
d=json.load(open(W+'/agg2.json')); owner=d['owner']; ev=d['ev']
plays=collections.defaultdict(list)       # kind -> [(play, live a-b, fields, ndiff, first, tail)]
files=[W+'/plays.txt']+[W+'/lv/'+f for f in sorted(os.listdir(W+'/lv')) if f.startswith('plays_')] if os.path.isdir(W+'/lv') else [W+'/plays.txt']
for fn in files:
    cur=None
    if not os.path.exists(fn): continue
    for l in open(fn):
        m=re.match(r'(\S+) (\d+)-(\d+) mass-free frames: (.*)',l)
        if m: cur=m.group(1).replace('rec.keepsstage_','')+' '+m.group(2)+'-'+m.group(3); continue
        m=re.match(r'  \$([0-9A-F]{2}) live (\d+)-(\d+): (\d+) fields, (\d+) differ(.*)',l)
        if m and cur: plays[m.group(1)].append((cur,int(m.group(2)),int(m.group(3)),int(m.group(4)),int(m.group(5)),m.group(6)))
out=[]
for k in sorted(owner):
    e=ev.get(k); ps=plays.get(k,[])
    parts=[]
    if e and e['ok']:
        byscript=collections.OrderedDict()
        for s,sec,n,ov in e['ok']:
            if ov<=0: continue
            byscript.setdefault(s,[]).append('%d-%d(%d live)'%(sec[0],sec[1],ov))
        if byscript:
            items=['%s %s'%(s[4:],','.join(v[:2])+('+%d'%(len(v)-2) if len(v)>2 else '')) for s,v in list(byscript.items())[:5]]
            parts.append('EXACT on script sections: '+'; '.join(items)+(' (+%d scripts)'%(len(byscript)-5) if len(byscript)>5 else ''))
    pe=[p for p in ps if p[4]==0]
    if pe: parts.append('EXACT on plays: '+'; '.join('%s (%d fields, live %d-%d)'%(p[0],p[3],p[1],p[2]) for p in pe[:4]))
    pd=[p for p in ps if p[4]>0]
    if pd: parts.append('plays with differences: '+'; '.join('%s %d differ, first %s%s'%(p[0],p[4],re.search(r'first (\d+)',p[5]).group(1) if re.search(r'first (\d+)',p[5]) else '?',p[5].split(')')[0].split('(')[-1] and ' ('+p[5].split('(')[1].split(')')[0]+')' if '(' in p[5] else '') for p in pd[:4]))
    if e:
        for key,label in (('restart','differs only after a restart/level end'),('freeall','differs after the game freed all records'),('poke','differs after the script poked a record/the player (not followable)')):
            if e[key]: parts.append('%s: %d sections (e.g. %s %d-%d, first %d)'%(label,len(e[key]),e[key][0][0][4:],e[key][0][1][0],e[key][0][1][1],e[key][0][2]))
        if e['unexpl']: parts.append('DIFFERS (not explained by the above): '+'; '.join('%s %d-%d first frame %d (%d fields)'%(u[0][4:],u[1][0],u[1][1],u[2],u[3]) for u in e['unexpl'][:5])+(' +%d more'%(len(e['unexpl'])-5) if len(e['unexpl'])>5 else ''))
    if not parts: parts.append('NOT MET by any script section or play of this table')
    if k in notes: parts.append(notes[k])
    out.append('$%s [%s] %s'%(k,owner[k],' | '.join(parts)))
print('\n'.join(out))
