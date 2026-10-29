#!/usr/bin/env python3
"""randplay.py: seeded random-play scripts, for coverage.

    randplay.py menus SEED FRAMES OUT.box
        from power-on, random taps on every button, wandering the menus
    randplay.py play STATE SEED FRAMES OUT.box
        from a saved state (a level's start), random play

The hand-written scripts follow one path each; random input reaches menu
entries, options and level events they never do. The same seed always gives
the same script, so a run is repeatable and the gates can use it.
"""
import random, sys

BUTTONS = ['UP', 'DOWN', 'LEFT', 'RIGHT', 'A', 'B', 'X', 'Y', 'L', 'R', 'START', 'SELECT']
# Control type 1: Y dash, B jump, A throw, R hover.
PLAY = ['RIGHT', 'RIGHT', 'RIGHT', 'LEFT', 'B', 'B+RIGHT', 'B+RIGHT', 'B+LEFT', 'A', 'A+RIGHT', 'Y+RIGHT',
        'Y+RIGHT+B', 'R', 'B+R', 'DOWN', 'UP', 'DOWN+A', 'UP+B', 'Y+LEFT', 'X', 'L']


def menus(r, frames):
    out, t = ['wait 300'], 300
    while t < frames:
        b = r.choice(BUTTONS + ['A', 'START', 'DOWN', 'RIGHT'])
        w = r.randrange(15, 140)
        out += ['press %s' % b, 'wait %d' % w]
        t += w + 4
    return out


def play(r, frames):
    out, t = [], 0
    while t < frames:
        b = r.choice(PLAY)
        n = r.randrange(4, 90)
        out.append('hold %s %d' % (b, n))
        t += n
        if r.random() < 0.01:
            out += ['press START', 'wait 60', 'press START']
            t += 66
    return out


def main(argv):
    if argv[:1] == ['menus'] and len(argv) == 4:
        seed, frames, path = int(argv[1]), int(argv[2]), argv[3]
        head = ['# Random menu play, seed %d (randplay.py menus %d %d).' % (seed, seed, frames)]
        body = menus(random.Random(seed), frames)
    elif argv[:1] == ['play'] and len(argv) == 5:
        state, seed, frames, path = argv[1], int(argv[2]), int(argv[3]), argv[4]
        head = ['# Random play from %s, seed %d (randplay.py play).' % (state.split('/')[-1], seed),
                'load %s' % state]
        body = play(random.Random(seed), frames) + ['shot played']
    else:
        raise SystemExit(__doc__)
    open(path, 'w').write('\n'.join(head + body) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
