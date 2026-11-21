#!/usr/bin/env python3
"""stage_dump.py OUT.box FIRST LAST STAGES.box NAME [PLAY.box]: like play_dump.py for a stage of tools/aladdin/states/
03_stages.box: the recipe from the title that ends at `save NAME` (its `repeat N` ... `end` loops and `poke` lines
included), then PLAY.box's inputs (its `load` dropped), run a frame at a time with `dump fNNNNN` after each frame
FIRST..LAST (N the frame since the script's first command). Prints the length in frames. With FIRST > LAST no dump
is written (the length only); with STEP in the environment (STEP=20) every STEP-th frame from 1 is dumped instead."""
import os, sys

def parse(lines):
    """nested commands: ('repeat', n, [..]) or tuples of words."""
    out, stack = [], []
    cur = out
    for raw in lines:
        line = raw.split('#')[0].strip()
        if not line:
            continue
        w = line.split()
        if w[0] == 'repeat':
            body = []
            cur.append(('repeat', int(w[1]), body))
            stack.append(cur)
            cur = body
        elif w[0] == 'end':
            cur = stack.pop()
        else:
            cur.append(tuple(w))
    return out

def run(cmds, emit, keep_load):
    for c in cmds:
        op = c[0]
        if op == 'repeat':
            for _ in range(c[1]):
                run(c[2], emit, keep_load)
        elif op == 'load':
            if keep_load:
                emit('load', c[1])
        elif op == 'wait':
            for _ in range(int(c[1])):
                emit('frame', '')
        elif op == 'hold':
            for _ in range(int(c[2])):
                emit('frame', c[1])
        elif op == 'press':
            n = int(c[2]) if len(c) > 2 else 2
            for _ in range(n):
                emit('frame', c[1])
            for _ in range(2):
                emit('frame', '')
        elif op == 'poke':
            emit('raw', ' '.join(c))
        elif op in ('shot', 'save', 'dump'):
            continue
        else:
            sys.exit('stage_dump: not an input-only command: ' + ' '.join(c))

def main():
    out, first, last, stages, name = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4], sys.argv[5]
    play = sys.argv[6] if len(sys.argv) > 6 else None
    step = int(os.environ.get('STEP', '0'))
    seg, got = [], None
    for raw in open(stages):
        line = raw.split('#')[0].strip()
        if line.startswith('dump '):
            seg = []
            continue
        if line == 'save ' + name:
            got = seg
            break
        seg.append(raw)
    if got is None:
        sys.exit('stage_dump: no ' + name)
    lines, f = [], [0]

    def emit(kind, arg):
        if kind in ('load', 'raw'):
            lines.append(arg if kind == 'raw' else 'load ' + arg)
            return
        lines.append(('hold %s 1' % arg) if arg else 'wait 1')
        f[0] += 1
        if (step and f[0] % step == 0) or (not step and first <= f[0] <= last):
            lines.append('dump f%05d' % f[0])
    run(parse(got), emit, True)
    if play:
        run(parse(open(play).read().split('\n')), emit, False)
    open(out, 'w').write('\n'.join(lines) + '\n')
    print(f[0])

main()
