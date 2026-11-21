#!/usr/bin/env python3
"""sheet.py DIR OUT.png UPDATE...: a contact sheet (3 across) of run.sh's pictures DIR/uNNNNNN.ppm."""
import sys
from PIL import Image
d, out = sys.argv[1], sys.argv[2]
ims = [Image.open('%s/u%06d.ppm' % (d, int(f))) for f in sys.argv[3:]]
w, h = ims[0].size
s = 2 if w > 600 else 1
ims = [i.resize((w // s, h // s)) for i in ims]
w //= s; h //= s
cols = 3
o = Image.new('RGB', (w * cols, h * ((len(ims) + cols - 1) // cols)))
for i, im in enumerate(ims):
    o.paste(im, ((i % cols) * w, (i // cols) * h))
o.save(out)
