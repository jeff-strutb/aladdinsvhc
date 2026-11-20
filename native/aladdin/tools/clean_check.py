#!/usr/bin/env python3
"""clean_check.py PATH...: the plan's clean check (docs/aladdin/native/PLAN.md
section 4, decision 2) over the native game's files: nshvc-find-hardware
reports nothing for them, and their code (comments left out) has none of
what the finder does not look for: the SPC700's register file, flag helpers,
pushed return frames, the decomp's yields and hand-offs, goto, the hardware's
register names, or a 24-bit SNES address outside data/. Prints each finding;
exits 1 when there is one. NSHVC_ROOT names the engine's checkout."""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
def engine_root():
    if os.environ.get('NSHVC_ROOT'):
        return os.environ['NSHVC_ROOT']
    d = REPO
    while d != os.path.dirname(d):          # the engine's checkout beside the repository (or beside a parent)
        c = os.path.join(os.path.dirname(d), 'nativeshvc')
        if os.path.isfile(os.path.join(c, 'tools', 'nshvc-find-hardware')):
            return c
        d = os.path.dirname(d)
    sys.exit('clean_check: set NSHVC_ROOT to the engine checkout')


NSHVC = engine_root()
HW = [l.split()[1] for l in open(os.path.join(REPO, 'src/aladdin/include/hw.h')) if l.startswith('#define')]

CHECKS = [
    (r'\bspc\.[a-z]', 'the SPC700 register file'),
    (r'\b(IF_[A-Z]+|nz8|nz16|flag_set|cmp8_of|cmp16_of|bit8_of|bit16_of|op_adc8|op_adc16|op_sbc8|op_sbc16)\b', 'a flag helper'),
    (r'\b(jsl_frame|call_frame|jsl_call)\b', 'a pushed return frame'),
    (r'\b(task_yield|task_hand_off|actor_yield|actor_wait|actor_leave|actor_hand_off|actor_loop_call|cpu_restart|cpu_loop_forever)\b', 'a decomp yield or hand-off'),
    (r'\bgoto\b', 'goto'),
    (r'\b(' + '|'.join(sorted(HW, key=len, reverse=True)) + r')\b', 'a hardware register name'),
]
ADDR24 = re.compile(r'\b0x[0-9A-Fa-f]{6}u?\b')


def files(paths):
    for p in paths:
        if os.path.isdir(p):
            for d, _, names in os.walk(p):
                for n in sorted(names):
                    if n.endswith(('.c', '.h')):
                        yield os.path.join(d, n)
        else:
            yield p


def code_of(text):
    text = re.sub(r'/\*.*?\*/', lambda m: '\n' * m.group(0).count('\n'), text, flags=re.S)
    text = re.sub(r'//[^\n]*', '', text)
    return re.sub(r'"(\\.|[^"\\])*"', '""', text)


def main():
    paths = list(files(sys.argv[1:]))
    if not paths:
        sys.exit('usage: clean_check.py PATH...')
    found = 0
    # the finder walks directories: give it each file's directory and keep its lines for these files
    want = set(os.path.abspath(p) for p in paths)
    dirs = sorted(set(os.path.dirname(os.path.abspath(p)) for p in paths))
    out = subprocess.run([sys.executable, os.path.join(NSHVC, 'tools', 'nshvc-find-hardware')] + dirs,
                         capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        m = re.match(r'(.+?):\d+: ', line)
        if m and os.path.abspath(m.group(1)) in want:
            print(line); found += 1
    for p in paths:
        in_data = os.sep + 'data' + os.sep in os.path.abspath(p)
        for no, line in enumerate(code_of(open(p, errors='replace').read()).split('\n'), 1):
            for rx, what in CHECKS:
                if re.search(rx, line):
                    print(f'{p}:{no}: {what}: {line.strip()[:100]}'); found += 1
            if not in_data and ADDR24.search(line):
                print(f'{p}:{no}: an SNES address outside data/: {line.strip()[:100]}'); found += 1
    print(f'clean check: {len(paths)} files, {found} findings')
    return 1 if found else 0


if __name__ == '__main__':
    sys.exit(main())
