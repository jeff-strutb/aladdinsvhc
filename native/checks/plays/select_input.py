#!/usr/bin/env python3
"""select_input.py PART STAGE [OUT]: the --input prefix that reaches level PART STAGE through the title's level select, for a build
whose title menu has no options screen. Run the game with ALADDIN_DEV_CHEAT=1 (system/dev_start.c: the level select's cheat set at boot).
Start at 2400 (skips the story), Down and Up at 2700 and 2715 (the cursor's moves keep the menu from handing over to the demo, as
the old prefix's visit to the options screen did), then R held with Up PART times and Right STAGE times (the level select, shown as
PART+1-STAGE+1), then Start (Game Start) at update 3900. The level plays from about update 3934; the bot's keys start at 4100.
The old prefix, level_select_input.py, went through the options screen and its cheat code and is kept as it was."""
import sys
part, stage = int(sys.argv[1]), int(sys.argv[2])
out = sys.argv[3] if len(sys.argv) > 3 else '/dev/stdout'
L = ['2400 key=0D', '2402']
def press(keys, at, hold=3):
    L.append('%d %s' % (at, keys)); L.append('%d' % (at + hold))
press('key=28', 2700); press('key=26', 2715)
L.append('3700 key=57')
t = 3720
for k in ['key=26'] * part + ['key=27'] * stage:
    L.append('%d key=57 %s' % (t, k)); L.append('%d key=57' % (t + 3)); t += 10
L.append('%d' % (t + 5))
press('key=0D', 3900)
open(out, 'w').write('\n'.join(L) + '\n') if out != '/dev/stdout' else print('\n'.join(L))
