/*
 * ground_slide.c: ground that slides Aladdin along it: tiles whose
 * behaviour has bit 4 set, in the levels where the slide replaces the push
 * (ground_push.c). Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorStepX(void);
void abi_GroundBehindBit4(void);

#define GROUND_PUSHED 0x1429u  /* 2 while the player stands on ground that pushes, 1 while he slides on it, else 0 */

/*
 * @routine 81:CEF4 PlayerGroundSlide
 * WHAT IT DOES: On ground that slides (tile behaviour bit 4), turns
 * Aladdin the way the tile's bit 7 says and moves him along it by half a
 * pixel a frame, marking him sliding.
 * HOW: GROUND_PUSHED cleared; GroundBehindBit4 with probe $18: zero, out
 * (the RTS at $81:CEF3). Else facing = 1 (bit 7 clear, C clear) or 0;
 * GROUND_PUSHED = 1; speed x = $80 (24-bit); ActorStepX.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:CD0A). Exit: as ActorStepX, or as
 * GroundBehindBit4 with Z set.
 */
void abi_PlayerGroundSlide(void)
{
    goto L_81CEF4;
L_81CEF3:
    return;  /* 81CEF3 rts */
L_81CEF4:
    stz8(ea_abs(GROUND_PUSHED));  /* 81CEF4 stz */
    lda8_v(0x18);  /* 81CEF7 lda */
    sta8(ea_dp(ACTOR_18));  /* 81CEF9 sta */
    abi_GroundBehindBit4();  /* 81CEFB jsl */
    if (IF_Z) goto L_81CEF3;  /* 81CEFF beq */
    if (IF_C) goto L_81CF09;  /* 81CF01 bcs */
    lda8_v(0x01);  /* 81CF03 lda */
    sta8(ea_dp(ACTOR_FACING));  /* 81CF05 sta */
    goto L_81CF0B;  /* 81CF07 bra */
L_81CF09:
    stz8(ea_dp(ACTOR_FACING));  /* 81CF09 stz */
L_81CF0B:
    lda8_v(0x01);  /* 81CF0B lda */
    sta8(ea_abs(GROUND_PUSHED));  /* 81CF0D sta */
    lda8_v(0x80);  /* 81CF10 lda */
    sta8(ea_dp(ACTOR_SPEED_X));  /* 81CF12 sta */
    stz8(ea_dp(ACTOR_SPEED_X + 1));  /* 81CF14 stz */
    stz8(ea_dp(ACTOR_SPEED_X + 2));  /* 81CF16 stz */
    abi_ActorStepX();  /* 81CF18 jsr */
    return;  /* 81CF1B rts */
}
