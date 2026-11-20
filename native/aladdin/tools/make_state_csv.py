#!/usr/bin/env python3
"""make_state_csv.py: tools/state_fields.csv from src/system/state.h, the
one source: each field of g, where the original kept it, for the state
comparison against a recording (plan sections 4 and 6). A row is a field,
or a field of every record of an array:

  field     the C path (actors[].x: every element of actors)
  address   the original's first byte, as $7E:xxxx (the low 8 KB of bank $00 is the same RAM)
  count     elements, stride their distance in the original (hex bytes, or N bits for packed fields)
  bytes     the original's bytes per element
  bits      the bits of those bytes the field holds, when not all
  how       value; NN.8 (a fraction byte below the pixels); pointer (a record's address in the original);
            ref (an address in the cartridge in the original); native (no original byte: not compared)

Run from anywhere; it rewrites state_fields.csv beside it."""
import csv, os, re

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'src', 'system', 'state.h')
OUT = os.path.join(HERE, 'state_fields.csv')
SIZE = {'uint8_t': 1, 'int8_t': 1, 'uint16_t': 2, 'int16_t': 2, 'uint32_t': 4, 'int32_t': 4}
# the original's record sizes (state.h's comments)
RECORDS = {'Task': 0x18, 'Actor': 0x50, 'VramCopy': 8, 'TileBlock': 10, 'ScrollLayer': 0x156, 'LayerEdge': 4}
DECL = re.compile(r'^\s*((?:const\s+)?(?:struct\s+)?\w+\s*\*?)\s*(\w+)((?:\[[^\]]+\])?)\s*;\s*/\*\s*(.*?)\s*(?:\*/)?\s*$')


def parse():
    text = open(SRC).read()
    consts = {m.group(1): int(m.group(2)) for m in re.finditer(r'#define (\w+) (\d+)', text)}
    structs, cur, body = {}, None, []
    lines = text.split('\n')
    i = 0
    while i < len(lines):
        line = lines[i]
        m = re.match(r'typedef struct (\w+) \{', line)
        if m:
            cur, body, depth = m.group(1), [], 1
        elif cur:
            depth += line.count('{') - line.count('}')
            if depth == 0:
                structs[cur] = body; cur = None
            else:
                full = line
                while '/*' in full and '*/' not in full and i + 1 < len(lines):
                    i += 1; full += ' ' + lines[i].strip()
                d = DECL.match(full)
                if d:
                    body.append(d.groups())
        i += 1
    return structs, consts


def count_of(dim, consts):
    if not dim:
        return 1
    n = 1
    for part in re.findall(r'\w+', dim[1:-1]):
        n *= int(part) if part.isdigit() else consts[part]
    return n


def where(comment):
    """(absolute address or None, record offset or None, bytes or None, stride or None, bits)"""
    absa = rel = nbytes = stride = None
    m = re.match(r'\+\$([0-9A-F]+)(?:-\$([0-9A-F]+))?', comment)
    if m:
        rel = int(m.group(1), 16)
        if m.group(2):
            nbytes = int(m.group(2), 16) - rel + 1
    m2 = re.match(r'\$(?:([0-9A-F]{2}):)?([0-9A-F]{4})(?:-\$([0-9A-F]{4}))?', comment)
    if m2:
        absa = (int(m2.group(1), 16) << 16 if m2.group(1) else 0x7E0000) | int(m2.group(2), 16)
        if m2.group(3):
            nbytes = int(m2.group(3), 16) - int(m2.group(2), 16) + 1
    s = re.search(r'every (\d+) bytes', comment)
    if s:
        stride = int(s.group(1))
    # bits only where the comment names them right after the address ("$03B8 bits 0-3")
    b = re.match(r'\+?\$(?:[0-9A-F]{2}:)?[0-9A-F]+(?:-\$[0-9A-F]+)? bits? (\d+)(?:-(\d+))?', comment)
    bits = (b.group(1) + ('-' + b.group(2) if b.group(2) else '')) if b else ''
    return absa, rel, nbytes, stride, bits


def how_of(typ, comment):
    if 'resume point' in comment:
        return 'native (a resume point for the original\'s PC)'
    if '*' in typ and 'uint8_t' in typ:
        return 'ref'
    if '*' in typ:
        return 'pointer'
    if typ.strip() == 'DataRef':
        return 'ref'
    m = re.search(r'\b(\d+)\.8\b', comment)
    return m.group(0) if m else 'value'


def main():
    structs, consts = parse()
    rows = []

    def emit(path, typ, comment, base, outer_count, outer_stride, dim):
        absa, rel, nbytes, stride, bits = where(comment)
        t = typ.replace('const', '').replace('*', '').replace('struct', '').strip()
        n = count_of(dim, consts)
        if absa is None and rel is None:
            rows.append([path, '', outer_count * n, '', '', '', 'native'])
            return
        addr = absa if absa is not None else base + rel
        if t in RECORDS and t in structs and '*' not in typ:
            for ft, fn, fd, fc in structs[t]:
                emit(f'{path}[].{fn}' if n > 1 or outer_count > 1 else f'{path}.{fn}', ft, fc, addr,
                     outer_count * n, RECORDS[t], fd)
            return
        if fn_own(path):
            # the actor's own bytes: the offsets its comment lists, in order
            offs = []
            for a, b in re.findall(r'\$([0-9A-F]{2})(?:-\$([0-9A-F]{2}))?', comment):
                offs += list(range(int(a, 16), int(b or a, 16) + 1))
            for k, o in enumerate(offs):
                rows.append([f'{path}[{k}]', f'{base + o:06X}', outer_count, f'{outer_stride:X}', 1, '', 'value'])
            return
        size = SIZE.get(t, 2 if '*' in typ or t == 'DataRef' else None)
        elem = nbytes if nbytes and n == 1 else size
        if t == 'nshvc_sprite':
            rows.append([path, f'{addr:06X}', n, '', '', '', 'native (the sprite table as the engine\'s sprites)'])
            return
        packed = re.search(r'a nibble each|(\d) bits each', comment)
        if n > 1 and packed:
            width = 4 if 'nibble' in packed.group(0) else int(packed.group(1))
            rows.append([path, f'{addr:06X}', n, f'{width} bits', 1, f'{width} each, from bit 0', how_of(typ, comment)])
            return
        if n > 1:
            st = stride or (elem if elem else 1)
            rows.append([path, f'{addr:06X}', outer_count * n if outer_count > 1 else n,
                         f'{outer_stride:X}+{st:X}' if outer_count > 1 else f'{st:X}', elem, bits, how_of(typ, comment)])
        else:
            rows.append([path, f'{addr:06X}', outer_count, f'{outer_stride:X}' if outer_count > 1 else '',
                         elem, bits, how_of(typ, comment)])

    def fn_own(path):
        return path.endswith('.bytes') and path.startswith('actors')

    for typ, name, dim, comment in structs['GameState']:
        emit(name, typ, comment, 0x7E0000, 1, 0, dim)
    # the union members sit one line below their own declaration: emit them through their parent record
    with open(OUT, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['field', 'address', 'count', 'stride', 'bytes', 'bits', 'how'])
        w.writerows(rows)
    print(f'{OUT}: {len(rows)} rows')


if __name__ == '__main__':
    main()
