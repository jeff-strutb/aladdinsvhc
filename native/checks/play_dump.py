#!/usr/bin/env python3
"""play_dump.py OUT.box FIRST LAST PLAY.box...: turns input-only snesbox plays (tools/aladdin/scripts: `load`,
`wait`, `hold`, `press`; `shot`/`save` are dropped) into one script that runs them one after another a frame at a
time and writes the work RAM after each frame FIRST..LAST (`dump fNNNNN`, N the frame since the script's first
command), the dumps native/checks/actors.sh and aladdin_actor_check read. With several files, the later files'
`load` lines are dropped (a recipe from power-on, then the play that starts where the recipe's `save` was). Prints
the length in frames. The checker starts each record from its start, so FIRST should be the frame the level is
loaded at, not later.

  e.g. play_dump.py rec.box 2880 7500 tools/aladdin/states/01_title.box tools/aladdin/scripts/rand_level1_1.box"""
import sys

def frames(path, keep_load):
    """yield (keys, count) per script command, in order, with 'load' lines passed through as ('load', path)."""
    for raw in open(path):
        line = raw.split('#')[0].strip()
        if not line:
            continue
        w = line.split()
        if w[0] == 'load':
            if keep_load:
                yield ('load', w[1])
        elif w[0] == 'wait':
            yield ('', int(w[1]))
        elif w[0] == 'hold':
            yield (w[1], int(w[2]))
        elif w[0] == 'press':
            n = int(w[2]) if len(w) > 2 else 2
            yield (w[1], n)
            yield ('', 2)
        elif w[0] in ('shot', 'save'):
            continue
        else:
            sys.exit('play_dump: not an input-only command: ' + line)

def main():
    out, first, last, srcs = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4:]
    lines, f = [], 0
    for i, src in enumerate(srcs):
        for keys, n in frames(src, i == 0):
            if keys == 'load':
                lines.append('load ' + n)
                continue
            for _ in range(n):
                lines.append(('hold %s 1' % keys) if keys else 'wait 1')
                f += 1
                if first <= f <= last:
                    lines.append('dump f%05d' % f)
    open(out, 'w').write('\n'.join(lines) + '\n')
    print(f)

main()
