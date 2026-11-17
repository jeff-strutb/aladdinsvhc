# Actors: their yields in the C and the oracles

The game's actors (the player, enemies, pickups, effects) are a second kind
of coroutine beside the tasks ([TASKS.md](TASKS.md)). Everything under "What
the game does" was measured on a lockstep trace of the first level
(2026-11-01) and read from the code.

## What the game does

| | |
|---|---|
| Records | the player's at $08DB, then $50-byte records from $092B up to $0B5B, each the actor's direct page while it runs |
| Record fields | $00 state (0 free, 1 waiting, 8 running, $0C and up: to start), $01 frames left to wait, $02-$05 where it resumes (P, the PC, the bank), $06 kind, $09 flags (bit 7: a routine of its kind runs each frame it waits) |
| Loop | $83:9475 (the player) and $83:94A4 (the others), each called by JSL, run at S = $015E after their PHD; for each record: D = the record; free: skip; waiting: count down, then resume; starting: jump through the kind's entry table |
| Start | JMP (table,X) by state and kind, to a stub that JMLs to the actor's code |
| Yield | BRK in the actor's code. The handler ($80:FFAC: JML $80:88E5) marks the record waiting one frame, pulls the BRK's frame (P, PC, bank) into $02-$05 and jumps back into the loop ($83:948B for the player, $83:94BF otherwise) |
| Resume | when the wait runs out, state 8, the loop pushes $04-$05 and $02-$03 (bank, PC, P) and returns into the actor by RTI: at the BRK's address + 2, at S = $015E, with D the record |
| End | a routine the actor calls frees the record, drops its own return (PLA PLA) and jumps into the loop at $83:94BF |

Measured: every BRK runs at S = $015E, the loop's own level: an actor yields
only from its own code, never from a routine it called. An actor is a
stackless coroutine: nothing of it is on the stack while it waits; its
whole state is its record and its resume address.

## In the C

An actor's code is a routine entered by a jump, whose C yields with
`actor_yield()` where the original has BRK. `actor_yield()` makes the
BRK's effects (the bank, PC + 2 and P pushed, I set, D clear, at $00:FFAC)
and stops the C until the loop resumes the actor; it then returns with the
registers the RTI restored. The handler and the loop are ordinary code.

## In the oracles

The same pieces as a task's waits:

- **Entry by jump.** An actor routine's first piece starts where the loop
  jumps to it (a routine tagged `@actor`), with no return of its own.
- **Yield.** A piece ends at the BRK's landing ($00:FFAC after a BRK), with
  the C stopped in `actor_yield` and the registers there compared. Its
  coroutine (A7) or watches (A5) are parked under the actor's record (D).
- **Resume.** When an RTI lands at a parked actor's resume address, with
  its S and its D, both sides open a record for the next piece and the C
  goes on.
- **End.** A piece ends where the actor's code leaves for the loop (S back
  at the actor's level and the PC in the loop), the C having left the same
  way (`cpu.unwound`).

Proofs as for tasks: planted bugs (a wrong resume address, a dropped push,
a wrong wait) turn A5 and A7 red, and every new piece reports pieces
compared above 0.

## A routine that ends its actor

A routine the actor's code calls (by JSR, so not @actor) may end the actor
itself: it pulls its own return address (PLA PLA, S back at the actor's
level, its exit level) and jumps into the actor loop ($83:96BE,
ActorEndOutOfRange). Its C does the same pulls and ends in actor_leave; a
C caller pushes the return address first (as for a call that can wait) and
pops it when the call returns. Both oracles end such a call where the PC
reaches the loop at its exit level. On rand_level1_1, ActorEndOutOfRange's
214 calls are identical on A5 and the lockstep run identical; one pull
left out, the free left out, or the other loop entry each fail both
(2026-11-01).

## Past its exit level

A call may pull more than its own return address before it jumps into
the loop: KindA1Touch ($84:A9C7), reached by JSL from its wait stub
($83:940D), which the loop reached by JSR, pulls both (five bytes) and
jumps to ActorEndPlaced at the loop's level. Both oracles end every call
whose exit level S has reached or passed there, and the actor loop's own
call where S is back at the loop's level (as at its hand-off). The C does
the same pulls and ends in actor_leave.

A call that pulls its own return address and then yields by BRK or COP
at its exit level (PlayerCrouch, $81:D361) ends at the yield when no piece
of its actor is open (the piece began before the run, from a loaded state,
and runs as original code), as its C stops in actor_yield: A5 ends its
watch there, A7 its record, and the hybrid drops its C (nothing resumes
it as C; what follows runs as original code on both sides). Where its
actor's piece is open it ends with the piece, as before.

A C caller of a routine that drops its return address must have pushed
that address as the original's JSR or JSL does: call_routine pushes none.
ActorKindA1Wait ($83:940D) makes its JSL's frame with jsl_frame before
KindA1Touch's C, and pulls it when KindA1Touch returns.

A routine the actor's code called that drops its return and jumps to an
@actor entry inside the piece (ActorFallTileTest, $83:C903, to
ActorEndPlaced) goes on in the same piece, as the original's jump does: its
C's actor_leave runs that entry's C on when it runs as an actor's piece.
Swapped in on its own (the piece running as original code), its call ends
at the jump, where the original side ends it, and a piece opens there.

## Entered by the loop's RTI at another entry

An actor's code may rewrite its record's resume address to another of its
entry points (kind $08's states, $A5:E5B6 and up): the loop's RTI then lands
on an @actor entry where no parked piece resumes. A piece starts there on
both sides, as at the loop's jump; the parked piece of that record is
dropped at its next yield or end. Where a parked piece does resume at that
same address, it resumes, and no new piece opens.
