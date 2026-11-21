import sys
from PIL import Image, ImageChops
sys.path.insert(0,'.')
def gam(c5):
    v=c5<<3|c5>>2; v=v<<8|v
    if v<=32767: v=int(32767*pow(v/32767.0,1.5))
    return v>>8
G=[gam(c>>3)>>3 for c in range(256)]*3; T=[c>>3 for c in range(256)]*3
o=Image.open(sys.argv[1]).convert('RGB'); o=o.resize((256,224),Image.NEAREST); ot=o.point(T)
n=Image.open(sys.argv[2]).convert('RGB'); nt=n.point(G)
d=ImageChops.difference(ot,nt).convert('L').point([0]+[255]*255)
W=Image.new('RGB',(768,224)); W.paste(o,(0,0)); W.paste(n.point([min(255,gam(c>>3)) for c in range(256)]*3) if False else n,(256,0)); W.paste(d.convert('RGB'),(512,0))
W=W.resize((1536,448),Image.NEAREST); W.save(sys.argv[3])
