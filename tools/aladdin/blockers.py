#!/usr/bin/env python3
"""blockers.py: the routines without C that hold up the most other code.

    blockers.py [PREFIX...]     the top 25 (only addresses starting with a PREFIX, e.g. 83 81F)

Bottom up, a routine can be certified only once everything it calls or
jumps to has C (worklist.py). This ranks each routine without C by the
code bytes of the routines without C that wait on it, directly or through
others, and lists what it waits on itself: file the top of the list first.
"""
import csv, os, sys, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
os.chdir(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
import codemap, progress
rows = list(csv.DictReader(open('build/aladdin/routines.csv')))
done = {k for k, v in progress.tagged().items() if v[0]}
starts = {int(r['offset'], 16): r for r in rows}
edges = []
for d in codemap.traces():
    edges += list(csv.DictReader(open(os.path.join(d, 'edges.csv'))))
calls = collections.defaultdict(set)
for e in edges:
    so, to = codemap.rom_offset(int(e['from'], 16)), codemap.rom_offset(int(e['to'], 16))
    if so is None or to is None or e['cpu'] != 'cpu': continue
    if int(e['kind'], 16) in (0x20, 0x22, 0xFC, 0x4C, 0x5C):
        calls[so].add(to)
need = {}   # routine start -> pending callees (starts)
size = {}
for r in rows:
    if r['cpu'] != 'cpu': continue
    lo, hi = int(r['offset'], 16), int(r['end'], 16)
    if codemap.run_addr(lo) in done: continue
    cs = set()
    for o in range(lo, hi): cs |= calls.get(o, set())
    need[lo] = {c for c in cs if c in starts and c != lo and codemap.run_addr(c) not in done}
    size[lo] = int(r['bytes'])
waiters = collections.defaultdict(set)
for r, cs in need.items():
    for c in cs: waiters[c].add(r)
def closure(c):
    seen, st = set(), [c]
    while st:
        x = st.pop()
        for w in waiters.get(x, ()):
            if w not in seen: seen.add(w); st.append(w)
    return seen
rank = []
for c in need:
    cl = closure(c)
    rank.append((sum(size[w] for w in cl), len(cl), c))
rank.sort(reverse=True)
want = sys.argv[1:]
n = 0
for b, k, c in rank:
    a = codemap.run_addr(c)
    tag = '%06X' % a
    if want and not any(tag.startswith(w) for w in want): continue
    print('%s %5d B own, blocks %6d B in %3d routines; waits on %s' % (tag, size[c], b, k, ' '.join('%06X' % codemap.run_addr(x) for x in sorted(need[c]))[:80]))
    n += 1
    if n >= 25: break
