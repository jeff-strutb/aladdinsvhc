#!/usr/bin/env python3
"""runner.py: the gate runner. Certifies each commit's routines as it lands.

    runner.py [--from REV] [--once] [--ledger-every N]

For each new commit on the checked-out branch (the batch: every commit since
the last one done), the runner:

  1. waits while asm/, src/ or tools/ has uncommitted changes (writers commit
     finished batches only; the build and the gates read the work tree);
  2. builds (build.py: traces new or changed scripts, rebuilds the checker
     when its sources changed) and takes the image gate (image.py), under the
     build lock, holding its cpu-run slots first;
  3. qualifies the routines the batch affects (t3.py --affected: C filed,
     changed, held or released, headers by compiled code, and the routines a
     new or changed script reaches) with t3.py --qualify --run, which runs
     A5 on their calls and A7 on the scripts reaching them only;
  4. holds the C that failed: a routine whose A5 diverged, and for each
     script red in A7, the routine(s) of the batch it reaches that turn it
     red, found by bisection (each test reruns that script alone with a set
     of routines held, on a commit object no branch names); a red the batch's
     routines do not cause (still red with all of them held) holds nothing.
     Holds are lines in src/aladdin/held.txt, committed by the runner alone;
     the next pass reruns the scripts reaching what the hold changed.
  5. a batch that changed the harness (the checker, core patches,
     hybrid.py) qualifies only the routines it changed, like any other;
     the whole game is checked again off the critical path: a full A7 on
     the new harness into the A7 ledger, run once no commit has landed for
     15 minutes, which reports red scripts (regressions) without holding;
  6. commits the ledgers (config/aladdin/t3_live.csv, whole_image.csv, and
     progress.py's README and map) every N batches (default 3), or when idle.

State and logs live in build/aladdin/gate-runner/ (runner.json, runner.log,
batch-REV.log). Every job runs through the machine's CPU and memory queue,
the command named by ALADDIN_CPU_RUN (called as CMD -j CORES -m MEMORY --
COMMAND), with the cores and memory it uses; builds and progress take the
build lock, the file ALADDIN_BUILD_LOCK (flock). Its commits take the
landing lock, ALADDIN_LAND_LOCK, which writers hold while they merge a
batch into the branch, so the two never meet in git's index. The runner
will not start without all three.
"""
import csv, datetime, json, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
D = os.path.join(ROOT, 'build', 'aladdin', 'gate-runner')
STATE = os.path.join(D, 'runner.json')
LOG = os.path.join(D, 'runner.log')
CPU_RUN = os.environ.get('ALADDIN_CPU_RUN')
LOCK = os.environ.get('ALADDIN_BUILD_LOCK')
LAND = os.environ.get('ALADDIN_LAND_LOCK')
HELD = os.path.join(ROOT, 'src', 'aladdin', 'held.txt')
LEDGERS = ['config/aladdin/t3_live.csv', 'config/aladdin/whole_image.csv', 'README.md', 'docs/aladdin/progress-map.svg']
BISECT_OUT = 'runner-bisect'
REGRESS_IDLE = 900
# Paths whose change needs no gate run (the runner's own outputs).
QUIET = tuple(LEDGERS) + ('docs/',)

# Sizes (cores, memory) by what each job really uses: build.py's retrace
# pool and the hybrid compile; A5 = PYTHON_CPU_COUNT single-process checks,
# A7 = half as many lockstep pairs (~1.6 cores, ~0.6G each); one lockstep
# pair for a bisection test; light Python reading the traces.
BUILD = ('6', '5G')
QUALIFY = ('12', '8G')
QUALIFY_TIMEOUT = 14400
BISECT = ('2', '2G')
LIGHT = ('1', '3G')


def log(msg):
    line = '%s %s' % (datetime.datetime.now().strftime('%F %T'), msg)
    print(line, flush=True)
    with open(LOG, 'a') as f:
        f.write(line + '\n')


def git(*a, check=False):
    p = subprocess.run(['git'] + list(a), capture_output=True, text=True, cwd=ROOT)
    if check and p.returncode:
        raise SystemExit('git %s failed: %s' % (' '.join(a), p.stderr.strip()))
    return p.stdout.strip()


def job(slots, cmd, env=None, lock=False, timeout=7200, out=None):
    """Run cmd (a shell string) through cpu-run with (cores, memory) slots;
    PYTHON_CPU_COUNT is the cores held. Returns (rc, output)."""
    j, m = slots
    body = 'PYTHON_CPU_COUNT=$CPU_RUN_JOBS timeout %d %s' % (timeout, cmd)
    argv = [CPU_RUN, '-j', j, '-m', m, '--']
    argv += (['flock', LOCK, 'sh', '-c', body] if lock else ['sh', '-c', body])
    e = dict(os.environ, **(env or {}))
    p = subprocess.run(argv, capture_output=True, text=True, cwd=ROOT, env=e)
    text = p.stdout + p.stderr
    if out:
        with open(out, 'a') as f:
            f.write('$ %s\n%s\n' % (cmd, text))
    return p.returncode, text


def load_state():
    try:
        return json.load(open(STATE))
    except (OSError, ValueError):
        return {}


def save_state(s):
    tmp = STATE + '.tmp'
    json.dump(s, open(tmp, 'w'), indent=1)
    os.replace(tmp, STATE)


def dirty():
    return [l for l in git('status', '--porcelain', '--', 'asm', 'src', 'tools').splitlines() if not l.startswith('??')]


def held_names(addrs, rev):
    """Address -> name for the routines named, from the C at rev."""
    out = {}
    for p in git('ls-tree', '-r', '--name-only', rev, '--', 'src/aladdin').split():
        if p.endswith('.c'):
            for m in re.finditer(r'@routine\s+([0-9A-Fa-f]{2}):([0-9A-Fa-f]{4})\s+(\w+)', git('show', '%s:%s' % (rev, p))):
                a = (m.group(1) + m.group(2)).upper()
                if a in addrs:
                    out[a] = m.group(3)
    return out


def probe(rev, extra):
    """A commit object (no branch names it) of rev with `extra` held lines added to held.txt."""
    cur = git('show', '%s:src/aladdin/held.txt' % rev)
    text = (cur + '\n' if cur else '') + ''.join('%s:%s probe\n' % (a[:2], a[2:]) for a in sorted(extra))
    blob = subprocess.run(['git', 'hash-object', '-w', '--stdin'], input=text, capture_output=True,
                          text=True, cwd=ROOT).stdout.strip()
    idx = os.path.join(D, 'probe.index')
    env = dict(os.environ, GIT_INDEX_FILE=idx)
    subprocess.run(['git', 'read-tree', rev], cwd=ROOT, env=env, check=True)
    subprocess.run(['git', 'update-index', '--add', '--cacheinfo', '100644,%s,src/aladdin/held.txt' % blob],
                   cwd=ROOT, env=env, check=True)
    tree = subprocess.run(['git', 'write-tree'], capture_output=True, text=True, cwd=ROOT, env=env).stdout.strip()
    os.remove(idx)
    return subprocess.run(['git', 'commit-tree', tree, '-p', rev, '-m', 'runner probe'], capture_output=True,
                          text=True, cwd=ROOT).stdout.strip()


def green(rev, script, hold, out):
    """True when `script` runs IDENTICAL in lockstep with `hold` added to rev's held C."""
    p = probe(rev, hold)
    path = os.path.join(ROOT, 'build', 'aladdin', BISECT_OUT, 'whole_image.csv')
    if os.path.exists(path):
        os.remove(path)  # a test that gives no result must not read the last one's
    rc, text = job(BISECT, 'python3 tools/aladdin/t3.py --lockstep %s' % script,
                   env={'ALADDIN_GATE_REV': p, 'ALADDIN_GATE_OUT': BISECT_OUT}, out=out)
    rows = {r['script']: r for r in csv.DictReader(open(path))} if os.path.exists(path) else {}
    v = rows.get(script, {}).get('verdict')
    log('  test %s holding %s: %s' % (script, ' '.join(sorted(hold)) or 'nothing more', v))
    if v is None:
        raise RuntimeError('bisection test of %s gave no result (rc %d)' % (script, rc))
    return v == 'IDENTICAL'


def culprits(rev, script, base, cands, out):
    """The routines among cands whose C turns `script` red, base held too.
    Holding more never turns a script red (a held routine runs the original),
    so: find the shortest prefix of cands whose hold turns it green; its last
    routine is a culprit; repeat on the rest with the culprits held."""
    cands = sorted(cands)
    found = set()
    if not green(rev, script, base | set(cands), out):
        return None  # red with every candidate held: not this batch's C
    while not green(rev, script, base | found, out):
        lo, hi = 1, len(cands)  # holding cands[:hi] (with found) is green
        while lo < hi:
            mid = (lo + hi) // 2
            if green(rev, script, base | found | set(cands[:mid]), out):
                hi = mid
            else:
                lo = mid + 1
        found.add(cands[hi - 1])
        cands = cands[:hi - 1]
    return found


def hold(rev, why, out):
    """Append the routines in `why` ({addr: reason}) to held.txt and commit it alone."""
    if git('status', '--porcelain', '--', HELD):
        log('held.txt has uncommitted changes: not holding %s' % ' '.join(sorted(why)))
        return False
    listed = set()
    if os.path.exists(HELD):
        for l in open(HELD):
            w = l.split('#')[0].split()
            if w:
                listed.add(w[0].replace(':', '').upper())
    why = {a: r for a, r in why.items() if a not in listed}
    if not why:
        return False
    names = held_names(set(why), rev)
    cur = open(HELD).read() if os.path.exists(HELD) else \
        '# Held C: routines whose C failed a gate (A5 or A7). The hybrid leaves each\n' \
        '# out of its routine table, with every routine whose C calls it, so the\n' \
        '# original runs in its place. The gate runner adds lines; a writer removes\n' \
        '# one in the commit that fixes the C. Format: BB:AAAA Name  # why\n'
    lines = ''.join('%s:%s %s  # %s\n' % (a[:2], a[2:], names.get(a, '?'), r) for a, r in sorted(why.items()))
    with open(HELD, 'w') as f:
        f.write(cur if cur.endswith('\n') else cur + '\n')
        f.write(lines)
    subprocess.run(['flock', LAND, 'git', 'add', HELD], cwd=ROOT)
    msg = 'Hold %s: %s' % (', '.join('%s %s' % (names.get(a, '?'), a) for a in sorted(why)),
                           '; '.join(sorted(set(why.values()))))
    p = subprocess.run(['flock', LAND, 'git', 'commit', '-q', '-m', msg[:400], '--', HELD], capture_output=True, text=True, cwd=ROOT)
    if p.returncode:
        log('hold commit failed: %s' % p.stderr.strip())
        return False
    log('held: %s' % msg)
    with open(out, 'a') as f:
        f.write('held:\n' + lines)
    return True


def commit_ledgers(state):
    rc, text = job(LIGHT, 'python3 tools/aladdin/progress.py', lock=True, timeout=900)
    if rc:
        log('progress.py failed (rc %d): %s' % (rc, text.strip().splitlines()[-1:] or ''))
    changed = [p for p in LEDGERS if git('status', '--porcelain', '--', p)]
    if not changed:
        state['batches'] = 0
        return
    p = subprocess.run(['flock', LAND, 'git', 'commit', '-q', '-m', 'Gate ledgers at %s (%d batch(es))' % (
        state.get('done', '')[:7], state.get('batches', 0)), '--'] + changed, capture_output=True, text=True, cwd=ROOT)
    if p.returncode:
        log('ledger commit failed: %s' % p.stderr.strip())
        return
    log('ledgers committed: %s' % git('log', '-1', '--format=%h %s'))
    state['batches'] = 0


def batch(state, head):
    done = state.get('done')
    short = head[:7]
    paths = git('diff', '--name-only', done, head).splitlines() if done else ['(first run)']
    if done and all(p.startswith(QUIET) for p in paths):
        return 'quiet'
    out = os.path.join(D, 'batch-%s.log' % short)
    log('%s: batch from %s (%d paths)' % (short, (done or '-')[:7], len(paths)))
    rc, text = job(BUILD, 'sh -c "python3 tools/aladdin/build.py || exit 3; python3 tools/aladdin/image.py"',
                   lock=True, timeout=3600, out=out)
    image_ok = '1' if rc == 0 else '0'
    if rc not in (0, 1):
        log('%s: build failed (rc %d), see %s' % (short, rc, out))
        return False
    if not done:
        log('%s: no earlier revision: run t3.py --qualify --run by hand once, then the runner takes over' % short)
        return True
    rc, text = job(LIGHT, 'python3 tools/aladdin/t3.py --affected %s' % done, env={'ALADDIN_GATE_REV': head}, out=out)
    if rc:
        log('%s: --affected failed, see %s' % (short, out))
        return False
    got = dict(l.split(' ', 1) if ' ' in l else (l, '') for l in text.splitlines() if l.startswith(('routines', 'scripts')))
    addrs = got.get('routines', '').split()
    if got.get('harness') == 'yes':
        state['regress'] = head
        log('%s: harness changed: whole-game A7 queued for when no commit lands for %d min' % (short, REGRESS_IDLE // 60))
    if not addrs:
        log('%s: no routine affected' % short)
        return True
    log('%s: qualifying %d routine(s)%s' % (short, len(addrs), ', new or changed scripts: ' + got['scripts'] if got.get('scripts') else ''))
    t0 = time.time()
    rc, text = job(QUALIFY, 'python3 tools/aladdin/t3.py --qualify --run %s' % ' '.join(addrs),
                   env={'ALADDIN_GATE_REV': head, 'ALADDIN_IMAGE_OK': image_ok}, timeout=QUALIFY_TIMEOUT, out=out)
    tiers = re.findall(r'^([0-9A-F]{6}) (\w+): (T3|T4|below T3 \(T\d\))', text, re.M)
    log('%s: qualified in %.0fs: %d at T3 or T4, %d below' % (
        short, time.time() - t0, sum(1 for t in tiers if t[2] in ('T3', 'T4')), sum(1 for t in tiers if t[2].startswith('below'))))
    if not tiers:
        log('%s: qualify gave no verdicts (rc %d), see %s' % (short, rc, out))
        return False
    rc, text = job(LIGHT, 'python3 tools/aladdin/t3.py --failures %s' % ' '.join(addrs), env={'ALADDIN_GATE_REV': head}, out=out)
    why = {}
    for l in text.splitlines():
        w = l.split()
        if w and w[0] == 'a5':
            for a in w[1:]:
                why[a] = 'A5 DIVERGENT at %s' % short
    for l in text.splitlines():
        w = l.split()
        if not w or w[0] != 'red':
            continue
        script, cands = w[1], set(w[2:]) - set(why)
        base = set(why)
        if base and green(head, script, base, out):
            log('%s: %s green with the A5 holds' % (short, script))
            continue
        if not cands:
            log('%s: %s red, no routine of the batch left to blame' % (short, script))
            continue
        try:
            got = culprits(head, script, base, cands, out)
        except RuntimeError as e:
            log('%s: %s' % (short, e))
            continue
        if got is None:
            log('%s: %s red with all %d of the batch\'s routines held: not their C' % (short, script, len(cands)))
            continue
        for a in got:
            why[a] = 'A7 %s red at %s' % (script, short)
    if why:
        hold(head, why, out)
    return True


def regress(state):
    """The off-path whole-game check: a full A7 at the queued revision into
    the A7 ledger (its rows are then measured on the new harness); red
    scripts are reported, not held."""
    rev = state['regress']
    log('%s: whole-game A7 (regression check) starts' % rev[:7])
    out = os.path.join(D, 'regress-%s.log' % rev[:7])
    t0 = time.time()
    rc, text = job(QUALIFY, 'python3 tools/aladdin/t3.py --image --full', env={
        'ALADDIN_GATE_REV': rev}, timeout=QUALIFY_TIMEOUT, out=out)
    red = [l.split()[0] for l in text.splitlines() if re.search(r': (DIVERGENT|FAILED)$', l.strip()) and l.split()]
    log('%s: whole-game A7 done in %.0fs: %s' % (rev[:7], time.time() - t0,
        'no red script' if not red and 'IDENTICAL' in text else '%d red: %s (see %s)' % (len(red), ' '.join(sorted(set(red))), out)))
    state.pop('regress', None)


def prune(head):
    """Drop objdiff's compiled trees of revisions other than head (they are rebuilt when needed)."""
    w = os.path.join(ROOT, 'build', 'aladdin', 'objdiff')
    for n in os.listdir(w) if os.path.isdir(w) else []:
        if n != head and re.fullmatch(r'[0-9a-f]{40}', n):
            subprocess.run(['rm', '-rf', os.path.join(w, n)])


def main(argv):
    once = '--once' in argv
    every = int(argv[argv.index('--ledger-every') + 1]) if '--ledger-every' in argv else 3
    if not CPU_RUN or not os.access(CPU_RUN, os.X_OK) or not LOCK or not LAND:
        print('runner: set ALADDIN_CPU_RUN (the CPU queue command), ALADDIN_BUILD_LOCK (the build lock file) and ALADDIN_LAND_LOCK (the landing lock file)')
        return 2
    os.makedirs(D, exist_ok=True)
    state = load_state()
    if '--from' in argv:
        # The revision the ledgers were last measured on: the first batch is everything since.
        state = {'done': git('rev-parse', argv[argv.index('--from') + 1], check=True), 'at': time.time(), 'batches': 0}
        save_state(state)
    waiting = None
    log('runner start at %s, done %s' % (git('rev-parse', '--short', 'HEAD'), state.get('done', '-')[:7]))
    while True:
        head = git('rev-parse', 'HEAD')
        if head == state.get('done'):
            if state.get('regress') and time.time() - state.get('at', 0) > REGRESS_IDLE and not dirty():
                regress(state)
                save_state(state)
                continue
            if state.get('batches') and (once or time.time() - state.get('at', 0) > 600):
                commit_ledgers(state)
                save_state(state)
                continue
            if once:
                return 0
            time.sleep(60)
            continue
        d = dirty()
        if d:
            if waiting != d:
                log('waiting: uncommitted changes in %s' % ' '.join(l[3:] for l in d[:5]))
                waiting = d
            if once:
                return 1
            time.sleep(60)
            continue
        waiting = None
        ok = batch(state, head)
        if not ok:
            if once:
                return 1
            time.sleep(300)  # a failed build or gate: try again later, not in a tight loop
            continue
        state.update(done=head, at=time.time(), batches=state.get('batches', 0) + (ok != 'quiet'))
        save_state(state)
        prune(head)
        if state['batches'] >= every:
            commit_ledgers(state)
            save_state(state)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
