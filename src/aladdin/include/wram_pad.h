/*
 * wram_pad.h: work RAM of the joypads, by name, as wram.h.
 */
#ifndef ALADDIN_WRAM_PAD_H
#define ALADDIN_WRAM_PAD_H

#define PAD1_HELD 0x00037Fu        /* joypad 1 buttons held (word): high byte B Y Select Start Up Down Left Right */

/* Where the frame's buttons come from: 0 the pads, 1 the pads and recorded
   (a development mode: retail code never sets it), 2 the demo's script. */
#define INPUT_MODE 0x000331u
#define RECORD_STEP 0x000334u      /* the recorder's step: 0 start, 1 recording */
#define RECORD_POS 0x000335u       /* the recorder's offset in its buffer (word) */
#define RECORD_END 0x000339u       /* the offset where the full buffer stopped */
#define DEMO_POS 0x00033Bu         /* the demo script's offset (word) */
#define DEMO_BUTTONS 0x00033Du     /* the demo's pad 1 high byte (START never set) */
#define DEMO_FRAMES 0x00033Eu      /* frames left on the demo's buttons */
/* The recorder's buffer: 2 B buttons and frames, 2 B a count per record, in
   cartridge RAM space; this board has none. */
#define RECORD_BUF 0x700000u
/* The demo's script: a pad 1 high byte and a frame count per step. */
#define DEMO_SCRIPT 0x84F700u

#endif
