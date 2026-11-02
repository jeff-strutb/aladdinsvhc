/*
 * player_bounds.c: where the player is against the screen and the level:
 * kept on screen across, fallen out of the bottom, and the countdown of
 * the time it cannot be hurt. Drafted by cdraft.py; named and described by
 * hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram_video.h"
#include "actor.h"
#include "wram_player.h"

/* The play field's scroll (whole pixels, words; see wram_video.h) */
#define FIELD_SCROLL_X (LAYER0 + LAYER_SCROLL_X)
#define FIELD_SCROLL_Y (LAYER0 + LAYER_SCROLL_Y)
/* In bank $80: the least and most x pixel from the screen's left edge */
#define SCREEN_X_LIMITS 0xC9B9u
/* In bank $80: the least and most y pixel from the screen's top */
#define SCREEN_Y_LIMITS 0xC9C9u
/* The player's y pixels (word), and its top byte */
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define V_0347 0x000347u

/*
 * @routine 81:E8EC PlayerFallenTest
 * WHAT IT DOES: Tells whether the player is still above the bottom of the
 * level: C clear once it has fallen 230 pixels below the scroll's lowest
 * place.
 * HOW: Its y pixels' top byte negative (above the level): C set. Else the
 * scroll Y limit, or the scroll Y itself while an empty block stops the
 * scroll up or down, plus $E6, compared with its y pixels.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: C set: above; C
 * clear: below. M 8-bit, X 8-bit; X = CAMERA_STOPPED_Y's low byte (unless
 * above).
 */
void abi_PlayerFallenTest(void)
{
    sec();  /* 81E8EC sec */
    lda8(ea_abs(PLAYER_Y_PIXELS + 1));  /* 81E8ED lda */
    if (IF_N) goto L_81E908;  /* 81E8F0 bmi */
    op_rep(0x20);  /* 81E8F2 rep */
    lda16(ea_abs(SCROLL_LIMIT_Y));  /* 81E8F4 lda */
    ldx8(ea_abs(CAMERA_STOPPED_Y));  /* 81E8F7 ldx */
    if (IF_Z) goto L_81E8FF;  /* 81E8FA beq */
    lda16(ea_abs(FIELD_SCROLL_Y));  /* 81E8FC lda */
L_81E8FF:
    clc();  /* 81E8FF clc */
    adc16_v(0x00E6);  /* 81E900 adc */
    cmp16_v(rd16(ea_abs(PLAYER_Y_PIXELS)));  /* 81E903 cmp */
    op_sep(0x20);  /* 81E906 sep */
L_81E908:
    return;  /* 81E908 rts */
}


/*
 * @routine 81:E95C PlayerHurtCountdown
 * WHAT IT DOES: Counts down the time the player cannot be hurt; when it
 * runs out, the player can be hit again.
 * HOW: PLAYER_HURT_TIME, when nonzero, steps down; at 0, bit 4 of the
 * player's $08, bit 7 of the actor's $08 and bit 2 of $0347 are cleared.
 * Entry: M 8-bit; D = the player's record; called by JSR. Exit: M 8-bit;
 * ran out: A low = the new $0347, N and Z from it; else A low = the count
 * before its step.
 */
void abi_PlayerHurtCountdown(void)
{
    lda8(ea_abs(PLAYER_HURT_TIME));  /* 81E95C lda */
    if (IF_Z) goto L_81E97C;  /* 81E95F beq */
    MEM8(dec, ea_abs(PLAYER_HURT_TIME));  /* 81E961 dec */
    if (!IF_Z) goto L_81E97C;  /* 81E964 bne */
    lda8(ea_abs(0x08E3));  /* 81E966 lda */
    and8_v(0xEF);  /* 81E969 and */
    sta8(ea_abs(0x08E3));  /* 81E96B sta */
    lda8(ea_dp(0x08));  /* 81E96E lda */
    and8_v(0x7F);  /* 81E970 and */
    sta8(ea_dp(0x08));  /* 81E972 sta */
    lda8(ea_abs(V_0347));  /* 81E974 lda */
    and8_v(0xFB);  /* 81E977 and */
    sta8(ea_abs(V_0347));  /* 81E979 sta */
L_81E97C:
    return;  /* 81E97C rts */
}


/*
 * @routine 81:E990 PlayerKeepOnScreenX
 * WHAT IT DOES: Keeps the actor's x on screen: between 16 and 240 pixels
 * from the play field's left edge.
 * HOW: The play field's scroll X plus each limit (from the table) compared
 * with its x pixels, unsigned; past one, its x set to it.
 * Entry: M 8-bit; D = the actor's record; DB = $80; called by JSR. Exit: M
 * 8-bit; A = the last limit computed; C from its comparison.
 */
void abi_PlayerKeepOnScreenX(void)
{
    op_rep(0x20);  /* 81E990 rep */
    clc();  /* 81E992 clc */
    lda16(ea_abs(FIELD_SCROLL_X));  /* 81E993 lda */
    adc16_v(rd16(ea_abs(SCREEN_X_LIMITS)));  /* 81E996 adc */
    cmp16_v(rd16(ea_dp(0x0C)));  /* 81E999 cmp */
    if (IF_C) goto L_81E9A8;  /* 81E99B bcs */
    clc();  /* 81E99D clc */
    lda16(ea_abs(FIELD_SCROLL_X));  /* 81E99E lda */
    adc16_v(rd16(ea_abs(SCREEN_X_LIMITS + 2)));  /* 81E9A1 adc */
    cmp16_v(rd16(ea_dp(0x0C)));  /* 81E9A4 cmp */
    if (IF_C) goto L_81E9AA;  /* 81E9A6 bcs */
L_81E9A8:
    sta16(ea_dp(0x0C));  /* 81E9A8 sta */
L_81E9AA:
    op_sep(0x20);  /* 81E9AA sep */
    return;  /* 81E9AC rts */
}

/*
 * @routine 81:E9AD PlayerKeepOnScreen
 * WHAT IT DOES: Keeps the actor on screen: x between 16 and 240 pixels
 * from the play field's left edge, y between 24 and 216 from its top.
 * HOW: PlayerKeepOnScreenX; then the play field's scroll Y plus each y
 * limit (from the table) compared with its y pixels, unsigned; past one,
 * its y set to it.
 * Entry: M 8-bit; D = the actor's record; DB = $80; called by JSR. Exit: M
 * 8-bit; A = the last limit computed; C from its comparison.
 */
void abi_PlayerKeepOnScreen(void)
{
    abi_PlayerKeepOnScreenX();  /* 81E9AD jsr */
    op_rep(0x20);  /* 81E9B0 rep */
    clc();  /* 81E9B2 clc */
    lda16(ea_abs(0x1777));  /* 81E9B3 lda */
    adc16_v(rd16(ea_abs(0xC9C9)));  /* 81E9B6 adc */
    cmp16_v(rd16(ea_dp(0x0F)));  /* 81E9B9 cmp */
    if (IF_C) goto L_81E9C8;  /* 81E9BB bcs */
    clc();  /* 81E9BD clc */
    lda16(ea_abs(0x1777));  /* 81E9BE lda */
    adc16_v(rd16(ea_abs(0xC9CB)));  /* 81E9C1 adc */
    cmp16_v(rd16(ea_dp(0x0F)));  /* 81E9C4 cmp */
    if (IF_C) goto L_81E9CA;  /* 81E9C6 bcs */
L_81E9C8:
    sta16(ea_dp(0x0F));  /* 81E9C8 sta */
L_81E9CA:
    op_sep(0x20);  /* 81E9CA sep */
    return;  /* 81E9CC rts */
}

