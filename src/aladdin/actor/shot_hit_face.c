/*
 * shot_hit_face.c: an actor a shot of the second pool touches: it turns to
 * face Aladdin and goes to its hit state, keeping where the shot was.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    uint16_t x, px, y;
    uint8_t face, hits;

    abi_ActorTouchesSecondPoolLong();
    if (cpu.p & P_C) {
        op_sep(0x30);
        return;
    }
    /* hit by the shot record Y: it faces the player */
    x = dp16(ACTOR_X + 1);
    px = rd16(dba(PLAYER + ACTOR_X + 1));
    cpu.x = x;
    face = x >= px;                       /* 1: he is to its left */
    a8_load(face);
    flag_set(P_C, 0);
    wr8(dpa(ACTOR_FACING), face);
    wr8(dpa(ACTOR_FLIP), face);
    a8_load(0xCC);
    wr8(dpa(ACTOR_STATE), 0xCC);
    hits = rd8(dpa(ACTOR_42));
    a8_load(hits);
    wr8(dpa(ACTOR_46), hits);
    if (!hits)
        abi_ActorStateSet8CLong();
    op_rep(P_M);
    dp16_set(HIT_SHOT_X, rd16(dba(ACTOR_X + 1) + cpu.y));
    y = rd16(dba(ACTOR_Y + 1) + cpu.y);
    dp16_set(HIT_SHOT_Y, y);
    a16_load(y);
    op_sep(0x30);
}
