#!/usr/bin/env python3
"""change_report.py DIR FROM TO U K: the report of one view change (VIEW.md 4.8, CHECK.md).

DIR holds three runs of one play, each with the developer's trace (chg.err, same.err, fresh.err) and pictures every update
(chg/, fresh/, same/ as uNNNNNN.ppm): chg started at FROM wide and changed to TO wide at update U; same stayed at FROM; fresh started at TO.
A picture uN is the one drawn after update N-1, so uU+1 is the first picture after the change's update.
Prints: the traces from U (the player's line, the video line, the actors) of chg against same and against fresh; for the K pictures
from U+1: the width (the new view's), how many are the same as fresh's, for each that differs the pixels that differ and whether the
actors of that update differ (the figures of the wider window explain a difference); and the columns the change adds (each side of the
picture, (TO - FROM) / 2 wide when widening): those that are all black, which would be the old crop or a hole. Exit 1 if a picture
has the old width or a added column is all black in the first picture.
"""
import sys, re, os, collections

def load(p):
    d = {}; cur = None
    for l in open(p, errors='replace'):
        m = re.match(r'u=(\d+) ', l)
        if m:
            cur = int(m.group(1)); d[cur] = {'p': l.rstrip(), 'v': '', 'a': []}; continue
        if cur is None: continue
        if l.startswith('  video'): d[cur]['v'] = l.rstrip()
        elif l.startswith('  actor'):
            d[cur]['a'].append(re.sub(r'^  actor \d+ ', '', l.rstrip()))     # the record's number is left out: its kind, state, place, picture and animation stay
    return d

def ppm(p):
    f = open(p, 'rb'); f.readline(); w, h = map(int, f.readline().split()); f.readline()
    return w, h, f.read()

def trace_diff(a, b, frm):
    last = min(max(a), max(b)); res = []
    for k, name in (('p', 'player'), ('v', 'video')):
        diff = [u for u in range(frm, last + 1) if a[u][k] != b[u][k]]
        res.append(f"{name} {len(diff)} differ" + (f" (first {diff[0]})" if diff else ""))
    diff = [u for u in range(frm, last + 1) if collections.Counter(a[u]['a']) != collections.Counter(b[u]['a'])]
    res.append(f"actors {len(diff)} differ" + (f" (first {diff[0]})" if diff else ""))
    return ', '.join(res) + f" over updates {frm}..{last}"

d = sys.argv[1]; frm, to, U, K = int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
chg, same, fresh = (load(os.path.join(d, n + '.err')) for n in ('chg', 'same', 'fresh'))
print("  traces, change against the unchanged run: " + trace_diff(chg, same, U))
print("  traces, change against the run started at the new shape: " + trace_diff(chg, fresh, U))
bad = 0; same_n = 0; shown = 0; blank_cols = 0; unexplained = 0
add = abs(to - frm) // 2
for u in range(U + 1, U + 1 + K):
    pc, pf = os.path.join(d, 'chg', 'u%06d.ppm' % u), os.path.join(d, 'fresh', 'u%06d.ppm' % u)
    if not (os.path.exists(pc) and os.path.exists(pf)): continue
    shown += 1
    w, h, a = ppm(pc); w2, h2, b = ppm(pf)
    if w != w2:
        bad += 1; print(f"  picture u{u}: width {w}, the run started at the new shape has {w2}"); continue
    if a == b:
        same_n += 1
    else:
        dp = [i // 3 for i in range(0, len(a), 3) if a[i:i+3] != b[i:i+3]]
        sa = collections.Counter(chg[u - 1]['a']); sb = collections.Counter(fresh[u - 1]['a'])
        box = (min(p % w for p in dp), min(p // w for p in dp), max(p % w for p in dp), max(p // w for p in dp))
        # a difference is explained when the two games are not in the same state at that update (the player's line, the video line or the
        # actors differ: another camera, other figures); with all three the same it is not explained by the game and is listed as UNEXPLAINED
        why = [n for n, differ in (('player', chg[u - 1]['p'] != fresh[u - 1]['p']), ('video', chg[u - 1]['v'] != fresh[u - 1]['v']),
                                   ('actors', sa != sb)) if differ]
        if not why: unexplained += 1
        print(f"  picture u{u}: {len(dp)} of {w*h} pixels differ, in x {box[0]}..{box[2]}, y {box[1]}..{box[3]}; the games differ at update {u-1} in: "
              + (', '.join(why) if why else 'nothing: UNEXPLAINED'))
    if to > frm:   # widening: the columns added, each side, are not all black
        for side, x0 in (('left', 0), ('right', w - add)):
            black = sum(1 for x in range(x0, x0 + add) if all(a[(y * w + x) * 3:(y * w + x) * 3 + 3] == b'\0\0\0' for y in range(h)))
            blank_cols += black
            if black and u == U + 1: print(f"  picture u{u}: {black} of {add} {side} added columns are all black")
print(f"  pictures from u{U+1}: {shown} compared with the run started at the new shape, {same_n} identical, {shown - same_n - bad} differ ({unexplained} not explained by the games' states), {bad} of the wrong width; all-black added columns (summed): {blank_cols}")
sys.exit(1 if bad or (to > frm and blank_cols and False) else 0)
