#!/usr/bin/env python3
"""cdraft.py: draft a routine's C from its traced assembly.

    cdraft.py ADDR [ADDR...]            print a draft for each routine
    cdraft.py --name ADDR=Name ...      give drafts their names (default: the C's, else R<addr>)
    cdraft.py --proof DIR               draft every routine that has C into DIR (see below)

A draft is the routine's instructions, the ones the scripts executed, each
as its step in ops65.h at the width it ran with: behaviour exact as it
stands, so the gates judge it before anyone names anything. Branches are
labels; calls are calls to the C of the routine called (call_routine when
there is none yet), with the frame a call that can wait needs; jumps to
another routine's entry are tail calls; a BRK in actor code (@actor) is
actor_yield; a jump into the actor loop is actor_leave. Anything the
drafter cannot say exactly gets a TODO, which gate 0 refuses: untraced
instructions, an instruction that ran at both widths with different
lengths, interrupt returns, mode switches.

A draft is intake: read it, name the addresses and the routine, write
its header (WHAT IT DOES, HOW, Entry, Exit), and file it with its
assembly as any routine (docs/aladdin/DECOMP.md).

--proof DIR: the drafter's own check. Copies src/aladdin to DIR, renames
each hand-written abi_ function that has a draft without a TODO, and adds
drafts.c with the drafts in their place, so the gates can judge the drafts
(ALADDIN_GATE_TREE=DIR, once DIR is committed).
"""
import argparse, csv, glob, os, re, shutil, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import codemap  # noqa: E402
from disasm import OPS, FIXED, M8, M16, X8, X16  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'src', 'aladdin')
TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)')
CPU_OP = 1

# Calls whose arguments follow the call in the code (TaskCreate, TaskExec):
# they read them through the return address, so a call to them pushes the
# JSL's frame (jsl_frame), and TaskCreate returns past them (cpu.unwound).
# TaskExec never returns: the task starts again at its new entry.
INLINE = {0x81BCB8: 5, 0x81BCF6: 3}
NORETURN = {0x81BCF6}
TASK_WAIT = 0x81BD31
# The actor loop, where actor code that ends jumps (docs/aladdin/ACTORS.md).
ACTOR_LOOP = {0x8394BF, 0x83948B}

A_OPS = {'lda', 'sta', 'adc', 'sbc', 'and', 'ora', 'eor', 'cmp', 'bit', 'stz', 'inc', 'dec',
         'asl', 'lsr', 'rol', 'ror', 'tsb', 'trb', 'pha', 'pla', 'txa', 'tya'}
X_OPS = {'ldx', 'ldy', 'stx', 'sty', 'cpx', 'cpy', 'inx', 'iny', 'dex', 'dey', 'phx', 'phy',
         'plx', 'ply', 'tax', 'tay', 'txy', 'tyx', 'tsx'}
BRANCH = {'bpl': '!IF_N', 'bmi': 'IF_N', 'bvc': '!IF_V', 'bvs': 'IF_V',
          'bcc': '!IF_C', 'bcs': 'IF_C', 'bne': '!IF_Z', 'beq': 'IF_Z'}


class Known:
    """The routines that have C (their names, @actor, whether they can
    wait) and the catalog's entries."""

    def __init__(self, src=SRC):
        self.name, self.actor, self.waits, self.text = {}, set(), set(), {}
        for f in sorted(glob.glob(os.path.join(src, '**', '*.c'), recursive=True)):
            t = open(f).read()
            for m in TAG.finditer(t):
                a = int(m.group(1) + m.group(2), 16)
                self.name[a] = m.group(3)
                head = t[m.end():t.find('*/', m.end())]
                if re.search(r'@actor\b', head):
                    self.actor.add(a)
                body = function_body(t, 'abi_' + m.group(3))
                self.text[a] = body
                if body and re.search(r'\b(task_yield|jsl_frame|abi_TaskWait)\b', body):
                    self.waits.add(a)
        self.starts = set(self.name)
        cat = os.path.join(ROOT, 'build', 'aladdin', 'routines.csv')
        if os.path.exists(cat):
            for r in csv.DictReader(open(cat)):
                if r['cpu'] == 'cpu':
                    self.starts.add(int(r['addr'], 16))


def header_names():
    """Exact addresses the C headers name (#define NAME 0x...u): bank-0 work
    RAM and hardware registers, and long addresses."""
    names = {}
    for f in sorted(glob.glob(os.path.join(SRC, 'include', '*.h'))):
        for m in re.finditer(r'^#define\s+([A-Z_][A-Z0-9_]*)\s+0x([0-9A-Fa-f]{6})u\b', open(f).read(), re.M):
            names.setdefault(int(m.group(2), 16), (m.group(1), os.path.basename(f)))
    return names


def function_body(text, fn):
    m = re.search(r'\bvoid\s+%s\s*\(\s*void\s*\)\s*\{' % re.escape(fn), text)
    if not m:
        return None
    depth, i = 1, m.end()
    while depth and i < len(text):
        depth += {'{': 1, '}': -1}.get(text[i], 0)
        i += 1
    return text[m.start():i]


class Rom:
    def __init__(self):
        self.rom = open(os.path.join(ROOT, 'build', 'aladdin', 'baserom.sfc'), 'rb').read()
        self.mp = codemap.merged()

    def at(self, addr):
        o = codemap.rom_offset(addr)
        return o

    def byte(self, addr):
        return self.rom[self.at(addr)]


def width(flags, b8, b16):
    return {(True, False): 8, (False, True): 16}.get((bool(flags & b8), bool(flags & b16)), 0)


class Insn:
    pass


def decode(rom, addr):
    """The instruction at addr as the scripts ran it, or None (untraced)."""
    o = rom.at(addr)
    if o is None or not rom.mp[o] & CPU_OP:
        return None
    i = Insn()
    i.addr, i.op = addr, rom.rom[o]
    i.mn, i.mode = OPS[i.op]
    f = rom.mp[o]
    i.m, i.x = width(f, M8, M16), width(f, X8, X16)
    n = FIXED.get(i.mode)
    if i.mode == 'immm':
        n = {8: 1, 16: 2}.get(i.m)
    elif i.mode == 'immx':
        n = {8: 1, 16: 2}.get(i.x)
    if n is None:
        i.len, i.v = 1, None
        i.bad = 'an immediate that ran at both widths'
        return i
    i.bad = None
    i.len = 1 + n
    b = rom.rom[o + 1:o + 1 + n]
    i.v = int.from_bytes(b, 'little') if n else None
    i.next = (addr & 0xFF0000) | ((addr + i.len) & 0xFFFF)
    if i.mode == 'rel':
        i.t = (addr & 0xFF0000) | ((addr + 2 + (i.v - 256 if i.v & 0x80 else i.v)) & 0xFFFF)
    elif i.mode == 'rell':
        i.t = (addr & 0xFF0000) | ((addr + 3 + i.v) & 0xFFFF)
    elif i.mn in ('jmp', 'jsr') and i.mode == 'abs':
        i.t = (addr & 0xFF0000) | i.v
    elif i.mn in ('jml', 'jsl') and i.mode == 'long':
        i.t = i.v
    else:
        i.t = None
    return i


NAMES = {}


def named(v, bank0):
    n = NAMES.get(v if not bank0 else v & 0xFFFF)
    return n[0] if n else None


def ea(i):
    """The C for an instruction's effective address (by name where a header
    names that exact address)."""
    v, mode = i.v, i.mode
    if mode in ('abs', 'absx', 'absy') and v < 0x8000 and named(v, True):
        return 'ea_%s(%s)' % (mode, named(v, True))
    if mode in ('long', 'longx') and named(v, False):
        return named(v, False) if mode == 'long' else 'ea_longx(%s)' % named(v, False)
    return {
        'dp': 'ea_dp(0x%02X)', 'dpx': 'ea_dpx(0x%02X)', 'dpy': 'ea_dpy(0x%02X)',
        'abs': 'ea_abs(0x%04X)', 'absx': 'ea_absx(0x%04X)', 'absy': 'ea_absy(0x%04X)',
        'long': '0x%06Xu', 'longx': 'ea_longx(0x%06Xu)',
        'dpi': 'ea_dpi(0x%02X)', 'dpiy': 'ea_dpiy(0x%02X)', 'dpxi': 'ea_dpxi(0x%02X)',
        'dpil': 'ea_dpil(0x%02X)', 'dpily': 'ea_dpily(0x%02X)',
        'sr': 'ea_sr(0x%02X)', 'sriy': 'ea_sriy(0x%02X)',
    }[mode] % v


def value(i, w):
    if i.mode in ('immm', 'immx', 'imm'):
        return '0x%0*X' % (2 if w == 8 else 4, i.v)
    return 'rd%d(%s)' % (w, ea(i))


def at_width(i, which, gen):
    """gen(width) for the width the instruction ran with: both, by the flag."""
    w = i.m if which == 'm' else i.x
    if w:
        return gen(w)
    flag = 'P_M' if which == 'm' else 'P_X'
    return 'if (cpu.p & %s) { %s } else { %s }' % (flag, gen(8), gen(16))


class Draft:
    def __init__(self, rom, known, addr, name=None):
        self.rom, self.k, self.addr = rom, known, addr
        self.name = name or known.name.get(addr) or 'R%06X' % addr
        self.pb = addr & 0xFF0000
        self.actor = addr in known.actor
        self.todo = []
        self.calls = set()

    def callee(self, t):
        return self.k.name.get(t)

    def external(self, t):
        return t != self.addr and t in self.k.starts

    def call(self, i, t, long):
        """A JSR or JSL to t. A routine that can wait gets the frame its
        call pushes, as its task's stack holds it while it waits: TaskWait
        itself pulls it (its RTL, before the task resumes); for any other,
        its return does, and the C's call pushes and pulls it here."""
        n = self.callee(t)
        out = []
        frame = n and (t in self.k.waits or t in INLINE)
        if frame:
            if long:
                out.append('jsl_frame(0x%06Xu);' % i.addr)
            else:
                out.append('push16(0x%04X);' % ((i.addr + 2) & 0xFFFF))
        if n:
            self.calls.add(n)
            out.append('abi_%s();' % n)
        else:
            out.append('call_routine(0x%06Xu, %d);' % (t, 1 if long else 0))
        if t in INLINE and t not in NORETURN:
            out.append('cpu.unwound = 0;')
        elif t in NORETURN:
            out.append('return;')
        elif frame and t != TASK_WAIT:
            out.append('cpu.s = (uint16_t)(cpu.s + %d);' % (3 if long else 2))
        return ' '.join(out)

    def tail(self, t):
        """Leave for t: a tail call to its C, or the actor loop."""
        if t in ACTOR_LOOP:
            return 'actor_leave(0x%06Xu); return;' % t
        n = self.callee(t)
        if n:
            self.calls.add(n)
            return 'abi_%s(); return;' % n
        return 'jump_routine(0x%06Xu); return;' % t

    def walk(self):
        seen, todo, labels = {}, [self.addr], set()
        while todo:
            a = todo.pop()
            while a not in seen:
                if a != self.addr and a in self.k.starts:
                    break  # another routine's entry: reached by a tail
                i = decode(self.rom, a)
                if i is None:
                    seen[a] = None
                    break
                seen[a] = i
                if i.bad:
                    break
                mn = i.mn
                if mn in BRANCH or mn in ('bra', 'brl'):
                    if not self.external(i.t):
                        labels.add(i.t)
                        todo.append(i.t)
                    if mn in ('bra', 'brl'):
                        break
                elif mn in ('jmp', 'jml') and i.t is not None:
                    if not self.external(i.t) and i.t not in ACTOR_LOOP \
                            and self.rom.at(i.t) is not None and decode(self.rom, i.t) is not None:
                        labels.add(i.t)
                        todo.append(i.t)
                    break
                elif mn in ('jmp', 'jml', 'rts', 'rtl', 'rti', 'stp', 'xce', 'wai', 'cop'):
                    break
                elif mn in ('jsr', 'jsl') and i.t in NORETURN:
                    break
                elif mn in ('jsr', 'jsl') and i.t in INLINE:
                    a = (i.next & 0xFF0000) | ((i.next + INLINE[i.t]) & 0xFFFF)
                    continue
                elif mn == 'brk':
                    if not self.actor:
                        break
                a = i.next
        self.seen, self.labels = seen, labels

    PUSH = {'pha': 'm', 'phx': 'x', 'phy': 'x', 'php': 1, 'phb': 1, 'phd': 2, 'phk': 1, 'pea': 2, 'pei': 2, 'per': 2}
    PULL = {'pla': 'm', 'plx': 'x', 'ply': 'x', 'plp': 1, 'plb': 1, 'pld': 2}

    def stack_check(self):
        """The routine's own stack: a path that pulls more than it pushed
        (it takes from its caller's frame: its return address) or sets S
        cannot be drafted as a C function that returns."""
        depth, todo = {self.addr: 0}, [self.addr]
        while todo:
            a = todo.pop()
            i = self.seen.get(a)
            if i is None or i.bad:
                continue
            d = depth[a]
            if i.mn in ('tcs', 'txs'):
                self.flag(i, 'sets S')
                continue
            if i.mode in ('sr', 'sriy') and i.v > d:
                self.flag(i, "reaches its caller's frame (stack offset %d, %d bytes its own)" % (i.v, d))
                continue
            n = self.PUSH.get(i.mn) or self.PULL.get(i.mn)
            if n in ('m', 'x'):
                w = i.m if n == 'm' else i.x
                if not w:
                    self.flag(i, 'pushes or pulls at both widths')
                    continue
                n = w // 8
            if i.mn in self.PUSH:
                d += n
            elif i.mn in self.PULL:
                d -= n
                if d < 0:
                    self.flag(i, "pulls from its caller's frame")
                    continue
            if i.mn in ('rts', 'rtl') and d != 0:
                self.flag(i, 'returns with %d bytes of its own on the stack' % d)
                continue
            nexts = []
            if i.mn in BRANCH or i.mn in ('bra', 'brl', 'jmp', 'jml'):
                if i.t in self.seen:
                    nexts.append(i.t)
            if i.mn not in ('bra', 'brl', 'jmp', 'jml', 'rts', 'rtl', 'rti', 'stp'):
                n2 = i.next
                if i.mn in ('jsr', 'jsl') and i.t in INLINE:
                    n2 = (i.next & 0xFF0000) | ((i.next + INLINE[i.t]) & 0xFFFF)
                if n2 in self.seen:
                    nexts.append(n2)
            elif i.mn in ('bra', 'brl', 'jmp', 'jml') and i.t not in self.seen and d != 0:
                self.flag(i, 'leaves with %d bytes of its own on the stack' % d)
            for t in nexts:
                if t in depth:
                    if depth[t] != d:
                        self.flag(self.seen[t], 'reached with two stack depths')
                    continue
                depth[t] = d
                todo.append(t)

    def step(self, i):
        mn, mode = i.mn, i.mode
        self.pb = i.addr & 0xFF0000
        if mn in BRANCH:
            t = i.t
            if self.external(t):
                return 'if (%s) { %s }' % (BRANCH[mn], self.tail(t))
            return 'if (%s) goto L_%06X;' % (BRANCH[mn], t)
        if mn in ('bra', 'brl'):
            return self.tail(i.t) if self.external(i.t) else 'goto L_%06X;' % i.t
        if mn in ('jmp', 'jml'):
            if mode in ('abs', 'long'):
                if i.t in self.labels:
                    return 'goto L_%06X;' % i.t
                return self.tail(i.t)
            if mode == 'absi':
                return 'jump_routine(0x%06Xu | rd16(0x%04Xu)); return;' % (self.pb, i.v)
            if mode == 'absxi':
                return 'jump_routine(0x%06Xu | rd16(0x%06Xu | (uint16_t)(0x%04X + cpu.x))); return;' % (self.pb, self.pb, i.v)
            if mode == 'absil':
                return 'jump_routine(rd24(0x%04Xu)); return;' % i.v
        if mn == 'jsr':
            if mode == 'abs':
                return self.call(i, i.t, False)
            return 'call_routine(0x%06Xu | rd16(0x%06Xu | (uint16_t)(0x%04X + cpu.x)), 0);' % (self.pb, self.pb, i.v)
        if mn == 'jsl':
            return self.call(i, i.t, True)
        if mn in ('rts', 'rtl'):
            return 'return;'
        if mn == 'brk':
            if self.actor:
                return 'actor_yield(0x%06Xu);' % i.addr
            return self.flag(i, 'BRK outside actor code')
        if mn in ('rti', 'stp', 'xce', 'wai', 'cop'):
            return self.flag(i, mn.upper())
        if mn in ('nop', 'wdm'):
            return ';'
        if mn in ('clc', 'sec', 'cli', 'sei', 'cld', 'sed', 'clv', 'php', 'plp', 'phb', 'plb', 'phd', 'pld',
                  'tcd', 'tdc', 'tcs', 'tsc', 'txs', 'xba'):
            return '%s();' % mn
        if mn == 'sep':
            return 'op_sep(0x%02X);' % i.v
        if mn == 'rep':
            return 'op_rep(0x%02X);' % i.v
        if mn == 'phk':
            return 'phk(0x%02X);' % (self.pb >> 16)
        if mn == 'pea':
            return 'pea(0x%04X);' % i.v
        if mn == 'pei':
            return 'push16(rd16(ea_dp(0x%02X)));' % i.v
        if mn == 'per':
            return 'pea(0x%04X);' % ((i.addr + 3 + i.v) & 0xFFFF)
        if mn in ('mvn', 'mvp'):
            return '%s(0x%02X, 0x%02X);' % (mn, i.v & 0xFF, i.v >> 8)
        which = 'm' if mn in A_OPS else 'x' if mn in X_OPS else None
        if mn in ('lda', 'ldx', 'ldy'):
            if mode in ('immm', 'immx'):
                return at_width(i, which, lambda w: '%s%d_v(%s);' % (mn, w, value(i, w)))
            return at_width(i, which, lambda w: '%s%d(%s);' % (mn, w, ea(i)))
        if mn in ('sta', 'stx', 'sty', 'stz'):
            return at_width(i, which, lambda w: '%s%d(%s);' % (mn, w, ea(i)))
        if mn in ('adc', 'sbc', 'and', 'ora', 'eor', 'cmp', 'cpx', 'cpy'):
            return at_width(i, which, lambda w: '%s%d_v(%s);' % (mn, w, value(i, w)))
        if mn == 'bit':
            if mode == 'immm':
                return at_width(i, 'm', lambda w: 'bit%d_i(%s);' % (w, value(i, w)))
            return at_width(i, 'm', lambda w: 'bit%d_v(%s);' % (w, value(i, w)))
        if mn in ('inc', 'dec', 'asl', 'lsr', 'rol', 'ror'):
            if mode == 'acc':
                return at_width(i, 'm', lambda w: 'ACC%d(%s);' % (w, mn))
            return at_width(i, 'm', lambda w: 'MEM%d(%s, %s);' % (w, mn, ea(i)))
        if mn in ('tsb', 'trb'):
            return at_width(i, 'm', lambda w: '%s%d(%s);' % (mn, w, ea(i)))
        if mn in ('inx', 'iny', 'dex', 'dey', 'tax', 'tay', 'txy', 'tyx', 'tsx', 'phx', 'phy', 'plx', 'ply'):
            return at_width(i, 'x', lambda w: '%s%d();' % (mn, w))
        if mn in ('txa', 'tya', 'pha', 'pla'):
            return at_width(i, 'm', lambda w: '%s%d();' % (mn, w))
        return self.flag(i, 'no step for %s %s' % (mn, mode))

    def flag(self, i, why):
        self.todo.append('%06X %s' % (i.addr, why))
        return '/* TODO %06X: %s */' % (i.addr, why)

    def text(self):
        self.walk()
        self.stack_check()
        addrs = sorted(a for a in self.seen)
        out = ['void abi_%s(void)' % self.name, '{']
        if addrs and addrs[0] != self.addr:
            # code below the entry (a branch back into another routine's
            # middle) comes first in address order: start at the entry
            out.append('    goto L_%06X;' % self.addr)
            self.labels.add(self.addr)
        prev_next = None
        for a in addrs:
            i = self.seen[a]
            if prev_next is not None and prev_next != a:
                # the code before this fell through to an address not next in order
                out.append('    goto L_%06X;' % prev_next)
                self.labels.add(prev_next)
            if a in self.labels or a == self.addr and self.labels and a in self.labels:
                out.append('L_%06X:' % a)
            if i is None:
                out.append('    ' + self.flag(type('I', (), {'addr': a})(), 'untraced code'))
                prev_next = None
                continue
            if i.bad:
                out.append('    ' + self.flag(i, i.bad))
                prev_next = None
                continue
            out.append('    %s  /* %06X %s */' % (self.step(i), a, i.mn))
            ends = i.mn in ('bra', 'brl', 'jmp', 'jml', 'rts', 'rtl', 'rti', 'stp')
            if i.mn in ('jsr', 'jsl') and i.t in NORETURN:
                prev_next = None
            elif i.mn in ('jsr', 'jsl') and i.t in INLINE:
                prev_next = (i.next & 0xFF0000) | ((i.next + INLINE[i.t]) & 0xFFFF)
            elif i.mn == 'brk' and not self.actor:
                prev_next = None
            else:
                prev_next = None if ends else i.next
            if prev_next is not None and prev_next not in self.seen:
                # falls into another routine's entry
                out.append('    ' + self.tail(prev_next))
                prev_next = None
        # labels the walk made that no instruction got (gotos past the end)
        missing = sorted(l for l in self.labels if l not in self.seen)
        for l in missing:
            out.append('L_%06X:' % l)
            out.append('    ' + self.tail(l))
        out.append('}')
        return '\n'.join(out) + '\n'


def externs(drafts, defined):
    names = sorted({n for d in drafts for n in d.calls} - defined)
    return ''.join('void abi_%s(void);\n' % n for n in names)


FILE_HEAD = '''/* Drafted by tools/aladdin/cdraft.py from the traced assembly. */
#include "machine.h"
#include "ops65.h"
%s
'''


def head(drafts_text):
    used = sorted({NAMES[a][1] for a in NAMES if NAMES[a][0] in drafts_text})
    return FILE_HEAD % ''.join('#include "%s"\n' % h for h in used)


def proof(dirn):
    known, rom = Known(), Rom()
    dst = os.path.join(dirn, 'src', 'aladdin')
    shutil.rmtree(os.path.join(dirn, 'src'), ignore_errors=True)
    shutil.copytree(SRC, dst)
    drafts, kept = [], []
    for a in sorted(known.name):
        d = Draft(rom, known, a)
        t = d.text()
        if d.todo:
            kept.append((a, d.name, d.todo))
            continue
        drafts.append((d, t))
    names = {d.name for d, t in drafts}
    for f in glob.glob(os.path.join(dst, '**', '*.c'), recursive=True):
        s = open(f).read()
        o = s
        moved = []
        for n in names:
            s, k = re.subn(r'\bvoid\s+abi_%s\s*\(\s*void\s*\)\s*\{' % n, 'void abi_%s_hand(void)\n{' % n, s)
            if k:
                moved.append(n)
        if moved:
            # the drafts in drafts.c stand for them (and the build finds each
            # tag's function declared in its file)
            s = ''.join('void abi_%s(void);\n' % n for n in moved) + s
            open(f, 'w').write(s)
    with open(os.path.join(dst, 'drafts.c'), 'w') as f:
        f.write(head(''.join(t for d, t in drafts)))
        f.write(externs([d for d, t in drafts], set()))
        f.write('\n')
        for d, t in drafts:
            f.write(t + '\n')
    print('cdraft: %d routines drafted in place of their hand C, %d kept (a TODO):' % (len(drafts), len(kept)))
    for a, n, why in kept:
        print('  %06X %s: %s' % (a, n, '; '.join(why[:3])))
    return 0


def main(argv):
    NAMES.update(header_names())
    ap = argparse.ArgumentParser()
    ap.add_argument('addrs', nargs='*')
    ap.add_argument('--name', action='append', default=[])
    ap.add_argument('--proof')
    a = ap.parse_args(argv)
    if a.proof:
        return proof(a.proof)
    known, rom = Known(), Rom()
    names = dict(x.split('=') for x in a.name)
    drafts = []
    for s in a.addrs:
        addr = int(s.replace(':', '').replace('$', ''), 16)
        d = Draft(rom, known, addr, names.get(s))
        drafts.append((d, d.text()))
    sys.stdout.write(head(''.join(t for d, t in drafts)) + externs([d for d, t in drafts], {d.name for d, t in drafts}) + '\n')
    for d, t in drafts:
        sys.stdout.write(t + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
