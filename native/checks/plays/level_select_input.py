#!/usr/bin/env python3
"""level_select_input.py PART STAGE [OUT]: an --input file for the native game: Start at 2400 (skips the story), the title's
Options (Down Down Start), the cheat code on pad 2 (controller=1: L R Start Select X Y A B, one press each within 32 frames),
Exit (Down x3, A), then at the title R held with Up PART times and Right STAGE times (the level select, shown as PART+1-STAGE+1),
then Start (Game Start) at update 3900. The level plays from about update 4000. Verified 2026-11-20: part 2 stage 1 = "3-2"."""
import sys
part, stage = int(sys.argv[1]), int(sys.argv[2])
out = sys.argv[3] if len(sys.argv) > 3 else '/dev/stdout'
L = ['2400 key=0D', '2402']
def press(keys, at, hold=3):
    L.append('%d %s' % (at, keys)); L.append('%d' % (at + hold))
press('key=28', 2700); press('key=28', 2715); press('key=0D', 2730)
t = 3100
for b in [0x10, 0x20, 0x80, 0x40, 0x08, 0x04, 0x02, 0x01]:
    L.append('%d controller=1,%X' % (t, b)); L.append('%d controller=1,0' % (t + 3)); t += 10
press('key=28', 3300); press('key=28', 3310); press('key=28', 3320); press('key=58', 3340)
L.append('3700 key=57')
t = 3720
for k in ['key=26'] * part + ['key=27'] * stage:
    L.append('%d key=57 %s' % (t, k)); L.append('%d key=57' % (t + 3)); t += 10
L.append('%d' % (t + 5))
press('key=0D', 3900)
open(out, 'w').write('\n'.join(L) + '\n') if out != '/dev/stdout' else print('\n'.join(L))
