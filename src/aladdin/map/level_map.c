/*
 * level_map.c: building a level's map in work RAM.
 *
 * A level's map is made of screens of 32x16 tiles (LEVEL_TILES, $200
 * bytes a screen), built from 8x8 blocks of 2x2 tiles, and a screen map
 * that places them (SCREEN_MAP, 32 screens across). The level's map
 * header, at $80:E3DC (a word per level, pointing to it in that table),
 * gives the screens to build, the banks of their block and tile data, and
 * the layout.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_LevelMapBuildLong(void);
void abi_LevelMapBuild(void);
void abi_ScreenMapLayout(void);
void abi_BlockSourceAdd(void);
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:BEEB LevelMapBuildLong
 * WHAT IT DOES: LevelMapBuild for callers in other banks.
 * HOW: LevelMapBuild.
 * Entry: as LevelMapBuild; called by JSL. Exit: as LevelMapBuild.
 */
void abi_LevelMapBuildLong(void)
{
    abi_LevelMapBuild();  /* 81BEEB jsr */
    return;  /* 81BEEE rtl */
}

/*
 * @routine 81:BEEF LevelMapBuild
 * WHAT IT DOES: Builds the level's map: its screens' tiles and the screen
 * map that places them.
 * HOW: Clears the screen map; from the level's map header takes the
 * screen count, the banks of the block lists, block tiles and metatiles,
 * and the main and sub screen layers; for each screen adds its block
 * source (BlockSourceAdd), copies its 64-block list and writes each
 * block's 2x2 tiles into the screen's 32x16 tiles; then lays out the
 * screen map from the level's layouts (ScreenMapLayout), a second layout
 * too when the level has one.
 * Entry: M 8-bit; D = 0; DB any; called by JSR. Exit: P and DB as at
 * entry (pulled); X, Y and A as the layouts left them (16-bit X, Y).
 */
void abi_LevelMapBuild(void)
{
    uint8_t p = cpu.p, db = cpu.db, blocks, tiles, screens, row, col;
    uint16_t level, head, at, x, y, block;

    for (at = 0; at < 0x0800; at += 2)
        wr16(ANIM_SET_OF_TYPE + at, 0);
    dp16_set(0x0C, 0);
    dp16_set(0x16, 0);
    /* the map header: screen count, the banks of the block lists, block
       tiles and metatiles, the layers */
    level = (uint16_t)(rd16(dba(LEVEL)) << 1);
    head = rd16(0x80E3DCu + level);
    screens = rd8(0x80E3DCu + head);
    wr8(dba(0x0056), rd8(0x80E3DDu + head));
    wr8(dba(0x0055), 0x80);
    wr8(dba(0x0059), rd8(0x80E3DEu + head));
    wr8(dba(0x0058), 0x80);
    wr8(dba(LEVEL_METATILE_BANK), rd8(0x80E3DFu + head));
    wr8(dba(0x005B), 0x80);
    wr8(dba(0x0399), rd8(0x80E3E0u + head));
    wr8(dba(0x039A), rd8(0x80E3E0u + head));
    op_rep(0x10);
    do {
        /* the screen: its source noted, its 64-block list (and 4 more
           bytes) copied, then each block's 2x2 tiles into the screen's
           32x16 */
        y = (uint16_t)(rd8(0x80E3E1u + head) << 7);
        head++;
        cpu.a = y;
        cpu.y = y;
        op_rep(P_M);
        flag_set(P_C, 0);
        abi_BlockSourceAdd();
        blocks = rd8(dba(0x0056));
        for (x = 0; x < 0x84; x++)
            wr8(((uint32_t)blocks << 16) + 0x1A94 + x,
                rd8(((uint32_t)blocks << 16) + (uint16_t)(0x8000 + y + x)));
        cpu.db = blocks;
        tiles = rd8(dba(0x0059));
        x = dp16(0x0C);
        y = 0;
        for (row = 0; row < 8; row++) {
            for (col = 0; col < 8; col++) {
                block = rd16(((uint32_t)tiles << 16) + (uint16_t)(0x1A94 + y));
                y += 2;
                wr16(0x7F0000u + x, rd16(((uint32_t)tiles << 16) + (uint16_t)(0x8000 + block)));
                wr16(0x7F0002u + x, rd16(((uint32_t)tiles << 16) + (uint16_t)(0x8002 + block)));
                wr16(0x7F0020u + x, rd16(((uint32_t)tiles << 16) + (uint16_t)(0x8004 + block)));
                wr16(0x7F0022u + x, rd16(((uint32_t)tiles << 16) + (uint16_t)(0x8006 + block)));
                x += 4;
            }
            x += 0x20;
        }
        dp16_set(0x0C, x);
        cpu.db = tiles;
    } while (--screens);
    wr8(dpa(0x10), 0);
    dp16_set(0x12, 0);
    dp16_set(0x14, 0);
    wr8(dpa(0x0E), 0);
    /* the screen map's layouts: the main one, the second when there */
    cpu.db = 0x7F;
    head = rd16(0x80E460u + level);
    cpu.x = rd16(0x80E460u + head);
    cpu.y = 0x0042;
    abi_ScreenMapLayout();
    cpu.x = head;
    cpu.a = rd16(0x80E462u + head);
    if (cpu.a) {
        cpu.x = cpu.a;
        cpu.y = 0x0442;
        abi_ScreenMapLayout();
    }
    cpu.db = db;
    cpu.p = 0;
    op_sep(p);
}

/*
 * @routine 81:BFEE ScreenMapLayout
 * WHAT IT DOES: Writes a layout into the screen map: which screen goes in
 * each place.
 * HOW: The layout's width and height (each less one), then its screen
 * numbers row by row; each number * 2 goes to the high byte of the place's
 * word (Y on, a row 64 bytes on).
 * Entry: M 16-bit, X 16-bit; X = the layout's offset in the layouts; Y =
 * the first place's byte offset; DB = $7F; D = 0; called by JSR. Exit: P
 * as at entry (pulled); X past the layout; Y = the last row's start + $40.
 */
void abi_ScreenMapLayout(void)
{
    uint8_t p = cpu.p, width = rd8(0x80E460u + cpu.x), rows = rd8(0x80E461u + cpu.x), n;
    uint16_t x = cpu.x, y = cpu.y, line = cpu.y;

    /* the layout's screens, row by row, each *2 to its place's high byte */
    /* (each count runs down past 0, as the original's DEC and BPL) */
    wr8(dpa(0x0C), width);
    do {
        n = width;
        do {
            wr8(dba((uint16_t)(0xF001 + y)), (uint8_t)(rd8(0x80E462u + x) << 1));
            x++;
            y += 2;
        } while (!(--n & 0x80));
        line = (uint16_t)(line + 0x40);
        y = line;
    } while (!(--rows & 0x80));
    dp16_set(0x10, line);
    wr8(dpa(0x0D), rows);
    wr8(dpa(0x0E), n);
    cpu.a = line;
    cpu.x = x;
    cpu.y = y;
    cpu.p = p;
}
