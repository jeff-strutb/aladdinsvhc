#!/usr/bin/env python3
"""play_kinds.py DIR FIRST LAST [STEP]: the actor kinds the per-frame work-RAM dumps DIR/fNNNNN.wram show
(records at $08DB + $50 n, kind at +6, live when its state is not 0), each with the first and last frame seen
and how many sampled frames had one; also the level ($035E)."""
import sys, collections

d, first, last = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
step = int(sys.argv[4]) if len(sys.argv) > 4 else 5
seen = {}
level = None
for f in range(first, last + 1, step):
    try:
        w = open('%s/f%05d.wram' % (d, f), 'rb').read()
    except OSError:
        continue
    level = w[0x35E] | w[0x35F] << 8
    for n in range(36):
        r = 0x8DB + 0x50 * n
        if w[r]:
            k = w[r + 6]
            s = seen.setdefault(k, [f, f, 0])
            s[1] = f
            s[2] += 1
print('level', level)
for k in sorted(seen):
    print('%02X first %d last %d samples %d' % (k, *seen[k]))
