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

## Running the game

- Title menu: about 480 frames after a START at frame 2400; GAME START,
  PASS WORD, OPTIONS. Default controls: Y dash, B jump, A throw, R hover
  ([decomp/log/bootstrap-2026-10-28](decomp/log/bootstrap-2026-10-28.md)).
