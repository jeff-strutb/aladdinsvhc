#!/usr/bin/env python3
"""objdiff.py: which routines' compiled C differs between two revisions.

    objdiff.py OLD [NEW]        the routines a header change reaches (NEW: HEAD)

A change to a shared header (src/aladdin/include/) changes the compiled code
of only some routines: the ones that use what changed. Each revision's C
files are compiled on their own, one section per function and per datum
(-ffunction-sections -fdata-sections, the hybrid's -O2 -fPIC otherwise), and
each section is compared with its counterpart: its bytes and its relocations,
with each relocation's target named (a symbol, or another section) rather
than placed. A section that differs, or is new, is changed; so is every
section that refers to a changed one, through any chain of references. A
routine is reached when its abi_ entry's section is changed. The checker's
own objects (tools/aladdin/snesbox, which includes machine.h and spc.h) are
compared the same way: when any of them differs, every result is affected.

Only ELF x86-64 objects are read; elsewhere the answer is "everything".
"""
import concurrent.futures, glob, hashlib, os, re, shutil, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
WORK = os.path.join(ROOT, 'build', 'aladdin', 'objdiff')
TREE = os.environ.get('ALADDIN_GATE_TREE', 'src/aladdin').rstrip('/')
BOX = 'tools/aladdin/snesbox'
CORE_INC = os.path.join(ROOT, 'tools', 'toolchains', 'src', 'bsnes', 'bsnes', 'target-libretro')
CFLAGS = ['-O2', '-std=c99', '-fPIC', '-ffunction-sections', '-fdata-sections']
BOXFLAGS = ['-O2', '-ffunction-sections', '-fdata-sections', '-D_GNU_SOURCE']
TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)')

SHT_SYMTAB, SHT_RELA, SHT_NOBITS, SHF_ALLOC, STT_SECTION = 2, 4, 8, 2, 3


def full_rev(rev):
    r = subprocess.run(['git', 'rev-parse', rev], capture_output=True, text=True, cwd=ROOT)
    if r.returncode:
        raise SystemExit('objdiff: no such revision: %s' % rev)
    return r.stdout.strip()


def _compile(job):
    cmd, out = job
    p = subprocess.run(cmd + ['-o', out], capture_output=True, text=True)
    return out if p.returncode == 0 else None


def compile_dir(cdir, out, boxdir=None):
    """Compile every .c under the C tree `cdir` into out/c (and the checker's
    sources in boxdir into out/box). A file that fails to compile has no object."""
    inc = os.path.join(cdir, 'include')
    jobs = []
    for f in sorted(glob.glob(os.path.join(cdir, '**', '*.c'), recursive=True)):
        o = os.path.join(out, 'c', os.path.relpath(f, cdir) + '.o')
        os.makedirs(os.path.dirname(o), exist_ok=True)
        jobs.append((['cc'] + CFLAGS + ['-I', inc, '-c', f], o))
    for f in sorted(glob.glob(os.path.join(boxdir, '*.c'))) if boxdir else []:
        o = os.path.join(out, 'box', os.path.basename(f) + '.o')
        os.makedirs(os.path.dirname(o), exist_ok=True)
        jobs.append((['cc'] + BOXFLAGS + ['-I', CORE_INC, '-I', inc, '-c', f], o))
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
        list(ex.map(_compile, jobs))


def build(rev):
    """Compile the C tree and the checker at `rev` into WORK/<rev>, once.
    Returns that directory (the source is in its src/)."""
    rev = full_rev(rev)
    d = os.path.join(WORK, rev)
    if os.path.exists(os.path.join(d, 'done')):
        return d
    shutil.rmtree(d, ignore_errors=True)
    src = os.path.join(d, 'src')
    os.makedirs(src)
    arc = subprocess.run(['git', 'archive', rev, TREE, BOX], capture_output=True, cwd=ROOT).stdout
    subprocess.run(['tar', '-x', '-C', src], input=arc, check=True)
    compile_dir(os.path.join(src, TREE), d, os.path.join(src, BOX))
    open(os.path.join(d, 'done'), 'w').close()
    return d


# Held C (src/aladdin/held.txt): routines whose C failed a gate, listed by
# address, one per line ("BB:AAAA Name  # why"). Their C still compiles, but
# the hybrid leaves them out of its routine table, so the original code runs
# in their place; and with them every routine whose C calls a held one's C
# (directly or through other functions), since that C would run the held C.
HELD = 'held.txt'


def held_list(cdir):
    """Addresses (BBAAAA) listed in cdir/held.txt."""
    path = os.path.join(cdir, HELD)
    out = set()
    if os.path.exists(path):
        for l in open(path):
            w = l.split('#')[0].split()
            if w:
                out.add(w[0].replace(':', '').replace('$', '').upper().zfill(6))
    return out


def tag_names(cdir):
    """Routine name -> address for the tags in the .c files under cdir."""
    out = {}
    for f in glob.glob(os.path.join(cdir, '**', '*.c'), recursive=True):
        for m in TAG.finditer(open(f).read()):
            out[m.group(3)] = '%02X%s' % (int(m.group(1), 16), m.group(2).upper())
    return out


def dropped_from(cdir, objdir):
    """Names of the routines the hybrid leaves out: the held ones and every
    routine whose abi_ entry reaches a held one's through its references.
    objdir holds cdir's objects (compile_dir)."""
    held = held_list(cdir)
    names = tag_names(cdir)
    want = {n for n, a in names.items() if a in held}
    if not want:
        return set()
    g, _ = graph(objdir, 'c')
    seeds = {k for k in g if k[1] in {'.text.abi_' + n for n in want}}
    back = {}
    for k, (h, refs) in g.items():
        for r in refs:
            back.setdefault(r, set()).add(k)
    seen, todo = set(seeds), list(seeds)
    while todo:
        for k in back.get(todo.pop(), ()):
            if k not in seen:
                seen.add(k); todo.append(k)
    return want | {s[len('.text.abi_'):] for o, s in seen if s.startswith('.text.abi_')}


def dropped(cdir, objdir=None):
    """dropped_from for a C tree, compiling it into objdir (a fresh folder
    under WORK when not given) only when something is held."""
    if not held_list(cdir):
        return set()
    if objdir is None:
        objdir = os.path.join(WORK, 'tree-%d' % os.getpid())
    shutil.rmtree(objdir, ignore_errors=True)
    compile_dir(cdir, objdir)
    try:
        return dropped_from(cdir, objdir)
    finally:
        shutil.rmtree(objdir, ignore_errors=True)


def dropped_at(rev):
    """Addresses the hybrid leaves out at committed revision `rev`."""
    d = build(rev)
    cdir = os.path.join(d, 'src', TREE)
    names = tag_names(cdir)
    return {names[n] for n in dropped_from(cdir, d) if n in names}


def read_elf(path):
    """{section name: (bytes or size, [(offset, type, addend, target)])} and
    {defined global symbol: section name}; a target is ('sec', name, value)
    for a section or a symbol defined here, ('sym', name) for one that isn't."""
    data = open(path, 'rb').read()
    if data[:4] != b'\x7fELF' or data[4] != 2 or data[5] != 1 or struct.unpack_from('<H', data, 18)[0] != 62:
        raise ValueError('not an ELF64 x86-64 object')
    shoff, = struct.unpack_from('<Q', data, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', data, 0x3A)
    sh = [struct.unpack_from('<IIQQQQIIQQ', data, shoff + i * shentsize) for i in range(shnum)]
    def strz(off):
        return data[off:data.index(b'\0', off)].decode()
    names = [strz(sh[shstrndx][4] + s[0]) for s in sh]
    syms = []
    for i, s in enumerate(sh):
        if s[1] == SHT_SYMTAB:
            stro = sh[s[6]][4]
            for k in range(s[5] // 24):
                nm, info, other, shndx, value, size = struct.unpack_from('<IBBHQQ', data, s[4] + k * 24)
                syms.append((strz(stro + nm), info >> 4, info & 15, shndx, value))
    globs = {}
    for nm, bind, typ, shndx, value in syms:
        if bind != 0 and 0 < shndx < 0xFF00:
            globs[nm] = names[shndx]
    secs = {}
    for i, s in enumerate(sh):
        if s[2] & SHF_ALLOC and names[i] != '.eh_frame' and not names[i].startswith('.note'):
            body = s[5] if s[1] == SHT_NOBITS else data[s[4]:s[4] + s[5]]
            secs[names[i]] = [body, []]
    for i, s in enumerate(sh):
        if s[1] == SHT_RELA and names[s[7]] in secs:
            rel = secs[names[s[7]]][1]
            for k in range(s[5] // 24):
                off, info, add = struct.unpack_from('<QQq', data, s[4] + k * 24)
                nm, bind, typ, shndx, value = syms[info >> 32]
                if typ == STT_SECTION or (bind == 0 and 0 < shndx < 0xFF00):
                    tgt = ('sec', names[shndx], value)
                elif 0 < shndx < 0xFF00:
                    tgt = ('sym', nm)
                else:
                    tgt = ('sym', nm)
                rel.append((off, info & 0xFFFFFFFF, add, tgt))
    return secs, globs


def graph(d, part):
    """Every section of the objects under d/part: {key: (hash, {keys it refers to})},
    a key being (object, section) or ('ext', symbol) for a symbol no object defines."""
    objs = {}
    for o in sorted(glob.glob(os.path.join(d, part, '**', '*.o'), recursive=True)):
        objs[os.path.relpath(o, os.path.join(d, part))] = read_elf(o)
    where = {}
    for obj, (secs, globs) in objs.items():
        for nm, sec in globs.items():
            where[nm] = (obj, sec)
    out = {}
    for obj, (secs, globs) in objs.items():
        for sec, (body, rels) in secs.items():
            h, refs = hashlib.sha1(), set()
            h.update(repr(body).encode() if isinstance(body, int) else body)
            for off, typ, add, tgt in sorted(rels):
                key = (obj, tgt[1]) if tgt[0] == 'sec' else where.get(tgt[1], ('ext', tgt[1]))
                refs.add(key)
                h.update(repr((off, typ, add, key, tgt[2] if tgt[0] == 'sec' else 0)).encode())
            out[(obj, sec)] = (h.hexdigest(), refs)
    return out, objs


def changed(old, new):
    """Keys of new's sections that differ from old's, or refer to one that does."""
    bad = {k for k, (h, refs) in new.items() if old.get(k, (None,))[0] != h}
    back = {}
    for k, (h, refs) in new.items():
        for r in refs:
            back.setdefault(r, set()).add(k)
    todo = list(bad)
    while todo:
        for k in back.get(todo.pop(), ()):
            if k not in bad:
                bad.add(k); todo.append(k)
    return bad


def tags(d):
    """Routine name -> address (BBAAAA) for every @routine tag in the C at d (held files too)."""
    out = {}
    for f in glob.glob(os.path.join(d, 'src', TREE, '**', '*.c*'), recursive=True):
        if f.endswith(('.c', '.c.held')):
            for m in TAG.finditer(open(f).read()):
                out[m.group(3)] = '%02X%s' % (int(m.group(1), 16), m.group(2).upper())
    return out


def affected(old_rev, new_rev='HEAD'):
    """(routine addresses whose compiled C differs, checker changed?). The
    checker counts as changed (everything affected) when objects cannot be
    read or a file compiles at one revision only."""
    if sys.platform != 'linux':
        return set(), True
    od, nd = build(old_rev), build(new_rev)
    try:
        (oc, oobjs), (nc, nobjs) = graph(od, 'c'), graph(nd, 'c')
        (ob, _), (nb, _) = graph(od, 'box'), graph(nd, 'box')
    except (ValueError, struct.error):
        return set(), True
    src = lambda d, part, ext: {os.path.relpath(f, os.path.join(d, 'src', part)) + '.o'
                                for f in glob.glob(os.path.join(d, 'src', part, '**', ext), recursive=True)}
    # A file that did not compile has no object: it cannot be compared.
    if src(od, TREE, '*.c') != set(oobjs) or src(nd, TREE, '*.c') != set(nobjs):
        return set(), True
    if changed(ob, nb) or set(ob) != set(nb):
        return set(), True
    bad = changed(oc, nc)
    # A section of old's that is gone: whatever referred to it in new refers
    # to something else now, so its own hash differs and it is in bad.
    names = {sec[len('.text.abi_'):] for obj, sec in bad if sec.startswith('.text.abi_')}
    # An abi_ entry compiled at one revision only (filed, held or released) has changed too.
    entry = lambda g: {s[len('.text.abi_'):] for (o, s) in g if s.startswith('.text.abi_')}
    names |= entry(oc) ^ entry(nc)
    nt, ot = tags(nd), tags(od)
    return {t[n] for n in names for t in (nt, ot) if n in t}, False


if __name__ == '__main__':
    if not 2 <= len(sys.argv) <= 3:
        print(__doc__); sys.exit(2)
    got, box = affected(*sys.argv[1:])
    if box:
        print('the checker (or an object that could not be compared) changed: every result is affected')
    else:
        print('%d routine(s): %s' % (len(got), ' '.join(sorted(got))))
    sys.exit(0)
