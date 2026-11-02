/*
 * player_attach.c: the player put at an offset from an actor (below or
 * above it, and to one side by the player's facing), from a table of
 * offsets.
 */
#include "machine.h"
#include "ops65.h"
void abi_PlayerToActorAbove(void);
void abi_PlayerToActorBelow(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 82:BB3A PlayerToActorAbove
 * WHAT IT DOES: Puts the player at offset A from the actor: above it by
 * the offset's y, and by its x to the right when the player faces right,
 * to the left otherwise.
 * HOW: Entry A of the offsets at $80:FE44 (x, y bytes): the
 * player's y pixels = the actor's less y (16 bits, borrowing); facing
 * right (its flip nonzero) its x pixels = the actor's plus x, else
 * less x.
 * Entry: M 8-bit, X 8-bit; A = the offset's number; D = the actor; DB =
 * $80; called by JSL. Exit: X = A * 2; A low byte = the high byte of
 * the player's x.
 */
void abi_PlayerToActorAbove(void)
{
    ACC8(asl);  /* 82BB3A asl */
    tax8();  /* 82BB3B tax */
    sec();  /* 82BB3C sec */
    lda8(ea_dp(0x0F));  /* 82BB3D lda */
    sbc8_v(rd8(ea_absx(0xFE45)));  /* 82BB3F sbc */
    sta8(ea_abs(0x08EA));  /* 82BB42 sta */
    lda8(ea_dp(0x10));  /* 82BB45 lda */
    sbc8_v(0x00);  /* 82BB47 sbc */
    sta8(ea_abs(0x08EB));  /* 82BB49 sta */
    lda8(ea_abs(0x08F0));  /* 82BB4C lda */
    if (IF_Z) goto L_82BB62;  /* 82BB4F beq */
    clc();  /* 82BB51 clc */
    lda8(ea_dp(0x0C));  /* 82BB52 lda */
    adc8_v(rd8(ea_absx(0xFE44)));  /* 82BB54 adc */
    sta8(ea_abs(0x08E7));  /* 82BB57 sta */
    lda8(ea_dp(0x0D));  /* 82BB5A lda */
    adc8_v(0x00);  /* 82BB5C adc */
    sta8(ea_abs(0x08E8));  /* 82BB5E sta */
    return;  /* 82BB61 rtl */
L_82BB62:
    sec();  /* 82BB62 sec */
    lda8(ea_dp(0x0C));  /* 82BB63 lda */
    sbc8_v(rd8(ea_absx(0xFE44)));  /* 82BB65 sbc */
    sta8(ea_abs(0x08E7));  /* 82BB68 sta */
    lda8(ea_dp(0x0D));  /* 82BB6B lda */
    sbc8_v(0x00);  /* 82BB6D sbc */
    sta8(ea_abs(0x08E8));  /* 82BB6F sta */
    return;  /* 82BB72 rtl */
}

/*
 * @routine 82:BB26 PlayerToActorBelow
 * WHAT IT DOES: Puts the player at offset A from the actor: below it by
 * the offset's y, and by its x to the right when the player faces right,
 * to the left otherwise.
 * HOW: Entry A of the offsets at $80:FE44 (x, y bytes): the
 * player's y pixels = the actor's plus y (16 bits, carried); then the x
 * as PlayerToActorAbove.
 * Entry: M 8-bit, X 8-bit; A = the offset's number; D = the actor; DB =
 * $80; called by JSL. Exit: as PlayerToActorAbove.
 */
void abi_PlayerToActorBelow(void)
{
    ACC8(asl);  /* 82BB26 asl */
    tax8();  /* 82BB27 tax */
    clc();  /* 82BB28 clc */
    lda8(ea_dp(0x0F));  /* 82BB29 lda */
    adc8_v(rd8(ea_absx(0xFE45)));  /* 82BB2B adc */
    sta8(ea_abs(0x08EA));  /* 82BB2E sta */
    lda8(ea_dp(0x10));  /* 82BB31 lda */
    adc8_v(0x00);  /* 82BB33 adc */
    sta8(ea_abs(0x08EB));  /* 82BB35 sta */
    goto L_82BB4C;  /* 82BB38 bra */
L_82BB4C:
    lda8(ea_abs(0x08F0));  /* 82BB4C lda */
    if (IF_Z) goto L_82BB62;  /* 82BB4F beq */
    clc();  /* 82BB51 clc */
    lda8(ea_dp(0x0C));  /* 82BB52 lda */
    adc8_v(rd8(ea_absx(0xFE44)));  /* 82BB54 adc */
    sta8(ea_abs(0x08E7));  /* 82BB57 sta */
    lda8(ea_dp(0x0D));  /* 82BB5A lda */
    adc8_v(0x00);  /* 82BB5C adc */
    sta8(ea_abs(0x08E8));  /* 82BB5E sta */
    return;  /* 82BB61 rtl */
L_82BB62:
    sec();  /* 82BB62 sec */
    lda8(ea_dp(0x0C));  /* 82BB63 lda */
    sbc8_v(rd8(ea_absx(0xFE44)));  /* 82BB65 sbc */
    sta8(ea_abs(0x08E7));  /* 82BB68 sta */
    lda8(ea_dp(0x0D));  /* 82BB6B lda */
    sbc8_v(0x00);  /* 82BB6D sbc */
    sta8(ea_abs(0x08E8));  /* 82BB6F sta */
    return;  /* 82BB72 rtl */
}
