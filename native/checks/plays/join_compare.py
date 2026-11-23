#!/usr/bin/env python3
"""join_compare.py BASE_TRACE NEW_TRACE: the proof that the game without the level-end screen is the original's on both sides of it
(docs/aladdin/native/CHECK.md "The level end's removal").

BASE_TRACE: the original behaviour (base-b36b712) running the OLD play; NEW_TRACE: the new build running the play moved back by
convert_join.py. Both are developer traces (ALADDIN_DEV_TRACE=1, ALADDIN_DEV_ACTORS=2, ALADDIN_DEV_VIDEO=1). For each stay on level $24
in the base trace (u0 to B-1; B the next screen's first update; N = u0 + 1 after a story card ($30), u0 before the next part's level):
  before:  every update before u0 has the same lines in both traces;
  after:   every update from B on has, in the new trace at update (u - L), L = B - N, the same lines as the base's at u (the update
           number apart), for as long as the traces run, or until a stay of the base trace begins (the next stay is compared in its turn);
and prints the first update at which they differ, and the fields that differ, if any. Exit 0: all the same."""
import re, sys

def load(path):
    t, cur = {}, None
    for l in open(path).read().split('\n'):
        m = re.match(r'u=(\d+) (.*)', l)
        if m:
            cur = int(m.group(1)); t[cur] = [m.group(2)]
        elif cur is not None and l:
            t[cur].append(l)
    return t

def level_of(rec):
    m = re.match(r'level=(\d+) ', rec[0])
    return int(m.group(1))

def fields(a, b):
    """the fields that differ between two records' lines"""
    out = []
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            fx, fy = re.split(r'\s+', x), re.split(r'\s+', y)
            if len(fx) == len(fy):
                out += ['%s -> %s' % (p, q) for p, q in zip(fx, fy) if p != q]
            else:
                out.append('line %d: %s | %s' % (i, x[:100], y[:100]))
    if len(a) != len(b):
        out.append('%d lines vs %d' % (len(a), len(b)))
    return out

base, new = load(sys.argv[1]), load(sys.argv[2])
us = sorted(base)
runs, cur = [], None
for u in us:
    lv = level_of(base[u])
    if lv == 36:
        cur = [u, u, None] if cur is None else [cur[0], u, None]
    elif cur is not None:
        runs.append((cur[0], cur[1], u, lv)); cur = None
if cur is not None:
    runs.append((cur[0], cur[1], None, None))
bad = 0
if runs:
    u0 = runs[0][0]
    diff = [u for u in range(us[0], u0) if u not in new or base[u] != new[u]]
    print('before the first stay (update %d): updates %d..%d %s' % (u0, us[0], u0 - 1, 'identical' if not diff else
          'DIFFER, first at %d: %s' % (diff[0], fields(base[diff[0]], new.get(diff[0], []))[:6])))
    bad += bool(diff)
for i, (u0, last, b, bl) in enumerate(runs):
    n = u0 if bl not in (None, 0x30) else u0 + 1
    if b is None:
        print('stay %d-: the base trace ends on the screen; nothing after it to compare' % u0)
        break
    L = b - n
    stop = runs[i + 1][0] - 1 if i + 1 < len(runs) else max(base)
    total = min(stop, max(new) + L) - b + 1
    same, first = 0, None
    for u in range(b, b + total):
        if base[u] == new[u - L]:
            same += 1
        elif first is None:
            first = u
    print('stay %d-%d (%d updates), next screen $%02X at %d, new at %d (moved by %d): base %d..%d against new %d..%d: %d of %d updates identical%s' %
          (u0, last, last - u0 + 1, bl, b, b - L, L, b, b + total - 1, b - L, b + total - 1 - L, same, total,
           '' if first is None else '; first difference at base update %d' % first))
    if first is not None:
        bad += 1
        lst = {}
        for u in range(first, b + total):
            if base[u] != new[u - L]:
                for f in fields(base[u], new[u - L]):
                    lst.setdefault(f.split('=')[0] if ' -> ' in f else f[:30], []).append(u)
        for k, v in sorted(lst.items(), key=lambda kv: kv[1][0])[:12]:
            print('    field %s: %d updates differ, first at base update %d, e.g. %s' % (k, len(v), v[0], fields(base[v[0]], new[v[0] - L])[:3]))
sys.exit(1 if bad else 0)
