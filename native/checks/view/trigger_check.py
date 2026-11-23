#!/usr/bin/env python3
"""trigger_check.py DIR: reads the TRIGGER lines the runs of wide.sh wrote (DIR/*.err) and checks that every trigger test that
turned true was inside its original limits: the player-to-actor distance (near_box, near_span_x/y, near_point) within the table's
half-size, or an actor's place against the range point within the original's 256 x 224 view box (screen). Prints, per test, the
count, the largest distance reached against its limit, and the number past it (must be 0). Exit 1 if any is past."""
import glob, re, sys
pat = re.compile(r'TRIGGER (\S+) kind=(\S+) actor=(\d+) dx=(-?\d+) dy=(-?\d+) x=\[(-?\d+),(-?\d+)\] y=\[(-?\d+),(-?\d+)\]')
stat = {}
bad = 0
for f in sorted(glob.glob(sys.argv[1] + '/*.err')):
    for line in open(f, errors='replace'):
        m = pat.match(line)
        if not m:
            continue
        what, kind = m.group(1), m.group(2)
        dx, dy, lx, hx, ly, hy = map(int, m.groups()[3:])
        axes = {'near_span_x': 'x', 'near_span_y': 'y'}.get(what, 'xy')
        past = ('x' in axes and not lx <= dx <= hx) or ('y' in axes and not ly <= dy <= hy)
        s = stat.setdefault(what, [0, 0, 0, 0])      # count, past, max |dx|/limit %, max |dy|/limit %
        s[0] += 1
        s[1] += past
        bad += past
        if past:
            print('PAST', f, line.strip())
        if 'x' in axes and hx > 0:
            s[2] = max(s[2], round(100 * max(dx / hx if dx > 0 else 0, dx / lx if dx < 0 and lx < 0 else 0)))
        if 'y' in axes and hy > 0:
            s[3] = max(s[3], round(100 * max(dy / hy if dy > 0 else 0, dy / ly if dy < 0 and ly < 0 else 0)))
for what, s in sorted(stat.items()):
    print('%-12s fired %6d  past its limit %d  furthest across %d%% / down %d%% of the limit' % (what, s[0], s[1], s[2], s[3]))
print('triggers past their original limit: %d' % bad)
sys.exit(1 if bad else 0)
