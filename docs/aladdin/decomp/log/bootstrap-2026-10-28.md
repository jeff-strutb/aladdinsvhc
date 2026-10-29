# Bootstrap and raw skeleton, 2026-10-28

## Core

The harness core is bsnes at the Stunt Race FX pin with the same 13 patches
(symbols renamed snesbox_). Its ROM trace hook sat in the Super FX's CPU ROM
reader, and its ROM memory view returned the Super FX's ROM, so on Aladdin
(no coprocessor) the first trace wrote an all-zero ROM map. Patch 0014 maps
the cartridge's ROM through a reader that calls the trace hook, and the view
falls back to the cartridge's ROM.

The first version of 0014 used the lambda form of Cartridge::loadMap, which
does not default the map size to the ROM's: the bus then never mirrored,
bank $80 read past the ROM, and the CPU ran a BRK at $80:8007 and looped in
the BRK vector (trace: 11 opcode bytes; edges $80:8007 -> $00:FFAC). Mapping
with size = the ROM's when the board gives none fixed it: 2,702 opcode bytes
in 1,200 frames, the Capcom logo and the intro story on screen.

## Flow

- Frames 0-2400: Capcom logo, then the intro story. START skips to the
  title; the menu is ready about 480 frames later, cursor on GAME START.
- GAME START: level 1 (Agrabah market) about 600 frames later.
- OPTIONS: control type (default Y dash, B jump, A throw, R hover), B.G.M.,
  sound, exit. PASS WORD: four portrait slots.

## Skeleton

10 scripts traced in parallel (38 s). 26,683 S-CPU code bytes; 2,257 SPC700
code bytes folded onto the upload lists (only the SPC700's own boot ROM,
$FFC0-$FFFD, stays unfolded, as it should). Intake: 12,820 instructions
(11,690 65816, 1,130 SPC700), 0 rejected. Image gate: 0 of 1,310,720 bytes
differ; a planted byte at ROM $000011 failed it.
