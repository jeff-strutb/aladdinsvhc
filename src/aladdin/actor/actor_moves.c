/*
 * actor_moves.c: small steps of an actor's motion: speeding up and
 * moving, moving along a direction from the ROM's table, keeping its
 * place; and a byte's sign spread to a word.
 *
 * A direction's step is kept as two 24-bit values, x then y, at $0C-$0E
 * and $10-$12; each is a byte at a time multiplied by the speed.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorSpeedUpMove(void);
void abi_ActorMoveAlong(void);
void abi_ActorMoveScaled(void);
void abi_ActorKeepSpot(void);
void abi_SignExtend(void);
#include "hw.h"
#include "actor.h"
#include "wram.h"
void abi_ActorMove(void);
void abi_MultiplyBy(void);

/*
 * @routine 81:A4AF ActorSpeedUpMove
 * WHAT IT DOES: Speeds the actor up by its acceleration, then moves it.
 * HOW: The 24-bit horizontal speed plus the acceleration (a fraction),
 * then ActorMove.
 * Entry: M 8-bit; D = the actor; called by JSL. Exit: as ActorMove.
 */
void abi_ActorSpeedUpMove(void)
{
    clc();  /* 81A4AF clc */
    lda8(ea_dp(0x19));  /* 81A4B0 lda */
    adc8_v(rd8(ea_dp(0x1C)));  /* 81A4B2 adc */
    sta8(ea_dp(0x19));  /* 81A4B4 sta */
    lda8(ea_dp(0x1A));  /* 81A4B6 lda */
    adc8_v(0x00);  /* 81A4B8 adc */
    sta8(ea_dp(0x1A));  /* 81A4BA sta */
    lda8(ea_dp(0x1B));  /* 81A4BC lda */
    adc8_v(0x00);  /* 81A4BE adc */
    sta8(ea_dp(0x1B));  /* 81A4C0 sta */
    goto L_81A4E0;  /* 81A4C2 bra */
L_81A4E0:
    abi_ActorMove(); return;  /* 81A4E0 jmp */
}

/*
 * @routine 81:A73D ActorMoveAlong
 * WHAT IT DOES: Moves the actor along direction Y of the ROM's direction
 * table ($1B6 on), at speed A.
 * HOW: The direction's step, a byte at a time from the six tables in bank
 * $82, to $0C-$12; then as ActorMoveScaled.
 * Entry: M 8-bit, X 16-bit; A = the speed; Y = the direction; D = the
 * actor; called by JSL. Exit: as ActorMoveScaled.
 */
void abi_ActorMoveAlong(void)
{
    pha8();  /* 81A73D pha */
    op_rep(0x20);  /* 81A73E rep */
    tya16();  /* 81A740 tya */
    clc();  /* 81A741 clc */
    adc16_v(0x01B6);  /* 81A742 adc */
    tax16();  /* 81A745 tax */
    goto L_81A759;  /* 81A746 bra */
L_81A759:
    op_sep(0x20);  /* 81A759 sep */
    lda8(ea_longx(0x82C000u));  /* 81A75B lda */
    sta8(ea_abs(SCRATCH));  /* 81A75F sta */
    lda8(ea_longx(0x82C409u));  /* 81A762 lda */
    sta8(ea_abs(SCRATCH + 1));  /* 81A766 sta */
    lda8(ea_longx(0x82C812u));  /* 81A769 lda */
    sta8(ea_abs(SCRATCH + 2));  /* 81A76D sta */
    lda8(ea_longx(0x82CC1Bu));  /* 81A770 lda */
    sta8(ea_abs(SCRATCH + 4));  /* 81A774 sta */
    lda8(ea_longx(0x82D024u));  /* 81A777 lda */
    sta8(ea_abs(SCRATCH + 5));  /* 81A77B sta */
    lda8(ea_longx(0x82D42Du));  /* 81A77E lda */
    sta8(ea_abs((SCRATCH + 6)));  /* 81A782 sta */
    pla8();  /* 81A785 pla */
    sta8(ea_abs(WRMPYA));  /* 81A786 sta */
    lda8(ea_abs(SCRATCH));  /* 81A789 lda */
    abi_MultiplyBy();  /* 81A78C jsr */
    lda8(ea_abs(SCRATCH + 1));  /* 81A78F lda */
    sty16(ea_abs(SCRATCH));  /* 81A792 sty */
    abi_MultiplyBy();  /* 81A795 jsr */
    tya8();  /* 81A798 tya */
    clc();  /* 81A799 clc */
    adc8_v(rd8(ea_abs(SCRATCH + 1)));  /* 81A79A adc */
    sta8(ea_abs(SCRATCH + 1));  /* 81A79D sta */
    lda8(ea_abs(SCRATCH + 4));  /* 81A7A0 lda */
    abi_MultiplyBy();  /* 81A7A3 jsr */
    lda8(ea_abs(SCRATCH + 5));  /* 81A7A6 lda */
    sty16(ea_abs(SCRATCH + 4));  /* 81A7A9 sty */
    abi_MultiplyBy();  /* 81A7AC jsr */
    tya8();  /* 81A7AF tya */
    clc();  /* 81A7B0 clc */
    adc8_v(rd8(ea_abs(SCRATCH + 5)));  /* 81A7B1 adc */
    sta8(ea_abs(SCRATCH + 5));  /* 81A7B4 sta */
    clc();  /* 81A7B7 clc */
    lda8(ea_dp(0x0B));  /* 81A7B8 lda */
    adc8_v(rd8(ea_abs(SCRATCH)));  /* 81A7BA adc */
    sta8(ea_dp(0x0B));  /* 81A7BD sta */
    lda8(ea_dp(0x0C));  /* 81A7BF lda */
    adc8_v(rd8(ea_abs(SCRATCH + 1)));  /* 81A7C1 adc */
    sta8(ea_dp(0x0C));  /* 81A7C4 sta */
    lda8(ea_dp(0x0D));  /* 81A7C6 lda */
    adc8_v(rd8(ea_abs(SCRATCH + 2)));  /* 81A7C8 adc */
    sta8(ea_dp(0x0D));  /* 81A7CB sta */
    clc();  /* 81A7CD clc */
    lda8(ea_dp(0x0E));  /* 81A7CE lda */
    adc8_v(rd8(ea_abs(SCRATCH + 4)));  /* 81A7D0 adc */
    sta8(ea_dp(0x0E));  /* 81A7D3 sta */
    lda8(ea_dp(0x0F));  /* 81A7D5 lda */
    adc8_v(rd8(ea_abs(SCRATCH + 5)));  /* 81A7D7 adc */
    sta8(ea_dp(0x0F));  /* 81A7DA sta */
    lda8(ea_dp(0x10));  /* 81A7DC lda */
    adc8_v(rd8(ea_abs((SCRATCH + 6))));  /* 81A7DE adc */
    sta8(ea_dp(0x10));  /* 81A7E1 sta */
    op_sep(0x30);  /* 81A7E3 sep */
    return;  /* 81A7E5 rtl */
}

/*
 * @routine 81:A748 ActorMoveScaled
 * WHAT IT DOES: Moves the actor by the step at $0C-$12 multiplied by
 * speed A: x by the step's x, y by its y.
 * HOW: Each step's low and middle bytes times the speed (MultiplyBy, the
 * products overlapped into 24 bits), then added to the actor's x and y.
 * Entry: M 8-bit; A = the speed; $0C-$0E = the step's x; $10-$12 = its y;
 * D = the actor; called by JSL. Exit: M 8-bit, X 8-bit; $0C-$12 the
 * scaled step.
 */
void abi_ActorMoveScaled(void)
{
    op_rep(0x10);  /* 81A748 rep */
    goto L_81A786;  /* 81A74A bra */
L_81A786:
    sta8(ea_abs(WRMPYA));  /* 81A786 sta */
    lda8(ea_abs(SCRATCH));  /* 81A789 lda */
    abi_MultiplyBy();  /* 81A78C jsr */
    lda8(ea_abs(SCRATCH + 1));  /* 81A78F lda */
    sty16(ea_abs(SCRATCH));  /* 81A792 sty */
    abi_MultiplyBy();  /* 81A795 jsr */
    tya8();  /* 81A798 tya */
    clc();  /* 81A799 clc */
    adc8_v(rd8(ea_abs(SCRATCH + 1)));  /* 81A79A adc */
    sta8(ea_abs(SCRATCH + 1));  /* 81A79D sta */
    lda8(ea_abs(SCRATCH + 4));  /* 81A7A0 lda */
    abi_MultiplyBy();  /* 81A7A3 jsr */
    lda8(ea_abs(SCRATCH + 5));  /* 81A7A6 lda */
    sty16(ea_abs(SCRATCH + 4));  /* 81A7A9 sty */
    abi_MultiplyBy();  /* 81A7AC jsr */
    tya8();  /* 81A7AF tya */
    clc();  /* 81A7B0 clc */
    adc8_v(rd8(ea_abs(SCRATCH + 5)));  /* 81A7B1 adc */
    sta8(ea_abs(SCRATCH + 5));  /* 81A7B4 sta */
    clc();  /* 81A7B7 clc */
    lda8(ea_dp(0x0B));  /* 81A7B8 lda */
    adc8_v(rd8(ea_abs(SCRATCH)));  /* 81A7BA adc */
    sta8(ea_dp(0x0B));  /* 81A7BD sta */
    lda8(ea_dp(0x0C));  /* 81A7BF lda */
    adc8_v(rd8(ea_abs(SCRATCH + 1)));  /* 81A7C1 adc */
    sta8(ea_dp(0x0C));  /* 81A7C4 sta */
    lda8(ea_dp(0x0D));  /* 81A7C6 lda */
    adc8_v(rd8(ea_abs(SCRATCH + 2)));  /* 81A7C8 adc */
    sta8(ea_dp(0x0D));  /* 81A7CB sta */
    clc();  /* 81A7CD clc */
    lda8(ea_dp(0x0E));  /* 81A7CE lda */
    adc8_v(rd8(ea_abs(SCRATCH + 4)));  /* 81A7D0 adc */
    sta8(ea_dp(0x0E));  /* 81A7D3 sta */
    lda8(ea_dp(0x0F));  /* 81A7D5 lda */
    adc8_v(rd8(ea_abs(SCRATCH + 5)));  /* 81A7D7 adc */
    sta8(ea_dp(0x0F));  /* 81A7DA sta */
    lda8(ea_dp(0x10));  /* 81A7DC lda */
    adc8_v(rd8(ea_abs((SCRATCH + 6))));  /* 81A7DE adc */
    sta8(ea_dp(0x10));  /* 81A7E1 sta */
    op_sep(0x30);  /* 81A7E3 sep */
    return;  /* 81A7E5 rtl */
}

/*
 * @routine 81:9D56 ActorKeepSpot
 * WHAT IT DOES: Keeps the actor's place: its x and y pixels into $4C and
 * $4E of its record.
 * HOW: Two 16-bit copies.
 * Entry: D = the actor; called by JSL. Exit: M 8-bit; A = the y pixels.
 */
void abi_ActorKeepSpot(void)
{
    op_rep(0x20);  /* 819D56 rep */
    lda16(ea_dp(0x0C));  /* 819D58 lda */
    sta16(ea_dp(0x4C));  /* 819D5A sta */
    lda16(ea_dp(0x0F));  /* 819D5C lda */
    sta16(ea_dp(0x4E));  /* 819D5E sta */
    op_sep(0x20);  /* 819D60 sep */
    return;  /* 819D62 rtl */
}

/*
 * @routine 81:945F SignExtend
 * WHAT IT DOES: Spreads the sign of A's low byte to its high byte.
 * HOW: Bit 7 clear: A & $FF; set: A | $FF00.
 * Entry: M 16-bit; called by JSR. Exit: A the word; N and Z from it.
 */
void abi_SignExtend(void)
{
    pha16();  /* 81945F pha */
    and16_v(0x0080);  /* 819460 and */
    if (!IF_Z) goto L_81946A;  /* 819463 bne */
    pla16();  /* 819465 pla */
    and16_v(0x00FF);  /* 819466 and */
    return;  /* 819469 rts */
L_81946A:
    pla16();  /* 81946A pla */
    ora16_v(0xFF00);  /* 81946B ora */
    return;  /* 81946E rts */
}
