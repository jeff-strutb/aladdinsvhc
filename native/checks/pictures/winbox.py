#!/usr/bin/env python3
"""winbox.py ORIGDIR ORIGPREFIX NATDIR NATPREFIX FIRST LAST STEP [Y0]: per update the box a window leaves lit (the pixels that differ from the
outside colour, taken at the lower left corner, rows from Y0, default 24 to keep the status bar out), in the original's picture and in the native one.
The original's shot is 512 x 448 and its columns are filtered: the odd column of each pair is a blend of its neighbours at an edge. So the original
is sampled at the EVEN column (2x) and every other row, never resized with a nearest-neighbour filter (which takes the odd column and reads every
left edge one pixel early). Files: ORIGDIR/ORIGPREFIXnnnnn.ppm and NATDIR/NATPREFIXnnnnnn.ppm (the native shots have six digits)."""
import sys
from PIL import Image

def box(get, w, h, y0):
    out = get(4, h - 4)
    xs, ys = [], []
    for y in range(y0, h):
        for x in range(w):
            p = get(x, y)
            if sum(abs(p[i] - out[i]) for i in range(3)) > 12:
                xs.append(x)
                ys.append(y)
    return (min(xs), min(ys), max(xs), max(ys)) if xs else None

od, op, nd, np_, first, last, step = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4], *map(int, sys.argv[5:8])
y0 = int(sys.argv[8]) if len(sys.argv) > 8 else 24
for u in range(first, last + 1, step):
    try:
        o = Image.open("%s/%s%05d.ppm" % (od, op, u)).convert("RGB")
        n = Image.open("%s/%s%06d.ppm" % (nd, np_, u)).convert("RGB")
    except OSError:
        continue
    po, pn = o.load(), n.load()
    bo = box(lambda x, y: po[2 * x, 2 * y], 256, 224, y0)
    bn = box(lambda x, y: pn[x, y], 256, 224, y0)
    d = tuple(a - b for a, b in zip(bo, bn)) if bo and bn else None
    print(u, "orig", bo, "native", bn, "orig-native", d)
