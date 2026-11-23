#!/bin/sh
# state.sh BIN: the runahead check, FRAMERATE.md 9.8. The same headless build is run twice over the boot and every play (as
# view/same.sh does), once with ALADDIN_RUNAHEAD=0 and once with ALADDIN_RUNAHEAD=1, each logging the real world's hash after
# every update (ALADDIN_DEV_STATE_HASH=1: g, the kept video parts, the registered statics, the layers' map words) besides the
# developer's trace (the player, every live actor with its own bytes, the video fields). The two logs must be the same line
# for line: the ahead steps leave the real world as it was. Run from the repository root. SAME_JOBS, SAME_OUT as same.sh.
# Exit 0: every run the same and every run exited 0.
set -u
[ $# = 1 ] || { echo "usage: state.sh HEADLESS_BIN" >&2; exit 2; }
SAME_OUT=${SAME_OUT:-$(pwd)/build/aladdin/native/state-ra} \
SAME_BASE_ENV="ALADDIN_RUNAHEAD=0 ALADDIN_DEV_STATE_HASH=1" SAME_NEW_ENV="ALADDIN_RUNAHEAD=1 ALADDIN_DEV_STATE_HASH=1" \
  exec sh native/checks/view/same.sh "$1" "$1"
