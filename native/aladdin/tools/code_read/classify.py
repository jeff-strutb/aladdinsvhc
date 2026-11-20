"""per byte of the 8,972: who reads it (rrb logs), and kept (a table) or left out (with the reason)"""
import os, re, glob, collections, sys, bisect
SP, OUT = sys.argv[1], sys.argv[2]
want = set()
for l in open(os.path.join(SP, 'runs.txt')):
    a, b = (int(x, 16) for x in l.split('-')); want |= set(range(a, b))
starts = []
for p in glob.glob('src/aladdin/**/*.c', recursive=True):
    for m in re.finditer(r'@routine ([0-9A-F]{2}):([0-9A-F]{4}) (\w+)', open(p, errors='replace').read()):
        starts.append((int(m.group(1) + m.group(2), 16), m.group(3)))
starts.sort(); keys = [s for s, _ in starts]
def routine(pc):
    i = bisect.bisect_right(keys, pc) - 1
    return starts[i][1]
NOT_TABLE = {
    'TaskCreate': "TaskCreate's inline arguments after its JSL: call arguments, which the native calls pass",
    'TaskExec': "TaskExec's inline arguments after its JSL: call arguments, which the native calls pass",
    'VramCopy': "VramCopy copying from a source in the code's own bank: code bytes sent to video memory, a block copy over code",
    'VramCopy2': "VramCopy2 copying code bytes to video memory, a block copy over code",
    'Level03Scene': "Level03Scene's checksum of its own code bytes (its copier check)",
    'SpcUpload': "SpcUpload sending S-CPU code bytes to the sound memory: a block copy over code",
}
readers = collections.defaultdict(set)      # table readers seen in play (scripts with no unit call)
unit_only = collections.defaultdict(set)    # table readers seen only in scripts with unit calls
others = collections.defaultdict(set)       # the non-table readers
for f in glob.glob(os.path.join(SP, 'rrb', '*.txt')):
    name = os.path.basename(f)[:-4]
    script = open(os.path.join('tools/aladdin/scripts', name + '.box')).read()
    has_unit = bool(re.search(r'^\s*(call|fx|spccall)\b', script, re.M))
    for l in open(f):
        pc, o = l.split(); o = int(o, 16)
        if o not in want:
            continue
        r = routine(int(pc, 16))
        if r in NOT_TABLE:
            others[o].add(r)
        elif has_unit:
            unit_only[o].add(r)
        else:
            readers[o].add(r)
rows = []
for o in sorted(want):
    if readers.get(o):
        rows.append((o, 'kept', ' '.join(sorted(readers[o]))))
    elif unit_only.get(o):
        rows.append((o, 'left', 'unit:' + ' '.join(sorted(unit_only[o]))))
    elif others.get(o):
        rows.append((o, 'left', ' '.join(sorted(others[o]))))
    else:
        rows.append((o, 'left', '-'))
# runs of the same decision and readers
out, i = [], 0
while i < len(rows):
    j = i
    while j + 1 < len(rows) and rows[j + 1][0] == rows[j][0] + 1 and rows[j + 1][1:] == rows[i][1:]:
        j += 1
    out.append((rows[i][0], rows[j][0] + 1, rows[i][1], rows[i][2])); i = j + 1
c = collections.Counter(); cb = collections.Counter()
for a, b, k, r in out:
    c[k] += 1; cb[k] += b - a
with open(OUT, 'w') as f:
    f.write('start,end,decision,readers,reason\n')
    for a, b, k, r in out:
        if k == 'kept':
            reason = 'read as a table in play (a script with no unit call)'
        elif r == '-':
            reason = 'every read the covering runs log is an instruction fetch or an interrupt reading the instruction at its PC, none a data read'
        elif r.startswith('unit:'):
            reason = 'read as a table only in scripts with unit calls, whose entry state the script chooses; no play shows it'
        else:
            reason = '; '.join(NOT_TABLE[x] for x in r.split())
        f.write(f'{a:06X},{b:06X},{k},{r},"{reason}"\n')
print('ranges', dict(c), 'bytes', dict(cb))
lt = collections.Counter()
for a, b, k, r in out:
    if k == 'left':
        for x in (['unit calls'] if r.startswith('unit:') else r.split()): lt[x] += b - a
print('left out by reader (bytes):', dict(lt))
