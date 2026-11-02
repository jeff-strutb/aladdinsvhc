#!/usr/bin/env python3
"""heldc.py: C held back until the code it reaches has C.

    heldc.py              list each routine whose C reaches code with no C
                          (a call or jump to an address, or to the C of
                          another such routine)
    heldc.py --hold       move each one's C into <file>.c.held beside its file
    heldc.py --restore    move all held C back, then hold again what still
                          reaches code with no C: run it once callees land

The whole-image lockstep (A7) refuses C that reaches code with no C, though
the per-call check passes it as UNVERIFIED. Held C stays drafted and ready;
its assembly stays filed. Computed calls (tables) are not seen: A7 names
those ("C reached ..., where no routine has C").
"""
import re, glob, os, sys
os.chdir(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
if '--restore' in sys.argv:
    for h in glob.glob('src/aladdin/**/*.c.held', recursive=True):
        c = h[:-5]
        open(c, 'a').write('\n' + open(h).read())
        os.remove(h)
    sys.argv.append('--hold')
files = [f for f in glob.glob('src/aladdin/**/*.c', recursive=True)]
FN = re.compile(r'(/\*\n \* @routine (\w\w):(\w{4}) (\w+)\n.*?\*/\n)?void (abi_\w+)\(void\)\n\{\n(.*?)^\}\n', re.S | re.M)
defs = {}
for f in files + glob.glob('src/aladdin/include/*.h'):
    for m in re.finditer(r'^#define (\w+) \(?(0x[0-9A-Fa-f]+)u?\)?', open(f).read(), re.M):
        defs[(f if f.endswith('.c') else None, m.group(1))] = m.group(2)
funcs = {}   # name -> (file, tag addr or None, body, full text)
for f in files:
    s = open(f).read()
    for m in FN.finditer(s):
        addr = int(m.group(2) + m.group(3), 16) if m.group(2) else None
        funcs[m.group(5)] = (f, addr, m.group(6), m.group(0))
have = {a for (_, a, _, _) in funcs.values() if a is not None}
# Every abi_ function defined anywhere (any form), so a call to one is a call to C.
defined = set()
for f in files:
    defined |= set(re.findall(r'^[A-Za-z_][\w \*]*?\b(abi_\w+)\s*\([^;{]*\)\s*\{', open(f).read(), re.M))
# Routines the per-call ledger already judges IDENTICAL are certified: leave them.
import csv
certified = {int(r['routine'], 16) for r in csv.DictReader(open('config/aladdin/t3_live.csv')) if r['verdict'] == 'IDENTICAL'}
held = set()
changed = True
while changed:
    changed = False
    for n, (f, a, body, _) in funcs.items():
        if n in held or a is None or a in certified:
            continue
        why = None
        for m in re.finditer(r'(?:call_routine|jump_routine(?:_pushed)?)\(\s*([A-Za-z_0-9]+)\s*[,)]', body):
            v = m.group(1)
            if not re.match(r'0x', v):
                v = defs.get((f, v)) or defs.get((None, v))
            if v is None:
                continue
            t = int(v.rstrip('uU'), 16) & 0xFFFFFF
            if t not in have:
                why = 'reaches %06X' % t
        for m in re.finditer(r'\b(abi_\w+)\(\)', body):
            c = m.group(1)
            if c in held:
                why = 'calls held %s' % c
            elif c not in funcs and c not in defined:
                why = 'calls %s, which has no C' % c
        if why:
            held.add(n); have.discard(a); changed = True
            print('%06X %-28s %s  (%s)' % (a, n, why, f))
if '--hold' in sys.argv:
    by = {}
    for n in held:
        by.setdefault(funcs[n][0], []).append(funcs[n][3])
    for f, texts in by.items():
        s = open(f).read()
        for t in texts:
            s = s.replace(t, '')
        open(f, 'w').write(s)
        with open(f + '.held', 'a') as h:
            h.write(''.join(texts))
print(len(held), 'held')
