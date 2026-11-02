#!/usr/bin/env python3
"""heldc.py: C held back until the code it reaches has C.

    heldc.py              list each routine whose C reaches code with no C
                          (a call or jump to an address, or to the C of
                          another such routine)
    heldc.py --hold       move each one's C into <file>.c.held beside its file
    heldc.py --restore    move all held C back, then hold again what still
                          reaches code with no C: run it once callees land
    heldc.py --keep ADDR REASON
                          hold one routine by hand (a divergence, a computed
                          jump to code with no C); --restore leaves it held

The whole-image lockstep (A7) refuses C that reaches code with no C, though
the per-call check passes it as UNVERIFIED. Held C stays drafted and ready;
its assembly stays filed. Computed calls (tables) are not seen: A7 names
those ("C reached ..., where no routine has C").
"""
import re, glob, os, sys
os.chdir(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
KEEP = re.compile(r'/\* heldc: keep \(.*?\) \*/\n/\*\n \* @routine .*?^\}\n', re.S | re.M)
if '--restore' in sys.argv:
    # Blocks marked "heldc: keep (reason)" are held by hand for a reason
    # the tool cannot see (a divergence, a computed jump): they stay.
    for h in glob.glob('src/aladdin/**/*.c.held', recursive=True):
        c = h[:-5]
        t = open(h).read()
        keep = ''.join(KEEP.findall(t))
        back = KEEP.sub('', t)
        if back.strip():
            open(c, 'a').write('\n' + back)
        if keep:
            open(h, 'w').write(keep)
        else:
            os.remove(h)
    sys.argv.append('--hold')
files = [f for f in glob.glob('src/aladdin/**/*.c', recursive=True)]
FN = re.compile(r'(/\*\n \* @routine (\w\w):(\w{4}) (\w+)\n.*?\*/\n)?void (abi_\w+)\(void\)\n\{\n(.*?)^\}\n', re.S | re.M)
defs = {}
for f in files + glob.glob('src/aladdin/include/*.h'):
    for m in re.finditer(r'^#define (\w+) \(?(0x[0-9A-Fa-f]+)u?\)?', open(f).read(), re.M):
        defs[(f if f.endswith('.c') else None, m.group(1))] = m.group(2)
# Static helpers of each file, so a routine's reach includes what they reach.
statics = {}
for f in files:
    for m in re.finditer(r'^static [\w \*]*?\b(\w+)\s*\([^;{]*\)\s*\{\n(.*?)^\}\n', open(f).read(), re.S | re.M):
        statics[(f, m.group(1))] = m.group(2)

def reach(f, body, seen=()):
    for (g, name), b in statics.items():
        if g == f and name not in seen and re.search(r'\b%s\s*\(' % name, body):
            body += reach(f, b, seen + (name,))
    return body

funcs = {}   # name -> (file, tag addr or None, body, full text)
for f in files:
    s = open(f).read()
    for m in FN.finditer(s):
        addr = int(m.group(2) + m.group(3), 16) if m.group(2) else None
        funcs[m.group(5)] = (f, addr, reach(f, m.group(6)), m.group(0))
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
        for m in re.finditer(r'\b(abi_\w+)\b', body):
            c = m.group(1)
            if c == n:
                continue
            if c in held:
                why = 'calls held %s' % c
            elif c not in funcs and c not in defined:
                why = 'calls %s, which has no C' % c
        if why:
            held.add(n); have.discard(a); changed = True
            print('%06X %-28s %s  (%s)' % (a, n, why, f))
if sys.argv[1:2] == ['--keep']:
    want, reason = int(sys.argv[2], 16), ' '.join(sys.argv[3:])
    for n, (f, a, _, text) in funcs.items():
        if a == want:
            s = open(f).read().replace(text, '')
            open(f, 'w').write(s)
            open(f + '.held', 'a').write('/* heldc: keep (%s) */\n' % reason + text)
            print('kept %06X %s' % (a, n))
    sys.exit(0)
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
