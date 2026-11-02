#!/usr/bin/env python3
"""spcdraft.py: SPC700 routines' C and assembly drafted from the traced intake.

    spcdraft.py [--c | --asm] APU...     (APU: a routine's entry, e.g. 0CB0)

Reads the sound driver's traced instructions from the raw intake
(build/aladdin/raw/bank05.asm, the lines tagged `spc AAAA`) and the routine
extents from build/aladdin/spc_routines.csv. Each instruction becomes the
spc_ops.h step that makes the same APU accesses in the same order as the
pinned core (processor/spc700/instructions.cpp): a store reads its address
first except MOV (X+),A and MOV dd,ss; a word read takes the low byte, then
the high. Calls go to the callee's C (spc_call with the return address),
jumps to another routine's entry are tail calls. Names come from the @spc
tags already filed and from NAME=APU arguments (VoiceRelease=0414).

What it cannot say exactly carries a TODO, which gate 0 refuses: code the
traces never ran (incbin bytes inside an extent), JMP (abs+X), a jump or
branch out of the routine to an address no routine starts at.

The assembly keeps the intake's addresses; name them by hand.
"""
import csv, glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'aladdin')
RAW = os.path.join(B, 'raw', 'bank05.asm')

LINE = re.compile(r'^\s+(\S+)(?:\s+(.*?))?\s*;\s*85([0-9A-F]{4}) spc ([0-9A-F]{4})\s*$')
LABEL = re.compile(r'^(spc_([0-9A-F]{4})_[0-9A-F]{6}):\s*$')
INCBIN = re.compile(r'incbin "[^"]*":\$([0-9A-F]+)\.\.\$([0-9A-F]+)')


def intake():
    """APU address -> (mnemonic, operand, ROM offset); label name -> APU; untraced ROM ranges."""
    ins, labels, untraced = {}, {}, []
    for l in open(RAW):
        m = LINE.match(l)
        if m:
            apu = int(m.group(4), 16)
            rom = 0x028000 + int(m.group(3), 16) - 0x8000
            ins[apu] = (m.group(1), (m.group(2) or '').strip(), rom)
            continue
        m = LABEL.match(l.rstrip())
        if m:
            labels[m.group(1)] = int(m.group(2), 16)
            continue
        m = INCBIN.search(l)
        if m:
            untraced.append((int(m.group(1), 16), int(m.group(2), 16)))
    return ins, labels, untraced


# The command table the driver jumps through (JMP (abs+X)): its address and
# how many entries are handlers; the entries past them are the next code's
# bytes, which no command the S-CPU sends reaches.
TABLES = {0x050E: (0x0511, 14)}
# The sequencer's command table: 32 handler addresses, pushed and reached by
# RET (the command loop at $0803); hand-written there, but its entries are
# entry points.
DISPATCH = {0x0831: 32}
BLOCK = (0x0280BD, 0x028ABF, 0x02C0)   # the driver's ROM bytes and their APU address


def apu_image():
    rom = open(os.path.join(B, 'baserom.sfc'), 'rb').read()
    mem = bytearray(0x10000)
    lo, hi, at = BLOCK
    mem[at:at + hi - lo] = rom[lo:hi]
    return mem


def add_untraced_code(ins, untraced):
    """Decode the untraced chunks that are code: reached by falling through
    from a traced instruction, or a command table entry. The others are data."""
    import spc700
    mem = apu_image()
    lo0, hi0, at = BLOCK
    targets = set()
    for tab, n in list(TABLES.values()) + list(DISPATCH.items()):
        for k in range(n):
            targets.add(mem[tab + 2 * k] | mem[tab + 2 * k + 1] << 8)
    by_rom = {v[2]: (a, v) for a, v in ins.items()}
    code = []
    for u0, u1 in untraced:
        if not (lo0 <= u0 < hi0):
            continue
        a0 = u0 - lo0 + at
        prev = max((r for r in by_rom if r < u0), default=None)
        falls = prev is not None and by_rom[prev][1][0] not in ('ret', 'jmp', 'bra', 'reti')
        if not falls and a0 not in targets:
            continue
        a = a0
        while a < u1 - lo0 + at:
            txt, n = spc700.decode(mem, a)
            mn, _, op = txt.partition(' ')
            ins[a] = (mn, op.strip(), a - at + lo0)
            untraced_ins.add(a)
            a += n
        code.append((u0, u1))
    return [c for c in untraced if c not in code], targets


untraced_ins = set()


def routines():
    out = {}
    for r in csv.DictReader(open(os.path.join(B, 'spc_routines.csv'))):
        out[int(r['apu'], 16)] = (int(r['offset'], 16), int(r['end'], 16))
    return out


def filed_names():
    names = {}
    for p in glob.glob(os.path.join(ROOT, 'src', 'aladdin', '**', '*.c'), recursive=True):
        t = open(p).read()
        for m in re.finditer(r'@routine \w\w:\w{4} (\w+)\n \* @spc ([0-9A-Fa-f]{4})', t):
            names[int(m.group(2), 16)] = m.group(1)
    return names


def hx(s):
    return int(s.replace('$', ''), 16)


class Draft:
    def __init__(self, apu, ins, labels, untraced, rts, names):
        self.apu, self.ins, self.labels, self.names = apu, ins, labels, names
        lo, hi = rts[apu]
        self.addrs = sorted(a for a, v in ins.items() if lo <= v[2] < hi)
        self.inside = set(self.addrs)
        self.untraced = [(a, b) for a, b in untraced if lo <= a < hi]
        self.lo, self.hi = lo, hi
        self.targets = set()
        self.decls = set()
        self.cuts = set()

    def name(self, a):
        if a in self.names:
            return self.names[a]
        return ('R%04X' if a in routines_all else 'P%04X') % a

    def nxt(self, a):
        """The address of the instruction after a (its length from the ROM offsets)."""
        i = self.ins[a]
        later = [x for x in self.ins if self.ins[x][2] > i[2]]
        if later:
            n = min(later, key=lambda x: self.ins[x][2])
            return a + self.ins[n][2] - i[2]
        return None

    def target(self, op, a):
        op = op.strip()
        if op in self.labels:
            return self.labels[op]
        v = hx(op)
        if len(op.replace('$', '')) <= 2:      # a raw relative displacement
            d = v - 256 if v > 127 else v
            return self.nxt(a) + d
        return v

    # operands -------------------------------------------------------
    def ea(self, o):
        """An effective address expression for a memory operand."""
        o = o.strip()
        m = re.fullmatch(r'\(\$([0-9A-F]{2})\)\+y', o)
        if m:
            return 'ind_y(0x%s)' % m.group(1)
        if o == '(x)':
            return 'spc_dp(spc.x)'
        m = re.fullmatch(r'\$([0-9A-F]{2})\+([xy])', o)
        if m:
            return 'spc_dp((uint8_t)(0x%s + spc.%s))' % (m.group(1), m.group(2))
        m = re.fullmatch(r'\$([0-9A-F]{4})\+([xy])', o)
        if m:
            return '(uint16_t)(0x%s + spc.%s)' % (m.group(1), m.group(2))
        m = re.fullmatch(r'\$([0-9A-F]{2})', o)
        if m:
            return 'spc_dp(0x%s)' % m.group(1)
        m = re.fullmatch(r'\$([0-9A-F]{4})', o)
        if m:
            return '0x%sU' % m.group(1)
        raise ValueError(o)

    def val(self, o):
        o = o.strip()
        if o.startswith('#'):
            return '0x%02X' % hx(o[1:])
        if o in ('a', 'x', 'y'):
            return 'spc.' + o
        return 'spc_rd(%s)' % self.ea(o)

    # one instruction ------------------------------------------------
    def step(self, a):
        mn, op, rom = self.ins[a]
        base = mn.split('.')[0]
        ops = [x.strip() for x in re.split(r',(?![^(]*\))', op)] if op else []
        R = ('a', 'x', 'y')
        alu = {'or': 'spc_or', 'and': 'spc_and', 'eor': 'spc_eor', 'adc': 'spc_adc', 'sbc': 'spc_sbc'}
        sh = {'asl': 'spc_asl', 'lsr': 'spc_lsr', 'rol': 'spc_rol', 'ror': 'spc_ror', 'inc': 'spc_inc', 'dec': 'spc_dec'}
        br = {'bne': '!spc.z', 'beq': 'spc.z', 'bcc': '!spc.c', 'bcs': 'spc.c', 'bpl': '!spc.n',
              'bmi': 'spc.n', 'bvc': '!spc.v', 'bvs': 'spc.v', 'bra': '1'}
        if base == 'mov':
            d, s = ops
            if d == 'sp':
                return 'spc.sp = spc.x;'
            if s == 'sp':
                return 'spc.x = spc_nz(spc.sp);'
            if d in R:
                return 'spc.%s = spc_nz(%s);' % (d, self.val(s))
            if d == '(x+)':
                return 'spc_wr(spc_dp(spc.x), spc.a); spc.x++;'
            if s.startswith('$') and re.fullmatch(r'\$[0-9A-F]{2}', s) and re.fullmatch(r'\$[0-9A-F]{2}', d):
                return '{ uint8_t t = spc_rd(spc_dp(0x%s)); spc_wr(spc_dp(0x%s), t); }' % (s[1:], d[1:])
            return 'spc_st(%s, %s);' % (self.ea(d), self.val(s))
        if base in alu or base == 'cmp':
            d, s = ops
            if d in R:
                if base == 'cmp':
                    return 'spc_cmp(spc.%s, %s);' % (d, self.val(s))
                return 'spc.%s = %s(spc.%s, %s);' % (d, alu[base], d, self.val(s))
            # memory destination: dd,ss or d,#imm (the source read first)
            if s.startswith('#'):
                rhs = '0x%02X' % hx(s[1:])
                pre = ''
            else:
                pre = 'uint8_t s = %s; ' % self.val(s)
                rhs = 's'
            if base == 'cmp':
                return '{ %suint8_t d = spc_rd(%s); spc_cmp(d, %s); }' % (pre, self.ea(d), rhs)
            return '{ %suint8_t d = spc_rd(%s); spc_wr(%s, %s(d, %s)); }' % (pre, self.ea(d), self.ea(d), alu[base], rhs)
        if base in sh:
            o = ops[0]
            if o in R:
                return 'spc.%s = %s(spc.%s);' % (o, sh[base], o)
            return 'spc_rmw(%s, %s);' % (self.ea(o), sh[base])
        if base in ('incw', 'decw'):
            return 'spc_incw(0x%02X, %d);' % (hx(ops[0]), 1 if base == 'incw' else -1)
        if base == 'movw':
            d, s = ops
            if d == 'ya':
                return 'spc_movw_ld(0x%02X);' % hx(s)
            return 'spc_movw_st(0x%02X);' % hx(d)
        if base in ('addw', 'subw', 'cmpw'):
            d = hx(ops[1])
            w = '(uint16_t)(spc_rd(spc_dp(0x%02X)) | spc_rd(spc_dp(0x%02X)) << 8)' % (d, (d + 1) & 0xFF)
            if base == 'cmpw':
                return '{ uint16_t w = %s; spc_cmpw(spc_ya(), w); }' % w
            return '{ uint16_t w = %s; spc_set_ya(spc_%s(spc_ya(), w)); }' % (w, base)
        if base == 'mul':
            return 'spc_mul();'
        if base == 'div':
            return 'spc_div();'
        if base == 'push':
            return 'spc_push(spc_psw());' if ops[0] == 'p' else 'spc_push(spc.%s);' % ops[0]
        if base == 'pop':
            return 'spc_set_psw(spc_pull());' if ops[0] == 'p' else 'spc.%s = spc_pull();' % ops[0]
        if base in ('set1', 'clr1'):
            d, bit = ops[0].split('.')
            return 'spc_bit_set(0x%02X, %s, %d);' % (hx(d), bit, base == 'set1')
        if base in ('bbs', 'bbc'):
            d, bit = ops[0].split('.')
            t = self.target(ops[1], a)
            c = '' if base == 'bbs' else '!'
            return self.go('%s(spc_rd(spc_dp(0x%02X)) >> %s & 1)' % (c, hx(d), bit), t)
        if base in br:
            return self.go(br[base], self.target(ops[0], a))
        if base == 'dbnz':
            t = self.target(ops[1], a)
            if ops[0] == 'y':
                return self.go('(spc.y = (uint8_t)(spc.y - 1)) != 0', t)
            d = hx(ops[0])
            return '{ uint8_t v = (uint8_t)(spc_rd(spc_dp(0x%02X)) - 1); spc_wr(spc_dp(0x%02X), v); %s }' % (d, d, self.go('v != 0', t))
        flags = {'clrc': 'spc.c = 0;', 'setc': 'spc.c = 1;', 'clrp': 'spc.p = 0;', 'setp': 'spc.p = 1;',
                 'clrv': 'spc.v = 0; spc.h = 0;', 'notc': 'spc.c ^= 1;', 'xcn': 'spc.a = spc_xcn(spc.a);',
                 'daa': 'spc_daa();', 'das': 'spc_das();', 'nop': ';'}
        if base in flags:
            return flags[base]
        if base == 'call':
            t = hx(ops[0])
            self.decls.add(t)
            return 'spc_call(abi_%s, 0x%04X);' % (self.name(t), self.nxt(a))
        if base == 'jmp':
            if ops[0].startswith('('):
                if a not in TABLES:
                    return '/* TODO %04X: jmp %s, a table jump */' % (a, ops[0])
                tab, n = TABLES[a]
                mem = apu_image()
                cases = []
                for k in range(n):
                    tg = mem[tab + 2 * k] | mem[tab + 2 * k + 1] << 8
                    self.decls.add(tg)
                    cases.append('case 0x%04X: abi_%s(); return;' % (tg, self.name(tg)))
                return ('{ uint16_t t = spc_rd((uint16_t)(0x%04X + spc.x)); t |= (uint16_t)(spc_rd((uint16_t)(0x%04X + spc.x)) << 8); '
                        'switch (t) { %s } }' % (tab, tab + 1, ' '.join(cases)))
            return self.go('1', hx(ops[0]))
        if base == 'ret':
            return 'spc_ret(); return;'
        return '/* TODO %04X: %s %s */' % (a, mn, op)

    def go(self, cond, t):
        if t in self.inside:
            self.targets.add(t)
            s = 'goto L_%04X;' % t
        elif t == self.apu:
            self.targets.add(t)
            s = 'goto L_%04X;' % t
        else:
            self.decls.add(t)
            s = '{ abi_%s(); return; }' % self.name(t)
            if t not in self.names and t not in routines_all and t not in splits and t not in self.cuts:
                s = '/* TODO: to %04X, no routine starts there */' % t
        return s if cond == '1' else 'if (%s) %s' % (cond, s)

    def segment(self, seg, start):
        """The C of the instructions seg (a run starting at start)."""
        self.inside = set(seg)
        self.apu = start
        self.targets = set()
        body = []
        prev_rom = None
        for a in seg:
            rom = self.ins[a][2]
            for u0, u1 in self.untraced:
                if prev_rom is not None and prev_rom < u0 <= rom and self.ins[prev_a][0] not in ('ret', 'jmp', 'bra', 'reti'):
                    body.append(('    /* TODO %06X: untraced code */' % u0, None))
            body.append((self.step(a), a))
            prev_rom = rom
            prev_a = a
        last = seg[-1]
        for u0, u1 in self.untraced:
            if u0 > prev_rom and (self.nxt_rom(last) is None or u0 <= self.nxt_rom(last)) and u0 < self.hi and self.ins[last][0] not in ('ret', 'jmp', 'bra', 'reti'):
                body.append(('    /* TODO %06X: untraced code */' % u0, None))
        # a run that ends without a RET or jump falls into the next one
        n = self.nxt(last)
        mn = self.ins[last][0]
        if mn not in ('ret', 'jmp', 'bra') and n is not None:
            self.decls.add(n)
            body.append(('    abi_%s();  /* on into %04X */' % (self.name(n), n), None))
        out = []
        for s, a in body:
            if a is not None and a in self.targets:
                out.append('L_%04X:' % a)
            if a is None:
                out.append(s)
            else:
                mn, op, _ = self.ins[a]
                out.append('    %s  /* %04X %s %s%s */' % (s, a, mn, op, ', not run by the traces' if a in untraced_ins else ''))
        return 'void abi_%s(void)\n{\n%s\n}\n' % (self.name(start), '\n'.join(out))

    def nxt_rom(self, a):
        n = self.nxt(a)
        return None if n is None else self.ins[a][2] + (n - a)

    def c(self):
        allin = list(self.addrs)
        cuts = set(a for a in allin if a in splits and a != allin[0])
        # a branch from one run to a label in another makes that label a run too
        while True:
            seg_of, s = {}, 0
            for a in allin:
                if a in cuts:
                    s += 1
                seg_of[a] = s
            more = set()
            for a in allin:
                tgs = [self.labels[w] for w in re.findall(r'spc_[0-9A-F]{4}_[0-9A-F]{6}', self.ins[a][1])]
                m = re.search(r'\$([0-9A-F]{4})$', self.ins[a][1])
                if a in untraced_ins and m and self.ins[a][0].startswith(('b', 'jmp', 'dbnz')):
                    tgs.append(int(m.group(1), 16))
                for tg in tgs:
                    if tg in seg_of and seg_of[tg] != seg_of[a] and tg not in cuts and tg != allin[0]:
                        more.add(tg)
            if not more:
                break
            cuts |= more
        self.cuts = cuts
        segs, cur, start = [], [], allin[0]
        for a in allin:
            if a in cuts and cur:
                segs.append((cur, start)); cur, start = [], a
            cur.append(a)
        segs.append((cur, start))
        return '\n'.join(self.segment(s, st) for s, st in segs)

    def asm(self):
        out = ['%s:' % self.name(self.apu)]
        targets = set()
        for a in self.addrs:
            mn, op, _ = self.ins[a]
            for w in re.findall(r'spc_[0-9A-F]{4}_[0-9A-F]{6}', op):
                targets.add(self.labels[w])
        for a in self.addrs:
            mn, op, _ = self.ins[a]
            if a in targets and a != self.apu:
                out.append('.l%04X:' % a)
            def lab(m):
                t = self.labels[m.group(0)]
                if t in self.inside:
                    return '.l%04X' % t if t != self.apu else self.name(self.apu)
                return self.name(t)
            op2 = re.sub(r'spc_[0-9A-F]{4}_[0-9A-F]{6}', lab, op)
            if mn in ('call', 'jmp') and re.fullmatch(r'\$[0-9A-F]{4}', op2):
                op2 = self.name(hx(op2))
            out.append('    %s%s' % (mn, ' ' + op2 if op2 else ''))
        return '\n'.join(out) + '\n'


routines_all = {}
splits = set()


def find_splits(ins, labels, rts):
    """Addresses a branch or jump reaches from outside the routine that holds them."""
    owner = {}
    for apu, (lo, hi) in rts.items():
        for a, v in ins.items():
            if lo <= v[2] < hi:
                owner[a] = apu
    out = set()
    for a, (mn, op, rom) in ins.items():
        for w in re.findall(r'spc_[0-9A-F]{4}_[0-9A-F]{6}', op):
            tg = labels[w]
            if owner.get(tg) is not None and owner.get(tg) != owner.get(a) and tg not in rts:
                out.add(tg)
        m = re.search(r'\$([0-9A-F]{4})$', op)
        if (mn in ('jmp', 'bra', 'beq', 'bne', 'bcc', 'bcs', 'bpl', 'bmi', 'bbs', 'bbc', 'dbnz') or mn.startswith('b')) and m and a in untraced_ins:
            tg = int(m.group(1), 16)
            if owner.get(tg) is not None and owner.get(tg) != owner.get(a) and tg not in rts:
                out.add(tg)
        if mn == 'jmp' and re.fullmatch(r'\$[0-9A-F]{4}', op):
            tg = hx(op)
            if tg in owner and owner[tg] != owner.get(a) and tg not in rts:
                out.add(tg)
    return out

if __name__ == '__main__':
    args = sys.argv[1:]
    mode = 'c'
    if '--asm' in args:
        mode = 'asm'
    names = filed_names()
    apus = []
    for a in args:
        if a.startswith('--'):
            continue
        if '=' in a:
            n, v = a.split('=')
            names[int(v, 16)] = n
            apus.append(int(v, 16))
        else:
            apus.append(int(a, 16))
    ins, labels, untraced = intake()
    untraced, table_targets = add_untraced_code(ins, untraced)
    rts = routines()
    routines_all.update(rts)
    splits.update(find_splits(ins, labels, rts))
    splits.update(x for x in table_targets if x not in rts)
    for apu in apus:
        d = Draft(apu, ins, labels, untraced, rts, names)
        if mode == 'c':
            print(d.c())
        else:
            print(d.asm())


def asm_block(lo, hi, names, ins, labels, rts):
    """The assembly of APU lo..hi: instructions with labels, data as bytes.
    Branch targets are labels named for their address within the routine
    that holds them (Routine_lXXXX outside it)."""
    mem = apu_image()
    owner = {}
    for apu, (r0, r1) in rts.items():
        for a, v in ins.items():
            if r0 <= v[2] < r1:
                owner[a] = apu
    entries = set(names) | set(rts) | splits
    def nm(a):
        if a in names:
            return names[a]
        return ('R%04X' if a in rts else 'P%04X') % a
    targets = set()
    for a, (mn, op, rom) in ins.items():
        if lo <= a < hi:
            for w in re.findall(r'spc_[0-9A-F]{4}_[0-9A-F]{6}', op):
                targets.add(labels[w])
            m = re.search(r'\$([0-9A-F]{4})$', op)
            if m and (mn.startswith('b') or mn in ('jmp', 'call', 'dbnz')):
                targets.add(int(m.group(1), 16))
    cur = [None]
    def ref(t, here):
        if t in entries:
            return nm(t)
        o = owner.get(t)
        # the label's main label: the last entry at or before it
        main = max(e for e in entries if e <= t)
        if main == cur[0]:
            return '.l%04X' % t
        return '%s_l%04X' % (nm(main), t)
    out = []
    a = lo
    while a < hi:
        if a in entries:
            out.append('')
            out.append('%s:' % nm(a))
            cur[0] = a
        elif a in targets:
            out.append('.l%04X:' % a)
        if a in ins:
            mn, op, rom = ins[a]
            def lab(m):
                return ref(labels[m.group(0)], a)
            op2 = re.sub(r'spc_[0-9A-F]{4}_[0-9A-F]{6}', lab, op)
            m = re.search(r'\$([0-9A-F]{4})$', op2)
            if m and (mn.startswith('b') or mn in ('jmp', 'call', 'dbnz')) and not op2.startswith('('):
                op2 = op2[:m.start()] + ref(int(m.group(1), 16), a)
            out.append('    %s%s' % (mn, ' ' + op2 if op2 else ''))
            nxt = [x for x in ins if ins[x][2] > rom]
            n = min(nxt, key=lambda x: ins[x][2]) if nxt else None
            step = (ins[n][2] - rom) if n is not None else 1
            # the next instruction may follow data
            import spc700
            length = spc700.length(mem[a])
            a += length
        else:
            # data up to the next instruction or entry
            b = a
            while b < hi and b not in ins and b not in entries and b not in targets:
                b += 1
            row = []
            for k in range(a, b):
                row.append('$%02X' % mem[k])
                if len(row) == 12:
                    out.append('    db %s' % ','.join(row)); row = []
            if row:
                out.append('    db %s' % ','.join(row))
            a = b
    return '\n'.join(out) + '\n'
