/*
 * level_end.c: pieces of the screen at a level's end (81:F7B1): 2x2-tile
 * blocks queued for VRAM, the four columns its picture grows in step by
 * step. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    uint32_t block = dba(TILE_BLOCKS) + cpu.x;

    wr16(block, cpu.a);                   /* its VRAM address, then four blank tiles */
    for (int i = 0; i < 4; i++)
        wr16(block + 2 + 2 * i, BLANK_TILE);
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.x, 0x000A, &cpu.p);
    cpu.x = cpu.a;
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
    uint8_t column, attr, level, at, picture;

    inc8_at(dpa(END_STEPS_DONE));
    column = rd8(dba(SCRATCH));
    x8_load(column);
    attr = rd8(dba(END_COLUMN_ATTR) + column);
    a8_load(attr);
    wr8(dba(SCRATCH + 5), attr);          /* the attribute word, high byte */
    wr8(dba(SCRATCH + 4), 0);
    /* the picture for this level's column: 4 a level, from level 1 */
    level = (uint8_t)(rd8(dba(LEVEL_035C)) - 1);
    flag_set(P_C, (level >> 6) & 1);     /* the second ASL's carry goes into the add */
    at = op_adc8((uint8_t)(level << 2), column, &cpu.p);
    cpu.y = at;
    x8_load(rd8(dba(END_LEVEL_PICTURES) + at));
    picture = rd8(dba(END_PICTURE_TILE) + cpu.x);
    op_rep(0x30);
    flag_set(P_C, 0);
    a16_load((uint16_t)((picture << 1) | rd16(dba(SCRATCH + 4))));
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
    uint16_t twice = (uint16_t)((uint8_t)cpu.x << 1), six;

    op_rep(P_M);
    wr16(dba(SCRATCH + 2), twice);
    flag_set(P_C, 0);
    six = op_adc16((uint16_t)(twice << 1), twice, &cpu.p);   /* 6 a column */
    wr16(dba(SCRATCH + 2), six);
    flag_set(P_C, 1);
    cpu.a = op_sbc16(rd16(dba(END_STEP_VRAM) + cpu.y), six, &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
    op_sep(P_M);
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
    op_rep(0x30);
    cpu.x = rd16(dba(TILE_BLOCKS_LENGTH));
    cpu.y = 0x001E;
    do {                                  /* the 16 blocks of the table */
        a16_load(rd16(dba(END_BLANK_BLOCKS) + cpu.y));
        abi_TileBlockBlank();
        cpu.y = (uint16_t)(cpu.y - 2);
        nz16_of(cpu.y);
    } while (!(cpu.y & 0x8000));
    wr16(dba(TILE_BLOCKS_LENGTH), cpu.x);
    op_sep(0x30);
    inc8_at(dba(TILE_BLOCKS_PENDING));
}

