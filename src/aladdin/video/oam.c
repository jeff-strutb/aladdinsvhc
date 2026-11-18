/*
 * oam.c: the sprite (OAM) image in work RAM.
 *
 * Each frame the game builds the 544-byte OAM image at $7E:FA80 (128
 * sprites of 4 bytes, then the 32-byte high table) and the NMI copies it to
 * the PPU. A sprite at y $E0 is off the screen.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"
#include "actor.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* Bytes of the frame's high-table accumulator (direct page). */
#define OAM_HIGH_BITS 0x61u        /* the bits gathered for the next high-table byte (word) */
#define OAM_HIGH_COUNT 0x63u       /* sprites left before it is full (word) */

/*
 * @routine 81:95D5 OamBufferClear
 * WHAT IT DOES: Takes every sprite off the screen.
 * HOW: Fills the 512 sprite bytes with $E0 (y below the screen) and the
 * high table with 0, last byte first.
 * Entry: M 8-bit. Exit: A low byte = 0; X = $FF (X 8-bit) or $FFFF; Y and
 * the flags as at entry (P pulled back).
 */
void abi_OamBufferClear(void)
{
    int x;

    op_push8(cpu.p);
    op_rep(P_X);
    for (x = 0x01FF; x >= 0; x--)
        wr8(OAM_BUFFER + x, 0xE0);
    for (x = 0x001F; x >= 0; x--)
        wr8(OAM_HIGH + x, 0x00);
    cpu.a &= 0xFF00;
    cpu.x = 0xFFFF;
    op_plp(op_pull8());
}

/*
 * @routine 81:9230 OamBuildBegin
 * WHAT IT DOES: Starts the frame's sprites: none used, all 128 free, the
 * high table cleared.
 * HOW: Clears the sprite and high-table offsets and $0857, sets the
 * high-table accumulator to empty (8 sprites to go, no bits), clears the
 * high table a word at a time and sets 128 free sprites.
 * Entry: M 8-bit, X 8-bit; D = 0; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M 8-bit; A = $0080; X = $FE; N, Z clear; Y, C, V unchanged.
 */
void abi_OamBuildBegin(void)
{
    int x;

    op_rep(P_M);
    wr16(abs_db(OAM_0857 & 0xFFFF), 0);
    wr16(abs_db(OAM_NEXT & 0xFFFF), 0);
    wr16(abs_db(OAM_HIGH_NEXT & 0xFFFF), 0);
    wr16(dp(OAM_HIGH_COUNT), 0x0008);
    wr16(dp(OAM_HIGH_BITS), 0);
    for (x = 0x1E; x >= 0; x -= 2)
        wr16(OAM_HIGH + x, 0);
    cpu.x = 0xFE;
    cpu.a = 0x0080;
    cpu.p = nz16(cpu.p, 0x0080);
    wr16(abs_db(OAM_FREE & 0xFFFF), 0x0080);
    op_sep(P_M);
}

/* The sprites of the frame's actors (OamDrawLists, OamDrawActor) */
#define SPRITE_SHIFT_AT 0x1A16u     /* which of the layers' shifts the sprites take (a byte offset) */
#define SPRITE_SHIFT_X 0x1A18u      /* the shifts, X then Y 4 bytes on, by SPRITE_SHIFT_AT (words) */
#define SPRITE_SHIFT_Y 0x1A1Cu
#define SPRITE_ORIGIN_X 0x1A38u     /* the level's point at the screen's left, for sprites (word) */
#define SPRITE_ORIGIN_Y 0x1A3Cu     /* and at its top (word) */
#define DRAW_LISTS 0x1492u          /* four lists of the actors to draw, $40 bytes each (record addresses) */
#define DRAW_COUNTS 0x1592u         /* each list's length (a byte, 2 apart) */
#define LIST_LEFT 0x0856u           /* records left in the list being drawn */
#define V_032D 0x032Du              /* bit 0 set: actors with ACTOR_08 bit 4 not drawn; bit 1: those with bit 5 in the flash palette */
#define ACTOR_PIECES 0x22u           /* the actor's sprite frame: its pieces' count, then 5 bytes a piece (word, bank $7E) */
/* Direct page 0 while the pieces go out */
#define PIECES_LEFT 0x0Cu           /* the frame's pieces left (word) */
#define SCREEN_X 0x0Eu              /* the actor's place on the screen (words) */
#define SCREEN_Y 0x10u
#define PIECE_X 0x12u               /* the piece's x: its high byte's bit 0 is x bit 8 */
#define FLIP_BITS 0x1Eu             /* the actor's flips, as the sprites' attribute bits */
#define PIECE_WORD 0x20u            /* the piece's tile word, shifted for its size bit */
#define TILE_BASE 0x26u             /* the actor's first tile (word) */
#define PALETTE_BITS 0x2Au          /* its palette, as attribute bits (word) */
#define FLASH_PALETTE 0x0A00u
#define LIST_COUNT 4

void abi_SignExtend(void);

/* OamDrawActor's pieces from Y (past the count) to the sprites from X,
   with D = 0 (.piece); mirrored (.mirrored): each piece's x from its
   second byte, and its flip across turned over. */
static void pieces_normal(void)
{
    do {
        op_rep(0x20);
        lda16(ea_absy(0));
        call_frame(0x9387, abi_SignExtend);
        clc();
        adc16_v(rd16(ea_dp(SCREEN_X)));
        sta16(ea_absx(0xFA80));
        sta16(ea_dp(PIECE_X));
        lda16(ea_absy(2));
        call_frame(0x9395, abi_SignExtend);
        clc();
        adc16_v(rd16(ea_dp(SCREEN_Y)));
        sta16(ea_absx(0xFA81));
        clc();
        adc16_v(0x0020);
        cmp16_v(0x0100);
        if (!IF_C) {                      /* on the screen */
            lda16(ea_absy(3));
            sta16(ea_dp(PIECE_WORD));
            adc16_v(rd16(ea_dp(TILE_BASE)));
            and16_v(0xCFFF);
            ora16_v(rd16(ea_dp(FLIP_BITS)));
            ora16_v(rd16(ea_dp(PALETTE_BITS)));
            sta16(ea_absx(0xFA82));
            MEM16(lsr, ea_dp(PIECE_X + 1));   /* x bit 8, then the size bit */
            MEM16(ror, ea_dp(OAM_HIGH_BITS));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(ror, ea_dp(OAM_HIGH_BITS));
            MEM16(dec, ea_dp(OAM_HIGH_COUNT));
            if (IF_Z) {                   /* a high-table word full */
                lda16_v(0x0008);
                sta16(ea_dp(OAM_HIGH_COUNT));
                phy16();
                ldy16(ea_abs((uint16_t)OAM_HIGH_NEXT));
                lda16(ea_dp(OAM_HIGH_BITS));
                sta16(ea_absy(0xFC80));
                MEM16(inc, ea_abs((uint16_t)OAM_HIGH_NEXT));
                MEM16(inc, ea_abs((uint16_t)OAM_HIGH_NEXT));
                ply16();
            }
            inx16();
            inx16();
            inx16();
            inx16();
        }
        clc();
        tya16();
        adc16_v(0x0005);
        tay16();
        MEM16(dec, ea_dp(PIECES_LEFT));
    } while (!IF_Z);
    op_sep(0x20);
}

static void pieces_mirrored(void)
{
    do {
        op_rep(0x20);
        lda16(ea_absy(1));
        call_frame(0x93F5, abi_SignExtend);
        clc();
        adc16_v(rd16(ea_dp(SCREEN_X)));
        sta16(ea_absx(0xFA80));
        sta16(ea_dp(PIECE_X));
        clc();
        lda16(ea_absy(2));
        call_frame(0x9404, abi_SignExtend);
        adc16_v(rd16(ea_dp(SCREEN_Y)));
        sta16(ea_absx(0xFA81));
        clc();
        adc16_v(0x0020);
        cmp16_v(0x0100);
        if (!IF_C) {                      /* on the screen */
            lda16(ea_absy(3));
            sta16(ea_dp(PIECE_WORD));
            adc16_v(rd16(ea_dp(TILE_BASE)));
            and16_v(0xCFFF);
            ora16_v(rd16(ea_dp(FLIP_BITS)));
            eor16_v(0x4000);
            ora16_v(rd16(ea_dp(PALETTE_BITS)));
            sta16(ea_absx(0xFA82));
            MEM16(lsr, ea_dp(PIECE_X + 1));   /* x bit 8, then the size bit */
            MEM16(ror, ea_dp(OAM_HIGH_BITS));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(asl, ea_dp(PIECE_WORD));
            MEM16(ror, ea_dp(OAM_HIGH_BITS));
            MEM16(dec, ea_dp(OAM_HIGH_COUNT));
            if (IF_Z) {                   /* a high-table word full */
                lda16_v(0x0008);
                sta16(ea_dp(OAM_HIGH_COUNT));
                phy16();
                ldy16(ea_abs((uint16_t)OAM_HIGH_NEXT));
                lda16(ea_dp(OAM_HIGH_BITS));
                sta16(ea_absy(0xFC80));
                MEM16(inc, ea_abs((uint16_t)OAM_HIGH_NEXT));
                MEM16(inc, ea_abs((uint16_t)OAM_HIGH_NEXT));
                ply16();
            }
            inx16();
            inx16();
            inx16();
            inx16();
        }
        clc();
        tya16();
        adc16_v(0x0005);
        tay16();
        MEM16(dec, ea_dp(PIECES_LEFT));
    } while (!IF_Z);
    op_sep(0x20);
}

/*
 * @routine 81:9309 OamDrawActor
 * WHAT IT DOES: Puts one actor's sprite frame into this frame's sprites,
 * at its place on the screen: each piece on the screen's height gets a
 * sprite; a frame with more pieces than are left is not drawn.
 * HOW: With ACTOR_08 bit 4 set and V_032D bit 0, not drawn (flicker).
 * Palette bits from ACTOR_LOOK, or FLASH_PALETTE with ACTOR_08 bit 5 and
 * V_032D bit 1 (no script runs that); the frame at ACTOR_PIECES: its count
 * at most OAM_FREE, which it takes; the screen place, the actor's x and y
 * less SPRITE_ORIGIN; tile base, flips (ACTOR_08 bits 12-13, inverted);
 * then with D = 0, per piece: x (byte 0, or byte 1 when ACTOR_FLIP bit 0
 * is set, flipping it) and y (byte 2) sign-extended plus the place; a
 * piece within 32 above to 224 down gets the sprite at X: tile word plus
 * the base, the flips and palette; its x bit 8 and size bit (the tile
 * word's bit 12) into the high table's bits, a word out each 8 sprites.
 * Entry: M 8-bit, X 16-bit; D = the actor's record; DB = $7E; X = the
 * next sprite's offset. Exit: M 8-bit; X past its sprites; drawn: D = 0,
 * Y past its pieces, A, N, Z, C, V from the last pointer step; skipped:
 * as the test left them (D, Y unchanged); too many pieces: M 8-bit, A =
 * the count, flags from its compare.
 */
void abi_OamDrawActor(void)
{
    lda8(ea_dp(ACTOR_08));
    bit8_i(0x10);
    if (!IF_Z) {                          /* flicker */
        lda8(ea_abs(V_032D));
        and8_v(0x01);
        if (!IF_Z)
            return;
    }
    phx16();
    clc();
    lda8(ea_dp(ACTOR_08));
    bit8_i(0x20);
    if (!IF_Z) {                          /* flash (no script runs it) */
        lda8(ea_abs(V_032D));
        and8_v(0x02);
        if (!IF_Z)
            sec();
    }
    op_rep(0x20);
    lda16(ea_dp(ACTOR_LOOK));
    and16_v(0x0E00);
    if (IF_C)
        lda16_v(FLASH_PALETTE);
    sta16(ea_abs(PALETTE_BITS));
    plx16();
    lda16(ea_dp(ACTOR_PIECES));
    tay16();
    lda16(ea_absy(0));
    and16_v(0x00FF);
    cmp16_v(rd16(ea_abs((uint16_t)OAM_FREE)));
    if (IF_C) {                           /* too many pieces: not drawn */
        op_sep(0x20);
        return;
    }
    sta16(ea_abs(PIECES_LEFT));
    eor16_v(0xFFFF);
    sec();
    adc16_v(rd16(ea_abs((uint16_t)OAM_FREE)));
    sta16(ea_abs((uint16_t)OAM_FREE));
    iny16();
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_abs(SPRITE_ORIGIN_X)));
    sta16(ea_abs(SCREEN_X));
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_abs(SPRITE_ORIGIN_Y)));
    sta16(ea_abs(SCREEN_Y));
    lda16(ea_dp(ACTOR_LOOK));
    and16_v(0x01FF);
    sta16(ea_abs(TILE_BASE));
    lda16(ea_dp(ACTOR_08));
    eor16_v(0x3000);
    and16_v(0x3000);
    sta16(ea_abs(FLIP_BITS));
    lda16(ea_dp(ACTOR_FLIP));
    cpu.a = lsr16_r(cpu.a);
    lda16_v(0x0000);
    tcd();
    if (IF_C)
        pieces_mirrored();
    else
        pieces_normal();
}

/*
 * @routine 81:92EE OamDrawList
 * WHAT IT DOES: Draws the actors of one of the draw lists.
 * HOW: LIST_LEFT = A; per record address from DRAW_LISTS + Y, D = it and
 * OamDrawActor.
 * Entry: M 8-bit, X 16-bit; A = the count (nonzero); Y = the list's
 * offset; DB = $7E; X = the next sprite's offset. Exit: M 8-bit; X past
 * the sprites; Y past the list; D = 0 or the last record; Z set.
 */
void abi_OamDrawList(void)
{
    sta8(ea_abs(LIST_LEFT));
    do {
        lda8(ea_absy(DRAW_LISTS + 1));
        xba();
        lda8(ea_absy(DRAW_LISTS));
        tcd();
        phy16();
        call_frame(0x92FC, abi_OamDrawActor);
        ply16();
        iny16();
        iny16();
        MEM8(dec, ea_abs(LIST_LEFT));
    } while (!IF_Z);
}

/*
 * @routine 81:9258 OamDrawActors
 * WHAT IT DOES: Puts this frame's actors into the sprites, list by list
 * (Aladdin after the first two), then takes the sprites left over off the
 * screen.
 * HOW: SPRITE_ORIGIN is the scroll plus the layers' shift SPRITE_SHIFT_AT
 * names; DB = $7E; from OAM_NEXT, for each of the four DRAW_LISTS with a
 * count, OamDrawList; Aladdin's record (state nonzero and ACTOR_08 bit 3)
 * between the second and third; OAM_NEXT kept. The high table's last
 * bits shifted down into place and written when the word is not full;
 * every sprite left gets y $E1.
 * Entry: M 8-bit, X 8-bit; D = 0 (the high table's accumulator); DB a
 * bank whose $0000-$1FFF is work RAM; called by JSL.
 * Exit: M 8-bit, X 8-bit; DB, D unchanged; A low byte = $E1; X = 0; C
 * set; N and Z from DB.
 */
void abi_OamDrawActors(void)
{
    op_sep(0x20);
    ldx8(ea_abs(SPRITE_SHIFT_AT));
    op_rep(0x30);
    clc();
    lda16(ea_abs((uint16_t)(LAYER0 + LAYER_SCROLL_X)));
    adc16_v(rd16(ea_absx(SPRITE_SHIFT_X)));
    sta16(ea_abs(SPRITE_ORIGIN_X));
    clc();
    lda16(ea_abs((uint16_t)(LAYER0 + LAYER_SCROLL_Y)));
    adc16_v(rd16(ea_absx(SPRITE_SHIFT_Y)));
    sta16(ea_abs(SPRITE_ORIGIN_Y));
    op_sep(0x20);
    ldx16(ea_abs((uint16_t)OAM_NEXT));
    phb();
    lda8_v(0x7E);
    pha8();
    plb();
    phd();
    lda8(ea_abs(DRAW_COUNTS));
    if (!IF_Z) {
        ldy16_v(0);
        call_frame(0x9288, abi_OamDrawList);
    }
    lda8(ea_abs(DRAW_COUNTS + 2));
    if (!IF_Z) {
        ldy16_v(64);
        call_frame(0x9293, abi_OamDrawList);
    }
    lda8(ea_abs((uint16_t)PLAYER));
    if (!IF_Z) {                          /* Aladdin, between lists 1 and 2 */
        lda8(ea_abs((uint16_t)(PLAYER + ACTOR_08)));
        and8_v(0x08);
        if (!IF_Z) {
            lda8_v((uint8_t)(PLAYER >> 8));
            xba();
            lda8_v((uint8_t)PLAYER);
            tcd();
            call_frame(0x92A8, abi_OamDrawActor);
        }
    }
    lda8(ea_abs(DRAW_COUNTS + 4));
    if (!IF_Z) {
        ldy16_v(128);
        call_frame(0x92B3, abi_OamDrawList);
    }
    lda8(ea_abs(DRAW_COUNTS + 6));
    if (!IF_Z) {
        ldy16_v(192);
        call_frame(0x92BE, abi_OamDrawList);
    }
    stx16(ea_abs((uint16_t)OAM_NEXT));
    pld();
    op_rep(0x20);
    lda16(ea_dp(OAM_HIGH_COUNT));         /* the high table's last bits into place */
    and16_v(0x0007);
    if (!IF_Z) {
        tay16();
        lda16(ea_dp(OAM_HIGH_BITS));
        do {
            cpu.a = lsr16_r(cpu.a);
            cpu.a = lsr16_r(cpu.a);
            dey16();
        } while (!IF_Z);
        ldy16(ea_abs((uint16_t)OAM_HIGH_NEXT));
        sta16(ea_absy(0xFC80));
    }
    op_sep(0x20);                         /* the sprites left off the screen */
    lda8_v(0xE1);
    do {
        sta8(ea_absx(0xFA81));
        inx16();
        inx16();
        inx16();
        inx16();
        cpx16_v(0x0200);
    } while (!IF_C);
    op_sep(0x10);
    plb();
}
