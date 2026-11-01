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

#endif
