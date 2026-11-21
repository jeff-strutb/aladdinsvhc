import sys,glob
from PIL import Image
def bbox(f,half):
    im=Image.open(f).convert('RGB')
    if half: im=im.resize((256,224),Image.NEAREST)
    bg=im.getpixel((2,60)) if True else None
    # outside colour = pixel at left edge middle
    w,h=im.size
    from PIL import ImageChops
    ref=Image.new('RGB',im.size,im.getpixel((1,h//2)))
    d=ImageChops.difference(im,ref).convert('L').point([0]+[255]*255)
    d=d.crop((0,9,w,h))
    b=d.getbbox()
    return b
d,half=sys.argv[1],int(sys.argv[2])
for f in sorted(glob.glob(d+'/*.ppm')):
    print(f.split('/')[-1],bbox(f,half))
