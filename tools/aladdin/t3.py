#!/usr/bin/env python3
"""t3.py: the T3 gates for the C decomp.

    t3.py --live [ADDR...]      gate A5: every script, every call of each routine
                                 checked against its C (all routines when none named)
    t3.py --image               gate A7: every routine's C replaces it at once, in
                                 lockstep with the original, every script, every frame
    t3.py --qualify ADDR...     every gate for each routine, and the tier it reaches
    t3.py --qualify --run ADDR...
                                 certify these routines alone: A5 on their calls only,
                                 A7 on only the scripts that reach them (each run
                                 again unless its result holds), their ledger rows
                                 only; then --qualify
    --full                      with --live or --image: run everything; by default
                                 only what changed since its last result runs

A routine passes A7 when every script that reaches it (its trace entered
the routine, the script has no trace, or it has a noreturn unit call of it)
is IDENTICAL in lockstep on committed source with the C's last change.

Gates for T3: 0 (the C says what it does, how, its entry and exit, and has
no unfinished markers), C (every byte of the routine executed by the
scripts, or, for a table inside it, read), A5 and A7 (IDENTICAL, measured on
committed source that includes the C's last change). T4 adds: the routine's
assembly is filed with the same header, its operands are symbolic, and the
whole ROM rebuilds byte-exact. Symbolic means no hexadecimal literal outside
an immediate: an address is a name, and a place inside a named table or
record is the name plus a decimal index (!oam_tile+4*5, !scroll_hdma+3).

A5 runs snesbox --check over every script in tools/aladdin/scripts/ in
parallel and merges the results per routine:
    IDENTICAL   called, every call matched
    DIVERGENT   at least one call differed with no interrupt or DMA overlapping it
    UNVERIFIED  some differing calls were overlapped (cannot be judged)
    UNCOVERED   never called by any script
Verdicts are written to config/aladdin/t3_live.csv with the date and the
source revision they were measured on. Only IDENTICAL passes.
"""
import atexit, concurrent.futures, csv, datetime, functools, glob, hashlib, json, os, signal, subprocess, sys, threading

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
B = os.path.join(ROOT, 'build', 'aladdin')
SNESBOX = os.path.join(B, 'box', 'snesbox')
LIBNAME = 'libaladdin.dylib' if sys.platform == 'darwin' else 'libaladdin.so'
LIB = os.path.join(B, 'hybrid', LIBNAME)
SCRIPTS = os.path.join(ROOT, 'tools', 'aladdin', 'scripts')


@functools.lru_cache(maxsize=None)
def gate_scripts():
    """The scripts of the revision the gates measure (REV), exported, sorted.
    A gate lists and runs these, never the work tree's: a script landed
    while a gate runs is not part of the commit it measures. (A script's
    state loads name paths from the repository root, where snesbox runs.)"""
    import shutil
    d = os.path.join(WORKD, 'gate_scripts')
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    arc = subprocess.run(['git', 'archive', REV, 'tools/aladdin/scripts'], capture_output=True, cwd=ROOT)
    if arc.returncode:
        raise SystemExit('no scripts at %s' % REV)
    subprocess.run(['tar', '-x', '-C', d], input=arc.stdout, check=True)
    return tuple(sorted(glob.glob(os.path.join(d, 'tools', 'aladdin', 'scripts', '*.box'))))
# The C judged (ALADDIN_GATE_TREE, default the decomp's) and where its records
# and work go (ALADDIN_GATE_OUT, a directory under build/aladdin: another
# tree's records kept apart from the decomp's; default the decomp's own).
TREE = os.environ.get('ALADDIN_GATE_TREE', 'src/aladdin').rstrip('/')
OUTD = os.environ.get('ALADDIN_GATE_OUT')
LIVE = os.path.join(B, OUTD, 't3_live.csv') if OUTD else os.path.join(ROOT, 'config', 'aladdin', 't3_live.csv')
IMAGE = os.path.join(B, OUTD, 'whole_image.csv') if OUTD else os.path.join(ROOT, 'config', 'aladdin', 'whole_image.csv')
WORKD = os.path.join(B, OUTD) if OUTD else B


# A run still going after this is stuck. The longest scripts take about 35
# s alone, but the gate runs ten at a time and other work shares the
# machine: at 120 s two of them timed out under load (load average 40)
# while passing alone. A side really stuck waiting on the other gives up
# by itself after about 30 s (WAIT_UNTIL in check.c). The longest lockstep
# piece, DemoRunThirdRest's two demos (about 6,050 frames), takes 337 s
# alone, so the limit is 420 s.
STUCK = 420
# A per-call check (A5) runs one side only, but the longest script
# (cov_title_idle_long, 18,000 frames) takes 191 s alone and the gate runs
# ten at a time beside the lockstep: its limit is longer, and a run past it
# fails that script instead of the whole check.
A5_STUCK = 1200


# Each snesbox runs as the leader of its own process group: the lockstep
# forks its second side, which a timeout must stop too (killing only the
# process started left the other side running on, with the gate's cores).
# A gate stopped from outside stops the groups it started.
_groups, _groups_lock = set(), threading.Lock()


def _kill_group(pgid):
    try:
        os.killpg(pgid, signal.SIGKILL)
    except (ProcessLookupError, PermissionError):
        pass


def _kill_all_groups(*sig):
    with _groups_lock:
        for g in list(_groups):
            _kill_group(g)
    if sig:
        # stopped from outside: at once, so no worker thread starts another run
        os._exit(128 + sig[0])


atexit.register(_kill_all_groups)
if threading.current_thread() is threading.main_thread():
    signal.signal(signal.SIGTERM, _kill_all_groups)


def run_group(cmd, timeout):
    """subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT,
    timeout=timeout), with cmd and everything it forks in a process group
    of its own that is killed whole when the run ends or times out."""
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=ROOT,
                         start_new_session=True)
    with _groups_lock:
        _groups.add(p.pid)
    try:
        out, err = p.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        _kill_group(p.pid)
        p.communicate()
        raise
    finally:
        _kill_group(p.pid)
        with _groups_lock:
            _groups.discard(p.pid)
    return subprocess.CompletedProcess(cmd, p.returncode, out, err)


def run(cmd, timeout=STUCK):
    p = run_group(cmd, timeout)  # stuck: fails
    if p.returncode:
        sys.stderr.write(p.stdout + p.stderr)
        raise SystemExit('failed: %s' % ' '.join(cmd))
    return p.stdout


def one(script, only):
    name = os.path.splitext(os.path.basename(script))[0]
    d = os.path.join(WORKD, 'check', name)
    os.makedirs(d, exist_ok=True)
    rep = os.path.join(d, 'check.txt')
    if os.path.exists(rep):
        os.remove(rep)
    cmd = [SNESBOX, '--rom', os.path.join(B, 'baserom.sfc'), '--script', script, '--out', d, '--check', LIB]
    if only:
        cmd += ['--only', ','.join(only)]
    try:
        run(cmd, A5_STUCK)
    except subprocess.TimeoutExpired:
        return name, None, ['%s: stopped (no result after %ds): FAILED' % (name, A5_STUCK)]
    rows, notes = [], []
    for line in open(rep):
        if line.startswith(('DIFF', 'INCONCLUSIVE')):
            notes.append('%s: %s' % (name, line.rstrip()))
        elif not line.startswith('routine,') and line.count(',') == 7:
            rows.append(line.rstrip().split(','))
    return name, rows, notes


# Incremental gates. A script's lockstep result depends only on the C of the
# routines it reaches (every other routine's C is never called), the harness
# and the core patches, and the script itself; a routine's per-call result
# only on its C and the harness. So a gate run reruns a script, or a
# routine, only when one of those changed since the revision its last result
# was measured on, and carries the other results forward to the new
# revision. A C file changed, held or released (.c.held) changes the
# routines tagged in it at either revision; a shared header changed changes
# the routines whose compiled code differs (objdiff.py), or everything when
# the checker's own compiled code does. --full reruns everything.
HARNESS = ('tools/aladdin/snesbox/', 'tools/toolchains/patches/', 'tools/aladdin/hybrid.py')
# The revision the gates measure (hybrid.export_head exports the same one).
REV = os.environ.get('ALADDIN_GATE_REV', 'HEAD')


@functools.lru_cache(maxsize=None)
def changed_since(rev):
    """Paths changed from committed source `rev` to REV; None when unknown."""
    if not rev or rev.endswith('+dirty'):
        return None
    r = git('diff', '--name-only', rev, REV)
    return None if r.returncode else frozenset(l for l in r.stdout.splitlines() if l)


@functools.lru_cache(maxsize=None)
def tags_in(rev, path):
    """Routine addresses tagged in the file `path` at committed revision `rev`."""
    r = git('show', '%s:%s' % (rev, path))
    text = r.stdout if r.returncode == 0 else ''
    return frozenset('%02X%s' % (int(m.group(1), 16), m.group(2).upper()) for m in TAG.finditer(text))


@functools.lru_cache(maxsize=None)
def affected_since(rev, harness=True):
    """(routines whose results may differ between `rev` and REV, everything?).
    harness=False leaves out what a harness change (the checker, the core
    patches, hybrid.py) does to every result: the routines the batch itself
    changed, for the runner's critical path (the whole game is checked
    again off it)."""
    paths = changed_since(rev)
    if paths is None:
        return frozenset(), True
    out, headers = set(), False
    for p in paths:
        if p.startswith(HARNESS):
            if harness:
                return frozenset(), True
            continue
        if not p.startswith(TREE + '/'):
            continue
        if p.endswith('.c'):
            # By compiled code, as for headers: a routine whose own compiled C
            # is unchanged keeps its results, whatever else its file gained
            # (Jeff, 2026-11-18: test what needs testing).
            headers = True
        elif p.endswith('.c.held'):
            out |= tags_in(rev, p) | tags_in(REV, p)
        elif p == TREE + '/held.txt':
            # A hold or release: the routines left out at one revision only.
            out |= dropped_at(rev) ^ dropped_at(REV)
        elif p.endswith('.h'):
            headers = True
        else:
            return frozenset(), True
    if headers:
        sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
        import objdiff
        got, everything = objdiff.affected(rev, REV, checker=harness)
        if everything:
            return frozenset(), True
        out |= got
    return frozenset(out), False


def stale(rev, addr):
    """True when a result for routine `addr` measured on `rev` may not hold
    for REV because its C, a held C, a header or its script changed. A
    harness change alone does not make it stale: the whole game is checked
    again on the new harness off the critical path (runner.py's regression
    check, which rewrites the A7 ledger)."""
    got, everything = affected_since(rev, False)
    return everything or addr in got


@functools.lru_cache(maxsize=None)
def dropped_at(rev):
    """Routines the hybrid leaves out at `rev` (held.txt and their C callers; objdiff.py)."""
    r = git('show', '%s:%s/held.txt' % (rev, TREE)).stdout
    if not any(l.split('#')[0].strip() for l in r.splitlines()):
        return frozenset()
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
    import objdiff
    return frozenset(objdiff.dropped_at(rev))


@functools.lru_cache(maxsize=None)
def c_routines():
    """Address -> name of every routine with C in the hybrid at REV (.c.held
    files and held.txt's routines, with their C callers, excluded)."""
    out = {}
    for p in git('ls-tree', '-r', '--name-only', REV, '--', TREE).stdout.split():
        if p.endswith('.c'):
            r = git('show', '%s:%s' % (REV, p))
            for m in TAG.finditer(r.stdout):
                out['%02X%s' % (int(m.group(1), 16), m.group(2).upper())] = m.group(3)
    for a in dropped_at(REV):
        out.pop(a, None)
    return out


@functools.lru_cache(maxsize=None)
def reached():
    """Script name -> addresses of the catalogued routines its trace entered."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
    import codemap
    rows = list(csv.DictReader(open(os.path.join(B, 'routines.csv'))))
    # Tagged routines the catalog has no row for (SPC700 routines, entries
    # reached only by a jump, a vector or a unit call): each at the ROM
    # offset of its first byte, reached where the trace ran it there (the
    # SPC700's opcode bit, folded back onto the ROM, for SPC700 code).
    have = {r['addr'] for r in rows}
    tags = []
    for dp, dn, fn in os.walk(os.path.join(ROOT, TREE)):
        for f in fn:
            if f.endswith('.c'):
                text = open(os.path.join(dp, f)).read()
                for m in TAG.finditer(text):
                    a = int(m.group(1), 16) << 16 | int(m.group(2), 16)
                    if '%06X' % a in have:
                        continue
                    sp = re.search(r'@spc\s', text[m.end():text.find('*/', m.end())])
                    tags.append(('%06X' % a, codemap.rom_offset(a), 0x80 if sp else 0x33))
    out = {}
    for s in gate_scripts():
        name = os.path.splitext(os.path.basename(s))[0]
        d = os.path.join(B, 'box', name)
        if not os.path.exists(os.path.join(d, 'rommap.bin')):
            continue
        mp = codemap.merged([d])
        out[name] = frozenset([r['addr'] for r in rows if int(r['offset'], 16) < len(mp) and mp[int(r['offset'], 16)] & 0x33]
                              + [a for a, o, bit in tags if 0 <= o < len(mp) and mp[o] & bit])
    return out


def script_stale(name, rev, prev):
    """Why `name` must run again for HEAD, given its last row; '' when its result carries."""
    if not prev or prev['verdict'] not in ('IDENTICAL', 'UNCOVERED'):
        return 'no passing result'
    paths = changed_since(prev['source'])
    if paths is None:
        return 'no committed revision to compare'
    got, everything = affected_since(prev['source'], False)
    if everything:
        return 'shared code changed'
    if 'tools/aladdin/scripts/%s.box' % name in paths:
        return 'script changed'
    if name not in reached():
        return 'no trace'
    hit = reached()[name] & got
    return 'C changed for %s' % ' '.join(sorted(hit)[:4]) if hit else ''


GATE_LIB = os.path.join(WORKD, LIBNAME) if OUTD else os.path.join(B, 'hybrid', 'gate', LIBNAME)


@functools.lru_cache(maxsize=None)
def build_gate_lib():
    """Build the hybrid from the last commit; return its revision."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
    import hybrid
    src, rev = hybrid.export_head()
    if hybrid.main(src, GATE_LIB):
        raise SystemExit('hybrid build of %s failed' % rev)
    return rev


def _cache_path(name):
    return os.path.join(WORKD, 'check', name, 'rows.json')


def _load_cache(name):
    """A script's kept per-routine results: {addr: [name, calls, match, diff,
    inconclusive, skipped, rev, script sha]}, each with the revision it was
    measured on (or carried to) and the script it was measured with."""
    try:
        c = json.load(open(_cache_path(name)))
    except (OSError, ValueError):
        return {}
    if c.get('v') == 2:
        return c['rows']
    # The first form kept one revision and script for all of a script's rows.
    return {a: r[:6] + [c.get('rev'), c.get('script')] for a, r in c.get('rows', {}).items()}


def _save_cache(name, rows):
    os.makedirs(os.path.dirname(_cache_path(name)), exist_ok=True)
    tmp = _cache_path(name) + '.tmp'
    json.dump({'v': 2, 'rows': rows}, open(tmp, 'w'))
    os.replace(tmp, _cache_path(name))


def _script_sha(script):
    return hashlib.sha1(open(script, 'rb').read()).hexdigest()


def live(only, full=False):
    """A5. Each script's per-routine results are kept, each with the
    revision and the script it was measured on
    (build/aladdin/check/NAME/rows.json). A result is measured again when
    it may not hold for REV: its script changed, or its routine's C, or the
    harness (affected_since); the others are carried to REV. A routine the
    script reaches with no kept result is measured too. With routines named
    (`only`), just theirs are measured and just their rows of the ledger
    rewritten; every other routine's row is left as it was. The verdicts are
    summed over every script's kept results."""
    global LIB
    rev = build_gate_lib()
    LIB = GATE_LIB
    scripts = list(gate_scripts())
    have = c_routines()
    targets = (set(only) & set(have)) if only else set(have)
    rch = reached()
    jobs, kept = [], {}
    for sc in scripts:
        name = os.path.splitext(os.path.basename(sc))[0]
        sha = _script_sha(sc)
        rows = _load_cache(name)
        reach = set(rch.get(name, have)) | {a for a, r in rows.items() if r[1]}
        want = set()
        for a in targets & reach:
            r = rows.get(a)
            if full or not r or r[7] != sha or stale(r[6], a):
                want.add(a)
        # Results that need no new measurement hold for REV: carry them. A
        # routine the script's trace (of this script: build.py retraces a
        # changed one) never entered has no call in it, whatever its C.
        for a, r in rows.items():
            if a in want or r[6] == rev and r[7] == sha:
                continue
            if a not in reach and not r[1]:
                r[6], r[7] = rev, sha
            elif r[7] == sha and not stale(r[6], a):
                r[6] = rev
        kept[name] = (sc, sha, rows)
        if want:
            jobs.append((sc, None if not only and want >= (reach & set(have)) else sorted(want)))
    print('%d script run(s): %d whole, %d for named or changed routines' % (
        len(jobs), sum(1 for _, w in jobs if w is None), sum(1 for _, w in jobs if w is not None)))
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as ex:
        results = list(ex.map(lambda j: (j,) + one(j[0], j[1]), jobs))
    stuck = 0
    for (sc, want), name, got, notes in results:
        if got is None:
            stuck += 1
            print(notes[0])
            continue
        rows = kept[name][2]
        for addr, rname, calls, match, diff, inc, skipped, _ in got:
            rows[addr] = [rname, int(calls), int(match), int(diff), int(inc), int(skipped), rev, kept[name][1]]
        for n in notes[:20]:
            print(n)
    for name, (sc, sha, rows) in kept.items():
        _save_cache(name, rows)
    agg = {}
    for name, (sc, sha, rows) in kept.items():
        for addr, (rname, calls, match, diff, inc, skipped, rrev, rsha) in rows.items():
            if addr not in have:
                continue
            a = agg.setdefault(addr, {'name': rname, 'calls': 0, 'diff': 0, 'inc': 0, 'scripts': 0, 'revs': set()})
            a['calls'] += calls; a['diff'] += diff; a['inc'] += inc
            a['scripts'] += calls > 0
            # A result not measured for REV (its script stopped, or a
            # changed script's other routines when only some were named)
            # leaves the routine's row on the revision it holds for.
            a['revs'].add(rrev if rsha == sha else None)
    stamp = datetime.date.today().isoformat()
    old = {r['routine']: r for r in csv.DictReader(open(LIVE))} if os.path.exists(LIVE) else {}
    out = {a: r for a, r in old.items() if a in have and a not in targets}
    failed = 0
    for addr in sorted(targets & set(agg)):
        a = agg[addr]
        v = 'DIVERGENT' if a['diff'] else 'UNCOVERED' if a['calls'] == 0 else 'UNVERIFIED' if a['inc'] else 'IDENTICAL'
        failed += v != 'IDENTICAL'
        src = rev if a['revs'] == {rev} else 'not measured' if None in a['revs'] else \
            sorted(a['revs'] - {rev})[0]
        prev = old.get(addr)
        same = prev and prev['verdict'] == v and int(prev['calls']) == a['calls'] and int(prev['diff']) == a['diff']
        if not same or only:
            print('%s %-28s calls %d in %d/%d scripts, diff %d, inconclusive %d: %s' % (
                addr, a['name'], a['calls'], a['scripts'], len(scripts), a['diff'], a['inc'], v))
        out[addr] = {'routine': addr, 'name': a['name'], 'verdict': v, 'calls': a['calls'], 'scripts': a['scripts'],
                     'diff': a['diff'], 'inconclusive': a['inc'], 'date': stamp if not same else prev['date'],
                     'source': src}
    os.makedirs(os.path.dirname(LIVE), exist_ok=True)
    with open(LIVE + '.tmp', 'w', newline='') as f:
        wr = csv.DictWriter(f, ['routine', 'name', 'verdict', 'calls', 'scripts', 'diff', 'inconclusive', 'date', 'source'])
        wr.writeheader()
        for k in sorted(out):
            wr.writerow(out[k])
    os.replace(LIVE + '.tmp', LIVE)
    print('%d routines: %s' % (len(out), ', '.join('%d %s' % (n, v) for v, n in sorted(
        {v: sum(1 for r in out.values() if r['verdict'] == v) for v in set(r['verdict'] for r in out.values())}.items()))))
    return 1 if failed or stuck else 0


def source_rev():
    rev = subprocess.run(['git', 'rev-parse', '--short', 'HEAD'], capture_output=True, text=True, cwd=ROOT).stdout.strip()
    dirty = subprocess.run(['git', 'status', '--porcelain', '--', TREE], capture_output=True, text=True, cwd=ROOT).stdout.strip()
    return rev + ('+dirty' if dirty else '')


def pieces(script):
    """A script of unit calls (snesbox call or fx), as one lockstep script per call.

    A unit call puts the machine back as it was when it returns, and a
    `load` replaces the whole machine: in one lockstep run, either can land
    where the original is inside a call the hybrid ran in one step as C,
    leaving the two sides' records of calls out of step. So each call is run
    on its own: its last `load`, every command since then except the earlier
    calls (each put back, so the machine is as if it had not run) and their
    `set`s, then the call, which is the run's last (nothing to put back),
    and the commands after it up to the next call or load. A call marked
    noreturn (a routine that stops or restarts the game) never returns, in
    the original or as C: its piece compares the frames it runs (frames=),
    the only judge of such a routine's C (A5 has no return to compare)."""
    lines = [l.split('#')[0].strip() for l in open(script)]
    lines = [l for l in lines if l]
    if not any(l.split()[0] in ('call', 'fx', 'spccall') for l in lines):
        return [script]
    name = os.path.splitext(os.path.basename(script))[0]
    d = os.path.join(WORKD, 'image', name, 'pieces')
    os.makedirs(d, exist_ok=True)
    for old in glob.glob(os.path.join(d, '*.box')):
        os.remove(old)
    out, prefix, sets = [], [], []
    for i, l in enumerate(lines):
        w = l.split()
        if w[0] == 'load':
            prefix, sets = [l], []
        elif w[0] in ('set', 'spcset'):
            sets.append(l)
        elif w[0] in ('call', 'fx', 'spccall'):
            if 'keep' in w:
                prefix += sets + [l]
            sets_now, sets = sets, []
            tail = []
            for t in lines[i + 1:]:
                if t.split()[0] in ('call', 'fx', 'spccall', 'load', 'set', 'spcset'):
                    break
                tail.append(t)
            body = prefix + ([] if 'keep' in w else sets_now + [l]) + tail
            # A kept call's piece ends at the call: the frame program the
            # game started (FxFrameProgram runs over several frames, one C
            # call in the hybrid) must reach its STOP in the original's
            # record, or every frame of the piece is past the record's end
            # and none is compared.
            if 'keep' in w:
                body.append('wait 8')
            path = os.path.join(d, '%s_p%03d.box' % (name, len(out)))
            with open(path, 'w') as f:
                f.write('\n'.join(body) + '\n')
            out.append(path)
        else:
            prefix.append(l)
    return out


def image_one(script):
    name = os.path.splitext(os.path.basename(script))[0]
    d = os.path.join(WORKD, 'image', name)
    os.makedirs(d, exist_ok=True)
    rep = os.path.join(d, 'lockstep.txt')
    if os.path.exists(rep):
        os.remove(rep)
    # A run that stops (a side stuck waiting) is that script's failure, not
    # the whole gate's: the other scripts' results still count.
    # One still running after STUCK seconds is stuck (a side waiting on the
    # other), and fails.
    try:
        p = run_group([SNESBOX, '--rom', os.path.join(B, 'baserom.sfc'), '--script', script, '--out', d, '--lockstep', LIB],
                      STUCK)
    except subprocess.TimeoutExpired:
        return name, ['stopped (no result after %ds): FAILED' % STUCK]
    lines = open(rep).read().splitlines() if os.path.exists(rep) else []
    if p.returncode or not lines or ': ' not in lines[-1]:
        why = (p.stderr.strip().splitlines() or ['no report'])[-1]
        lines.append('stopped (%s): FAILED' % why)
    elif re.search(r'\bframes [1-9][0-9]* compared 0,', lines[-1]):
        # Nothing was judged: a run of frames whose hybrid side never got
        # one to compare (it stopped, or the record ended under it) proves
        # nothing.
        lines.append('no frame compared: FAILED')
    return name, lines


def reaching(addrs):
    """The scripts whose results judge routines `addrs` in A7: every script
    whose trace entered one of them, every script with no trace (it may),
    and every script with a noreturn unit call of one."""
    want, out = set(addrs), set()
    for s in gate_scripts():
        name = os.path.splitext(os.path.basename(s))[0]
        if name not in reached() or reached()[name] & want:
            out.add(name)
    for a in want:
        cpath, ctext, cm = find_tagged(os.path.join(ROOT, TREE), ('.c',), int(a, 16))
        sm = re.search(r'@spc\s+([0-9A-Fa-f]{4})', header_of(ctext, cm, False)) if cm else None
        out |= set(noreturn_scripts(int(a, 16), int(sm.group(1), 16) if sm else None))
    return out


def image(full=False, routines=None, names=None):
    """A7. With routines named, only the scripts reaching them (reaching())
    are judged, and only their rows of the ledger written; each runs again
    unless its result holds for REV (script_stale), when it is carried.
    With script names given, just those are judged (with full, run)."""
    global LIB
    rev = build_gate_lib()
    LIB = GATE_LIB
    scripts = list(gate_scripts())
    old = {r['script']: r for r in csv.DictReader(open(IMAGE))} if os.path.exists(IMAGE) else {}
    stamp, verdicts = datetime.date.today().isoformat(), []
    rows, run_these = [], []
    judged = reaching(routines) if routines else set(names) if names else None
    if routines:
        print('%d script(s) reach %s' % (len(judged), ' '.join(sorted(routines))))
    for s in scripts:
        name = os.path.splitext(os.path.basename(s))[0]
        if judged is not None and name not in judged:
            if name in old:
                rows.append(old[name])
            continue
        why = 'full run' if full else script_stale(name, rev, old.get(name))
        if why:
            run_these.append(s)
            print('%-22s runs: %s' % (name, why))
            continue
        r = dict(old[name])
        print('%-22s %s (carried from %s)' % (name, r['verdict'], r['source']))
        r['source'] = rev
        verdicts.append(r['verdict'])
        rows.append(r)
    work = [(s, p) for s in run_these for p in pieces(s)]
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, os.cpu_count() // 2)) as ex:
        done = list(ex.map(image_one, [p for _, p in work]))
    results = []
    for s in run_these:
        name = os.path.splitext(os.path.basename(s))[0]
        got = [r for (src, _), r in zip(work, done) if src == s]
        if len(got) == 1 and got[0][0] == name:
            results.append(got[0])
            continue
        # A script of unit calls: its pieces' verdicts together.
        vs = [ls[-1].rsplit(': ', 1)[-1] if ls and ': ' in ls[-1] else 'FAILED' for _, ls in got]
        bad = [(n, ls) for (n, ls), v in zip(got, vs) if v not in ('IDENTICAL', 'UNCOVERED')]
        v = 'DIVERGENT' if bad else 'IDENTICAL' if 'IDENTICAL' in vs else 'UNCOVERED'
        lines = ['%s: %s' % (n, ls[-1] if ls else 'no report') for n, ls in bad]
        lines.append('%d calls run one per lockstep run, %d IDENTICAL, %d UNCOVERED, %d not: %s' % (
            len(vs), vs.count('IDENTICAL'), vs.count('UNCOVERED'), len(bad), v))
        results.append((name, lines))
    for name, lines in results:
        last = lines[-1] if lines else 'FAILED'
        v = last.rsplit(': ', 1)[-1] if ': ' in last else 'FAILED'
        verdicts.append(v)
        for l in lines[:-1][:3]:
            print('%s: %s' % (name, l))
        print('%-22s %s' % (name, last))
        rows.append({'script': name, 'verdict': v, 'detail': last, 'date': stamp, 'source': rev})
    os.makedirs(os.path.dirname(IMAGE), exist_ok=True)
    with open(IMAGE + '.tmp', 'w', newline='') as f:
        wr = csv.DictWriter(f, ['script', 'verdict', 'detail', 'date', 'source'])
        wr.writeheader()
        wr.writerows(sorted(rows, key=lambda r: r['script']))
    os.replace(IMAGE + '.tmp', IMAGE)
    ok = all(v in ('IDENTICAL', 'UNCOVERED') for v in verdicts) and 'IDENTICAL' in verdicts
    print('%s: %s' % ('whole image' if judged is None else 'scripts reaching them', 'IDENTICAL' if ok else 'DIVERGENT'))
    return 0 if ok else 1


import re, struct

TAG = re.compile(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)')
MARKERS = re.compile(r'\b(TODO|FIXME|XXX|HACK|guess|placeholder)\b|\?\?\?|#if 0', re.I)


def git(*a):
    return subprocess.run(['git'] + list(a), capture_output=True, text=True, cwd=ROOT)


def find_tagged(root, exts, addr):
    for dp, dn, fn in os.walk(root):
        for f in fn:
            if f.endswith(exts):
                path = os.path.join(dp, f)
                text = open(path).read()
                for m in TAG.finditer(text):
                    if (int(m.group(1), 16) << 16 | int(m.group(2), 16)) == addr:
                        return path, text, m
    return None, None, None


def header_of(text, m, asm):
    """The tag's line and the header comment lines that follow it."""
    lines = text[text.rfind('\n', 0, m.start()) + 1:].splitlines()
    out = [lines[0]]
    for l in lines[1:]:
        t = l.strip()
        if asm and not t.startswith(';'):
            break
        out.append(t)
        if not asm and '*/' in t:
            break
    return '\n'.join(out)


def rel(path):
    return os.path.relpath(os.path.join(ROOT, path), ROOT)


@functools.lru_cache(maxsize=None)
def dirty_paths():
    """Every path with uncommitted changes, from one git status."""
    out, paths = iter(git('status', '--porcelain', '-z', '--untracked-files=all').stdout.split('\0')), set()
    for e in out:
        if len(e) > 3:
            paths.add(e[3:])
            if e[0] in 'RC':  # a rename or copy names its source next
                paths.add(next(out, ''))
    return frozenset(paths)


def dirty(path):
    return rel(path) in dirty_paths()


@functools.lru_cache(maxsize=None)
def last_change(path):
    return git('log', '-1', '--format=%H', '--', path).stdout.strip()


@functools.lru_cache(maxsize=None)
def is_ancestor(a, b):
    return git('merge-base', '--is-ancestor', a, b).returncode == 0


def contains(rev, path):
    """True when committed source `rev` includes the last change to `path`."""
    if not rev or rev.endswith('+dirty'):
        return False
    last = last_change(rel(path))
    return bool(last) and is_ancestor(last, rev)


def holds(rev, addr, path):
    """True when a result for routine `addr` measured on committed `rev`
    holds for REV: `rev` has the last change to its file, or `rev` is an
    ancestor of REV and the routine's compiled C (and the held C, headers)
    did not change since (stale())."""
    if contains(rev, path):
        return True
    if not rev or rev.endswith('+dirty') or not is_ancestor(rev, REV):
        return False
    return not stale(rev, addr)


def merged_map():
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
    import codemap
    return codemap.merged()


def span_of(addr, asm_path):
    """ROM offsets of the routine: from its address through the filed bytes that follow."""
    owned = set()
    for line in open(os.path.join(B, 'claimed.txt')):
        lo, hi = (int(x, 16) for x in line.split('-'))
        owned.update(range(lo, hi))
    return filed_span(addr, owned, asm_tag_offsets())


def asm_tag_offsets():
    """ROM offsets of every @routine tag in the filed assembly."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
    import codemap
    others = set()
    for dp, dn, fn in os.walk(os.path.join(ROOT, 'asm', 'aladdin')):
        for f in fn:
            if f.endswith('.asm'):
                for m in TAG.finditer(open(os.path.join(dp, f)).read()):
                    a = int(m.group(1), 16) << 16 | int(m.group(2), 16)
                    others.add(codemap.rom_offset(a))
    return others


def filed_span(addr, owned, others):
    """span_of with the filed bytes (owned) and the tag offsets (others) given."""
    sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
    import codemap
    start = codemap.rom_offset(addr)
    span, o = [], start
    while o in owned and (o == start or o not in others):
        span.append(o); o += 1
    return span


def noreturn_scripts(addr, spc=None):
    """The scripts with a unit call of addr marked noreturn (spccall of its
    APU address spc, for an SPC700 routine)."""
    out = []
    for path in gate_scripts():
        for l in open(path):
            w = l.split('#')[0].split()
            if len(w) >= 2 and 'noreturn' in w and (
                    (w[0] == 'call' and int(w[1].replace('$', '').replace(':', ''), 16) == addr)
                    or (w[0] == 'spccall' and spc is not None and int(w[1].replace('$', ''), 16) == spc)):
                out.append(os.path.splitext(os.path.basename(path))[0])
                break
    return out


def assess(addrs, image_ok=None):
    """Every gate for each routine: [(key, name, tier, [(gate, ok, why)])]."""
    live = {r['routine']: r for r in csv.DictReader(open(LIVE))} if os.path.exists(LIVE) else {}
    image_rows = list(csv.DictReader(open(IMAGE))) if os.path.exists(IMAGE) else []
    mp, out = None, []
    for a in addrs:
        addr = int(a, 16)
        key = '%06X' % addr
        cpath, ctext, cm = find_tagged(os.path.join(ROOT, TREE), ('.c',), addr)
        apath, atext, am = find_tagged(os.path.join(ROOT, 'asm', 'aladdin'), ('.asm',), addr)
        if not cpath:
            out.append((key, am.group(3) if am else '', 'T2' if apath else 'T1', [('C', False, 'no C')]))
            continue
        gates = []
        name = cm.group(3)
        if key in dropped_at(REV):
            # Held: the hybrid runs the original in its place, so no gate judges its C.
            sys.path.insert(0, os.path.join(ROOT, 'tools', 'aladdin'))
            import objdiff
            own = key in objdiff.held_list(os.path.join(ROOT, TREE))
            gates.append(('held', False, 'listed in %s/held.txt' % TREE if own else 'its C calls held C'))
        h = header_of(ctext, cm, False)
        g0 = all(k in h for k in ('WHAT IT DOES:', 'HOW:', 'Entry:', 'Exit:')) and not MARKERS.search(ctext)
        gates.append(('0 complete', g0, os.path.relpath(cpath, ROOT)))
        span = span_of(addr, apath) if apath else []
        if mp is None:
            mp = merged_map()
        # Executed, or (a table inside the routine, such as a JMP (abs,X)
        # list) read as data: each entry counts once its case is taken.
        # SPC700 code by its own trace bits only (spcmap: opcode, operand,
        # data): the S-CPU reads every byte it uploads to the APU, so its
        # bits say nothing of what the SPC700 ran.
        cov = 0x80 | 0x2000 | 0x4000 if re.search(r'@spc\s', h) else 0x77
        covered = bool(span) and all(mp[o] & cov for o in span)
        gates.append(('C covered', covered, '%d of %d bytes executed or read' % (sum(1 for o in span if mp[o] & cov), len(span))
                      if span else 'no filed assembly gives the routine\'s extent'))
        lr = live.get(key)
        # A call an interrupt overlapped cannot be judged per call (the handler can
        # change what the routine reads mid-call); A7 judges those. A5 passes with
        # no divergence and at least one identical call.
        # A routine every call of which is inconclusive (an
        # interrupt overlapped it, or it reached code without C) has no call
        # A5 can judge; with no divergent call it is left to A7.
        a5ok = bool(lr) and (lr['verdict'] == 'IDENTICAL' or (lr['verdict'] == 'UNVERIFIED' and int(lr['diff']) == 0))
        g5 = a5ok and holds(lr['source'], key, cpath) and not dirty(cpath)
        why5 = ('%s on %s' % (lr['verdict'], lr['source']) if lr else 'never run')
        # A routine that never returns (it stops or restarts the game) gives
        # A5 no call to compare. Its unit calls marked noreturn run as
        # lockstep pieces that compare the frames after it, with its C in
        # its place: when every script with one is IDENTICAL, that is its
        # per-call judgement.
        if not g5 and (not lr or int(lr['calls']) == 0) and not dirty(cpath):
            sm = re.search(r'@spc\s+([0-9A-Fa-f]{4})', h)
            scripts = noreturn_scripts(addr, int(sm.group(1), 16) if sm else None)
            rows = {r['script']: r for r in image_rows}
            if scripts and all(n in rows and rows[n]['verdict'] == 'IDENTICAL' and holds(rows[n]['source'], key, cpath)
                               for n in scripts):
                g5 = True
                why5 = 'never returns; its noreturn unit calls IDENTICAL in A7 (%s)' % ', '.join(scripts)
        gates.append(('A5 live', g5, why5 + ('; the C has uncommitted changes' if dirty(cpath) else '')))
        # A7: every script that reaches the routine runs IDENTICAL in
        # lockstep, measured on committed source with the C's last change.
        rows = {r['script']: r for r in image_rows}
        judged = sorted(reaching([key]))
        miss = [n for n in judged if n not in rows or rows[n]['verdict'] != 'IDENTICAL'
                or not holds(rows[n]['source'], key, cpath)]
        g7 = bool(judged) and not miss and not dirty(cpath)
        why7 = 'no script reaches it' if not judged else '%d of %d scripts reaching it IDENTICAL on its C' % (
            len(judged) - len(miss), len(judged))
        if miss:
            n = miss[0]
            why7 += '; %s: %s' % (n, '%s on %s' % (rows[n]['verdict'], rows[n]['source']) if n in rows else 'never run')
        gates.append(('A7 image', g7, why7))
        t3 = all(g for _, g, _ in gates)
        t4 = False
        if apath:
            ah = header_of(atext, am, True)
            ga = all(k in ah for k in ('WHAT IT DOES:', 'HOW:', 'Entry:', 'Exit:')) and not MARKERS.search(atext)
            body = atext[am.end():]
            nxt = re.search(r'@routine\s', body)
            if nxt:  # this routine only, up to the next one's header
                body = body[:body.rfind('\n', 0, nxt.start()) + 1]
            literal = [l.strip() for l in body.splitlines()
                       if l.strip() and not l.strip().startswith((';', '%', '!', 'db ', 'dw ', 'dl '))
                       and not l.strip().endswith(':')
                       and re.search(r'(?<!#)\$[0-9A-Fa-f]', l.split(';')[0])]
            gs = not literal
            if image_ok is None:
                image_ok = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'aladdin', 'image.py')],
                                          capture_output=True, text=True, cwd=ROOT).returncode == 0
            gates += [('T4 header', ga, os.path.relpath(apath, ROOT)),
                      ('T4 symbolic', gs, 'literal addresses: %s' % literal[:3] if literal else 'no literal addresses'),
                      ('T4 image', image_ok, 'whole ROM byte-exact' if image_ok else 'image gate fails')]
            t4 = t3 and ga and gs and image_ok
        out.append((key, name, 'T4' if t4 else 'T3' if t3 else 'T2', gates))
    return out


def qualify(addrs):
    worst = 0
    # ALADDIN_IMAGE_OK=0/1: the image gate's result, taken by the caller
    # under the build lock right after the build (build.py deletes the ROM
    # first, so image.py run beside a build can find none).
    ok = os.environ.get('ALADDIN_IMAGE_OK')
    for key, name, tier, gates in assess(addrs, None if ok is None else ok == '1'):
        print('%s %s: %s' % (key, name, tier if tier in ('T3', 'T4') else 'below T3 (%s)' % tier))
        for g, ok, why in gates:
            print('  %-12s %s  %s' % (g, 'pass' if ok else 'FAIL', why))
        worst = max(worst, 0 if tier in ('T3', 'T4') else 1)
    return worst


def gate_lock():
    """One gate run at a time per output folder (WORKD: build/aladdin, or
    ALADDIN_GATE_OUT's): two runs there would share output files. A run
    into another folder (a probe, a test) never blocks the ledger's. A lock
    left by a run that died is taken over."""
    os.makedirs(WORKD, exist_ok=True)
    lock = os.path.join(WORKD, 'gate.lock')
    if os.path.exists(lock):
        try:
            pid = int(open(lock).read().strip())
            os.kill(pid, 0)
            raise SystemExit('another gate run (pid %d) holds %s' % (pid, lock))
        except (ValueError, ProcessLookupError):
            pass
    open(lock, 'w').write(str(os.getpid()))
    import atexit
    atexit.register(lambda: os.path.exists(lock) and os.remove(lock))


def main(argv):
    if argv[:1] in (['--live'], ['--image']) or argv[1:2] in (['--live'], ['--image']):
        gate_lock()
    full = '--full' in argv
    argv = [a for a in argv if a != '--full']
    if argv[:1] == ['--live']:
        return live([a.upper().replace(':', '').replace('$', '') for a in argv[1:]], full)
    if argv[:1] == ['--image']:
        return image(full)
    if argv[:1] == ['--affected'] and len(argv) == 2:
        # The routines whose results may differ from revision argv[1] to REV
        # (ALL: everything), and the scripts added or changed since.
        # A script added or changed reaches routines whose results it adds
        # to: they are qualified again too (run after build.py traced it).
        # Routines the batch changed, harness changes left out (they queue
        # the off-path whole-game check: the "harness" line).
        got, everything = affected_since(argv[1], False)
        harness = affected_since(argv[1])[1] and not everything
        paths = changed_since(argv[1]) or set()
        new = sorted(os.path.splitext(os.path.basename(p))[0] for p in paths
                     if p.startswith('tools/aladdin/scripts/') and p.endswith('.box'))
        have = c_routines()
        out = set(have) if everything else set(got)
        for s in new:
            out |= set(reached().get(s, have)) & set(have)
        print('routines', ' '.join(sorted(out)))
        print('scripts', ' '.join(new))
        print('harness', 'yes' if harness else 'no')
        return 0
    if argv[:1] == ['--failures'] and argv[1:]:
        # After --qualify --run ADDR...: routines whose A5 diverged, and each
        # script reaching them that is red in A7, with the routines it reaches.
        addrs = set(argv[1:])
        live_rows = {r['routine']: r for r in csv.DictReader(open(LIVE))} if os.path.exists(LIVE) else {}
        print('a5', ' '.join(sorted(a for a in addrs if a in live_rows and (
            live_rows[a]['verdict'] == 'DIVERGENT' or int(live_rows[a]['diff'])))))
        img = {r['script']: r for r in csv.DictReader(open(IMAGE))} if os.path.exists(IMAGE) else {}
        for s in sorted(reaching(addrs)):
            r = img.get(s)
            if r and r['verdict'] not in ('IDENTICAL', 'UNCOVERED'):
                print('red', s, ' '.join(sorted(addrs & set(reached().get(s, addrs)))))
        return 0
    if argv[:1] == ['--lockstep'] and argv[1:]:
        # A7 on just these scripts, all run (the runner's bisection, under its own ALADDIN_GATE_OUT).
        gate_lock()
        return image(True, names=argv[1:])
    if argv[:2] == ['--qualify', '--run'] and argv[2:]:
        # Per-routine certification: A5 on just these routines' calls, A7 on
        # just the scripts reaching them, their ledger rows only; then grade.
        addrs = ['%06X' % int(x.replace(':', '').replace('$', ''), 16) for x in argv[2:]]
        gate_lock()
        live(addrs)
        image(routines=addrs)
        return qualify(addrs)
    if argv[:1] == ['--qualify'] and argv[1:]:
        return qualify([x.replace(':', '').replace('$', '') for x in argv[1:]])
    print(__doc__)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
