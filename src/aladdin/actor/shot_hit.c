/*
 * shot_hit.c: an actor hit by a shot: an actor of the second pool (the
 * five records from $09CB) that can hit, found by the test at $83:8578.
 * The shot is stopped, or turned back, and a small actor marks the hit.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorStateSet8C(void);
void abi_ActorStateSet8CSelf(void);
void abi_ActorStateSetCC(void);
void abi_ActorTouchesSecondPool(void);
void abi_ActorStopsShot(void);
void abi_Spawn11AtY(void);
void abi_Spawn13AtY(void);

#define ACTOR_11 0x11u              /* a shot's: negative, it does not hit */
#define TURNED_BACK_AT 0xEB7Cu      /* where a turned-back shot's code resumes (its own bank) */

#define PLAYER_2F (PLAYER + 0x2Fu)  /* $0B or more: the side a harmless shot turns the actor to flips */










/*
 * @routine 83:819A ActorTurnsShotBack
 * WHAT IT DOES: When a shot hits the actor, turns it back: it flies the
 * other way, harmless, and a small actor (kind $13) marks the hit.
 * HOW: The shot test; on a hit, kind $13 started at the shot's place, the
 * shot's facing flipped, its code sent on at $EB7C (in its bank), bit 7
 * of its $08 set and of its $09 cleared.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: M and X 8-bit. Hit: C clear; A low = the shot's $09; N
 * clear, Z from it; Y = the low byte of the shot's record; X as $83:82B5
 * leaves it (low byte). No hit: C set; the rest as the shot test leaves
 * them.
 */
void abi_ActorTurnsShotBack(void)
{
    abi_ActorTouchesSecondPool();
    if (IF_C) {
        op_sep(0x31);
        return;
    }
    abi_Spawn13AtY();
    lda8(ea_absy(ACTOR_FACING));
    eor8_v(0x01);
    sta8(ea_absy(ACTOR_FACING));
    lda8_v((uint8_t)TURNED_BACK_AT);
    sta8(ea_absy(ACTOR_03));
    lda8_v((uint8_t)(TURNED_BACK_AT >> 8));
    sta8(ea_absy(ACTOR_04));
    lda8_v(0x80);
    ora8_v(rd8(ea_absy(ACTOR_08)));
    sta8(ea_absy(ACTOR_08));
    lda8(ea_absy(ACTOR_09));
    and8_v(0x7F);
    sta8(ea_absy(ACTOR_09));
    op_sep(0x30);
    clc();
}
/*
 * @routine 83:82B1 Spawn13AtYLong
 * WHAT IT DOES: The routine at $83:82B5, which starts an actor of kind
 * $13 at record Y's place, for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as $83:82B5: M 8-bit, X 16-bit; Y = the record; called by JSL.
 * Exit: as $83:82B5.
 */
void abi_Spawn13AtYLong(void)
{
    abi_Spawn13AtY();
}
/*
 * @routine 83:8196 ActorTurnsShotBackLong
 * WHAT IT DOES: ActorTurnsShotBack for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorTurnsShotBack; called by JSL. Exit: as
 * ActorTurnsShotBack.
 */
void abi_ActorTurnsShotBackLong(void)
{
    abi_ActorTurnsShotBack();
}
/*
 * @routine 83:81E8 ActorTakesShot
 * WHAT IT DOES: When a shot that hits (its $11 not negative) reaches the
 * actor: the shot stops (state $8C), a small actor (kind $11) marks the
 * hit, and the actor turns the way the shot flew.
 * HOW: The shot test; then the shot's $11; kind $11 started at the shot's
 * place; the shot's facing in the actor's facing and flip bytes;
 * ActorStateSet8C on the shot.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: M and X 8-bit. Hit: C clear; A low = $8C, N set, Z
 * clear; Y = the low byte of the shot's record. The shot's $11 negative:
 * C set, N set, A low = it. No shot: C set; the rest as the shot test
 * leaves them.
 */
void abi_ActorTakesShot(void)
{
    abi_ActorTouchesSecondPool();
    if (!IF_C) {
        sec();
        lda8(ea_absy(ACTOR_11));
        if (!IF_N) {
            abi_Spawn11AtY();
            lda8(ea_absy(ACTOR_FACING));
            sta8(ea_dp(ACTOR_FACING));
            sta8(ea_dp(ACTOR_FLIP));
            abi_ActorStateSet8C();
            clc();
        }
    }
    op_sep(0x30);
}

/* A shot that does not hit: the actor faces away from Aladdin's side,
   bit 0 of his flip byte plus 1 when his $2F is $0B or more, bit 7 set. */
static void facing_away(void)
{
    lda8(ea_abs(PLAYER_2F));
    cmp8_v(0x0B);
    if (IF_C)
        lda8_v(0x01);
    else
        lda8_v(0x00);
    clc();
    adc8_v(rd8(ea_abs(PLAYER + ACTOR_FLIP)));
    and8_v(0x01);
    ora8_v(0x80);
    sta8(ea_dp(ACTOR_FACING));
}

/*
 * @routine 83:80FC ActorDownedByShot
 * WHAT IT DOES: When a shot reaches the actor, the actor is downed (state
 * $8C): a shot that hits stops too (state $8C) and the actor turns the way
 * it flew; a small actor (kind $11) marks the hit.
 * HOW: ActorTouchesSecondPool; on a touch Spawn11AtY at the shot; the
 * shot's $11 not negative: the shot's facing taken, ActorStateSet8C on the
 * shot; negative (code the scripts have not run): its facing is bit 0 of
 * Aladdin's flip byte plus 1 when his $2F is $0B or more, with bit 7 set.
 * Then ActorStateSet8CSelf; C cleared.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the hit boxes' bank; called by JSR. Exit: M and X 8-bit.
 * Touched: C clear; A low = $8C, N set, Z clear; Y = the shot's record
 * (low byte). No touch: C set; as ActorTouchesSecondPool leaves the rest.
 */
void abi_ActorDownedByShot(void)
{
    abi_ActorTouchesSecondPool();
    if (IF_C)
        goto out;
    abi_Spawn11AtY();
    lda8(ea_absy(ACTOR_11));
    if (IF_N) {
        facing_away();
    } else {
        lda8(ea_absy(ACTOR_FACING));
        sta8(ea_dp(ACTOR_FACING));
        abi_ActorStateSet8C();
    }
    abi_ActorStateSet8CSelf();
    clc();
out:
    op_sep(0x30);
}

/*
 * @routine 83:8135 ActorHitByShotCC
 * WHAT IT DOES: When a shot reaches the actor: a shot that hits stops
 * (state $8C), the actor turns the way it flew and goes into state $CC; a
 * shot that does not hit downs the actor (state $8C); a small actor (kind
 * $11) marks either.
 * HOW: ActorTouchesSecondPool; on a touch Spawn11AtY at the shot; the
 * shot's $11 not negative: the shot's facing taken, ActorStateSet8C on the
 * shot, ActorStateSetCC; negative (code the scripts have not run): its
 * facing is bit 0 of Aladdin's flip byte plus 1 when his $2F is $0B or
 * more, with bit 7 set, then ActorStateSet8CSelf. C cleared.
 * Entry: as ActorDownedByShot. Exit: as ActorDownedByShot, A low = $CC or
 * $8C.
 */
void abi_ActorHitByShotCC(void)
{
    abi_ActorTouchesSecondPool();
    if (IF_C)
        goto out;
    abi_Spawn11AtY();
    lda8(ea_absy(ACTOR_11));
    if (IF_N) {
        facing_away();
        abi_ActorStateSet8CSelf();
    } else {
        lda8(ea_absy(ACTOR_FACING));
        sta8(ea_dp(ACTOR_FACING));
        abi_ActorStateSet8C();
        abi_ActorStateSetCC();
    }
    clc();
out:
    op_sep(0x30);
}

/*
 * @routine 83:816D ActorStopsShotLong
 * WHAT IT DOES: ActorStopsShot for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorStopsShot; called by JSL. Exit: as ActorStopsShot.
 */
void abi_ActorStopsShotLong(void)
{
    abi_ActorStopsShot();
}

/*
 * @routine 83:8171 ActorStopsShot
 * WHAT IT DOES: When a shot reaches the actor, a small actor (kind $11)
 * marks it; a shot that hits stops (state $8C); one that does not downs
 * the actor (state $8C), facing Aladdin's flip with bit 7 set.
 * HOW: ActorTouchesSecondPool; on a touch (code the scripts have not run)
 * Spawn11AtY at the shot; the shot's $11 not negative: ActorStateSet8C on
 * the shot; negative: the shot's facing stored, then Aladdin's flip byte
 * with bit 7 set as the actor's, and ActorStateSet8CSelf. C cleared.
 * Entry: as ActorDownedByShot. Exit: as ActorDownedByShot.
 */
void abi_ActorStopsShot(void)
{
    abi_ActorTouchesSecondPool();
    if (IF_C)
        goto out;
    abi_Spawn11AtY();
    lda8(ea_absy(ACTOR_11));
    if (IF_N) {
        lda8(ea_absy(ACTOR_FACING));
        sta8(ea_dp(ACTOR_FACING));
        lda8(ea_abs(PLAYER + ACTOR_FLIP));
        ora8_v(0x80);
        sta8(ea_dp(ACTOR_FACING));
        abi_ActorStateSet8CSelf();
    } else {
        abi_ActorStateSet8C();
    }
    clc();
out:
    op_sep(0x30);
}

/*
 * @routine 83:80F8 ActorDownedByShotLong
 * WHAT IT DOES: ActorDownedByShot for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorDownedByShot; called by JSL. Exit: as ActorDownedByShot.
 */
void abi_ActorDownedByShotLong(void)
{
    abi_ActorDownedByShot();
}
