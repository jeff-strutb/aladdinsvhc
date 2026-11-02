/*
 * player_relation.c: where the player is from an actor: its direction, and
 * whether it is inside a box in front of the actor.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorDirectionToPlayer(void);
void abi_PlayerInFrontBox(void);
#include "actor.h"
#include "wram.h"

/*
 * @routine 81:ABEA ActorDirectionToPlayer
 * WHAT IT DOES: The direction from the actor to the player, as one of the
 * directions in the table at $BBE3.
 * HOW: The distances across and up or down (made positive, the quadrant
 * kept: 4 for the player above, 2 for left); the band is 0 when the smaller
 * distance is under a quarter of the larger, 1 under three quarters, else
 * 2; the quadrant * 4 plus the band picks the direction.
 * Entry: D = the actor; DB = $81; called by JSL. Exit: M and X 8-bit; A =
 * the direction (16 bits, low byte as M 8-bit); X = its index; Y = the
 * quadrant; scratch $0C-$13 the distances.
 */
void abi_ActorDirectionToPlayer(void)
{
    op_rep(0x30);  /* 81ABEA rep */
    ldx16_v(0x08DB);  /* 81ABEC ldx */
    lda16(ea_absx(SCRATCH));  /* 81ABEF lda */
    sta16(ea_abs(0x0010));  /* 81ABF2 sta */
    lda16(ea_absx(0x000F));  /* 81ABF5 lda */
    sta16(ea_abs((SCRATCH + 6)));  /* 81ABF8 sta */
    ldy16_v(0x0000);  /* 81ABFB ldy */
    lda16(ea_abs((SCRATCH + 6)));  /* 81ABFE lda */
    sec();  /* 81AC01 sec */
    sbc16_v(rd16(ea_dp(0x0F)));  /* 81AC02 sbc */
    if (IF_C) goto L_81AC0D;  /* 81AC04 bcs */
    eor16_v(0xFFFF);  /* 81AC06 eor */
    ACC16(inc);  /* 81AC09 inc */
    ldy16_v(0x0004);  /* 81AC0A ldy */
L_81AC0D:
    sta16(ea_abs(SCRATCH));  /* 81AC0D sta */
    lda16(ea_abs(0x0010));  /* 81AC10 lda */
    sec();  /* 81AC13 sec */
    sbc16_v(rd16(ea_dp(0x0C)));  /* 81AC14 sbc */
    if (IF_C) goto L_81AC1E;  /* 81AC16 bcs */
    eor16_v(0xFFFF);  /* 81AC18 eor */
    ACC16(inc);  /* 81AC1B inc */
    iny16();  /* 81AC1C iny */
    iny16();  /* 81AC1D iny */
L_81AC1E:
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81AC1E cmp */
    if (IF_C) goto L_81AC2B;  /* 81AC21 bcs */
    iny16();  /* 81AC23 iny */
    ldx16(ea_abs(SCRATCH));  /* 81AC24 ldx */
    sta16(ea_abs(SCRATCH));  /* 81AC27 sta */
    txa16();  /* 81AC2A txa */
L_81AC2B:
    ldx16_v(0x0000);  /* 81AC2B ldx */
    ACC16(lsr);  /* 81AC2E lsr */
    ACC16(lsr);  /* 81AC2F lsr */
    sta16(ea_abs(0x000E));  /* 81AC30 sta */
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81AC33 cmp */
    if (IF_C) goto L_81AC45;  /* 81AC36 bcs */
    inx16();  /* 81AC38 inx */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81AC39 adc */
    adc16_v(rd16(ea_abs(0x000E)));  /* 81AC3C adc */
    cmp16_v(rd16(ea_abs(SCRATCH)));  /* 81AC3F cmp */
    if (IF_C) goto L_81AC45;  /* 81AC42 bcs */
    inx16();  /* 81AC44 inx */
L_81AC45:
    tya16();  /* 81AC45 tya */
    ACC16(asl);  /* 81AC46 asl */
    ACC16(asl);  /* 81AC47 asl */
    sta16(ea_abs(SCRATCH));  /* 81AC48 sta */
    txa16();  /* 81AC4B txa */
    clc();  /* 81AC4C clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 81AC4D adc */
    tax16();  /* 81AC50 tax */
    lda16(ea_absx(0xBBE3));  /* 81AC51 lda */
    op_sep(0x30);  /* 81AC54 sep */
    return;  /* 81AC56 rtl */
}

/*
 * @routine 81:AEFD PlayerInFrontBox
 * WHAT IT DOES: Says whether the player is inside box X in front of the
 * actor: C clear when inside.
 * HOW: Height: the actor's y plus the half-height less the player's y,
 * below the height (twice the half-height). Width: the actor's x plus the
 * box's offset for its facing less the player's x, below the box's width
 * (the two offsets' sum), tested only when the height passes.
 * Entry: M 8-bit, X 8-bit; D = the actor; X = the box's byte offset; DB =
 * $81; called by JSL. Exit: M 8-bit; C as above; A = the last sum (16
 * bits); Y = the facing less one when the height passed; scratch $0C-$11 the
 * half-height, height and width.
 */
void abi_PlayerInFrontBox(void)
{
    ldy8(ea_dp(0x14));  /* 81AEFD ldy */
    goto L_81AF03;  /* 81AEFF bra */
L_81AF03:
    op_rep(0x20);  /* 81AF03 rep */
    lda16(ea_absx(0xBC8F));  /* 81AF05 lda */
    sta16(ea_abs(SCRATCH));  /* 81AF08 sta */
    ACC16(asl);  /* 81AF0B asl */
    sta16(ea_abs(0x000E));  /* 81AF0C sta */
    lda16(ea_absx(0xBC91));  /* 81AF0F lda */
    adc16_v(rd16(ea_absx(0xBC93)));  /* 81AF12 adc */
    sta16(ea_abs(0x0010));  /* 81AF15 sta */
    lda16(ea_dp(0x0F));  /* 81AF18 lda */
    clc();  /* 81AF1A clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 81AF1B adc */
    sec();  /* 81AF1E sec */
    sbc16_v(rd16(ea_abs(0x08EA)));  /* 81AF1F sbc */
    cmp16_v(rd16(ea_abs(0x000E)));  /* 81AF22 cmp */
    if (IF_C) goto L_81AF38;  /* 81AF25 bcs */
    dey8();  /* 81AF27 dey */
    if (IF_N) goto L_81AF2C;  /* 81AF28 bmi */
    inx8();  /* 81AF2A inx */
    inx8();  /* 81AF2B inx */
L_81AF2C:
    lda16(ea_dp(0x0C));  /* 81AF2C lda */
    adc16_v(rd16(ea_absx(0xBC91)));  /* 81AF2E adc */
    sec();  /* 81AF31 sec */
    sbc16_v(rd16(ea_abs(0x08E7)));  /* 81AF32 sbc */
    cmp16_v(rd16(ea_abs(0x0010)));  /* 81AF35 cmp */
L_81AF38:
    op_sep(0x20);  /* 81AF38 sep */
    return;  /* 81AF3A rtl */
}
