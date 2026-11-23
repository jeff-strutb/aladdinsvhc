#!/usr/bin/env python3
"""camera_check.py LOG FIRST LAST EVERY KIND: a picture's camera must be one the original can have (ALADDIN_DEV_CAMERA, ALADDIN_DEV_ACTORS and
ALADDIN_DEV_TRACE=EVERY on the run, LOG its stderr). For every sample from update FIRST to LAST: the camera inside the scroll limits the game
set (x <= limit x, y <= limit y, and between the lock's min and max when the lock is held), the player inside the layout and in the camera's window (x < limit x + 256,
y < limit y + 224 + 32), and, unless KIND is room or arena, standing (his y the same at every sample: not falling). Prints the camera and player at
the last sample and exits 0, or says what is impossible and exits 1."""
import re, sys

log, first, last, every, kind = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), sys.argv[5]
cams, players, u = [], [], -1
for line in open(log, errors='replace'):
    line = line.strip()
    m = re.match(r'u=(\d+) ', line)
    if m:
        u = int(m.group(1))
        continue
    if not first <= u <= last:
        continue
    m = re.match(r'camera x=(\d+) y=(\d+) limit=(\d+),(\d+) min=(\d+) max=(\d+) held=(\d+),(\d+)', line)
    if m:
        cams.append(tuple(map(int, m.groups())))
    m = re.match(r'actor 0 kind=\w+ state=\w+ x=(-?\d+) y=(-?\d+)', line)
    if m:
        players.append((int(m.group(1)), int(m.group(2))))
if not cams or not players or len(cams) != len(players):
    print('no camera or player samples (%d, %d)' % (len(cams), len(players))); sys.exit(1)
bad = []
for i, (c, p) in enumerate(zip(cams, players)):
    cx, cy, lx, ly, mn, mx, hx, hy = c
    if cx > lx or cy > ly:
        bad.append('sample %d: camera %d,%d past the limits %d,%d' % (i, cx, cy, lx, ly))
    if hx and not (mn <= cx <= mx):
        bad.append('sample %d: camera x %d outside the lock %d..%d' % (i, cx, mn, mx))
    if p[0] < 0 or p[0] >= lx + 256 or p[1] < 0 or p[1] >= ly + 224 + 32:
        bad.append('sample %d: player %d,%d outside the layout (limits %d,%d)' % (i, p[0], p[1], lx, ly))
    if not (cx - 16 <= p[0] < cx + 272 and cy - 16 <= p[1] < cy + 240):
        bad.append('sample %d: the player %d,%d is not in the camera window at %d,%d' % (i, p[0], p[1], cx, cy))
if kind not in ('room', 'arena') and len({p[1] for p in players}) > 1:
    bad.append('the player is not standing: y %s' % sorted({p[1] for p in players}))
if bad:
    print('IMPOSSIBLE camera or place:', '; '.join(bad[:3])); sys.exit(1)
print('camera %d,%d (limits %d,%d, lock %d..%d held %d,%d), player %d,%d' % (cams[-1][0], cams[-1][1], cams[-1][2], cams[-1][3], cams[-1][4], cams[-1][5], cams[-1][6], cams[-1][7], players[-1][0], players[-1][1]))
