import json,os,re,sys
# assemble.py WORKDIR HEADER.txt TAIL.txt OUT : the open list (unexplained sections with what the original did there) and the final text
W=sys.argv[1]
d=json.load(open(W+'/agg2.json')); owner=d['owner']; ev=d['ev']
cl={}
for l in open(W+'/classify2.txt'):
    m=re.match(r'(\S+) (\d+) \| f\d+: L=(\d+) live=(\d+) player (\w+) tick (\d+) \| f\d+: L=(\d+) live=(\d+) player (\w+) tick (\d+)',l)
    if m:
        s,fr=m.group(1),int(m.group(2)); L0,l0,p0,t0,L1,l1,p1,t1=int(m.group(3)),int(m.group(4)),m.group(5),int(m.group(6)),int(m.group(7)),int(m.group(8)),m.group(9),int(m.group(10))
        if t1==0 and t0>0: tag='a restart (play-time tick reset to 0, player state $%s the frame before)'%p0
        elif L0 or L1: tag="the original's loop is cut at this frame ($032F=%d: a record mid-turn)"%(L1 or L0)
        elif l1>l0: tag='%d records appear in this frame (a spawn by an unlisted kind)'%(l1-l0)
        elif l1<l0: tag='%d records vanish in this frame'%(l0-l1)
        else: tag='no restart, cut or record change at this frame: NOT SETTLED'
        cl[(s,fr)]=tag
out=[]
for k in sorted(owner):
    e=ev.get(k)
    if not e or not e['unexpl']: continue
    for u in e['unexpl']:
        tag=cl.get((u[0],u[2]),'a long script (frames beyond 9,999: not re-recorded for this tag)')
        out.append('  $%s [%s] %s %d-%d: first difference at frame %d (%d fields): %s'%(k,owner[k],u[0][4:],u[1][0],u[1][1],u[2],u[3],tag))
open(W+'/open.txt','w').write('\n'.join(out)+'\n')
