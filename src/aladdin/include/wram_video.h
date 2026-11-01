/*
 * wram_video.h: work RAM the screen code keeps: VRAM upload requests, the
 * text buffer, scroll positions. The same names as
 * asm/aladdin/include/wram_video.inc, in upper case. Addresses as in
 * wram.h: bank $00's view of the low 8 KB, or a full $7E address.
 */
#ifndef ALADDIN_WRAM_VIDEO_H
#define ALADDIN_WRAM_VIDEO_H

/* The text buffer (2 KB of BG tilemap at $7E:D800) and its upload. */
#define TEXT_UPLOAD_VRAM 0x0003DFu    /* VRAM word address it goes to (word) */
#define TEXT_UPLOAD_SIZE 0x0003E1u    /* bytes to upload (word) */
#define TILE_BEHAVIOUR 0x7EC800u      /* per map tile, how it behaves (solid, slope ...); 4 KB, cleared before a level starts */

/* Upload requests the NMI serves (video/vram_queue.c). */
#define TEXT_PARTS_PENDING 0x000836u  /* 3 flags: nonzero, upload that part of the text buffer */
#define VRAM_UPLOAD_NUMBER 0x000834u  /* nonzero: the number of a fixed upload to make */
#define VRAM_QUEUE_PENDING 0x000858u  /* nonzero: the queue below is ready */
#define VRAM_QUEUE_COUNT 0x000859u    /* records in it */
#define VRAM_QUEUE 0x00085Au          /* 8-byte records: VRAM address, source, bank, count, VMAIN */
#define TILE_BLOCKS_PENDING 0x001BE6u /* nonzero: the block queue below is ready */
#define TILE_BLOCKS_LENGTH 0x001BE4u  /* its length in bytes */
#define TILE_BLOCKS 0x001B44u         /* 10-byte records: VRAM address, four tilemap words */
#define VRAM_LIST_PENDING 0x001649u   /* per slot: nonzero, upload the slot's list */
#define VRAM_LIST_POINTER 0x00164Du   /* per slot: the list's address (word) */

/* Scroll positions, each the integer part (word) of a 4-byte position. */
#define BG1_X 0x0019EDu
#define BG1_Y 0x0019F1u
#define BG2_X 0x0019F5u
#define BG2_Y 0x0019F9u
#define BG3_X 0x0019FDu
#define BG3_Y 0x001A01u
/* Added to the positions as they go to the PPU (words, 4 bytes apart). */
#define BG1_X_SHIFT 0x001A18u
#define BG2_X_SHIFT 0x001A20u
#define BG2_Y_SHIFT 0x001A24u
#define BG2_X_HELD 0x00083Cu          /* nonzero: BG2's horizontal scroll is not written */

/* Scroll layers: records of $156 bytes, the play field's (layer 0) and the
   backdrop's (layer 1); the fields are offsets into a record */
#define LAYER0 0x001738u
#define LAYER1 0x00188Eu
#define LAYER_SCROLL_X 0x3Bu       /* scroll X, whole pixels (word); the fraction is the byte below */
#define LAYER_SCROLL_Y 0x3Fu       /* scroll Y, whole pixels (word); the fraction is the byte below */
#define LAYER_COLUMN_LENGTH 0x28u   /* tiles a column draws (word) */
#define LAYER_ROW_LENGTH 0x2Au      /* tiles a row draws (word) */
#define LAYER_TILEMAP_BASE 0x2Eu   /* bits every tilemap word of the layer carries (word) */
#define LAYER_TILEMAP_MASK 0x30u   /* the tilemap word bits a position gives (word) */
#define LAYER_SCREEN_FLIP 0x32u    /* tilemap bits a row step past the top or bottom flips (word) */
#define LAYER_DRAWN_X 0x34u        /* the X its drawn tiles reach to (word) */
#define LAYER_DRAWN_Y 0x36u        /* the Y its drawn tiles reach to (word) */
#define LAYER_COLUMN_SIDE 0x38u    /* the side of the column being drawn: 0 right, 1 left */
#define LAYER_ROW_SIDE 0x39u       /* the side of the row being drawn: 0 down, 1 up */
#define LAYER_MAP_BASE 0x42u       /* the layer's block map in $7F:F000 (word offset) */
#define LAYER_BLOCK_PTR 0x44u      /* a block's metatile list (long) */
#define LAYER_METATILE_PTR 0x4Au   /* a metatile's four tiles (long) */
#define LAYER_EDGES 0x4Fu          /* the edges it draws: 1 right, 2 left, 4 bottom, 8 top */
#define LAYER_COLUMN_PARTS 0x50u   /* a column drawn, to copy to VRAM: its parts (1, or 2 past 32 tiles) */
#define LAYER_COLUMN_DEST 0x51u    /* its tilemap word (word) */
#define LAYER_COLUMN_BUF 0x53u     /* its tiles, a ring of 32 words from its row */
#define LAYER_ROW_PARTS 0xD3u      /* a row drawn, to copy to VRAM: its parts (2: the two screens) */
#define LAYER_ROW_DEST 0xD4u       /* its tilemap word (word) */
#define LAYER_ROW_BUF 0xD6u        /* its tiles, a ring of 64 words from its column */

/* Columns and rows of tiles queued to draw: rings of 8 entries of 4 bytes
   (the layer's record, the side: 0 right or down, 1 left or up) */
#define COLUMN_QUEUE 0x001A48u
#define ROW_QUEUE 0x001A68u
#define COLUMN_QUEUE_HEAD 0x001A88u /* index of the next entry to draw */
#define COLUMN_QUEUE_TAIL 0x001A8Au /* index where the next entry goes */
#define ROW_QUEUE_HEAD 0x001A8Cu
#define ROW_QUEUE_TAIL 0x001A8Eu

/* The level */
#define LEVEL 0x00035Eu            /* the level being played (word) */
#define SCROLL_LIMIT_X 0x001A04u   /* the play field's scroll X limit (word) */
#define SCROLL_LIMIT_Y 0x001A06u   /* the play field's scroll Y limit (word) */
#define SCROLL_X_BEFORE 0x0019E5u  /* the play field's scroll X before this frame's move (word) */
#define SCROLL_Y_BEFORE 0x0019E9u  /* the play field's scroll Y before this frame's move (word) */
#define CAMERA_STOPPED_X 0x001A0Cu /* nonzero: an empty block stopped the scroll across this frame (word) */
#define CAMERA_STOPPED_Y 0x001A0Eu /* nonzero: an empty block stopped the scroll up or down (word) */
#define BLOCK_SOURCE_END 0x000016u /* while a level loads: the byte offset past the last block source listed (word) */
#define BLOCK_SOURCES 0x001BF5u    /* while a level loads: each block's source in its bank, a word per block */
#define LEVEL_METATILE_BANK 0x00005Cu /* the bank of the level's metatiles */

/* Backdrop bands that drift on their own: 11 positions (4 bytes, whole
   pixels in the middle two) and their speeds, and the HDMA table of
   3-byte entries (lines, scroll) that scrolls each band */
#define BAND_POS 0x7EF800u
#define BAND_SPEED 0x7EF900u
#define BAND_HDMA 0x7EF400u

#endif
