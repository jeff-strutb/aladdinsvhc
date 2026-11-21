import sys, glob
from PIL import Image, ImageChops
orgd,natd=sys.argv[1],sys.argv[2]
def gam(c5):
    v=c5<<3|c5>>2; v=v<<8|v
    if v<=32767: v=int(32767*pow(v/32767.0,1.5))
    return v>>8
G=[gam(c>>3)>>3 for c in range(256)]*3   # native: top5 -> gamma -> top5
T=[c>>3 for c in range(256)]*3          # original: top 5
def lo(f):
    a=Image.open(f).convert('RGB'); a=a.resize((256,224),Image.NEAREST,box=None) if a.size!=(256,224) else a
    return a.point(T)
def ln(f): return Image.open(f).convert('RGB').point(G)
O=sorted(glob.glob(orgd+'/*.ppm')); N=sorted(glob.glob(natd+'/*.ppm'))
on=[lo(f) for f in O]; nn=[ln(f) for f in N]
def nd(a,b):
    d=ImageChops.difference(a,b).convert('L').point([0]+[255]*255)
    return d.histogram()[255]
for i,o in enumerate(on):
    best=min(range(len(nn)),key=lambda j:nd(o,nn[j]))
    print(O[i].split('/')[-1],N[best].split('/')[-1],nd(o,nn[best]))
