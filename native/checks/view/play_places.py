#!/usr/bin/env python3
"""play_places.py APP PLAY.txt [N]: the places a recorded play stands on firm ground (the original's view, 256 x 224, where a play is the original's),
for fill_shots.sh to take its end pictures from a real position and not a hand-picked coordinate. Prints, furthest right first, N (default 3)
samples `x,y camera_x,camera_y update` where the player's y was the same at the sample and its neighbours (standing, not falling) and no probe point
of his body box was in solid ground (ALADDIN_DEV_CAMERA's solid=). A camera lock (a room) is skipped unless PLACES_HELD is set."""
import os, re, subprocess, sys
app, play = sys.argv[1], sys.argv[2]
n = int(sys.argv[3]) if len(sys.argv) > 3 else 3
last = 600 + max(int(l.split()[0]) for l in open(play) if l.split() and l.split()[0].isdigit())
res = os.path.join(os.path.dirname(app), '..', 'Resources')
env = dict(os.environ, PORT_RESDIR=res, ALADDIN_VIEW='256x224', ALADDIN_DEV_CAMERA='1', ALADDIN_DEV_ACTORS='1', ALADDIN_DEV_TRACE='20')
err = subprocess.run(['perl', '-e', 'alarm 900; exec @ARGV', app, '--headless', '--updates', str(last), '--input', play], env=env,
                     stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, errors='replace').stderr
cams, pls, u = {}, {}, -1
for line in err.splitlines():
    line = line.strip()
    m = re.match(r'u=(\d+) ', line)
    if m:
        u = int(m.group(1)); continue
    m = re.match(r'camera x=(\d+) y=(\d+) limit=(\d+),(\d+) .* held=(\d+),(\d+) solid=(-?\d+)', line)
    if m:
        cams[u] = tuple(map(int, m.groups()))
    m = re.match(r'actor 0 kind=\w+ state=(\w+) x=(-?\d+) y=(-?\d+)', line)
    if m:
        pls[u] = (m.group(1), int(m.group(2)), int(m.group(3)))
ups = sorted(set(cams) & set(pls))
found = []
for i in range(1, len(ups) - 1):
    a, b, c = ups[i - 1], ups[i], ups[i + 1]
    if pls[a][2] == pls[b][2] == pls[c][2] and pls[b][1] == pls[c][1] and cams[b][6] == 0 and cams[b][0] <= cams[b][2] and cams[b][1] <= cams[b][3] \
            and (os.environ.get('PLACES_HELD') or (cams[b][4] == 0 and cams[b][5] == 0)):
        found.append((pls[b][1], pls[b][2], cams[b][0], cams[b][1], b))
found.sort(reverse=True)
for x, y, cx, cy, b in found[:n]:
    print('%d,%d %d,%d %d' % (x, y, cx, cy, b))
