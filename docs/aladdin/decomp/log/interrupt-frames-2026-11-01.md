# An interrupt inside running C, and calls from a routine's exit level (2026-11-01)

## The interrupt overwrote the C's own pushes

ActorOutOfGround ($81:B4F3) was IDENTICAL in A5 on every call and
DIVERGENT in A7: the player 24 pixels too high from rand_level1_3 frame 45
(the same in seven other scripts). Logging the C's own steps in both runs
found the call where they parted. The tile lookup it makes (TileAtPlace,
$81:B880) keeps the tile's behaviour byte on the stack across two
instructions (PHA ... PLA). In A7 the PLA read back 1 where 0 was pushed.

When the hybrid replays an interrupt in the middle of a C call, the core
runs the handler with the original's registers, S included. The C makes
no frames for its calls to C, so its S stands higher than the original's.
The handler's frame, below the original's S, can lie on the C's live
stack, and it overwrote the byte the C had pushed. The original never sees
that: its handler stacks below its own pushes. A5 cannot see it either:
the calls it judges are never interrupted.

replay_to now keeps the 128 bytes below the original's S before such an
interrupt and puts back, after the handler, those at or above the C's S
that it changed.

| C, rand_level1_3 | A7 lockstep |
|---|---|
| as filed, before the change | DIVERGENT from frame 45 |
| as filed, after | IDENTICAL, 4,060 frames |
| ActorOutOfGround rising 16 for 15 | DIVERGENT (frame 1733) |
| TileAtPlace pushing the wrong byte | DIVERGENT (the hybrid derails) |

## A call made from the exit level is not the routine's return

The rule "an RTS or RTL back to the exit level ends the call wherever it
lands" (inline arguments, above) ended $83:96BE ActorEndOutOfRange at the
return of a JSR it makes after pulling its own return address. Both
oracles now count the calls a routine makes from its exit level, and the
return of such a call is nested, not the end. TaskCreate still IDENTICAL,
and still DIVERGENT when it steps its return by 4.

## Reads the C cannot make the same: the open bus and cycle-dependent registers

The act 8 scene's script (rand_stage_a8_s0) has VramCopy ($81:F5DC) copy
table entries the game indexes mid-record, so it copies from garbage
sources: the whole register window, the open bus at $6000-$7FFF, banks
$40 and $70 (22fb42 found the sources). Its C is the instruction-exact
copy; the oracles could not judge it:

- A5 aborted the C at the first read of an unmapped address (no value to
  give it). It now records the original's reads of the open bus as it does
  register reads, and answers the C's in order.
- A7 read those addresses live. The open bus reads back the CPU's last bus
  byte, an instruction byte in the original, which the C does not fetch;
  and the H/V counters, their latch, RDNMI, TIMEUP and HVBJOY read what
  the cycle of the read makes them, which the C does not reproduce (the
  standard is not cycle exact). The original side now records every
  read's value beside its op, and the hybrid answers these reads with the
  original's value at their matching read: the open bus on this board
  ($2000-$20FF, $2181-$3FFF, $4000-$4015, $4018-$41FF, $4220-$42FF, each
  DMA channel's $43xC-$43xE, $4380-$7FFF, the low halves of banks $40-$7D
  and $C0-$FF), the PPU's write-only registers ($2100-$2136), $2137,
  $213C-$213F and $4210-$4212. Each range was found by the read that
  still differed after the last (logging every read whose live value
  differed from the original's): $2182/$2183, then $43xC-$43xE.

| rand_stage_a8_s0 | A5 VramCopy | A7 |
|---|---|---|
| before | DIVERGENT, 6 of 544 calls | DIVERGENT from frame 19 |
| after | IDENTICAL, 544 calls | IDENTICAL, 2,407 frames |
| VramCopy one word short (planted) | DIVERGENT, 544 calls | DIVERGENT |

A gap that was there before and remains: A7 judges frames, so a C that
leaves out a register read whose value does not change what it does
passes A7; A5 catches it (JoypadRead without its HVBJOY wait: A5
DIVERGENT, "io-reads 0/9"; A7 IDENTICAL with the old checker and the new).
