#!/bin/bash
# pointers.sh: every pointer in the game state is one the snapshot writes as a code (FRAMERATE.md 10.2). The compiler's own record
# layouts give the members of every type GameState holds; the pointer members must be exactly the list system/snapshot.c walks:
# GameState: chain_head, chain_tail, free_pool, free_pool_b, draw_list; Actor: next, code; VramQueueEntry: from. A new pointer
# member in any type g holds ends this check with its name. Run from the repository root. Exit 0: the lists are the same.
set -u
src=native/aladdin/src
tmp=$(mktemp -d)
cat > "$tmp/t.c" <<'C'
#include "system/state.h"
GameState probe;
C
clang -std=gnu11 -fsyntax-only -I"$src" -I"${NSHVC_ROOT:-../nativeshvc}/include" -Xclang -fdump-record-layouts-complete "$tmp/t.c" > "$tmp/layout.txt" 2>&1
python3 - "$tmp/layout.txt" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
recs = re.split(r'\*\*\* Dumping AST Record Layout\n', text)
types = {}                       # name -> [(member type, member name)]
for r in recs:
    lines = r.splitlines()
    if not lines: continue
    head = re.match(r'\s*0 \| (struct|union) ([A-Za-z_]\w*)$', lines[0])
    if not head: continue
    mem = []
    for l in lines[1:]:
        m = re.match(r'\s+\d+(?::\d+-\d+)? \|   (.+) (\w+)$', l)
        if m: mem.append((m.group(1), m.group(2)))
    types[head.group(2)] = mem
seen = set(); pointers = set(); stack = ['GameState']
while stack:
    t = stack.pop()
    if t in seen or t not in types: continue
    seen.add(t)
    for ty, name in types[t]:
        if '*' in ty:
            pointers.add('%s.%s' % (t, name))
        for w in re.findall(r'(?:struct|union) ([A-Za-z_]\w*)', ty):
            stack.append(w)
        for w in re.findall(r'\b([A-Z][A-Za-z0-9_]*)\b', ty):
            if w in types: stack.append(w)
known = {'GameState.chain_head', 'GameState.chain_tail', 'GameState.free_pool', 'GameState.free_pool_b', 'GameState.draw_list',
         'Actor.next', 'Actor.code', 'VramQueueEntry.from'}
print('types reached from GameState: %d; pointer members: %s' % (len(seen), ', '.join(sorted(pointers))))
if pointers != known:
    print('DIFFERS: not written as codes: %s; listed but absent: %s' % (sorted(pointers - known), sorted(known - pointers)))
    sys.exit(1)
print('the pointer members are the ones the snapshot writes as codes')
PY
rc=$?; rm -rf "$tmp"; exit $rc
