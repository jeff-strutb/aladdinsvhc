#!/usr/bin/env python3
"""make_assets_csv.py DECOMP_ROOT: writes assets.csv beside this, the list of
the cartridge's bytes that are not code, from the decompilation's own records
in DECOMP_ROOT (its checkout with build/aladdin traced).

A byte is code when the game's code map (tools/aladdin/codemap.py, every
current script's trace) shows the S-CPU running it (flags 0x33) or the SPC700
running it (codemap.SPC_CODE), or the routine census (tools/aladdin/census.py)
decodes it as code no script has run: the 90,269 B the README's bars count.
These are the same tests as tools/aladdin/progress.py's code_bytes. Every
other byte goes in the list: 'data' where a trace read it (flags 0x4C), else
'unread', in runs of one kind."""
import os, sys

root = os.path.abspath(sys.argv[1])
sys.path.insert(0, os.path.join(root, 'tools', 'aladdin'))
import codemap  # noqa: E402
import census   # noqa: E402

c = census.census()[0]
mp = c.mp
kinds = []
for o in range(len(mp)):
    v = mp[o]
    if v & 0x33 or v & codemap.SPC_CODE or c.code[o] or c.gcode[o]:
        kinds.append(None)
    elif v & 0x4C:
        kinds.append('data')
    else:
        kinds.append('unread')
rows, o = [], 0
while o < len(kinds):
    k, s = kinds[o], o
    while o < len(kinds) and kinds[o] == k:
        o += 1
    if k:
        a = codemap.run_addr(s)
        rows.append('%s_%06X,%06X,%06X,%s' % (k, a, s, o, k))
here = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(here, 'assets.csv'), 'w') as f:
    f.write('name,start,end,reader\n' + '\n'.join(rows) + '\n')
n = {k: kinds.count(k) for k in (None, 'data', 'unread')}
print('ROM %d B: code %d B (S-CPU run %d, SPC700 run %d, census only %d); data %d B; unread %d B; %d ranges' % (
    len(mp), n[None], sum(1 for v in mp if v & 0x33),
    sum(1 for v in mp if v & codemap.SPC_CODE and not v & 0x33),
    sum(1 for o in range(len(mp)) if (c.code[o] or c.gcode[o]) and not mp[o] & 0x33 and not mp[o] & codemap.SPC_CODE),
    n['data'], n['unread'], len(rows)))
print('README classes: run as code %d B, read as data %d B, untouched %d B' % (
    sum(1 for v in mp if v & 0x33), sum(1 for v in mp if not v & 0x33 and v & 0x4C), sum(1 for v in mp if not v)))
