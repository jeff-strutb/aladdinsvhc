# Aladdin handbook

What this decomp learns about the hardware, the code and the method, one rule
per finding, each linked to the dated note that measured it.

## Toolchain

- The Stunt Race FX core traced ROM bytes only through the Super FX's CPU
  ROM window, so a cartridge with no coprocessor got an empty ROM map; patch
  0014 traces the cartridge's own ROM. Its map must take the ROM's size when
  the board gives none, or the bus does not mirror and bank $80 reads past
  the ROM ([decomp/log/bootstrap-2026-10-28](decomp/log/bootstrap-2026-10-28.md)).
- asar evaluates expressions left to right with no operator precedence:
  `!task_state+$18*2` is ($6F+$18)*2. Parenthesise: `!task_state+(24*2)`.
- asar 1.91 encodes all 268 65816 instruction forms and all 256 SPC700
  opcodes exactly, given explicit operand sizes.

## Code

- No C-compiler stack frames: hand-written assembly.
- Reset jumps to $80:8007; the game runs from the FastROM banks.
- The SPC700 program is uploaded by index (X, a multiple of 3) from the
  pointer table at $85:8000: driver (0), two sample sets (3, 6), empty
  slots ($09-$2D), then songs, each one block to APU $0D20. Lists may cross
  banks (the upload steps the bank when Y reaches $8000).

- Tasks end by jumping to TaskExit ($81:BD20, JMP or JML), never by
  calling it: the per-call check never watches it; a unit call marked
  noreturn judges it ([decomp/log/tasks-2026-10-28](decomp/log/tasks-2026-10-28.md)).
- Each task has a $38-byte stack under the top in its slot ($0162, $019A
  ... $02EA); the scheduler's own runs above them to $0326.

## Oracle

- A routine's C runs on a coroutine of its own; while it runs it is the
  S-CPU's thread (core patch 0015), or a chip its bus accesses synchronize
  with returns to the S-CPU's own thread, not to the C. The checker must
  call the core's export to get that thread's address; storing the export
  itself as the address wrote into code and died with SIGBUS
  ([decomp/log/tasks-2026-10-28](decomp/log/tasks-2026-10-28.md)).
- A call whose task waits is judged in pieces: entry to the wait, and each
  resume to the next wait or the return, on both oracles.
- Frames compare each task's stack above its own live S. The one-stack
  rule (the $200 below the S at the frame's end) skipped every task's stack
  whenever a frame ended in the scheduler's idle loop; a dropped push in
  TaskWait is caught only with the per-stack rule.
- A routine that steps its own return address (TaskCreate, over the five
  bytes after its JSL) ends where its RTL lands at its exit level. Its C
  does the RTL itself and sets `cpu.unwound`; both oracles compare where it
  lands. A7 catches every planted bug in it; A5 rates a call an interrupt
  overlapped as inconclusive
  ([decomp/log/inline-args-2026-11-01](decomp/log/inline-args-2026-11-01.md)).
- PLB, PLD and PLA at a routine's end set N and Z from the pulled value:
  the exit flags are the pull's, not the last arithmetic's.
- A task's stack is (top - $35, top + 3]: its own code runs with S up to
  three above the top, once the scheduler's RTL has pulled the entry
  ([decomp/log/inline-args-2026-11-01](decomp/log/inline-args-2026-11-01.md)).
- An interrupt the hybrid replays inside running C stacks its frame at
  the original's S, which can lie on the C's own live stack (its calls to
  C push nothing): the bytes it overwrites at or above the C's S are put
  back after it. A5 never sees this; A7 does
  ([decomp/log/interrupt-frames-2026-11-01](decomp/log/interrupt-frames-2026-11-01.md)).
- A return to a routine's exit level from a call it made from that level
  (after pulling its own return address) is nested, not its end.
- A task's start is where the scheduler's RTL lands with S three above
  the running task's stack top. Both oracles open a call there that never
  returns, judged in pieces at its waits, its exit or its exec; the
  original side must keep the calls a parked call made from its exit
  level (a task's level is its exit level), or the first return to that
  level after a resume splits the call. On attract, rand_pw1_1, rand_pw7_1
  and cov_title_idle the six task starts with C are identical in
  lockstep; a planted wrong sway count in ShakeTask fails (2026-11-02).
- A routine that calls one that can wait can wait too: its caller's C
  pushes the JSR's return address (cdraft follows calls, and the file's
  helpers, transitively). TaskExit's C leaves its JSR's return address on
  the task's stack as the original does.
- Actors also wait by COP (A = the frames; vector $00:FFB0 into the
  BRK handler past its LDA #1): `actor_wait()`, judged as a BRK's yield. A
  wrong wait count fails A5 (Kind70Start, rand_stage_a1_s1, 2026-11-02).
- A routine an actor calls that drops its return and jumps into more actor
  code (ActorFallTileTest into ActorEndPlaced, $83:96CA) ends its call at
  that jump on both sides (`actor_leave(entry)`), and the entry's piece
  opens at the same instruction; the original side once returned there
  without opening it, and the two sides then paired different calls (A7
  off by the scheduler's scan count, cov_actor_fall_top, 2026-11-02).
- Bottom up is not optional: C that reaches code with no C fails the whole
  image (FrameStep reached the frame update, $81:830D), so no routine can
  certify until it is held back.

## Running the game

- Title menu: about 480 frames after a START at frame 2400; GAME START,
  PASS WORD, OPTIONS. Default controls: Y dash, B jump, A throw, R hover
  ([decomp/log/bootstrap-2026-10-28](decomp/log/bootstrap-2026-10-28.md)).
