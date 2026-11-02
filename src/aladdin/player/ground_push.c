/*
 * ground_push.c: ground that pushes the player along: tiles whose behaviour has bit 4
 * set. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_GroundBehindBit4(void);

#define GROUND_PUSHED 0x001429u     /* 2 while the player stands on ground that pushes, else 0 */

/*
 * @routine 81:CE86 PlayerGroundPush
 * WHAT IT DOES: On ground that pushes (tile behaviour bit 4), steps the
 * player's speed across by $80 one way or the other by its facing and the
 * tile's bit 7, up to $0280 and down to 0, and marks it pushed.
 * HOW: GROUND_PUSHED cleared; GroundBehindBit4 with probe $18: zero, done.
 * Else GROUND_PUSHED = 2; bit 7 clear (C clear): facing left adds $80 (at
 * most $0280), facing right takes $80 (not below 0); bit 7 set: the other
 * way round.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:CD87) in levels with $035C = 4 or level
 * $1A. Exit: M 8-bit; A low as the last step left it.
 */
void abi_PlayerGroundPush(void)
{
    stz8(ea_abs(0x1429));  /* 81CE86 stz */
    lda8_v(0x18);  /* 81CE89 lda */
    sta8(ea_dp(0x18));  /* 81CE8B sta */
    abi_GroundBehindBit4();  /* 81CE8D jsl */
    if (IF_Z) goto L_81CEF3;  /* 81CE91 beq */
    if (IF_C) goto L_81CEC5;  /* 81CE93 bcs */
    lda8_v(0x02);  /* 81CE95 lda */
    sta8(ea_abs(0x1429));  /* 81CE97 sta */
    lda8(ea_dp(0x14));  /* 81CE9A lda */
    if (IF_Z) goto L_81CEB5;  /* 81CE9C beq */
    op_rep(0x20);  /* 81CE9E rep */
    lda16(ea_dp(0x19));  /* 81CEA0 lda */
    clc();  /* 81CEA2 clc */
    adc16_v(0x0080);  /* 81CEA3 adc */
    sec();  /* 81CEA6 sec */
    cmp16_v(0x0280);  /* 81CEA7 cmp */
    if (!IF_C) goto L_81CEAF;  /* 81CEAA bcc */
    lda16_v(0x0280);  /* 81CEAC lda */
L_81CEAF:
    sta16(ea_dp(0x19));  /* 81CEAF sta */
    op_sep(0x20);  /* 81CEB1 sep */
    goto L_81CEF3;  /* 81CEB3 bra */
L_81CEB5:
    op_rep(0x20);  /* 81CEB5 rep */
    lda16(ea_dp(0x19));  /* 81CEB7 lda */
    sec();  /* 81CEB9 sec */
    sbc16_v(0x0080);  /* 81CEBA sbc */
    if (!IF_C) goto L_81CEC1;  /* 81CEBD bcc */
    sta16(ea_dp(0x19));  /* 81CEBF sta */
L_81CEC1:
    op_sep(0x20);  /* 81CEC1 sep */
    goto L_81CEF3;  /* 81CEC3 bra */
L_81CEC5:
    lda8_v(0x02);  /* 81CEC5 lda */
    sta8(ea_abs(0x1429));  /* 81CEC7 sta */
    lda8(ea_dp(0x14));  /* 81CECA lda */
    if (IF_Z) goto L_81CEDE;  /* 81CECC beq */
    op_rep(0x20);  /* 81CECE rep */
    lda16(ea_dp(0x19));  /* 81CED0 lda */
    sec();  /* 81CED2 sec */
    sbc16_v(0x0080);  /* 81CED3 sbc */
    if (!IF_C) goto L_81CEDA;  /* 81CED6 bcc */
    sta16(ea_dp(0x19));  /* 81CED8 sta */
L_81CEDA:
    op_sep(0x20);  /* 81CEDA sep */
    goto L_81CEF3;  /* 81CEDC bra */
L_81CEDE:
    op_rep(0x20);  /* 81CEDE rep */
    lda16(ea_dp(0x19));  /* 81CEE0 lda */
    clc();  /* 81CEE2 clc */
    adc16_v(0x0080);  /* 81CEE3 adc */
    sec();  /* 81CEE6 sec */
    cmp16_v(0x0280);  /* 81CEE7 cmp */
    if (!IF_C) goto L_81CEEF;  /* 81CEEA bcc */
    lda16_v(0x0280);  /* 81CEEC lda */
L_81CEEF:
    sta16(ea_dp(0x19));  /* 81CEEF sta */
    op_sep(0x20);  /* 81CEF1 sep */
L_81CEF3:
    return;  /* 81CEF3 rts */
}

