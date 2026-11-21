import sys,re
# scriptdump.py OUT.box SCRIPT.box : the script with a dump after every frame (frame N: dump fNNNNN); load/poke kept; unsupported commands reported
out,src=sys.argv[1],sys.argv[2]
def toks(path):
    for raw in open(path):
        l=raw.split('#')[0].strip()
        if l: yield l.split()
lines=[];f=0;bad=set()
stack=[]
def emit_frame(keys):
    global f
    lines.append(('hold %s 1'%keys) if keys else 'wait 1'); f+=1; lines.append('dump f%05d'%f)
cmds=list(toks(src))
def run(cs):
    i=0
    while i<len(cs):
        w=cs[i]; op=w[0]
        if op=='repeat':
            depth=1;j=i+1
            while depth:
                if cs[j][0]=='repeat': depth+=1
                if cs[j][0]=='end': depth-=1
                j+=1
            for _ in range(int(w[1])): run(cs[i+1:j-1])
            i=j; continue
        if op=='wait':
            for _ in range(int(w[1])): emit_frame('')
        elif op=='hold':
            for _ in range(int(w[2])): emit_frame(w[1])
        elif op=='press':
            n=int(w[2]) if len(w)>2 else 2
            for _ in range(n): emit_frame(w[1])
            for _ in range(2): emit_frame('')
        elif op in ('load','poke'): lines.append(' '.join(w))
        elif op in ('shot','save','dump'): pass
        else: bad.add(op); lines.append(' '.join(w))
        i+=1
run(cmds)
open(out,'w').write('\n'.join(lines)+'\n')
print(f, 'UNSUPPORTED:'+','.join(sorted(bad)) if bad else 'ok')
