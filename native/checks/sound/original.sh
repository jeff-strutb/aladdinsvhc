#!/bin/sh
# original.sh [NAME...]: the original's runs for sound_check, in snesbox (the
# decomp's harness on the project's bsnes core), each writing
# build/aladdin/sound-check/runs/NAME/apu.bin (--apulog) and sound.raw (the
# DSP's 32,040 Hz output). No NAME: all 41. Run from the repository root.
#
# Each run: power-on; at frame 400 (the sample sets and the first song in) the
# S-CPU is parked at 80:BB5C, a branch to itself, so the game sends nothing
# more; then the test:
#   song_10 ... song_36  the game's own SongPlay (81:FA23) of the song, then 60 s
#   effects              each effect entry 1-80 by port 0 after a $F0, 2.5 s each
#                        (entry 0 is never run: the driver takes no command 0)
#   mixed                song $19, entries 1-80 over it 20 frames apart, then
#                        $F3 $F4 $F8 $F7, fades out ($F5 speed $0400), in ($F6
#                        $0800), $F5 speed 0, $F1 $FB $FC $FD, entry $31, $F2
#                        $FB $F0
# A song is started by SongPlay and never by a port write first: SongPlay
# waits for the driver's command count to equal the game's.
# ALADDIN_CPU_RUN: the machine's CPU queue (each run goes through it).
# SNESBOX (default build/aladdin/sound-check/snesbox: build a private one,
# tools/aladdin/snesbox's Makefile), CORE, ROM, JOBS (default 6).
M=$(pwd)
D=build/aladdin/sound-check/runs
SNESBOX=${SNESBOX:-$M/build/aladdin/sound-check/snesbox}
CORE=${CORE:-$M/tools/toolchains/bsnes/bsnes_libretro.so}
ROM=${ROM:-$M/reference/aladdin/Aladdin (USA).sfc}
head() { printf 'wait 400\ncall 80BB5C noreturn frames=1\nwait 2\n'; }
for s in $(seq 16 54); do
  n=$(printf 'song_%02X' $s); mkdir -p $D/$n
  { head; printf 'call 81FA23 long keep db=80 p=30 a=%02X frames=600\nwait 3600\n' $s; } > $D/$n/run.box
done
mkdir -p $D/effects
{ head; printf 'apuport 0 F0\nwait 10\n'; for e in $(seq 1 80); do printf 'apuport 0 F0\nwait 2\napuport 0 %02X\nwait 150\n' $e; done; } > $D/effects/run.box
mkdir -p $D/mixed
{ head; printf 'call 81FA23 long keep db=80 p=30 a=19 frames=600\nwait 300\n'
  for e in $(seq 1 80); do printf 'apuport 0 %02X\nwait 20\n' $e; done
  printf 'apuport 0 F3\nwait 120\napuport 0 F4\nwait 240\napuport 0 F8\nwait 240\napuport 0 F7\nwait 120\n'
  printf 'apuport 2 00\napuport 3 04\napuport 0 F5\nwait 400\n'
  printf 'apuport 2 00\napuport 3 08\napuport 0 F6\nwait 300\n'
  printf 'apuport 2 00\napuport 3 00\napuport 0 F5\nwait 60\n'
  printf 'apuport 0 F1\nwait 60\napuport 0 FB\nwait 300\napuport 0 FC\nwait 60\napuport 0 FD\nwait 60\n'
  printf 'apuport 0 31\nwait 10\napuport 0 F2\nwait 60\napuport 0 FB\nwait 120\napuport 0 F0\nwait 60\n'; } > $D/mixed/run.box
[ $# -gt 0 ] && names="$*" || names=$(ls $D)
export SNESBOX CORE ROM D
[ -n "$ALADDIN_CPU_RUN" ] || { echo "original.sh: set ALADDIN_CPU_RUN to the machine's CPU queue"; exit 2; }
printf '%s\n' $names | xargs -P "${JOBS:-6}" -I{} $ALADDIN_CPU_RUN -j 1 -m 1G -- sh -c \
  'cd "$D/{}" && timeout 900 "$SNESBOX" --rom "$ROM" --core "$CORE" --script run.box --out . --apulog apu.bin --sound sound.raw > log.txt 2>&1; echo "{} exit $?"'
