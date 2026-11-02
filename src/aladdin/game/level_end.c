/*
 * level_end.c: pieces of the screen at a level's end (81:F7B1): 2x2-tile
 * blocks queued for VRAM, the four columns its picture grows in step by
 * step. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram_video.h"
#include "wram.h"

#define BLANK_TILE 0x080Eu          /* the tilemap word a blanked block gets */
#define END_STEPS_DONE 0xADu        /* in the direct page: counts the pieces placed */
/* In the data bank: per column the tile's high byte; per step the VRAM
   word address of column 0 (words); per level (LEVEL_035C less 1) four
   picture indices, a byte per column; per index the picture's first tile */
#define END_COLUMN_ATTR 0xFDF7u
#define END_STEP_VRAM 0xFDC5u
#define END_LEVEL_PICTURES 0xFDD5u
#define END_PICTURE_TILE 0xFDF1u
/* In the data bank: the VRAM word addresses of the 16 blocks blanked (words) */
#define END_BLANK_BLOCKS 0xFE02u

/*
 * @routine 81:F973 TileBlockBlank
 * WHAT IT DOES: Writes a blank 2x2-tile block record at X in the tile-
 * block queue: VRAM address A, its four tilemap words $080E.
 * HOW: Five word stores in the 10-byte record; X moves on by 10.
 * Entry: M 16-bit, X 16-bit; A = the VRAM word address; X = the record's
 * offset in TILE_BLOCKS; called by JSR (by 81:F958, for the 16 blocks of a
 * table). Exit: M 16-bit, X 16-bit; X = A = the next record's offset; C
 * from the add.
 */
void abi_TileBlockBlank(void)
{
    sta16(ea_absx(TILE_BLOCKS));  /* 81F973 sta */
    lda16_v(BLANK_TILE);  /* 81F976 lda */
    sta16(ea_absx(0x1B46));  /* 81F979 sta */
    sta16(ea_absx(0x1B48));  /* 81F97C sta */
    sta16(ea_absx(0x1B4A));  /* 81F97F sta */
    sta16(ea_absx(0x1B4C));  /* 81F982 sta */
    txa16();  /* 81F985 txa */
    clc();  /* 81F986 clc */
    adc16_v(0x000A);  /* 81F987 adc */
    tax16();  /* 81F98A tax */
    return;  /* 81F98B rts */
}


/*
 * @routine 81:F98C LevelEndColumnTile
 * WHAT IT DOES: Gives the tilemap word of the picture piece column $000C
 * shows for the level, and counts the piece.
 * HOW: END_STEPS_DONE steps up; the column's attribute byte as the high
 * byte; the level's picture index for the column (LEVEL_035C less 1, times
 * 4, plus the column) picks the picture's first tile, doubled, as the low
 * bits.
 * Entry: M 8-bit, X 8-bit; $000C = the column (0-3); D the direct page of
 * END_STEPS_DONE; called by JSR. Exit: M 16-bit, X 16-bit; A = the tilemap
 * word; $0010-$0011 = the attribute word; X = the picture index; Y = its
 * offset in the level table.
 */
void abi_LevelEndColumnTile(void)
{
    MEM8(inc, ea_dp(END_STEPS_DONE));  /* 81F98C inc */
    ldx8(ea_abs(SCRATCH));  /* 81F98E ldx */
    lda8(ea_absx(END_COLUMN_ATTR));  /* 81F991 lda */
    sta8(ea_abs(0x0011));  /* 81F994 sta */
    stz8(ea_abs(0x0010));  /* 81F997 stz */
    lda8(ea_abs(LEVEL_035C));  /* 81F99A lda */
    ACC8(dec);  /* 81F99D dec */
    ACC8(asl);  /* 81F99E asl */
    ACC8(asl);  /* 81F99F asl */
    adc8_v(rd8(ea_abs(SCRATCH)));  /* 81F9A0 adc */
    tay8();  /* 81F9A3 tay */
    ldx8(ea_absy(END_LEVEL_PICTURES));  /* 81F9A4 ldx */
    lda8(ea_absx(END_PICTURE_TILE));  /* 81F9A7 lda */
    op_rep(0x30);  /* 81F9AA rep */
    and16_v(0x00FF);  /* 81F9AC and */
    ACC16(asl);  /* 81F9AF asl */
    ora16_v(rd16(ea_abs(0x0010)));  /* 81F9B0 ora */
    return;  /* 81F9B3 rts */
}


/*
 * @routine 81:F9B4 LevelEndColumnPlace
 * WHAT IT DOES: Gives the VRAM address of column X's piece at step Y of
 * the level-end picture: the step's address for column 0, less 6 words a
 * column.
 * HOW: X times 6 (16-bit, by a double and an add) taken from the step's
 * word in END_STEP_VRAM; kept at $000E.
 * Entry: M 8-bit; X = the column; Y = the step (0, 2, 4, 6); called by
 * JSR. Exit: M 8-bit; $000E = A = the VRAM word address; C from the
 * subtraction.
 */
void abi_LevelEndColumnPlace(void)
{
    txa8();  /* 81F9B4 txa */
    op_rep(0x20);  /* 81F9B5 rep */
    and16_v(0x00FF);  /* 81F9B7 and */
    ACC16(asl);  /* 81F9BA asl */
    sta16(ea_abs(0x000E));  /* 81F9BB sta */
    ACC16(asl);  /* 81F9BE asl */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81F9BF adc */
    sta16(ea_abs(0x000E));  /* 81F9C2 sta */
    lda16(ea_absy(END_STEP_VRAM));  /* 81F9C5 lda */
    sec();  /* 81F9C8 sec */
    sbc16_v(rd16(ea_abs(0x000E)));  /* 81F9C9 sbc */
    sta16(ea_abs(0x000E));  /* 81F9CC sta */
    op_sep(0x20);  /* 81F9CF sep */
    return;  /* 81F9D1 rts */
}

/*
 * @routine 81:F958 LevelEndBlanks
 * WHAT IT DOES: Queues 16 blank 2x2-tile blocks for VRAM, at the addresses
 * of a table, and has the NMI send them.
 * HOW: From the queue's length, TileBlockBlank for each word of the table
 * at $FE02 (the last first); the new length kept, then the queue marked
 * ready (TILE_BLOCKS_PENDING counted up).
 * Entry: DB = $81 (the table's bank); called by JSR from the level-end
 * screen's set-up (81:F6B7). Exit: M 8-bit, X 8-bit; Y = $FE; A low = the
 * last address's low byte.
 */
void abi_LevelEndBlanks(void)
{
    op_rep(0x30);  /* 81F958 rep */
    ldx16(ea_abs(TILE_BLOCKS_LENGTH));  /* 81F95A ldx */
    ldy16_v(0x001E);  /* 81F95D ldy */
L_81F960:
    lda16(ea_absy(0xFE02));  /* 81F960 lda */
    abi_TileBlockBlank();  /* 81F963 jsr */
    dey16();  /* 81F966 dey */
    dey16();  /* 81F967 dey */
    if (!IF_N) goto L_81F960;  /* 81F968 bpl */
    stx16(ea_abs(TILE_BLOCKS_LENGTH));  /* 81F96A stx */
    op_sep(0x30);  /* 81F96D sep */
    MEM8(inc, ea_abs(TILE_BLOCKS_PENDING));  /* 81F96F inc */
    return;  /* 81F972 rts */
}

