/*
 * shot_hit_face.c: an actor a shot of the second pool touches: it turns to
 * face Aladdin and goes to its hit state, keeping where the shot was.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorStateSet8CLong(void);
void abi_ActorTouchesSecondPoolLong(void);

#define HIT_SHOT_X 0x48u  /* word: the shot's x pixels */
#define HIT_SHOT_Y 0x4Au  /* word: the shot's y pixels */

/*
 * @routine 84:821A ActorShotHitFace
 * WHAT IT DOES: When one of the second pool's shots touches the actor: it
 * faces Aladdin, takes state $CC (hit), copies $42 to $46 (with $42 zero,
 * ActorStateSet8CLong as well), and keeps the shot's position in $48 and
 * $4A.
 * HOW: ActorTouchesSecondPoolLong; C set (none): out. Its x compared with
 * the player's: facing and flip = the carry (1: he is left of it); state
 * $CC; $46 = $42, zero: ActorStateSet8CLong; the touching record's (Y) x
 * and y pixels to $48 and $4A. M and X 8-bit.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; called by JSR
 * (84:8200). Exit: M and X 8-bit; C as ActorTouchesSecondPoolLong left it
 * when none touched.
 */
void abi_ActorShotHitFace(void)
{
    abi_ActorTouchesSecondPoolLong();  /* 84821A jsl */
    if (IF_C) goto L_848246;  /* 84821E bcs */
    ldx16(ea_dp(ACTOR_X + 1));  /* 848220 ldx */
    cpx16_v(rd16(ea_abs(PLAYER + ACTOR_X + 1)));  /* 848222 cpx */
    lda8_v(0x00);  /* 848225 lda */
    ACC8(rol);  /* 848227 rol */
    sta8(ea_dp(ACTOR_FACING));  /* 848228 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 84822A sta */
    lda8_v(0xCC);  /* 84822C lda */
    sta8(ea_dp(ACTOR_STATE));  /* 84822E sta */
    lda8(ea_dp(ACTOR_42));  /* 848230 lda */
    sta8(ea_dp(ACTOR_46));  /* 848232 sta */
    if (!IF_Z) goto L_84823A;  /* 848234 bne */
    abi_ActorStateSet8CLong();  /* 848236 jsl */
L_84823A:
    op_rep(0x20);  /* 84823A rep */
    lda16(ea_absy(ACTOR_X + 1));  /* 84823C lda */
    sta16(ea_dp(HIT_SHOT_X));  /* 84823F sta */
    lda16(ea_absy(ACTOR_Y + 1));  /* 848241 lda */
    sta16(ea_dp(HIT_SHOT_Y));  /* 848244 sta */
L_848246:
    op_sep(0x30);  /* 848246 sep */
    return;  /* 848248 rts */
}
