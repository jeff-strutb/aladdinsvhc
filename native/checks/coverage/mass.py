import sys,os
# mass.py DIR : the frames of DIR's per-frame dumps at which 3 or more records (not the player's) go from live to free at once (a death restart or a level's end)
d=sys.argv[1]; prev=None; mass=[]
for fn in sorted(x for x in os.listdir(d) if x.endswith('.wram')):
    w=open(d+'/'+fn,'rb').read(); f=int(fn[1:6]); live={n for n in range(1,36) if w[0x8DB+0x50*n]}
    if prev is not None and len(prev-live)>=3: mass.append(f)
    prev=live
print(' '.join(map(str,mass)))
