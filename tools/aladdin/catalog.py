#!/usr/bin/env python3
"""catalog.py: the routine catalog, built from the traces.

    catalog.py          write build/aladdin/routines.csv and print a summary

A routine starts at every place the traces saw code entered as a routine:
targets of JSR, JSL and JSR (abs,X), interrupt entries, reset, and task
entries: where the task scheduler's RTL ($81:BCB7) starts a task. Where that
RTL lands right after a task's wait call (JSL $81:BD31) the task resumes:
no routine starts there; build/aladdin/resumes.csv lists those places with
their wait call. Its bytes are the executed code bytes from its start
up to the next routine start. Code before the first entry in a bank (reached
only by jumps) starts a routine of its own. Code no trace has run is not in the catalog
yet, so the catalog (and every total built on it) grows with coverage.
Code the S-CPU runs from work RAM is copied there from ROM
(config/aladdin/ramcode.csv); it is counted at its ROM bytes and named
by its run address.
"""
import csv, glob, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'aladdin')
OUT = os.path.join(B, 'routines.csv')
CPU_CODE = 0x03
RESET = 0x008000
SCHEDULER_RTL = 0x81BCB7
TASK_WAIT = 0x81BD31
RESUMES = os.path.join(B, 'resumes.csv')


rom_offset = codemap.rom_offset


def cpu_addr(off):
    return codemap.run_addr(off)


def load():
    rom = open(os.path.join(B, 'baserom.sfc'), 'rb').read()
    mp = codemap.merged()
    edges = []
    for d in codemap.traces():
        e = os.path.join(d, 'edges.csv')
        if os.path.exists(e):
            edges += list(csv.DictReader(open(e)))
    return rom, mp, edges


def build():
    rom, mp, edges = load()
    starts = {}  # offset -> (cpu, how)
    resumes = {}  # resume address -> its wait call
    o = rom_offset(RESET)
    starts[o] = ('cpu', 'reset')
    for r in edges:
        kind, src, dst = int(r['kind'], 16), int(r['from'], 16), int(r['to'], 16)
        o = rom_offset(dst)
        if o is None or o >= len(rom):
            continue
        if r['cpu'] == 'cpu':
            if kind in (0x20, 0x22, 0xFC):
                starts.setdefault(o, ('cpu', 'call'))
            elif kind == 0x100:
                starts.setdefault(o, ('cpu', 'interrupt'))
            elif src == SCHEDULER_RTL:
                w = o - 4
                if w >= 0 and rom[w] == 0x22 and rom[w + 1] | rom[w + 2] << 8 | rom[w + 3] << 16 == TASK_WAIT:
                    resumes[dst] = cpu_addr(w)
                else:
                    starts.setdefault(o, ('cpu', 'task'))
    # Code reached only by jumps (for example the interrupt handlers the RAM
    # vectors jump to) and found before any known entry in its bank.
    for bank in range(len(rom) // 0x8000):
        lo, hi = bank * 0x8000, (bank + 1) * 0x8000
        for mask, cpu in ((CPU_CODE, 'cpu'),):
            first = next((k for k in range(lo, hi) if mp[k] & mask), None)
            if first is None:
                continue
            known = [k for k, (c, _) in starts.items() if c == cpu and lo <= k < hi]
            if not known or first < min(known):
                starts.setdefault(first, (cpu, 'jump'))
    order = sorted(starts)
    rows = []
    for i, o in enumerate(order):
        cpu, how = starts[o]
        mask = CPU_CODE
        if not mp[o] & mask:
            continue
        end = order[i + 1] if i + 1 < len(order) else len(rom)
        end = min(end, (o // 0x8000 + 1) * 0x8000)
        size = sum(1 for k in range(o, end) if mp[k] & mask)
        last = max((k for k in range(o, end) if mp[k] & mask), default=o)
        rows.append({'addr': '%06X' % cpu_addr(o), 'cpu': cpu, 'entry': how, 'offset': '%06X' % o,
                     'end': '%06X' % (last + 1), 'bytes': size})
    os.makedirs(B, exist_ok=True)
    with open(RESUMES, 'w') as f:
        f.write('resume,wait\n')
        for a in sorted(resumes):
            f.write('%06X,%06X\n' % (a, resumes[a]))
    with open(OUT, 'w', newline='') as f:
        w = csv.DictWriter(f, ['addr', 'cpu', 'entry', 'offset', 'end', 'bytes'])
        w.writeheader()
        w.writerows(rows)
    return rows, mp


def main():
    rows, mp = build()
    print('S-CPU: %d routines (%d task entries), %d bytes (%d executed code bytes in ROM); %d resume points' % (
        len(rows), sum(1 for r in rows if r['entry'] == 'task'), sum(r['bytes'] for r in rows),
        sum(1 for v in mp if v & CPU_CODE), sum(1 for _ in open(RESUMES)) - 1))
    return 0


if __name__ == '__main__':
    sys.exit(main())
