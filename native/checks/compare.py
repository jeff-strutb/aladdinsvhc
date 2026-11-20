#!/usr/bin/env python3
"""compare.py: a recording's bsnes pictures beside the engine's pictures of the same frames.

  compare.py RECORDING SHOTS [--offset K] [--beside DIR]

RECORDING is a native/checks/record directory (its shots/F.ppm: bsnes's picture at the
end of frame F, counted from 1); SHOTS the engine's --shots directory (NNNNNN.ppm, the
engine's pictures counted from 0). Picture F of bsnes is the engine's picture F - 1 + K
(K, --offset, default 0: the replay draws the recording's picture N as its picture N,
and the core's frame F shows the picture that starts in update F - 1).

bsnes's libretro front end draws every colour through a gamma of 1.5 (the core's
target-libretro/program.cpp, updateVideoPalette: hard-coded, not an option), so its
pictures are not the console's colours: a colour (2, 1, 10) shows as (0, 0, 8) in 5 bits.
Each engine pixel's colour (the top 5 bits of each channel, the console's colour) is put
through that same function, and its top 5 bits must equal bsnes's (the recorder keeps the
core's top 5 bits of each channel); --raw
compares the top 5 bits of each instead, with no gamma. Prints one
line per picture: the frame, the pixels that differ and their share; then the totals.
--beside DIR writes DIR/F.ppm for each picture: bsnes left, the engine right, and below
them the differing pixels in white. Pictures find bugs; they are not a target.
Exits 0 when every compared picture is the same, 1 when any differs, 2 when none could be
compared.
"""
import argparse, os, re, sys


def bsnes_gamma(c5):
    """the core's output for a 5-bit channel: widened to 16 bits, then
    32767 * (v / 32767) ** 1.5 below 32768, truncated; its top 8 bits"""
    v = c5 << 3 | c5 >> 2
    v = v << 8 | v
    if v <= 32767:
        v = int(32767 * pow(v * (1.0 / 32767.0), 1.5))
    return v >> 8


GAMMA = [bsnes_gamma(c) >> 3 for c in range(32)]   # as 5 bits: the recorder keeps the core's top 5 bits


def read_ppm(path):
    with open(path, 'rb') as f:
        data = f.read()
    m = re.match(rb'P6\s+(\d+)\s+(\d+)\s+(\d+)\s', data)
    if not m:
        raise SystemExit(f'compare: {path} is not a P6 picture')
    w, h = int(m.group(1)), int(m.group(2))
    px = data[m.end():m.end() + w * h * 3]
    if len(px) != w * h * 3:
        raise SystemExit(f'compare: {path} is short')
    return w, h, px


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('recording')
    ap.add_argument('shots')
    ap.add_argument('--offset', type=int, default=0)
    ap.add_argument('--beside', default='')
    ap.add_argument('--raw', action='store_true')
    a = ap.parse_args()
    names = sorted((int(n[:-4]) for n in os.listdir(os.path.join(a.recording, 'shots')) if n.endswith('.ppm')))
    if a.beside:
        os.makedirs(a.beside, exist_ok=True)
    compared = same = 0
    total = 0
    for f in names:
        mine = os.path.join(a.shots, f'{f - 1 + a.offset:06d}.ppm')
        if not os.path.exists(mine):
            continue
        bw, bh, bp = read_ppm(os.path.join(a.recording, 'shots', f'{f}.ppm'))
        nw, nh, np_ = read_ppm(mine)
        if (bw, bh) != (nw, nh):
            print(f'{f}: sizes differ, bsnes {bw}x{bh}, engine {nw}x{nh}')
            compared += 1
            continue
        mask = bytearray(bw * bh)
        bad = 0
        for i in range(bw * bh):
            o = i * 3
            if a.raw:
                differ = (bp[o] >> 3, bp[o + 1] >> 3, bp[o + 2] >> 3) != (np_[o] >> 3, np_[o + 1] >> 3, np_[o + 2] >> 3)
            else:
                differ = (bp[o] >> 3, bp[o + 1] >> 3, bp[o + 2] >> 3) != (GAMMA[np_[o] >> 3], GAMMA[np_[o + 1] >> 3], GAMMA[np_[o + 2] >> 3])
            if differ:
                mask[i] = 1
                bad += 1
        compared += 1
        same += bad == 0
        total += bad
        print(f'{f}: {bad} pixels differ ({100.0 * bad / (bw * bh):.2f}%)')
        if a.beside:
            out = bytearray()
            for y in range(bh):
                out += bp[y * bw * 3:(y + 1) * bw * 3] + np_[y * bw * 3:(y + 1) * bw * 3]
            for y in range(bh):
                row = bytearray()
                for x in range(bw):
                    row += b'\xff\xff\xff' if mask[y * bw + x] else b'\x00\x00\x00'
                out += row + bytes(bw * 3)
            with open(os.path.join(a.beside, f'{f}.ppm'), 'wb') as g:
                g.write(b'P6\n%d %d\n255\n' % (bw * 2, bh * 2) + bytes(out))
    if not compared:
        print('compare: no picture of the recording has an engine picture to compare with')
        return 2
    print(f'{compared} pictures compared: {same} the same, {compared - same} differ, {total} pixels in all')
    return 0 if same == compared else 1


if __name__ == '__main__':
    sys.exit(main())
