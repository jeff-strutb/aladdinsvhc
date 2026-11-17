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

static uint32_t abs_y(uint16_t off)
{
    return (((uint32_t)cpu.db << 16 | off) + cpu.y) & 0xFFFFFFu;
}

static uint32_t abs_x(uint16_t off)
{
    return (((uint32_t)cpu.db << 16 | off) + cpu.x) & 0xFFFFFFu;
}

/* A 16-bit shift of a direct-page word, the carry in and out as the CPU's. */
static void lsr_at(uint8_t off)
{
    uint16_t v = rd16(dp(off));

    flag_set(P_C, v & 1);
    v >>= 1;
    wr16(dp(off), v);
    nz16_of(v);
}

static void ror_at(uint8_t off)
{
    uint16_t v = rd16(dp(off)), c = (uint16_t)(cpu.p & P_C);

    flag_set(P_C, v & 1);
    v = (uint16_t)(v >> 1 | c << 15);
    wr16(dp(off), v);
    nz16_of(v);
}

static void asl_at(uint8_t off)
{
    uint16_t v = rd16(dp(off));

    flag_set(P_C, v & 0x8000);
    v = (uint16_t)(v << 1);
    wr16(dp(off), v);
    nz16_of(v);
}

/* The pieces from Y (past the count) to the sprites from X; mirrored:
   each piece's x from its second byte, and its flip across turned over. */
static void pieces(int mirrored)
{
    uint16_t v, t;

    do {
        op_rep(P_M);
        cpu.a = rd16(abs_y(mirrored ? 1 : 0));
        abi_SignExtend();
        flag_set(P_C, 0);
        v = op_adc16(cpu.a, rd16(dp(SCREEN_X)), &cpu.p);
        wr16(abs_x(0xFA80), v);
        wr16(dp(PIECE_X), v);
        if (mirrored)
            flag_set(P_C, 0);
        cpu.a = rd16(abs_y(2));
        abi_SignExtend();
        if (!mirrored)
            flag_set(P_C, 0);
        v = op_adc16(cpu.a, rd16(dp(SCREEN_Y)), &cpu.p);
        wr16(abs_x(0xFA81), v);
        flag_set(P_C, 0);
        cpu.a = op_adc16(v, 0x0020, &cpu.p);
        op_cmp16(0x0100);
        if (!(cpu.p & P_C)) {
            /* on the screen: its tile word, then its two high-table bits */
            t = rd16(abs_y(3));
            wr16(dp(PIECE_WORD), t);
            v = op_adc16(t, rd16(dp(TILE_BASE)), &cpu.p);
            v = (uint16_t)((v & 0xCFFF) | rd16(dp(FLIP_BITS)));
            if (mirrored)
                v ^= 0x4000;
            v |= rd16(dp(PALETTE_BITS));
            cpu.a = v;
            nz16_of(v);
            wr16(abs_x(0xFA82), v);
            lsr_at(PIECE_X + 1);
            ror_at(OAM_HIGH_BITS);
            asl_at(PIECE_WORD);
            asl_at(PIECE_WORD);
            asl_at(PIECE_WORD);
            asl_at(PIECE_WORD);
            ror_at(OAM_HIGH_BITS);
            if (dec16_at(dp(OAM_HIGH_COUNT)) == 0) {
                op_lda16_imm(0x0008);
                wr16(dp(OAM_HIGH_COUNT), 0x0008);
                v = rd16(abs_db(OAM_HIGH_NEXT & 0xFFFF));
                op_lda16(dp(OAM_HIGH_BITS));
                wr16(((uint32_t)cpu.db << 16 | 0xFC80) + v, cpu.a);
                (void)inc16_at(abs_db(OAM_HIGH_NEXT & 0xFFFF));
                (void)inc16_at(abs_db(OAM_HIGH_NEXT & 0xFFFF));
            }
            cpu.x = (uint16_t)(cpu.x + 4);
            nz16_of(cpu.x);
        }
        flag_set(P_C, 0);
        cpu.a = op_adc16(cpu.y, 0x0005, &cpu.p);
        cpu.y = cpu.a;
        nz16_of(cpu.y);
    } while (dec16_at(dp(PIECES_LEFT)) != 0);
    op_sep(P_M);
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
    uint8_t f;
    uint16_t at, n, left, pos;

    f = rd8(dp(ACTOR_08));
    a8_load(f);
    flag_set(P_Z, !(f & 0x10));
    if (f & 0x10) {
        a8_load((uint8_t)(rd8(abs_db(V_032D)) & 0x01));
        if (cpu.a & 0x01)
            return;
    }
    at = cpu.x;
    flag_set(P_C, 0);
    a8_load(f);
    flag_set(P_Z, !(f & 0x20));
    if (f & 0x20) {
        a8_load((uint8_t)(rd8(abs_db(V_032D)) & 0x02));
        if (cpu.a & 0x02)
            flag_set(P_C, 1);
    }
    op_rep(P_M);
    a16_load(rd16(dp(ACTOR_LOOK)) & 0x0E00);
    if (cpu.p & P_C)
        a16_load(FLASH_PALETTE);
    wr16(abs_db(PALETTE_BITS), cpu.a);
    cpu.x = at;
    nz16_of(at);
    cpu.y = rd16(dp(ACTOR_PIECES));
    nz16_of(cpu.y);
    n = rd16(abs_y(0)) & 0x00FF;
    a16_load(n);
    left = rd16(abs_db(OAM_FREE & 0xFFFF));
    cmp16_of(n, left);
    if (n >= left) {
        op_sep(P_M);
        return;
    }
    wr16(abs_db(PIECES_LEFT), n);
    flag_set(P_C, 1);
    wr16(abs_db(OAM_FREE & 0xFFFF), op_adc16((uint16_t)~n, left, &cpu.p));
    cpu.y++;
    flag_set(P_C, 1);
    pos = rd16(dp(ACTOR_X + 1));
    wr16(abs_db(SCREEN_X), op_sbc16(pos, rd16(abs_db(SPRITE_ORIGIN_X)), &cpu.p));
    flag_set(P_C, 1);
    pos = rd16(dp(ACTOR_Y + 1));
    wr16(abs_db(SCREEN_Y), op_sbc16(pos, rd16(abs_db(SPRITE_ORIGIN_Y)), &cpu.p));
    wr16(abs_db(TILE_BASE), rd16(dp(ACTOR_LOOK)) & 0x01FF);
    wr16(abs_db(FLIP_BITS), (rd16(dp(ACTOR_08)) ^ 0x3000) & 0x3000);
    flag_set(P_C, rd16(dp(ACTOR_FLIP)) & 1);
    a16_load(0);
    cpu.d = 0;
    pieces(cpu.p & P_C);
}

/* OamDrawList from 81:92EE: A records of the list at DRAW_LISTS + Y. */
static void draw_list(uint16_t y)
{
    cpu.y = y;
    nz16_of(y);
    wr8(abs_db(LIST_LEFT), (uint8_t)cpu.a);
    do {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(abs_y(DRAW_LISTS + 1)));
        op_xba();
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(abs_y(DRAW_LISTS)));
        cpu.d = cpu.a;
        nz16_of(cpu.d);
        y = cpu.y;
        abi_OamDrawActor();
        cpu.y = (uint16_t)(y + 2);
        nz16_of(cpu.y);
    } while (dec8_at(abs_db(LIST_LEFT)) != 0);
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
    draw_list(cpu.y);
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
    static const uint16_t list_at[LIST_COUNT] = { 0x00, 0x40, 0x80, 0xC0 };
    uint8_t db = cpu.db;
    uint16_t d = cpu.d, s, i, n, scroll;

    op_sep(P_M);
    s = rd8(abs_db(SPRITE_SHIFT_AT));
    x8_load((uint8_t)s);
    op_rep(P_M | P_X);
    flag_set(P_C, 0);
    scroll = rd16(abs_db(LAYER0 + LAYER_SCROLL_X));
    cpu.a = op_adc16(scroll, rd16(abs_db((uint16_t)(SPRITE_SHIFT_X + s))), &cpu.p);
    wr16(abs_db(SPRITE_ORIGIN_X), cpu.a);
    flag_set(P_C, 0);
    scroll = rd16(abs_db(LAYER0 + LAYER_SCROLL_Y));
    cpu.a = op_adc16(scroll, rd16(abs_db((uint16_t)(SPRITE_SHIFT_Y + s))), &cpu.p);
    wr16(abs_db(SPRITE_ORIGIN_Y), cpu.a);
    op_sep(P_M);
    cpu.x = rd16(abs_db(OAM_NEXT & 0xFFFF));
    nz16_of(cpu.x);
    cpu.db = 0x7E;
    for (i = 0; i < LIST_COUNT; i++) {
        if (i == 2) {
            a8_load(rd8(abs_db(PLAYER & 0xFFFF)));
            if (cpu.a & 0xFF) {
                a8_load((uint8_t)(rd8(abs_db((PLAYER + ACTOR_08) & 0xFFFF)) & 0x08));
                if (cpu.a & 0xFF) {
                    a8_load((uint8_t)(PLAYER >> 8));
                    op_xba();
                    a8_load((uint8_t)PLAYER);
                    cpu.d = cpu.a;
                    nz16_of(cpu.d);
                    abi_OamDrawActor();
                }
            }
        }
        a8_load(rd8(abs_db((uint16_t)(DRAW_COUNTS + 2 * i))));
        if (cpu.a & 0xFF) {
            cpu.y = list_at[i];
            nz16_of(cpu.y);
            draw_list(list_at[i]);
        }
    }
    wr16(abs_db(OAM_NEXT & 0xFFFF), cpu.x);
    cpu.d = d;
    nz16_of(d);
    op_rep(P_M);
    a16_load(rd16(dp(OAM_HIGH_COUNT)) & 0x0007);
    if (cpu.a != 0) {
        n = cpu.a;
        cpu.y = n;
        a16_load(rd16(dp(OAM_HIGH_BITS)));
        do {
            op_lsr16();
            op_lsr16();
            cpu.y--;
            nz16_of(cpu.y);
        } while (cpu.y != 0);
        cpu.y = rd16(abs_db(OAM_HIGH_NEXT & 0xFFFF));
        nz16_of(cpu.y);
        wr16(abs_y(0xFC80), cpu.a);
    }
    op_sep(P_M);
    a8_load(0xE1);
    do {
        wr8(abs_x(0xFA81), 0xE1);
        cpu.x = (uint16_t)(cpu.x + 4);
        nz16_of(cpu.x);
        cmp16_of(cpu.x, 0x0200);
    } while (cpu.x < 0x0200);
    op_sep(P_X);
    cpu.db = db;
    nz8_of(db);
}
