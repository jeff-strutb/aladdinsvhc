#!/usr/bin/env python3
"""data_check.py ROM: stage 2's data checks (plan section 4).

1. extract_assets.py makes cart.dat from ROM; every range of assets.csv must
   hold the ROM's bytes ('data', 'unread', and 'code-read': code the game reads
   as a table) and every other byte must be 0 (the code left out).
2. data/'s readers, built with tools/data_test.c outside the engine, must give
   what the ROM's bytes give, read here a second way from the formats the
   readers' comments state: every sound list the decomp knows
   (config/aladdin/spclists.csv), the quarter sine, the demo scripts' range,
   the 98 palette loads, the level graphics lists, the unpack sets, the
   level maps.
3. No game code outside data/ holds a ROM place (clean_check.rom_places over src/).
4. With PLANT=1 one byte of a read range is changed in cart.dat first: the
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
    rows = list(csv.DictReader(open(os.path.join(HERE, 'assets.csv'))))
    ranges = [(int(r['start'], 16), int(r['end'], 16)) for r in rows]
    by_kind = {}
    for r in rows:
        k = by_kind.setdefault(r['reader'], [0, 0]); k[0] += 1; k[1] += int(r['end'], 16) - int(r['start'], 16)
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
    print(f'ranges: {len(ranges)}, {kept} B ' + '(' + ', '.join(f'{k} {n} ranges {b} B' for k, (n, b) in sorted(by_kind.items())) + ')'
          f'; differing from the ROM: {bad}; bytes outside them not 0: {nonzero_out}; code left out: {len(rom) - kept} B')
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
    pal, odd = [], 0
    for i in range(98):
        e = lorom(0x84, 0xC000) + 6 * i
        src, n, dst = (rom[e] | rom[e + 1] << 8, rom[e + 2] | rom[e + 3] << 8, rom[e + 4] | rom[e + 5] << 8)
        odd += (dst & 1) or dst >= 0x200
        s = 0
        for j in range(n):
            o = lorom(0x84, (0xC000 + src + 2 * j) & 0xFFFF)
            s = (s * 31 + (rom[o] | rom[o + 1] << 8)) & 0xFFFFFFFF
        pal.append(f'{n}/{dst >> 1}/{s:08X}')
    pal_ok = out[len(numbers) + 2] == 'palettes ' + ' '.join(pal) and not odd
    fails += not pal_ok
    print(f'readers: palette loads {"98 of 98 match" if pal_ok else "DIFFER"} (odd or past-the-palette places: {odd})')
    # packed graphics, a second implementation from the format the readers state
    def unpack(at, groups):
        out = bytearray()
        for _ in range(groups):
            flags, fill = cart[at], cart[at + 1]; at += 2
            for i in range(8):
                if flags & 0x80:
                    out.append(cart[at]); at += 1
                else:
                    out.append(fill)
                flags = (flags << 1) & 0xFF
        return out, at

    low_src, code_read = [], 0

    def summ(bank, addr, length):
        nonlocal code_read
        groups = ((length + 7) & 0xFFF8) >> 3
        if addr < 0x8000:
            low_src.append(f'{bank:02X}:{addr:04X}')
        at = lorom(bank, addr)
        out, end = unpack(at, groups)
        code_read += sum(1 for i in range(at, end) if not inside[i])
        s = 0
        for x in out:
            s = (s * 31 + x) & 0xFFFFFFFF
        return f'{s ^ len(out):08X}', groups

    w = lambda o: rom[o] | rom[o + 1] << 8
    lv = []
    for lvl in range(66):
        x0 = w(lorom(0x80, 0xDF98) + 2 * lvl)
        g8 = lambda a: rom[lorom(0x80, (0xDF98 + a) & 0xFFFF)]
        g16 = lambda a: g8(a) | g8(a + 1) << 8
        n = g8(x0); part = f'{lvl}:'
        for i in range(n):
            e = x0 + 7 * i
            sm, _ = summ(g8(e + 3), g16(e + 1), g16(e + 4))
            part += f'v{g16(e + 6):04X}/{g16(e + 4)}/{sm}'
        if n:
            kx = x0 + 7 * n
            for i in range(g8(kx + 1)):
                e = kx + 5 * i
                sm, _ = summ(g8(e + 4), g16(e + 2), g16(e + 5))
                part += f'k{sm}'
        lv.append(part)
    lv_ok = out[len(numbers) + 3] == 'levels ' + ' '.join(lv)
    sets = []
    for s_ in range(50):
        e = lorom(0x80, 0xF039) + 7 * s_
        sm, groups = summ(rom[e + 4], w(e + 2), w(e + 5))
        sets.append(f'{w(e):04X}/{groups}/{sm}')
    sets_ok = out[len(numbers) + 4] == 'sets ' + ' '.join(sets)
    fails += (not lv_ok) + (not sets_ok)
    print(f'readers: level graphics (66 levels) {"match" if lv_ok else "DIFFER"}; unpack sets 0-49 {"match" if sets_ok else "DIFFER"}; '
          f'sources below $8000: {len(low_src)} {low_src[:6]}; packed bytes read from code left out: {code_read}')
    # the level maps, a second reading
    m8 = lambda a: rom[lorom(0x80, a & 0xFFFF)]
    m16 = lambda a: m8(a) | m8(a + 1) << 8
    maps, map_code = [], 0
    for lvl in range(66):
        h = (0xE3DC + m16(0xE3DC + 2 * lvl)) & 0xFFFF
        n = m8(h) or 256
        bl, ti = m8(h + 1), m8(h + 2)
        s = 0
        for b_ in range(n):
            sc = m8(h + 5 + b_)
            base = lorom(bl, (0x8000 + (sc << 7)) & 0xFFFF)
            map_code += sum(1 for i in range(base, base + 132) if not inside[i])
            for j in range(0, 128, 2):
                blk = rom[base + j] | rom[base + j + 1] << 8
                for k in range(4):
                    o = lorom(ti, (0x8000 + blk + 2 * k) & 0xFFFF)
                    map_code += (not inside[o]) + (not inside[o + 1])
                    s = (s * 31 + (rom[o] | rom[o + 1] << 8)) & 0xFFFFFFFF
            for j in range(128, 132):
                s = (s * 31 + rom[base + j]) & 0xFFFFFFFF
        part = f'{lvl}:{n}/{m8(h + 4):02X}/{s:08X}'
        ent = m16(0xE460 + 2 * lvl)
        for lay in range(2 if m16(0xE460 + ent + 2) else 1):
            at = (0xE460 + m16(0xE460 + ent + 2 * lay)) & 0xFFFF
            cnt = lambda v: v + 1 if v < 0x80 else 1
            c, r_ = cnt(m8(at)), cnt(m8(at + 1))
            ls = 0
            for j in range(c * r_):
                ls = (ls * 31 + m8(at + 2 + j)) & 0xFFFFFFFF
            part += f'L{c}x{r_}/{ls:08X}'
        maps.append(part)
    maps_ok = out[len(numbers) + 5] == 'maps ' + ' '.join(maps)
    fails += not maps_ok
    print(f'readers: level maps (66 levels) {"match" if maps_ok else "DIFFER"}; map bytes read from code left out: {map_code}')
    # the animation sets the traces read (32-36: their entry words are the bytes read at $82:AC58-$AC61)
    anims = []
    for s_ in range(32, 37):
        x = w(lorom(0x82, 0xAC18 + 2 * s_))
        e = lorom(0x82, (0xAC58 + x) & 0xFFFF)
        ln = w(e)
        groups = (ln & 0x3FFF) >> 3
        if ln:
            at = lorom(rom[e + 4], w(e + 2))
            out_, end = unpack(at, groups)
            code_read += sum(1 for i in range(at, end) if not inside[i])
            s = 0
            for v in out_:
                s = (s * 31 + v) & 0xFFFFFFFF
            anims.append(f'{ln:04X}/{groups}/{s ^ len(out_):08X}')
        else:
            anims.append(f'{ln:04X}/{groups}/00000000')
        anims[-1] += f'/{ln >> 14}/{groups}' + ''.join(
            f',{rom[lorom(0x82, 0xAC5D) + ((x + v) & 0xFFFF)]:02X}' for v in range(4))
    anims_ok = out[len(numbers) + 6] == 'animsets ' + ' '.join(anims)
    fails += not anims_ok
    print(f'readers: animation sets 32-36 (and by kind: extra slots, variant bytes) {"match" if anims_ok else "DIFFER"}; packed bytes read from code left out (all packed): {code_read}')
    print(f'readers: sound lists {len(numbers) - lists_bad} of {len(numbers)} match; quarter sine {"matches" if sine_ok else "DIFFERS"}; '
          f'demo scripts {"match" if demo_ok else "DIFFER"}')
    fails += lists_bad + (not sine_ok) + (not demo_ok)
    # no ROM place in game code outside data/ (the master's work split, PLAN.md)
    sys.path.insert(0, HERE)
    import clean_check
    places = clean_check.rom_places(list(clean_check.files([os.path.join(GAME, 'src')])))
    for f in places:
        print('  ', f)
    print(f'ROM places outside data/: {len(places)}')
    fails += len(places)
    shutil.rmtree(tmp)
    print('data check:', 'PASS' if not fails else f'FAIL ({fails})')
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
