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
