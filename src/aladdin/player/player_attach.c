/*
 * player_attach.c: the player put at an offset from an actor (below or
 * above it, and to one side by the player's facing), from a table of
 * offsets; or only the spot it would go to, kept.
 */
#include "machine.h"
#include "ops65.h"
void abi_PlayerSpotAboveActor(void);
void abi_PlayerSpotBelowActor(void);
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

/*
 * @routine 82:BB73 PlayerSpotAboveActor
 * WHAT IT DOES: Works out where PlayerToActorAbove would put the player for
 * offset A, keeping that spot in player_pos_saved and leaving the player
 * where it was.
 * HOW: The player's x and y pushed; PlayerToActorAbove; the x and y it set
 * copied to player_pos_saved (x, then y at +2); the pushed x and y put
 * back.
 * Entry: M 8-bit; A = the offset's number; D = the actor; DB = $80;
 * called by JSL. Exit: M 8-bit, X 8-bit; X = the player's x low byte.
 */
void abi_PlayerSpotAboveActor(void)
{
    op_rep(0x10);  /* 82BB73 rep */
    ldx16(ea_abs(0x08E7));  /* 82BB75 ldx */
    phx16();  /* 82BB78 phx */
    ldx16(ea_abs(0x08EA));  /* 82BB79 ldx */
    phx16();  /* 82BB7C phx */
    op_sep(0x10);  /* 82BB7D sep */
    abi_PlayerToActorAbove();  /* 82BB7F jsl */
    op_rep(0x10);  /* 82BB83 rep */
    ldx16(ea_abs(0x08E7));  /* 82BB85 ldx */
    stx16(ea_abs(PLAYER_POS_SAVED));  /* 82BB88 stx */
    ldx16(ea_abs(0x08EA));  /* 82BB8B ldx */
    stx16(ea_abs(0x145F));  /* 82BB8E stx */
    plx16();  /* 82BB91 plx */
    stx16(ea_abs(0x08EA));  /* 82BB92 stx */
    plx16();  /* 82BB95 plx */
    stx16(ea_abs(0x08E7));  /* 82BB96 stx */
    op_sep(0x10);  /* 82BB99 sep */
    return;  /* 82BB9B rtl */
}

/*
 * @routine 82:BB9C PlayerSpotBelowActor
 * WHAT IT DOES: Works out where PlayerToActorBelow would put the player for
 * offset A, keeping that spot in player_pos_saved and leaving the player
 * where it was.
 * HOW: The player's x and y pushed; PlayerToActorBelow; then as
 * PlayerSpotAboveActor.
 * Entry: M 8-bit; A = the offset's number; D = the actor; DB = $80;
 * called by JSL. Exit: as PlayerSpotAboveActor.
 */
void abi_PlayerSpotBelowActor(void)
{
    goto L_82BB9C;
L_82BB83:
    op_rep(0x10);  /* 82BB83 rep */
    ldx16(ea_abs(0x08E7));  /* 82BB85 ldx */
    stx16(ea_abs(PLAYER_POS_SAVED));  /* 82BB88 stx */
    ldx16(ea_abs(0x08EA));  /* 82BB8B ldx */
    stx16(ea_abs(0x145F));  /* 82BB8E stx */
    plx16();  /* 82BB91 plx */
    stx16(ea_abs(0x08EA));  /* 82BB92 stx */
    plx16();  /* 82BB95 plx */
    stx16(ea_abs(0x08E7));  /* 82BB96 stx */
    op_sep(0x10);  /* 82BB99 sep */
    return;  /* 82BB9B rtl */
L_82BB9C:
    op_rep(0x10);  /* 82BB9C rep */
    ldx16(ea_abs(0x08E7));  /* 82BB9E ldx */
    phx16();  /* 82BBA1 phx */
    ldx16(ea_abs(0x08EA));  /* 82BBA2 ldx */
    phx16();  /* 82BBA5 phx */
    op_sep(0x10);  /* 82BBA6 sep */
    abi_PlayerToActorBelow();  /* 82BBA8 jsl */
    goto L_82BB83;  /* 82BBAC bra */
}
