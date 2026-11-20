#!/usr/bin/env python3
"""check_inputs.py: the decomp's input scripts as nativeshvc's replay inputs.

  check_inputs.py SCRIPT [--out INPUTS]   write INPUTS (default: stdout), print the frames
  check_inputs.py --classify [--root .]   classify every script in tools/aladdin/scripts
  check_inputs.py SCRIPT --out INPUTS --oracle-script FILE --nshvc NSHVC_ROOT
                                         also the oracle's script for the same buttons,
                                         made by nshvc-check's own write_script (for a
                                         run of the original alone)

A script (tools/aladdin/scripts/*.box, the format in tools/aladdin/snesbox/snesbox.c)
drives the original from power-on with wait / hold / press / repeat. The engine's
inputs.txt (tools/nshvc-check in nativeshvc) is one line per change: the update
number, then each pad's buttons in hex; the original's frame F (counted from 1)
is update F - 1, the buttons of a frame set before the frame runs, as in snesbox.

A script that starts with `load build/aladdin/states/NAME.state` is replayable
when the state was itself made by input only: the recipe in tools/aladdin/states
that saves NAME is replayed from power-on up to its `save NAME` (and through any
load that recipe starts from), then the script. The oracle replays a load the
same way. Not replayable on a native game: poke, set, call, spcset, spccall,
cartram, region, apuport (anywhere in the script or the recipes it loads), and a
load after the script has run frames.
"""
import argparse, glob, os, re, sys

BITS = {'B': 0x8000, 'Y': 0x4000, 'SELECT': 0x2000, 'START': 0x1000, 'UP': 0x0800, 'DOWN': 0x0400,
        'LEFT': 0x0200, 'RIGHT': 0x0100, 'A': 0x0080, 'X': 0x0040, 'L': 0x0020, 'R': 0x0010}
NOT_INPUT = ('poke', 'set', 'call', 'spcset', 'spccall', 'cartram', 'region', 'apuport')
IGNORED = ('shot', 'dump', 'save')


class NotReplayable(Exception):
    pass


def read_lines(path):
    out = []
    with open(path) as f:
        for n, line in enumerate(f, 1):
            w = line.split('#')[0].split()
            if w:
                out.append((path, n, w))
    return out


def unroll(lines):
    """repeat N ... end expanded"""
    out, i = [], 0
    while i < len(lines):
        path, n, w = lines[i]
        if w[0] == 'repeat':
            depth, j = 0, i + 1
            while j < len(lines):
                if lines[j][2][0] == 'repeat':
                    depth += 1
                elif lines[j][2][0] == 'end':
                    if depth == 0:
                        break
                    depth -= 1
                j += 1
            if j == len(lines):
                raise SystemExit(f'{path}:{n}: repeat with no end')
            body = unroll(lines[i + 1:j])
            out += body * int(w[1])
            i = j + 1
        else:
            out.append(lines[i])
            i += 1
    return out


def buttons(spec):
    pads = [0, 0]
    for t in spec.split('+'):
        p = 0
        if t.startswith('2.'):
            p, t = 1, t[2:]
        if t not in BITS:
            raise SystemExit(f'unknown button {t!r}')
        pads[p] |= BITS[t]
    return tuple(pads)


class Recipes:
    """the state recipes: NAME -> (file, the lines up to its `save NAME`)"""

    def __init__(self, root):
        self.saves = {}
        for path in sorted(glob.glob(os.path.join(root, 'tools/aladdin/states/*.box'))):
            lines = unroll(read_lines(path))
            start = 0
            for i, (_, _, w) in enumerate(lines):
                if w[0] == 'load':
                    start = i          # a recipe's part starts at its load
                elif w[0] == 'save':
                    self.saves[w[1]] = lines[start:i]


def state_name(path):
    m = re.fullmatch(r'build/aladdin/states/([A-Za-z0-9_]+)\.state', path)
    if not m:
        raise NotReplayable(f'load of {path}, not a recipe state')
    return m.group(1)


def steps(lines, recipes, chain):
    """[(pad0, pad1, frames)] from power-on; chain records the states loaded"""
    out, ran = [], False
    for path, n, w in lines:
        c = w[0]
        if c in NOT_INPUT:
            raise NotReplayable(f'{c} ({os.path.basename(path)}:{n})')
        if c == 'load':
            if ran:
                raise NotReplayable(f'load after frames ({os.path.basename(path)}:{n})')
            name = state_name(w[1])
            if name not in recipes.saves:
                raise NotReplayable(f'no recipe saves {name}')
            chain.append(name)
            out += steps(recipes.saves[name], recipes, chain)
            continue
        if c in IGNORED:
            continue
        if c == 'wait':
            out.append((0, 0, int(w[1])))
        elif c == 'hold':
            out.append(buttons(w[1]) + (int(w[2]),))
        elif c == 'press':
            out.append(buttons(w[1]) + (int(w[2]) if len(w) > 2 else 2,))
            out.append((0, 0, 2))
        else:
            raise SystemExit(f'{path}:{n}: unknown command {c}')
        ran = True
    return out


def changes(st):
    """[(update, pad0, pad1)] at each change, and the frames in all"""
    out, u, held = [], 0, None
    for p0, p1, k in st:
        if k <= 0:
            continue
        if (p0, p1) != held:
            out.append((u, p0, p1))
            held = (p0, p1)
        u += k
    return out, u


def convert(script, recipes):
    chain = []
    st = steps(unroll(read_lines(script)), recipes, chain)
    ch, frames = changes(st)
    return ch, frames, chain


# the act and stage each input-only state starts (the password table $80:FE36 gives
# password K act K; its stage is 0; level1 is GAME START from the title)
STATE_AREA = {'title': 'title menu', 'level1': 'act 0 stage 0'}
STATE_AREA.update({f'pw{k}': f'act {k} stage 0' for k in range(1, 8)})


def classify(root):
    recipes = Recipes(root)
    plain, why = {}, {}
    for s in sorted(glob.glob(os.path.join(root, 'tools/aladdin/scripts/*.box'))):
        name = os.path.basename(s)[:-4]
        try:
            ch, frames, chain = convert(s, recipes)
            start = chain[0] if chain else 'power-on'
            plain[name] = (start, frames)
        except NotReplayable as e:
            why[name] = str(e).split(' (')[0]
    print(f'{len(plain) + len(why)} scripts: {len(plain)} replayable by input, {len(why)} not')
    reasons = {}
    for r in why.values():
        reasons[r] = reasons.get(r, 0) + 1
    for r, k in sorted(reasons.items(), key=lambda x: -x[1]):
        print(f'  not: {k:4d}  first blocker {r}')
    by = {}
    for name, (start, frames) in plain.items():
        by.setdefault(start, []).append((name, frames))
    for start in sorted(by):
        area = STATE_AREA.get(start, 'from power-on')
        print(f'start {start} ({area}): {len(by[start])}')
        for name, frames in by[start]:
            print(f'    {name} {frames}')


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('script', nargs='?')
    ap.add_argument('--out', default='')
    ap.add_argument('--classify', action='store_true')
    ap.add_argument('--root', default='.')
    ap.add_argument('--oracle-script', default='')
    ap.add_argument('--nshvc', default='')
    ap.add_argument('--frames', type=int, default=0, help='play only this many frames')
    a = ap.parse_args()
    if a.classify:
        classify(a.root)
        return 0
    if not a.script:
        ap.error('a script or --classify')
    try:
        ch, frames, chain = convert(a.script, Recipes(a.root))
    except NotReplayable as e:
        print(f'check_inputs: {a.script} is not replayable by input: {e}', file=sys.stderr)
        return 2
    if a.frames:
        frames = a.frames
        ch = [c for c in ch if c[0] < frames]
    text = ''.join(f'{u} {p0:04X} {p1:04X}\n' for u, p0, p1 in ch)
    if a.out:
        with open(a.out, 'w') as f:
            f.write(text)
    else:
        sys.stdout.write(text)
    if a.oracle_script:
        # nshvc-check's own conversion, so a run of the original alone is given
        # exactly what the full check gives it
        from importlib.machinery import SourceFileLoader
        sys.dont_write_bytecode = True   # nothing is written into the nativeshvc checkout
        nc = SourceFileLoader('nshvc_check', os.path.join(a.nshvc, 'tools/nshvc-check')).load_module()
        nc.write_script(a.oracle_script, {u: (p0, p1) for u, p0, p1 in ch}, frames)
    print(frames, file=sys.stderr if not a.out else sys.stdout)
    return 0


if __name__ == '__main__':
    sys.exit(main())
