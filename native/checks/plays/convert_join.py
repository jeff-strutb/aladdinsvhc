#!/usr/bin/env python3
"""convert_join.py OLD_PLAY BASE_TRACE NEW_PLAY: a play's keys moved back across the screen the level end no longer has
(docs/aladdin/native/DEPARTURES.md section 8; CHECK.md "The level end's removal").

BASE_TRACE is the developer trace (ALADDIN_DEV_TRACE=1, ALADDIN_DEV_ACTORS=2, ALADDIN_DEV_VIDEO=1) of OLD_PLAY on the build that still has the
screen (base-b36b712). Each stay on level $24 in it (the level-end screen: g.level = 36, from update u0) ends at update B with the next
screen's first update: level $30 (a story card: CARD_SETUP) or the next part's level (NEXT_PART). The game without the screen reaches that
same point at N: u0 + 1 after a card (the task goes on to the card in update u0, and the card's first turn waits one update), u0 for the next
part (the level is set in update u0). So the keys are moved back by L = B - N: the state the old play held at B + d is held at N + d. Lines
before N are kept as they are; the lines between N and B (typed during the removed screen) are dropped, and a line at N sets the state
held at B; later lines move back by L. Several stays are handled in order. A stay the trace ends on (the play never presses a button there: no B) leaves the keys after it as they are.

A key file is lines "UPDATE key=VKHEX ...": the keys held from that update on (a line with none releases all)."""
import re, sys

def parse_play(path):
    lines = []
    for l in open(path).read().split('\n'):
        if not l.strip():
            continue
        f = l.split()
        lines.append((int(f[0]), f[1:]))
    return lines

def level_runs(trace, level=36):
    """[(u0, last, B, B's level)] for each run of updates at `level` in the trace."""
    runs, prev, cur = [], None, None
    for m in re.finditer(r'^u=(\d+) level=(\d+) ', open(trace).read(), re.M):
        u, lv = int(m.group(1)), int(m.group(2))
        if cur is not None and lv != level:
            runs.append((cur[0], cur[1], u, lv))
            cur = None
        if lv == level:
            cur = (u, u) if cur is None else (cur[0], u)
    if cur is not None:
        runs.append((cur[0], cur[1], None, None))     # the trace ends on the screen
    return runs

def state_at(lines, u):
    s = []
    for (lu, keys) in lines:
        if lu > u:
            break
        s = keys
    return s

def convert(lines, runs):
    out, shift, pos = [], 0, 0          # shift: how far back old updates now move; pos: the old update up to which lines are copied
    for (u0, last, b, blevel) in runs:
        n = u0 if blevel not in (None, 0x30) else u0 + 1     # the old update the next screen's first update corresponds to, before the shift
        if b is None:                    # the old play never left the screen: its keys after u0 are copied as they are (moved by the earlier stays)
            break
        for (lu, keys) in lines:
            if pos <= lu < n:
                out.append((lu - shift, keys))
        out.append((n - shift, state_at(lines, b)))
        pos = b + 1
        shift += b - n
    for (lu, keys) in lines:
        if lu >= pos:
            out.append((lu - shift, keys))
    return out

if __name__ == '__main__':
    old, trace, new = sys.argv[1:4]
    lines = parse_play(old)
    runs = level_runs(trace)
    out = convert(lines, runs)
    with open(new, 'w') as f:
        for (u, keys) in out:
            f.write(' '.join([str(u)] + keys) + '\n')
    for (u0, last, b, bl) in runs:
        if b is None:
            print('%s: level-end screen from %d to the trace\'s end (the play never leaves it): its keys are not moved' % (old, u0))
        else:
            print('%s: level-end screen %d-%d, next screen at %d (level $%02X): moved back by %d' %
                  (old, u0, last, b, bl, b - (u0 if bl != 0x30 else u0 + 1)))
    if not runs:
        print('%s: no level-end screen in the trace; copied' % old)
