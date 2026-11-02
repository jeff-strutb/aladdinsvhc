/*
 * player_near.c: where an actor is against the player, or a point.
 *
 * The actor is the record the direct page is; the player's is PLAYER.
 * Coordinates are the positions' whole pixels.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* The player's x and y pixels. */
#define PLAYER_X_PIXEL (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXEL (PLAYER + ACTOR_Y + 1)

/* Boxes round an actor: a half-width (and a half-height) per entry. */
#define NEAR_BOX_TABLE 0xBC43u     /* half-width, half-height: 4 bytes an entry */
#define NEAR_POINT_X (SCRATCH + 8)  /* ActorNearPoint's point (words) */
#define NEAR_POINT_Y (SCRATCH + 10)
#define NEAR_SPAN_TABLE 0xBC7Bu    /* half-width: 2 bytes an entry */

/*
 * @routine 81:AB85 ActorSideOfPlayer
 * WHAT IT DOES: Says which way the actor must face to look at the player.
 * HOW: 0 (right) when the player's x is at or right of the actor's, else
 * 1 (left).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A low byte and X = the facing; A high byte = the
 * player's x high byte; N clear, Z from the facing; C from the compare;
 * Y, V unchanged.
 */
void abi_ActorSideOfPlayer(void)
{
    uint8_t side = 0;

    op_rep(P_M);
    cpu.x = 0;
    cpu.p |= P_C;
    op_lda16(abs_db(PLAYER_X_PIXEL));
    op_cmp16(rd16(dp(ACTOR_X + 1)));
    if (!(cpu.p & P_C)) {
        side = 1;
        cpu.x = 1;
        cpu.p = nz8(cpu.p, 1);
    }
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | side);
    cpu.p = nz8(cpu.p, side);
}

/* x - player's x + half, compared with 2 * half: C clear when the actor's
   x is within half of the player's. */
static void within(uint8_t field, uint16_t player_field, uint16_t half_at, uint16_t span_at)
{
    cpu.p |= P_C;
    op_lda16(dp(field));
    cpu.a = op_sbc16(cpu.a, rd16(abs_db(player_field)), &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(abs_db(half_at)), &cpu.p);
    op_cmp16(rd16(abs_db(span_at)));
}

/* LDA abs,Y (16-bit) / STA scratch / ASL / STA scratch+2. */
static void half_and_span(uint16_t table, uint16_t at)
{
    op_lda16(((uint32_t)cpu.db << 16) + table + cpu.y);
    wr16(abs_db(at), cpu.a);
    op_asl16();
    wr16(abs_db(at + 2), cpu.a);
}

/*
 * @routine 81:ACDE ActorNearPlayer
 * WHAT IT DOES: Says whether the player is inside a box round the actor:
 * within a half-width across and a half-height up or down.
 * HOW: Takes box Y's half-width and half-height and keeps them and their
 * doubles in scratch; x - player's x + half-width is below the width
 * (unsigned) when within it across; the same for y, tested only when x
 * passes.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the box's byte
 * offset. Exit: C clear when inside; A = the last sum (16 bits); N, Z, C
 * from its compare; scratch $0C-$13 = half-width, width, half-height,
 * height; M 8-bit; X, Y, V unchanged.
 */
void abi_ActorNearPlayer(void)
{
    op_rep(P_M);
    half_and_span(NEAR_BOX_TABLE, SCRATCH);
    half_and_span(NEAR_BOX_TABLE + 2, SCRATCH + 4);
    within(ACTOR_X + 1, PLAYER_X_PIXEL, SCRATCH, SCRATCH + 2);
    if (!(cpu.p & P_C))
        within(ACTOR_Y + 1, PLAYER_Y_PIXEL, SCRATCH + 4, SCRATCH + 6);
    op_sep(P_M);
}

/*
 * @routine 81:AD68 ActorNearPlayerX
 * WHAT IT DOES: Says whether the player is within a half-width of the
 * actor across, whatever the height.
 * HOW: Takes span Y's half-width and keeps it and its double in scratch;
 * x - player's x + half-width is below the width (unsigned) when within.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the span's byte
 * offset. Exit: C clear when within; A = the sum (16 bits); N, Z, C from
 * its compare; scratch $0C-$0F = half-width, width; M 8-bit; X, Y, V
 * unchanged.
 */
void abi_ActorNearPlayerX(void)
{
    op_rep(P_M);
    half_and_span(NEAR_SPAN_TABLE, SCRATCH);
    within(ACTOR_X + 1, PLAYER_X_PIXEL, SCRATCH, SCRATCH + 2);
    op_sep(P_M);
}

/*
 * @routine 81:AD17 ActorNearPoint
 * WHAT IT DOES: Says whether the actor is inside a box round the point
 * in scratch $14, $16: within a half-width across and a half-height up or
 * down.
 * HOW: Takes box Y's half-width and half-height and keeps them and their
 * doubles in scratch; the actor's x - the point's x + half-width is below
 * the width (unsigned) when within it across; the same for y, tested only
 * when x passes.
 * Entry: M 16-bit; D = the actor; DB = $81; Y = the box's byte offset;
 * $14, $16 = the point (words); called by JSL. Exit: C clear when inside;
 * A = the last sum; N, Z, C from its compare; scratch $0C-$13 =
 * half-width, width, half-height, height; M 16-bit; X, Y, V unchanged.
 */
void abi_ActorNearPoint(void)
{
    lda16(ea_absy(NEAR_BOX_TABLE));  /* 81AD17 lda */
    sta16(ea_abs(SCRATCH));  /* 81AD1A sta */
    ACC16(asl);  /* 81AD1D asl */
    sta16(ea_abs(SCRATCH + 2));  /* 81AD1E sta */
    lda16(ea_absy(NEAR_BOX_TABLE + 2));  /* 81AD21 lda */
    sta16(ea_abs(SCRATCH + 4));  /* 81AD24 sta */
    ACC16(asl);  /* 81AD27 asl */
    sta16(ea_abs(SCRATCH + 6));  /* 81AD28 sta */
    sec();  /* 81AD2B sec */
    lda16(ea_dp(ACTOR_X + 1));  /* 81AD2C lda */
    sbc16_v(rd16(ea_abs(NEAR_POINT_X)));  /* 81AD2E sbc */
    clc();  /* 81AD31 clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 81AD32 adc */
    cmp16_v(rd16(ea_abs(SCRATCH + 2)));  /* 81AD35 cmp */
    if (IF_C) goto L_81AD47;  /* 81AD38 bcs */
    sec();  /* 81AD3A sec */
    lda16(ea_dp(ACTOR_Y + 1));  /* 81AD3B lda */
    sbc16_v(rd16(ea_abs(NEAR_POINT_Y)));  /* 81AD3D sbc */
    clc();  /* 81AD40 clc */
    adc16_v(rd16(ea_abs(SCRATCH + 4)));  /* 81AD41 adc */
    cmp16_v(rd16(ea_abs(SCRATCH + 6)));  /* 81AD44 cmp */
L_81AD47:
    return;  /* 81AD47 rtl */
}

/*
 * @routine 81:AC68 ActorAimAt
 * WHAT IT DOES: The direction from the actor to the point in scratch $10,
 * $12: an entry of the direction table at $BC03.
 * HOW: The differences across (point x - actor's x) and up or down, made
 * positive; the octant from their signs (+4 the point above, +2 left) and
 * which is larger (+1 up or down); the step in it from the smaller against
 * the larger (under 1/8, 3/8, 5/8, 7/8 of it, else 4, the larger / 8 kept
 * at $0E); the table's byte at the octant * 8 + the step.
 * Entry: M 16-bit, X 16-bit; D = the actor; DB = $81; $10, $12 = the point
 * (words); called by JSL. Exit: M and X 8-bit; A low byte = the direction;
 * X = the table offset, Y = the octant (low bytes); scratch $0C = the
 * octant * 8, $0E = the larger difference / 8.
 */
void abi_ActorAimAt(void)
{
    ldy16_v(0x0000);  /* 81AC68 ldy */
    lda16(ea_abs(V_0012));  /* 81AC6B lda */
    sec();  /* 81AC6E sec */
    sbc16_v(rd16(ea_dp(0x0F)));  /* 81AC6F sbc */
    if (IF_C) goto L_81AC7A;  /* 81AC71 bcs */
    eor16_v(0xFFFF);  /* 81AC73 eor */
    ACC16(inc);  /* 81AC76 inc */
    ldy16_v(0x0004);  /* 81AC77 ldy */
L_81AC7A:
    sta16(ea_abs(SCRATCH));  /* 81AC7A sta */
    lda16(ea_abs(0x0010));  /* 81AC7D lda */
    sec();  /* 81AC80 sec */
    sbc16_v(rd16(ea_dp(0x0C)));  /* 81AC81 sbc */
    if (IF_C) goto L_81AC8B;  /* 81AC83 bcs */
    eor16_v(0xFFFF);  /* 81AC85 eor */
    ACC16(inc);  /* 81AC88 inc */
    iny16();  /* 81AC89 iny */
    iny16();  /* 81AC8A iny */
L_81AC8B:
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81AC8B cmp */
    if (IF_C) goto L_81AC98;  /* 81AC8E bcs */
    iny16();  /* 81AC90 iny */
    ldx16(ea_abs(SCRATCH));  /* 81AC91 ldx */
    sta16(ea_abs(SCRATCH));  /* 81AC94 sta */
    txa16();  /* 81AC97 txa */
L_81AC98:
    ldx16_v(0x0000);  /* 81AC98 ldx */
    ACC16(lsr);  /* 81AC9B lsr */
    ACC16(lsr);  /* 81AC9C lsr */
    ACC16(lsr);  /* 81AC9D lsr */
    sta16(ea_abs(0x000E));  /* 81AC9E sta */
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81ACA1 cmp */
    if (IF_C) goto L_81ACCB;  /* 81ACA4 bcs */
    inx16();  /* 81ACA6 inx */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81ACA7 adc */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81ACAA adc */
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81ACAD cmp */
    if (IF_C) goto L_81ACCB;  /* 81ACB0 bcs */
    inx16();  /* 81ACB2 inx */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81ACB3 adc */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81ACB6 adc */
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81ACB9 cmp */
    if (IF_C) goto L_81ACCB;  /* 81ACBC bcs */
    inx16();  /* 81ACBE inx */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81ACBF adc */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81ACC2 adc */
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81ACC5 cmp */
    if (IF_C) goto L_81ACCB;  /* 81ACC8 bcs */
    inx16();  /* 81ACCA inx */
L_81ACCB:
    tya16();  /* 81ACCB tya */
    ACC16(asl);  /* 81ACCC asl */
    ACC16(asl);  /* 81ACCD asl */
    ACC16(asl);  /* 81ACCE asl */
    sta16(ea_abs(SCRATCH));  /* 81ACCF sta */
    txa16();  /* 81ACD2 txa */
    clc();  /* 81ACD3 clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 81ACD4 adc */
    tax16();  /* 81ACD7 tax */
    lda16(ea_absx(0xBC03));  /* 81ACD8 lda */
    op_sep(0x30);  /* 81ACDB sep */
    return;  /* 81ACDD rtl */
}
