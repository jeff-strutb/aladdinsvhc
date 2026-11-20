import os, sys, struct
sys.path.insert(0, 'tools/aladdin')
import codemap
SP = sys.argv[1]
want = set()
for l in open(os.path.join(SP, 'runs.txt')):
    a, b = (int(x, 16) for x in l.split('-')); want |= set(range(a, b))
per = {}
for d in codemap.traces():
    raw = open(os.path.join(d, 'rommap.bin'), 'rb').read()
    m = struct.unpack('<%dH' % (len(raw) // 2), raw)
    per[os.path.basename(d)] = {o for o in want if m[o] & 0x4C}
left, chosen = set(want), []
while left:
    best = max(per, key=lambda k: len(per[k] & left))
    got = per[best] & left
    if not got: break
    chosen.append(best); left -= got
print(len(chosen), 'scripts cover', len(want) - len(left), 'of', len(want), 'bytes')
open(os.path.join(SP, 'scripts2.txt'), 'w').write('\n'.join(chosen) + '\n')
