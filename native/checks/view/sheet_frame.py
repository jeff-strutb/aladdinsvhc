#!/usr/bin/env python3
"""sheet_frame.py OUT.ppm SCALE PICTURE.ppm [PICTURE.ppm ...]: the pictures (each W x 224, P6) stacked, the original's 256 x 224 frame
outlined (a dashed magenta line on the frame's left and right columns, the picture's own columns dx - 1 and dx + 256), each enlarged
SCALE times with whole pixels, a 4-pixel white gap between them. Used for docs/aladdin/native/shots/fix-*.png (VIEW.md 5.w)."""
import sys

def read(p):
    f = open(p, 'rb'); assert f.readline().strip() == b'P6'
    w, h = map(int, f.readline().split()); f.readline()
    return w, h, f.read()

out, scale = sys.argv[1], int(sys.argv[2])
pics = [read(p) for p in sys.argv[3:]]
W = max(p[0] for p in pics)
rows = []
for w, h, d in pics:
    dx = (w - 256) // 2
    for y in range(h):
        r = bytearray(d[y * w * 3:(y + 1) * w * 3])
        if y % 4 < 2:
            for x in (dx - 1, dx + 256):
                if 0 <= x < w:
                    r[x * 3:x * 3 + 3] = bytes((255, 0, 255))
        r = bytes(r) + bytes([255] * 3 * (W - w))
        big = b''.join(r[x * 3:x * 3 + 3] * scale for x in range(W))
        rows.extend([big] * scale)
    rows.extend([bytes([255] * 3 * W * scale)] * 4)
o = open(out, 'wb'); o.write(b'P6\n%d %d\n255\n' % (W * scale, len(rows)))
for r in rows:
    o.write(r)
