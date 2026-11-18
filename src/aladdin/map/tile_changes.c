/*
 * tile_changes.c: changing a tile of the level's map while it is played
 * (a door opening, a block broken), in the map and on screen.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "cpu_ops.h"
#include "native.h"
void abi_TileChangeAdd(void);
void abi_TileChangesApply(void);
void abi_TilemapWordLong(void);
void abi_TileChangeQueueQuarter(void);
void abi_MapTileAddress(void);
void abi_MapTileAt(void);
void abi_MapTileSet(void);
void abi_MapQuarterAt(void);
#include "wram.h"
#include "wram_video.h"

#define LEVEL_TILES 0x7F0000u      /* the screens' tiles, a word each */

/* A word of the data bank. */
static uint16_t db16(uint16_t addr) { return rd16(dba(addr)); }

/*
 * @routine 81:9A64 TileChangeQueueQuarter
 * WHAT IT DOES: Queues one 8x8 quarter of a changed map tile for the NMI
 * to write to VRAM.
 * HOW: The quarter's VRAM offset (from the table at $BA46 + Y) added to
 * the tile's address, and its tilemap word (from the new tile's quarters,
 * at its place) go to the queue at X; the queue's count steps up.
 * Entry: M 16-bit, X 16-bit; D = 0; X = the queue's byte offset; Y = the
 * quarter's table offset; $32 = the tile's VRAM address; $34-$36 = the new
 * tile's quarters; DB = $81; called by JSR. Exit: X past the record; Y =
 * the quarter's place; A = the word queued.
 */
void abi_TileChangeQueueQuarter(void)
{
    uint16_t x = cpu.x, y = cpu.y, vram;

    /* the quarter's VRAM address and its tilemap word */
    flag_set(P_C, 0);
    vram = dp16(0x32);
    wr16(dba(0x0619) + x, op_adc16(vram, rd16(dba(0xBA46) + y), &cpu.p));
    y = rd16(dba(0xBA48) + y);
    cpu.a = rd16(dp_long_y(0x34, y));
    wr16(dba(0x061B) + x, cpu.a);
    inc16_at(dba(0x0617));
    x += 4;
    nz16_of(x);
    cpu.x = x;
    cpu.y = y;
}

/*
 * @routine 81:9C50 MapTileAddress
 * WHAT IT DOES: The place of the map tile under point X, Y in the level's
 * tiles.
 * HOW: The point rounded to the 16-pixel tile; its screen place (x / 256
 * and y / 1024 at 64 a row) gives the screen's tiles from the screen map;
 * plus the tile's place in the screen (x / 8 and y * 2).
 * Entry: M 16-bit, X 16-bit; D = 0; X = the x; Y = the y; called by JSR.
 * Exit: X = the tile's offset in the level's tiles; Y as at entry; $0C,
 * $0E = the rounded point; $10 = the place in the screen; $14 = the
 * screen's tiles.
 */
void abi_MapTileAddress(void)
{
    /* the point to 16 pixels; its screen, 32 across in the screen map;
       its tile there, 16 across. As the original, instruction by
       instruction: the x kept on the stack while the y is rounded, the
       screen's tiles pushed and pulled into Y at the end. */
    txa16();
    and16_v(0xFFF0);
    sta16(ea_dp(0x0C));
    pha16();
    tya16();
    and16_v(0xFFF0);
    sta16(ea_dp(0x0E));
    pla16();
    xba();
    cpu.a = asl16_r(cpu.a);
    and16_v(0x003E);
    sta16(ea_dp(0x10));
    tya16();
    cpu.a = lsr16_r(cpu.a);
    cpu.a = lsr16_r(cpu.a);
    and16_v(0x03C0);
    ora16_v(rd16(ea_dp(0x10)));
    tax16();
    lda16(0x7FF042u + cpu.x);
    sta16(ea_dp(0x14));
    pha16();
    lda16(ea_dp(0x0C));
    cpu.a = lsr16_r(cpu.a);
    cpu.a = lsr16_r(cpu.a);
    cpu.a = lsr16_r(cpu.a);
    and16_v(0x001E);
    sta16(ea_dp(0x10));
    lda16(ea_dp(0x0E));
    cpu.a = asl16_r(cpu.a);
    and16_v(0x01E0);
    ora16_v(rd16(ea_dp(0x10)));
    clc();
    adc16_v(rd16(ea_dp(0x14)));
    tax16();
    ply16();
}

/* The view's span on one axis from the camera at `camera` ($28 its start
   less 8): its size (in `size`), half of it (`half`) and its middle
   (`mid`). */
static void view_span(uint16_t camera, uint16_t extent, uint16_t size, uint16_t half, uint16_t mid)
{
    uint16_t at = db16(camera), v;

    flag_set(P_C, 0);
    dp16_set(0x28, op_adc16(at, 0xFFF8, &cpu.p));
    flag_set(P_C, 0);
    v = op_adc16(at, extent, &cpu.p);
    flag_set(P_C, 1);
    v = op_sbc16(v, dp16(0x28), &cpu.p);
    dp16_set(size, v);
    flag_set(P_C, v & 1);
    v >>= 1;
    dp16_set(half, v);
    flag_set(P_C, 0);
    cpu.a = op_adc16(v, dp16(0x28), &cpu.p);
    dp16_set(mid, cpu.a);
}

/* `at` less `mid` plus `half` below `size`: inside the view. */
static int in_view(uint16_t at, uint16_t mid, uint16_t half, uint16_t size)
{
    uint16_t v, s;

    flag_set(P_C, 1);
    v = op_sbc16(at, dp16(mid), &cpu.p);
    flag_set(P_C, 0);
    v = op_adc16(v, dp16(half), &cpu.p);
    cpu.a = v;
    s = dp16(size);                              /* one read: the CMP's */
    cmp16_of(v, s);
    return v < s;
}

/*
 * @routine 81:999A TileChangesApply
 * WHAT IT DOES: Applies the map tile changes queued this frame: each
 * change's new tile goes into the level's tiles, and one in view has its
 * four quarters queued for the NMI to write to VRAM.
 * HOW: With D = 0: the view box (the drawn area with 8 pixels round it);
 * for each change, its tile's VRAM address (TilemapWordLong) and place in
 * the level's tiles (MapTileAddress); the new tile is stored and stepped
 * to its quarters; a change inside the box queues its four quarters
 * (TileChangeQueueQuarter). Then the queue empties, and the NMI's write is
 * requested when anything was queued.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSL. Exit: M and X 8-bit;
 * D as at entry, N and Z from it (PLD); no changes: as at entry, A low
 * byte = 0.
 */
void abi_TileChangesApply(void)
{
    uint16_t y;

    a8_load(rd8(dba(0x0719)));
    if (!(uint8_t)cpu.a)
        return;
    op_rep(0x30);
    phd();
    cpu.d = 0;
    view_span(0x176C, 0x0118, 0x1C, 0x1E, 0x20);
    view_span(0x176E, 0x00F0, 0x22, 0x24, 0x26);
    dp16_set(0x30, 0);
    wr16(dba(0x0617), 0);
    y = 0;
    cpu.y = 0;
    do {
        /* each change: its tile into the level's map, and its four
           quarters queued for VRAM when it is in view. Y (the change's
           offset) on the stack while it is made; the point's x and y
           pushed for TilemapWordLong and pulled into Y and X for
           MapTileAddress, as the original's PHA/PHY and PLY/PLX. */
        phy16();
        dp16_set(0x2C, db16((uint16_t)(0x071D + y)));
        dp16_set(0x2E, db16((uint16_t)(0x071F + y)));
        dp16_set(0x34, db16((uint16_t)(0x0721 + y)));
        lda16(ea_dp(0x2C));
        pha16();
        ldy16(ea_dp(0x2E));
        phy16();
        jsl_frame(0x8199FCu);
        abi_TilemapWordLong();
        cpu.s = (uint16_t)(cpu.s + 3);
        dp16_set(0x32, cpu.a);
        ply16();
        plx16();
        call_frame(0x9A06, abi_MapTileAddress);
        cpu.a = dp16(0x34);
        wr16(0x7F0000u + cpu.x, cpu.a);
        flag_set(P_C, 0);
        dp16_set(0x34, op_adc16(cpu.a, dp16(0x5A), &cpu.p));
        cpu.a = dp16(0x5C);
        dp16_set(0x36, cpu.a);
        if (in_view(dp16(0x2C), 0x20, 0x1E, 0x1C) && in_view(dp16(0x2E), 0x26, 0x24, 0x22)) {
            cpu.x = dp16(0x30);
            cpu.y = 0x0018;
            call_frame(0x9A35, abi_TileChangeQueueQuarter);
            cpu.y = 0x0010;
            call_frame(0x9A3B, abi_TileChangeQueueQuarter);
            cpu.y = 0x0008;
            call_frame(0x9A41, abi_TileChangeQueueQuarter);
            cpu.y = 0x0000;
            call_frame(0x9A47, abi_TileChangeQueueQuarter);
            dp16_set(0x30, cpu.x);
        }
        pla16();                                 /* the change's offset */
        flag_set(P_C, 0);
        y = op_adc16(cpu.a, 0x0008, &cpu.p);
        cpu.a = y;
        cpu.y = y;
    } while (dec16_at(dba(0x0719)));
    wr16(dba(0x071B), 0);
    op_sep(0x30);
    a8_load(rd8(dba(0x0617)));
    if ((uint8_t)cpu.a)
        inc8_at(dba(0x0616));
    pld();
}

/*
 * @routine 81:9978 TileChangeAdd
 * WHAT IT DOES: Queues a change of the map tile at a point to a new tile,
 * for TileChangesApply to make.
 * HOW: The record at the queue's end ($071B) gets the point's x and y, each
 * with 8 added, and the new tile; the end moves on 8 bytes and the count
 * ($0719) steps up.
 * Entry: M 16-bit, X 16-bit; A = the new tile; X = the point's x; Y = its
 * y; DB = $00-$3F or $80-$BF; called by JSL. Exit: X = the record's
 * offset; A = it plus 8; Y unchanged.
 */
void abi_TileChangeAdd(void)
{
    uint16_t x = db16(0x071B);

    /* a record: the tile, the point's y and x (+8, to the middle) */
    wr16(dba((uint16_t)(0x0721 + x)), cpu.a);
    wr16(dba((uint16_t)(0x071F + x)), (uint16_t)(cpu.y | 0x0008));
    wr16(dba((uint16_t)(0x071D + x)), (uint16_t)(cpu.x | 0x0008));
    flag_set(P_C, 0);
    cpu.a = op_adc16(x, 0x0008, &cpu.p);
    wr16(dba(0x071B), cpu.a);
    cpu.x = x;
    inc16_at(dba(0x0719));
}

/*
 * @routine 81:9A84 MapTileAt
 * WHAT IT DOES: Reads the level's map tile under point X, Y.
 * HOW: With the direct page at 0 (its own kept on the stack),
 * MapTileAddress gives the tile's offset; the tile word is read from the
 * level's tiles at $7F:0000.
 * Entry: M 16-bit, X 16-bit; X = the x; Y = the y; called by JSL. Exit: A
 * = the tile word; X = its offset; Y as at entry; D as at entry, N and Z
 * from it (PLD); scratch as MapTileAddress leaves it.
 */
void abi_MapTileAt(void)
{
    uint16_t d = cpu.d;

    cpu.a = 0;
    cpu.d = 0;
    abi_MapTileAddress();
    cpu.a = rd16(LEVEL_TILES + cpu.x);
    cpu.d = d;
    nz16_of(d);
}

/*
 * @routine 81:9B6D MapTileSet
 * WHAT IT DOES: Puts tile word A into the level's map under point X, Y.
 * HOW: With the direct page at 0 (its own kept on the stack, and A),
 * MapTileAddress gives the tile's offset; A is written to the level's
 * tiles there.
 * Entry: M 16-bit, X 16-bit; A = the tile word; X = the x; Y = the y;
 * called by JSL. Exit: A as at entry; X = the tile's offset; Y as at entry;
 * D as at entry, N and Z from it (PLD); scratch as MapTileAddress leaves
 * it.
 */
void abi_MapTileSet(void)
{
    uint16_t d = cpu.d, tile = cpu.a;

    cpu.a = 0;
    cpu.d = 0;
    abi_MapTileAddress();
    cpu.a = tile;
    wr16(LEVEL_TILES + cpu.x, tile);
    cpu.d = d;
    nz16_of(d);
}

/*
 * @routine 81:9B7D MapQuarterAt
 * WHAT IT DOES: Reads the tilemap word of the 8x8 quarter of the map tile
 * under the point in scratch $0C, $0E.
 * HOW: With the direct page at 0 (its own kept on the stack):
 * MapTileAddress; the point is put back; the quarter's offset (x bit 3
 * gives 2, y bit 3 gives 4) is kept at $12 and added to the tile word,
 * which indexes the level's tile quarters through the long pointer at
 * $5A.
 * Entry: DB any; $0C = the x; $0E = the y (words); $5A-$5C = the level's
 * tile quarters; called by JSL. Exit: A = the tilemap word (16 bits); M
 * and X 8-bit; Y = the low byte of its offset; X = the low byte of the
 * tile's; $12 = the quarter's offset; D as at entry, N and Z from it
 * (PLD).
 */
void abi_MapQuarterAt(void)
{
    uint16_t d = cpu.d, x, y, quarter;

    op_rep(0x30);
    cpu.d = 0;
    x = dp16(0x0C);
    y = dp16(0x0E);
    cpu.x = x;
    cpu.y = y;
    abi_MapTileAddress();
    /* the quarter of the tile the point is in: 2 for right, 4 for down */
    dp16_set(0x0C, x);
    dp16_set(0x0E, y);
    quarter = (uint16_t)(((x & 0x0008) >> 2) | ((y & 0x0008) >> 1));
    dp16_set(0x12, quarter);
    flag_set(P_C, 0);
    cpu.y = op_adc16(rd16(LEVEL_TILES + cpu.x), quarter, &cpu.p);
    cpu.a = rd16(dp_long_y(0x5A, cpu.y));
    op_sep(0x30);
    cpu.d = d;
    nz16_of(d);
}
