#!/bin/sh
# boss-flow.sh 16|17 [UPDATES [SHOTS_DIR [EVERY [FROM]]]]: replays, in one command, a play that lands one real stomp on the
# level's boss, with the boss's health set by the developer's patch (ALADDIN_DEV_OWN, system/dev_bot.c) so that stomp is the
# last. The fights are NOT won in full: this checks the flow after the last hit (the boss's defeat, the level's end, the next
# screens, the ending). Arguments after the level are run.sh's. Run from the repository root with ALADDIN_CPU_RUN set.
#
#   16  Jafar's first form (kind $98, level 16 = part 5 stage 2), native/checks/plays/boss16-2hits.txt. Its first stomp lands at
#       update ~8379 (hits 6 -> 5 in the play). At update 8000 every live kind $98 record gets own byte 1 (its hits left) = 1,
#       so that stomp takes the last hit. Default 10500 updates: defeat 8379-8436, the exit 8540, the stage-clear screen 8700,
#       level 17 loads at 9660 (the play's jump key ends the stage-clear screen).
#   17  the snake (kind $73, level 17 = part 5 stage 3), native/checks/plays/p5-3.txt. Its stomp lands at update 5577 (health
#       7 -> 6 in the play). At update 5500 every live kind $73 record gets own byte 1 (its health) = 0, so that stomp takes
#       it below 0, which is beaten. Default 40000 updates: the chain falls 5578-5720, the finale to 8740, then the ending's
#       story (level $39) from 8740, the credits (level $1C) from 26560 and "The End" held to the end of the run.
set -e
case "$1" in
  16) file=boss16-2hits.txt; patch=8000:98:1:1; last=10500 ;;
  17) file=p5-3.txt; patch=5500:73:1:0; last=40000 ;;
  *) echo "usage: boss-flow.sh 16|17 [UPDATES [SHOTS_DIR [EVERY [FROM]]]]" >&2; exit 2 ;;
esac
shift
updates=${1:-$last}
[ $# -gt 0 ] && shift
dir=$(dirname "$0")
ALADDIN_DEV_OWN=$patch exec sh "$dir/run.sh" "$dir/$file" "$updates" "$@"
