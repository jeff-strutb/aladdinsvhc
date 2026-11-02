/*
 * tile_changes.c: changing a tile of the level's map while it is played
 * (a door opening, a block broken), in the map and on screen.
 */
#include "machine.h"
#include "ops65.h"
void abi_TileChangeAdd(void);
void abi_TileChangesApply(void);
void abi_TilemapWordLong(void);
void abi_TileChangeQueueQuarter(void);
void abi_MapTileAddress(void);
#include "wram.h"
#include "wram_video.h"

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
    clc();  /* 819A64 clc */
    lda16(ea_dp(0x32));  /* 819A65 lda */
    adc16_v(rd16(ea_absy(0xBA46)));  /* 819A67 adc */
    sta16(ea_absx(0x0619));  /* 819A6A sta */
    lda16(ea_absy(0xBA48));  /* 819A6D lda */
    tay16();  /* 819A70 tay */
    lda16(ea_dpily(0x34));  /* 819A71 lda */
    sta16(ea_absx(0x061B));  /* 819A73 sta */
    MEM16(inc, ea_abs(0x0617));  /* 819A76 inc */
    inx16();  /* 819A79 inx */
    inx16();  /* 819A7A inx */
    inx16();  /* 819A7B inx */
    inx16();  /* 819A7C inx */
    return;  /* 819A7D rts */
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
    txa16();  /* 819C50 txa */
    and16_v(0xFFF0);  /* 819C51 and */
    sta16(ea_dp(0x0C));  /* 819C54 sta */
    pha16();  /* 819C56 pha */
    tya16();  /* 819C57 tya */
    and16_v(0xFFF0);  /* 819C58 and */
    sta16(ea_dp(0x0E));  /* 819C5B sta */
    pla16();  /* 819C5D pla */
    xba();  /* 819C5E xba */
    ACC16(asl);  /* 819C5F asl */
    and16_v(0x003E);  /* 819C60 and */
    sta16(ea_dp(0x10));  /* 819C63 sta */
    tya16();  /* 819C65 tya */
    ACC16(lsr);  /* 819C66 lsr */
    ACC16(lsr);  /* 819C67 lsr */
    and16_v(0x03C0);  /* 819C68 and */
    ora16_v(rd16(ea_dp(0x10)));  /* 819C6B ora */
    tax16();  /* 819C6D tax */
    lda16(ea_longx(0x7FF042u));  /* 819C6E lda */
    sta16(ea_dp(0x14));  /* 819C72 sta */
    pha16();  /* 819C74 pha */
    lda16(ea_dp(0x0C));  /* 819C75 lda */
    ACC16(lsr);  /* 819C77 lsr */
    ACC16(lsr);  /* 819C78 lsr */
    ACC16(lsr);  /* 819C79 lsr */
    and16_v(0x001E);  /* 819C7A and */
    sta16(ea_dp(0x10));  /* 819C7D sta */
    lda16(ea_dp(0x0E));  /* 819C7F lda */
    ACC16(asl);  /* 819C81 asl */
    and16_v(0x01E0);  /* 819C82 and */
    ora16_v(rd16(ea_dp(0x10)));  /* 819C85 ora */
    clc();  /* 819C87 clc */
    adc16_v(rd16(ea_dp(0x14)));  /* 819C88 adc */
    tax16();  /* 819C8A tax */
    ply16();  /* 819C8B ply */
    return;  /* 819C8C rts */
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
    goto L_81999A;
L_819999:
    return;  /* 819999 rtl */
L_81999A:
    lda8(ea_abs(0x0719));  /* 81999A lda */
    if (IF_Z) goto L_819999;  /* 81999D beq */
    op_rep(0x30);  /* 81999F rep */
    phd();  /* 8199A1 phd */
    lda16_v(0x0000);  /* 8199A2 lda */
    tcd();  /* 8199A5 tcd */
    clc();  /* 8199A6 clc */
    lda16(ea_abs(0x176C));  /* 8199A7 lda */
    pha16();  /* 8199AA pha */
    adc16_v(0xFFF8);  /* 8199AB adc */
    sta16(ea_dp(0x28));  /* 8199AE sta */
    pla16();  /* 8199B0 pla */
    clc();  /* 8199B1 clc */
    adc16_v(0x0118);  /* 8199B2 adc */
    sec();  /* 8199B5 sec */
    sbc16_v(rd16(ea_dp(0x28)));  /* 8199B6 sbc */
    sta16(ea_dp(0x1C));  /* 8199B8 sta */
    ACC16(lsr);  /* 8199BA lsr */
    sta16(ea_dp(0x1E));  /* 8199BB sta */
    clc();  /* 8199BD clc */
    adc16_v(rd16(ea_dp(0x28)));  /* 8199BE adc */
    sta16(ea_dp(0x20));  /* 8199C0 sta */
    clc();  /* 8199C2 clc */
    lda16(ea_abs(0x176E));  /* 8199C3 lda */
    pha16();  /* 8199C6 pha */
    adc16_v(0xFFF8);  /* 8199C7 adc */
    sta16(ea_dp(0x28));  /* 8199CA sta */
    pla16();  /* 8199CC pla */
    clc();  /* 8199CD clc */
    adc16_v(0x00F0);  /* 8199CE adc */
    sec();  /* 8199D1 sec */
    sbc16_v(rd16(ea_dp(0x28)));  /* 8199D2 sbc */
    sta16(ea_dp(0x22));  /* 8199D4 sta */
    ACC16(lsr);  /* 8199D6 lsr */
    sta16(ea_dp(0x24));  /* 8199D7 sta */
    clc();  /* 8199D9 clc */
    adc16_v(rd16(ea_dp(0x28)));  /* 8199DA adc */
    sta16(ea_dp(0x26));  /* 8199DC sta */
    stz16(ea_dp(0x30));  /* 8199DE stz */
    stz16(ea_abs(0x0617));  /* 8199E0 stz */
    ldy16_v(0x0000);  /* 8199E3 ldy */
L_8199E6:
    phy16();  /* 8199E6 phy */
    lda16(ea_absy(0x071D));  /* 8199E7 lda */
    sta16(ea_dp(0x2C));  /* 8199EA sta */
    lda16(ea_absy(0x071F));  /* 8199EC lda */
    sta16(ea_dp(0x2E));  /* 8199EF sta */
    lda16(ea_absy(0x0721));  /* 8199F1 lda */
    sta16(ea_dp(0x34));  /* 8199F4 sta */
    lda16(ea_dp(0x2C));  /* 8199F6 lda */
    pha16();  /* 8199F8 pha */
    ldy16(ea_dp(0x2E));  /* 8199F9 ldy */
    phy16();  /* 8199FB phy */
    abi_TilemapWordLong();  /* 8199FC jsl */
    sta16(ea_dp(0x32));  /* 819A00 sta */
    ply16();  /* 819A02 ply */
    plx16();  /* 819A03 plx */
    abi_MapTileAddress();  /* 819A04 jsr */
    lda16(ea_dp(0x34));  /* 819A07 lda */
    sta16(ea_longx(0x7F0000u));  /* 819A09 sta */
    clc();  /* 819A0D clc */
    adc16_v(rd16(ea_dp(0x5A)));  /* 819A0E adc */
    sta16(ea_dp(0x34));  /* 819A10 sta */
    lda16(ea_dp(0x5C));  /* 819A12 lda */
    sta16(ea_dp(0x36));  /* 819A14 sta */
    sec();  /* 819A16 sec */
    lda16(ea_dp(0x2C));  /* 819A17 lda */
    sbc16_v(rd16(ea_dp(0x20)));  /* 819A19 sbc */
    clc();  /* 819A1B clc */
    adc16_v(rd16(ea_dp(0x1E)));  /* 819A1C adc */
    cmp16_v(rd16(ea_dp(0x1C)));  /* 819A1E cmp */
    if (IF_C) goto L_819A4A;  /* 819A20 bcs */
    sec();  /* 819A22 sec */
    lda16(ea_dp(0x2E));  /* 819A23 lda */
    sbc16_v(rd16(ea_dp(0x26)));  /* 819A25 sbc */
    clc();  /* 819A27 clc */
    adc16_v(rd16(ea_dp(0x24)));  /* 819A28 adc */
    cmp16_v(rd16(ea_dp(0x22)));  /* 819A2A cmp */
    if (IF_C) goto L_819A4A;  /* 819A2C bcs */
    ldx16(ea_dp(0x30));  /* 819A2E ldx */
    ldy16_v(0x0018);  /* 819A30 ldy */
    abi_TileChangeQueueQuarter();  /* 819A33 jsr */
    ldy16_v(0x0010);  /* 819A36 ldy */
    abi_TileChangeQueueQuarter();  /* 819A39 jsr */
    ldy16_v(0x0008);  /* 819A3C ldy */
    abi_TileChangeQueueQuarter();  /* 819A3F jsr */
    ldy16_v(0x0000);  /* 819A42 ldy */
    abi_TileChangeQueueQuarter();  /* 819A45 jsr */
    stx16(ea_dp(0x30));  /* 819A48 stx */
L_819A4A:
    pla16();  /* 819A4A pla */
    clc();  /* 819A4B clc */
    adc16_v(0x0008);  /* 819A4C adc */
    tay16();  /* 819A4F tay */
    MEM16(dec, ea_abs(0x0719));  /* 819A50 dec */
    if (!IF_Z) goto L_8199E6;  /* 819A53 bne */
    stz16(ea_abs(0x071B));  /* 819A55 stz */
    op_sep(0x30);  /* 819A58 sep */
    lda8(ea_abs(0x0617));  /* 819A5A lda */
    if (IF_Z) goto L_819A62;  /* 819A5D beq */
    MEM8(inc, ea_abs(0x0616));  /* 819A5F inc */
L_819A62:
    pld();  /* 819A62 pld */
    return;  /* 819A63 rtl */
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
    phx16();  /* 819978 phx */
    ldx16(ea_abs(0x071B));  /* 819979 ldx */
    sta16(ea_absx(0x0721));  /* 81997C sta */
    tya16();  /* 81997F tya */
    ora16_v(0x0008);  /* 819980 ora */
    sta16(ea_absx(0x071F));  /* 819983 sta */
    pla16();  /* 819986 pla */
    ora16_v(0x0008);  /* 819987 ora */
    sta16(ea_absx(0x071D));  /* 81998A sta */
    clc();  /* 81998D clc */
    txa16();  /* 81998E txa */
    adc16_v(0x0008);  /* 81998F adc */
    sta16(ea_abs(0x071B));  /* 819992 sta */
    MEM16(inc, ea_abs(0x0719));  /* 819995 inc */
    return;  /* 819998 rtl */
}
