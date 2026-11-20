#!/usr/bin/env python3
"""data_check.py ROM: stage 2's data checks (plan section 4).

1. extract_assets.py makes cart.dat from ROM; every range of assets.csv must
   hold the ROM's bytes and every other byte must be 0 (the code left out).
2. data/'s readers, built with tools/data_test.c outside the engine, must give
   what the ROM's bytes give, read here a second way from the formats the
   readers' comments state: every sound list the decomp knows
   (config/aladdin/spclists.csv), the quarter sine, the demo scripts' range.
3. With PLANT=1 one byte of a read range is changed in cart.dat first: the
   checks must then fail (they can).
Exits 0 when everything matches. CC names the compiler (default cc); NSHVC_ROOT the engine."""
import csv, os, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.join(HERE, '..')
REPO = os.path.normpath(os.path.join(GAME, '..', '..'))


def engine():
    if os.environ.get('NSHVC_ROOT'):
        return os.environ['NSHVC_ROOT']
    d = REPO
    while d != os.path.dirname(d):
        c = os.path.join(os.path.dirname(d), 'nativeshvc')
        if os.path.isfile(os.path.join(c, 'include', 'nshvc', 'nshvc.h')):
            return c
        d = os.path.dirname(d)
    sys.exit('data_check: set NSHVC_ROOT')


def lorom(bank, addr):
    return (bank & 0x7F) * 0x8000 + (addr & 0x7FFF)


def sum31(bs):
    s = 0
    for x in bs:
        s = (s * 31 + x) & 0xFFFFFFFF
    return s


def expected_list(rom, n):
    e = lorom(0x85, 0x8000) + 3 * n
    addr = rom[e] | rom[e + 1] << 8
    bank = rom[e + 2]
    if addr == 0 and bank == 0:
        return f'list {n} blocks -1 start 0000'
    at = lorom(bank | 0x80, addr | 0x8000)
    out = []
    while True:
        length = rom[at] | rom[at + 1] << 8
        to = rom[at + 2] | rom[at + 3] << 8
        if length == 0:
            break
        out.append((to, length, at + 4))
        at += 4 + length
    return out, to


def main():
    rom = open(sys.argv[1], 'rb').read()
    if len(rom) & 0x200:
        rom = rom[0x200:]
    fails = 0
    tmp = tempfile.mkdtemp(prefix='aladdin-data-check-')
    subprocess.run([sys.executable, os.path.join(HERE, 'extract_assets.py'), sys.argv[1], tmp], check=True,
                   capture_output=True)
    cart = bytearray(open(os.path.join(tmp, 'cart.dat'), 'rb').read())
    ranges = [(int(r['start'], 16), int(r['end'], 16)) for r in csv.DictReader(open(os.path.join(HERE, 'assets.csv')))]
    if os.environ.get('PLANT'):
        s, e = ranges[len(ranges) // 2]
        cart[s] ^= 0xFF
        open(os.path.join(tmp, 'cart.dat'), 'wb').write(cart)
        print(f'planted: byte {s:06X} changed')
    # code bytes, from the ROM itself: the image is left zero for each of them
    inside = bytearray(len(rom))
    bad = 0
    for s, e in ranges:
        inside[s:e] = b'\1' * (e - s)
        if cart[s:e] != rom[s:e]:
            bad += 1
    nonzero_out = sum(1 for i in range(len(rom)) if not inside[i] and cart[i])
    kept = sum(e - s for s, e in ranges)
    print(f'ranges: {len(ranges)}, {kept} B; differing from the ROM: {bad}; bytes outside them not 0: {nonzero_out}; '
          f'left out: {len(rom) - kept} B')
    fails += bad + nonzero_out + (len(cart) != len(rom))
    # the readers
    exe = os.path.join(tmp, 'data_test')
    srcs = [os.path.join(HERE, 'data_test.c')] + [os.path.join(GAME, 'src', 'data', f)
                                                   for f in sorted(os.listdir(os.path.join(GAME, 'src', 'data'))) if f.endswith('.c')]
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu11', '-Wall', '-Wextra', '-Werror', '-O1',
                    '-I', os.path.join(GAME, 'src'), '-I', os.path.join(engine(), 'include'), '-o', exe] + srcs, check=True)
    lists = [int(r['address'], 16) for r in csv.DictReader(open(os.path.join(REPO, 'config', 'aladdin', 'spclists.csv')))]
    # spclists.csv gives each list's address; its number is its place in the table
    table = {}
    for n in range(55):
        e = lorom(0x85, 0x8000) + 3 * n
        a = rom[e] | rom[e + 1] << 8
        if a:
            table[(rom[e + 2] | 0x80, a | 0x8000)] = n
    numbers = []
    for r in csv.DictReader(open(os.path.join(REPO, 'config', 'aladdin', 'spclists.csv'))):
        numbers.append(table[(int(r['bank'], 16), int(r['address'], 16))])
    out = subprocess.run([exe] + [str(n) for n in numbers], capture_output=True, text=True, check=True,
                         env=dict(os.environ, DATA_TEST_DIR=tmp)).stdout.split('\n')
    lists_bad = 0
    for n, line in zip(numbers, out):
        blocks, start = expected_list(rom, n)
        # the expected bytes are the ROM's where the range is data, and 0 where cart.dat left code out
        want = f'list {n} blocks {len(blocks)} start {start:04X}' + ''.join(
            f' {to:04X}:{ln}:{sum31(bytes(rom[i] if inside[i] else 0 for i in range(at, at + ln))):08X}'
            for to, ln, at in blocks)
        if line != want:
            lists_bad += 1
            print('  differs:', line[:120], '| want', want[:120])
    sine = ' '.join(str(rom[lorom(0x82, 0x87D4) + 2 * i]) for i in range(64))
    demo = ' '.join(f'{rom[lorom(0x84, 0xF700) + i]:02X}/{rom[lorom(0x84, 0xF700) + i + 1]}' for i in range(0, 0x82, 2))
    sine_ok = out[len(numbers)] == 'sine ' + sine
    demo_ok = out[len(numbers) + 1] == 'demo ' + demo
    print(f'readers: sound lists {len(numbers) - lists_bad} of {len(numbers)} match; quarter sine {"matches" if sine_ok else "DIFFERS"}; '
          f'demo scripts {"match" if demo_ok else "DIFFER"}')
    fails += lists_bad + (not sine_ok) + (not demo_ok)
    shutil.rmtree(tmp)
    print('data check:', 'PASS' if not fails else f'FAIL ({fails})')
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
