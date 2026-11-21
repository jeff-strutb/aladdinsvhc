import sys
from PIL import Image, ImageDraw
out=sys.argv[1]; fs=sys.argv[2:]
cols=6; w,h=128,112
o=Image.new('RGB',(w*cols,h*((len(fs)+cols-1)//cols)))
for i,f in enumerate(fs):
    im=Image.open(f).resize((w,h)); o.paste(im,((i%cols)*w,(i//cols)*h))
    ImageDraw.Draw(o).text(((i%cols)*w+2,(i//cols)*h+2),f.split('/')[-1][:-4],fill=(255,255,0))
o.save(out)
