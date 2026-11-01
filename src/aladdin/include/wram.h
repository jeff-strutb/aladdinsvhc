/*
 * wram.h: work RAM, by name. Addresses are in bank $00's view of the low
 * 8 KB ($0000-$1FFF, the same bytes as $7E:0000-$1FFF), as the original's
 * absolute and direct-page accesses reach them.
 */
#ifndef ALADDIN_WRAM_H
#define ALADDIN_WRAM_H

/* Sound commands to the SPC700 */
#define SOUND_SEQ 0x000002u        /* sequence number of the last command sent; the driver echoes it on APU port 2 */
#define SOUND_HOLD 0x000371u       /* nonzero: no command is sent this frame */
#define SOUND_SENDING 0x000372u    /* nonzero while a command is being sent */
#define SOUND_QUEUE_READ 0x0003BBu /* queue index of the next command to send (0-31) */
#define SOUND_QUEUE_WRITE 0x0003BDu /* queue index where the next command is added (0-31) */
#define SOUND_QUEUE 0x0003BFu      /* 32-entry ring of command bytes */

/* Tasks (system/task.c): 8 slots of $18 bytes from $006F; offsets are a
   slot's byte offset (0, $18, ... $A8) */
#define TASK_STATE 0x00006Fu       /* 0 free, 1 waiting, 4 ready, 8 running, $0C new */
#define TASK_WAIT 0x000070u        /* frames left to wait */
#define TASK_SAVED_S 0x000071u     /* S where it stopped (word) */
#define TASK_STACK_TOP 0x000073u   /* top of its stack (word) */
#define TASK_PARAM 0x000075u       /* the byte its creator gave it */
#define TASK_CURRENT 0x000327u     /* the running task's slot */
#define TASK_SLOT_COUNT 0x000328u  /* how many slots the scheduler scans */

/* Scratch bytes many routines use for their own working values, $000C up
   (offsets from SCRATCH) */
#define SCRATCH 0x00000Cu

/* Joypads (system/joypad.c): pad 1's word, then pad 2's */
#define JOY_HELD 0x000377u         /* buttons held */
#define JOY_PRESSED 0x00037Bu      /* buttons newly pressed this frame */

/* Shadows of PPU and CPU registers, written to them in the vertical blank */
#define NMITIMEN_SHADOW 0x0003B7u  /* NMITIMEN: interrupts and auto joypad read */
#define INIDISP_SHADOW 0x0003B8u   /* INIDISP: forced blank and brightness */

/* The text layer (video/tilemap.c) */
#define TEXT_TILEMAP 0x7ED800u     /* its 32x32 tilemap, 2 KB */
#define TEXT_TILEMAP_UPLOAD 0x000835u /* nonzero: the NMI copies the tilemap to VRAM */

/* Sprites (video/oam.c): the OAM image the NMI copies to the PPU */
#define OAM_BUFFER 0x7EFA80u       /* 128 sprites of 4 bytes */
#define OAM_HIGH 0x7EFC80u         /* their size and x bit 8, 2 bits each */
#define OAM_FREE 0x00084Du         /* sprites not yet used this frame (word) */
#define OAM_NEXT 0x000852u         /* the next sprite's byte offset in OAM_BUFFER (word) */
#define OAM_HIGH_NEXT 0x000854u    /* the next byte offset in OAM_HIGH (word) */
#define OAM_0857 0x000857u         /* cleared with them (word); purpose not yet established */

/* Colour math (video/colour_fade.c): shadows the NMI writes to the PPU */
#define WOBJSEL_SHADOW 0x0003ACu   /* WOBJSEL: sprite and colour windows */
#define CGWSEL_SHADOW 0x0003AFu    /* CGWSEL: where colour math applies */
#define CGADSUB_SHADOW 0x0003B0u   /* CGADSUB: add or subtract, and on which layers */
#define COLDATA_RED 0x0003B2u      /* the fixed colour's red (low 5 bits) */
#define COLDATA_GREEN 0x0003B3u    /* its green */
#define COLDATA_BLUE 0x0003B4u     /* its blue */

/* Layers: each layer's offset from the camera, a word per layer */
#define LAYER_OFFSET_Y 0x001A1Cu   /* vertical */

/* Palettes (video/palette.c) */
#define PALETTE_BUFFER 0x7EFD00u   /* CGRAM's image: 16 palettes of 16 colours */
#define PALETTE_UPLOAD 0x000844u   /* nonzero: the NMI copies PALETTE_BUFFER to CGRAM */
#define PALETTE_SAVED 0x000406u    /* colours kept to fade from, laid out as PALETTE_BUFFER */
#define PALETTE_SAVED_AT 0x000606u /* byte offset of the first colour kept (word) */

/* Actor animation sets (actor/anim_sets.c) */
#define ANIM_SET_OF_TYPE 0x7FF000u /* per actor type, its animation set; $FF: none */
#define ANIM_SETS_USED 0x7FF100u   /* the level's sets: (set, 0) pairs, $FF after the last */

/* The random number (system/random.c) */
#define RANDOM 0x00032Bu           /* word, stepped by RandomNext */

/* Per-level values from tables (level/level_tables.c) */
#define LEVEL_035C 0x00035Cu       /* two bytes from the level's entry at $81:B933; purpose not yet established */
#define LEVEL_SPOT_COUNT 0x000821u /* how many trigger spots the level has */
#define LEVEL_SPOTS 0x000822u      /* their list's offset from $81:EC1E (word) */

/* Which BG positions follow the scroll layers (video/bg_follow.c) */
#define BG1_HELD 0x001A14u         /* nonzero: BG1's position is not taken from the play field */
#define BG2_FOLLOWS 0x001A15u      /* 0: BG2 follows the backdrop layer; $40: its y only; $80: the play field's x; else neither */

#endif
