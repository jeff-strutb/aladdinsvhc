#!/usr/bin/env python3
"""convert_prefix.py OLD_DIR NEW_DIR: every play file of OLD_DIR (p*.txt, end-*.txt, boss16-2hits.txt) written to NEW_DIR with its
prefix (the lines up to update 3903) replaced by select_input.py's for the same level. The old prefix must be exactly
level_select_input.py's for the part and stage it selects (R held with Up and Right presses counted from it), or the file is refused;
the bot's keys (every line after 3903) are copied as they are."""
import glob, os, subprocess, sys
here = os.path.dirname(os.path.abspath(__file__))
old, new = sys.argv[1], sys.argv[2]
os.makedirs(new, exist_ok=True)
def gen(script, part, stage):
    r = subprocess.run([sys.executable, os.path.join(here, script), str(part), str(stage)], capture_output=True, text=True, check=True)
    return [l for l in r.stdout.split('\n') if l.strip()]
files = sorted(glob.glob(os.path.join(old, 'p*.txt')) + glob.glob(os.path.join(old, 'end-*.txt')) + [os.path.join(old, 'boss16-2hits.txt')])
for f in files:
    lines = [l for l in open(f).read().split('\n') if l.strip()]
    pre = [l for l in lines if int(l.split()[0]) <= 3903]
    post = [l for l in lines if int(l.split()[0]) > 3903]
    part = sum(1 for l in pre if ' key=26' in l and 3720 <= int(l.split()[0]) < 3900)
    stage = sum(1 for l in pre if ' key=27' in l and 3720 <= int(l.split()[0]) < 3900)
    if pre != gen('level_select_input.py', part, stage):
        sys.exit('%s: its prefix is not level_select_input.py %d %d; not converted' % (f, part, stage))
    open(os.path.join(new, os.path.basename(f)), 'w').write('\n'.join(gen('select_input.py', part, stage) + post) + '\n')
    print('converted', os.path.basename(f), part, stage, '+', len(post), 'lines')
