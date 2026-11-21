import sys,os,subprocess
# why.py SCRIPT FRAME...: records SCRIPT if its covrec dir is missing (kept), prints per frame around each FRAME the live records (index:kind/state) of the original
M=os.environ['M'];W=os.environ['W'];s=sys.argv[1];R='%s/covrec_%s'%(W,s)
if not os.path.isdir(R):
    subprocess.run(['sh',os.environ['WTREE']+'/native/checks/coverage/covrun.sh',s,'00'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
for fr in map(int,sys.argv[2:]):
    for f in range(fr-2,fr+1):
        fn='%s/f%05d.wram'%(R,f)
        if not os.path.exists(fn): print(f,'missing');continue
        w=open(fn,'rb').read()
        live=['%d:%02X/%02X'%(n,w[0x8DB+0x50*n+6],w[0x8DB+0x50*n]) for n in range(36) if w[0x8DB+0x50*n]]
        print(s,f,'live',len(live),' '.join(live[:14]))
