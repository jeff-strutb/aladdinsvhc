# Aladdin handbook

What this decomp learns about the hardware, the code and the method, one rule
per finding, each linked to the dated note that measured it.

## Toolchain

- The Stunt Race FX core traced ROM bytes only through the Super FX's CPU
  ROM window, so a cartridge with no coprocessor got an empty ROM map; patch
  0014 traces the cartridge's own ROM. Its map must take the ROM's size when
  the board gives none, or the bus does not mirror and bank $80 reads past
  the ROM ([decomp/log/bootstrap-2026-10-28](decomp/log/bootstrap-2026-10-28.md)).
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
- Bottom up is not optional: C that reaches code with no C fails the whole
  image (FrameStep reached the frame update, $81:830D), so no routine can
  certify until it is held back.

## Running the game

- Title menu: about 480 frames after a START at frame 2400; GAME START,
  PASS WORD, OPTIONS. Default controls: Y dash, B jump, A throw, R hover
  ([decomp/log/bootstrap-2026-10-28](decomp/log/bootstrap-2026-10-28.md)).
