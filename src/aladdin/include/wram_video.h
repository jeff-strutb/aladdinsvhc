/*
 * wram_video.h: work RAM the screen code keeps: VRAM upload requests, the
 * text buffer, scroll positions. The same names as
 * asm/aladdin/include/wram_video.inc, in upper case. Addresses as in
 * wram.h: bank $00's view of the low 8 KB, or a full $7E address.
 */
#ifndef ALADDIN_WRAM_VIDEO_H
#define ALADDIN_WRAM_VIDEO_H

/* The text buffer (2 KB of BG tilemap at $7E:D800) and its upload. */
#define TEXT_UPLOAD_PENDING 0x000835u /* nonzero: upload the text buffer at the next NMI */
#define TEXT_UPLOAD_VRAM 0x0003DFu    /* VRAM word address it goes to (word) */
#define TEXT_UPLOAD_SIZE 0x0003E1u    /* bytes to upload (word) */
#define TEXT_BUFFER 0x7ED800u         /* the tilemap words */
#define WRAM_C800 0x7EC800u           /* a 4 KB buffer, cleared before a level starts */

/* Scroll positions, each the integer part (word) of a 4-byte position. */
#define BG1_X 0x0019EDu
#define BG1_Y 0x0019F1u
#define BG2_X 0x0019F5u
#define BG2_Y 0x0019F9u
#define BG3_X 0x0019FDu
#define BG3_Y 0x001A01u
/* Added to the positions as they go to the PPU (words, 4 bytes apart). */
#define BG1_X_SHIFT 0x001A18u
#define BG1_Y_SHIFT 0x001A1Cu
#define BG2_X_SHIFT 0x001A20u
#define BG2_Y_SHIFT 0x001A24u
#define BG2_X_HELD 0x00083Cu          /* nonzero: BG2's horizontal scroll is not written */

#endif
