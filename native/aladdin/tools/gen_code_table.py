#!/usr/bin/env python3
"""gen_code_table.py: writes (or with --check, compares) src/actor/code_table.c, the table of every function an actor's record can
run in (Actor.code): each start, second and third entry of every row of the kinds_*.c tables and every function ActorRunAs is
given. The snapshot (docs/aladdin/native/FRAMERATE.md section 10) writes an actor's code as the hash of its name. Run from the
repository root: python3 native/aladdin/tools/gen_code_table.py [--check]"""
import glob, os, re, sys

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'src')

def names():
    found = set()
    for f in sorted(glob.glob(os.path.join(SRC, 'actor', 'kinds*.c'))):
        for m in re.finditer(r'\{\s*0x[0-9A-Fa-f]+\s*,([^}]*)\}', open(f).read()):
            for p in [x.strip() for x in m.group(1).split(',')][:3]:
                if re.match(r'^[A-Za-z_]\w*$', p) and p != 'NULL':
                    found.add(p)
    for f in sorted(glob.glob(os.path.join(SRC, '**', '*.c'), recursive=True)):
        for m in re.finditer(r'ActorRunAs\(\s*\w+\s*,\s*([A-Za-z_]\w*)\s*,', open(f).read()):
            found.add(m.group(1))
    return sorted(found)

def text():
    out = ['/*\n * code_table.c: every function an actor record can run in, by name (made by tools/gen_code_table.py; its --check\n'
           ' * fails when a row of a kinds_*.c table or an ActorRunAs names a function not here). The snapshot writes\n'
           ' * Actor.code as the hash of the name, which holds across builds.\n */\n',
           '#include "actor/actor_own.h"\n#include "actor/code_table.h"\n']
    out.append('\n/* each declared here again (a repeated declaration is the same one): the headers that declare them are many */\n')
    for n in names():
        out.append('int %s(Actor *a);\n' % n)
    out.append('\nconst ActorCodeEntry actor_code_table[] = {\n')
    for n in names():
        out.append('    { "%s", %s },\n' % (n, n))
    out.append('    { 0, 0 }\n};\n')
    return ''.join(out)

if __name__ == '__main__':
    path = os.path.join(SRC, 'actor', 'code_table.c')
    t = text()
    if '--check' in sys.argv:
        if open(path).read() != t:
            print('code_table.c is not what tools/gen_code_table.py makes: run it'); sys.exit(1)
        print('code_table.c: %d functions, the rows and ActorRunAs name no other' % len(names()))
    else:
        open(path, 'w').write(t)
        print('wrote %s: %d functions' % (path, len(names())))
