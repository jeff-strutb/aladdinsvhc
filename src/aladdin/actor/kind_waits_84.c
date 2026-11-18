/*
 * kind_waits_84.c: the bank $84 work of nine actor kinds' waits (what each
 * runs every frame it waits, by JSL from its stub in the actor loop's
 * wait table): nothing, its animation, or its animation, a touch that
 * hurts the player, and its end when out of range.
 */
#include "machine.h"
#include "native.h"
#include "actor.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorHurtsPlayerOnTouchLong(void);
void abi_ActorBouncesPlayer(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_ActorBouncesPlayerLong(void);
void abi_ActorEndPlaced(void);
void abi_ActorEndsShotLong(void);
void abi_ActorHitsTestBoxIfActiveLong(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_ActorSideOfPlayer(void);
void abi_PlayerPushAsideLong(void);
void abi_ActorFreeOutOfRangeLong(void);

#define K66_HELD 0x4Bu               /* nonzero: no touch or shot tests; purpose not yet established */
#define K66_TOUCHED 0x1435u          /* set while the player touches a kind $66 (cleared by the player's frame work) */
#define K66_TEST_BOX_Y 0x1486u       /* the test box's centre y (word) */
#define K66_TEST_BOX_HALF_W 0x1488u  /* its half-width (word) */
#define K66_TEST_BOX_HALF_H 0x148Au  /* its half-height (word) */

#define K7E_BOUNCE 0x42u        /* frames left of a bounce (the player held on it) */
#define K7E_STAYS 0x47u         /* nonzero: never freed out of range */
#define K9A_BOUNCES 0x41u       /* nonzero: Aladdin bounces off it (else touching it hurts him) */
#define K9E_FRAME 0x27u         /* its animation frame: 7, 8, 9, $0C and $0D are drawn in front */

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:951C Kind81WaitWork
 * WHAT IT DOES: What actors of kind $81 do each frame they wait: nothing.
 * HOW: RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
 * actor loop's wait table. Exit: as entered.
 */
void abi_Kind81WaitWork(void)
{
}

/*
 * @routine 84:85DC Kind66WaitWork
 * WHAT IT DOES: What actors of kind $66 do each frame they wait: unless
 * their $4B is set, a shot may end them, and the player touching one is
 * pressed down when his box is below its middle, else pushed aside (with
 * his box a pixel narrower when he is to its side); not touching (or $4B
 * set), they end out of range.
 * HOW: $4B nonzero: K66_TOUCHED cleared, ActorFreeOutOfRangeLong with X =
 * 0. Else ActorEndsShotLong; ActorHitsTestBoxIfActiveLong; no touch: as
 * above. Touch: 16-bit, its y pixels + $20 in SCRATCH; TEST_BOX_Y less
 * TEST_BOX_HALF_H, not below it: JML PlayerEnter8CFacingLong. Else
 * K66_TOUCHED = 1; ActorSideOfPlayer: zero, JML PlayerPushAsideLong; else
 * TEST_BOX_HALF_W down one around PlayerPushAsideLong, RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM, the hit boxes' bank; called by JSL from
 * ActorKind66Wait. Exit: as the last call leaves them.
 */
void abi_Kind66WaitWork(void)
{
    lda8(ea_dp(K66_HELD));
    if (IF_Z) {
        jsl_frame(0x8485E0u);
        abi_ActorEndsShotLong();
        cpu.s = (uint16_t)(cpu.s + 3);
        jsl_frame(0x8485E4u);
        abi_ActorHitsTestBoxIfActiveLong();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!IF_C) {
            /* touching: below its middle, pressed down; else pushed aside */
            op_rep(0x31);
            lda16(ea_dp(ACTOR_Y + 1));
            adc16_v(0x0020);
            sta16(ea_abs(SCRATCH));
            lda16(ea_abs(K66_TEST_BOX_Y));
            sec();
            sbc16_v(rd16(ea_abs(K66_TEST_BOX_HALF_H)));
            cmp16_v(rd16(ea_abs(SCRATCH)));
            op_sep(0x30);
            if (IF_C) {
                abi_PlayerEnter8CFacingLong();   /* JML */
                return;
            }
            lda8_v(0x01);
            sta8(ea_abs(K66_TOUCHED));
            jsl_frame(0x84860Bu);
            abi_ActorSideOfPlayer();
            cpu.s = (uint16_t)(cpu.s + 3);
            if (IF_Z) {
                abi_PlayerPushAsideLong();       /* JML */
                return;
            }
            MEM8(dec, ea_abs(K66_TEST_BOX_HALF_W));
            jsl_frame(0x848614u);
            abi_PlayerPushAsideLong();
            cpu.s = (uint16_t)(cpu.s + 3);
            MEM8(inc, ea_abs(K66_TEST_BOX_HALF_W));
            return;
        }
    }
    stz8(ea_abs(K66_TOUCHED));
    ldx8_v(0x00);
    abi_ActorFreeOutOfRangeLong();               /* JML */
}

/*
 * @routine 84:95E3 Kind84Start
 * @actor
 * WHAT IT DOES: Kind $84's start (GrabberStart spawns it): shows its first
 * animation for 16 frames, then ends.
 * HOW: Bit 1 of $08 and bits $90 of $09 set; AnimStart 0; a wait of 16
 * frames (COP, Kind84WaitWork each frame); JML ActorEndPlaced.
 * Entry: D = the actor's record; M 8-bit, X 8-bit; reached by JML from
 * kind $84's start entry ($83:92E5). Exit: never returns: as
 * ActorEndPlaced. */
void abi_Kind84Start(void)
{
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0x00);
    jsl_frame(0x8495F1u);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8_v(0x10);
    actor_wait(0x8495F7u);
    abi_ActorEndPlaced();
}

/*
 * @routine 84:95FD Kind84WaitWork
 * WHAT IT DOES: What actors of kind $84 do each frame they wait: their
 * animation steps.
 * HOW: AnimStep, RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
 * actor loop's wait table. Exit: as AnimStep leaves them.
 */
void abi_Kind84WaitWork(void)
{
    abi_AnimStep();
}

/*
 * @routine 84:99E7 Kind8CWaitWork
 * WHAT IT DOES: What actors of kind $8C do each frame they wait: their
 * animation steps.
 * HOW: AnimStep, RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
 * actor loop's wait table. Exit: as AnimStep leaves them.
 */
void abi_Kind8CWaitWork(void)
{
    abi_AnimStep();
}

/*
 * @routine 84:AB97 KindA4WaitWork
 * WHAT IT DOES: What actors of kind $A4 do each frame they wait: their
 * animation steps.
 * HOW: AnimStep, RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
 * actor loop's wait table. Exit: as AnimStep leaves them.
 */
void abi_KindA4WaitWork(void)
{
    jsl_call(0x84AB97u, abi_AnimStep);
}

/*
 * @routine 84:A438 Kind99WaitWork
 * WHAT IT DOES: What actors of kind $99 do each frame they wait: their
 * animation steps, Aladdin touching them is hurt, and they are freed
 * once out of range by box $18.
 * HOW: AnimStep, ActorHurtsPlayerOnTouchLong; X = $18; on into
 * ActorFreeOutOfRangeLong (JML), whose RTL is this routine's.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
 * actor loop's wait table. Exit: as ActorFreeOutOfRangeLong leaves them.
 */
void abi_Kind99WaitWork(void)
{
    jsl_call(0x84A438u, abi_AnimStep);
    jsl_call(0x84A43Cu, abi_ActorHurtsPlayerOnTouchLong);
    x8_load(0x18);
    abi_ActorFreeOutOfRangeLong();
}

/*
 * @routine 84:AC31 KindA5WaitWork
 * WHAT IT DOES: What actors of kind $A5 do each frame they wait: their
 * animation steps, Aladdin touching them is hurt, and they are freed
 * once out of range by box $18.
 * HOW: AnimStep, ActorHurtsPlayerOnTouchLong; X = $18; on into
 * ActorFreeOutOfRangeLong (JML), whose RTL is this routine's.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
 * actor loop's wait table. Exit: as ActorFreeOutOfRangeLong leaves them.
 */
void abi_KindA5WaitWork(void)
{
    jsl_call(0x84AC31u, abi_AnimStep);
    jsl_call(0x84AC35u, abi_ActorHurtsPlayerOnTouchLong);
    x8_load(0x18);
    abi_ActorFreeOutOfRangeLong();
}

/*
 * @routine 83:840B ActorBouncesPlayerLong
 * WHAT IT DOES: ActorBouncesPlayer for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorBouncesPlayer; called by JSL. Exit: as ActorBouncesPlayer.
 */
void abi_ActorBouncesPlayerLong(void)
{
    abi_ActorBouncesPlayer();
}

/*
 * @routine 84:A4E3 Kind9AWaitWork
 * WHAT IT DOES: What actors of kind $9A do each frame they wait: their
 * animation steps; Aladdin touching one is hurt, or, with K9A_BOUNCES
 * set, coming down on it bounces him (sound $12, animation $18); freed
 * once out of range by box $18.
 * HOW: AnimStep. K9A_BOUNCES zero: ActorHurtsPlayerOnTouchLong. Else
 * ActorBouncesPlayerLong, bounced (Z clear): SoundQueueAdd $12, AnimStart
 * $18. X = $18; on into ActorFreeOutOfRangeLong (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub through the
 * actor loop's wait table. Exit: as ActorFreeOutOfRangeLong leaves them.
 */
void abi_Kind9AWaitWork(void)
{
    jsl_call(0x84A4E3u, abi_AnimStep);
    a8_load(rd8(dpa(K9A_BOUNCES)));
    if (!(uint8_t)cpu.a) {
        jsl_call(0x84A4EBu, abi_ActorHurtsPlayerOnTouchLong);
    } else {
        jsl_call(0x84A4F1u, abi_ActorBouncesPlayerLong);
        if (!(cpu.p & P_Z)) {
            a8_load(0x12);
            jsl_call(0x84A4F9u, abi_SoundQueueAdd);
            a8_load(0x18);
            jsl_call(0x84A4FFu, abi_AnimStart);
        }
    }
    x8_load(0x18);
    abi_ActorFreeOutOfRangeLong();
}

/*
 * @routine 84:A8BD Kind9EWaitWork
 * WHAT IT DOES: What actors of kind $9E do each frame they wait: their
 * animation steps, and they are drawn in front (ACTOR_09 bits 4-5) on the
 * frames 7, 8, 9, $0C and $0D.
 * HOW: AnimStep; ACTOR_09 bit 5 cleared; K9E_FRAME one of those: ACTOR_09
 * bits $30 set. RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL from the
 * kind's stub through the actor loop's wait table. Exit: M 8-bit; A =
 * K9E_FRAME or $30; N, Z and C from the last test.
 */
void abi_Kind9EWaitWork(void)
{
    uint8_t f;

    jsl_call(0x84A8BDu, abi_AnimStep);
    a8_load(0x20);
    trb_at(dpa(ACTOR_09), 0x20);
    f = rd8(dpa(K9E_FRAME));
    a8_load(f);
    if (f == 0x07 || f == 0x08 || f == 0x09 || f == 0x0C) {
        cmp8_of(f, f);
    } else {
        cmp8_of(f, 0x0D);
        if (f != 0x0D)
            return;
    }
    a8_load(0x30);
    tsb_at(dpa(ACTOR_09), 0x30);
}

/*
 * @routine 84:931C Kind7EWaitWork
 * WHAT IT DOES: What actors of kind $7E (a platform Aladdin bounces on) do
 * each frame they wait: for 8 frames after a bounce the player is held
 * 38 pixels above it; else coming down on it bounces him (sound $12);
 * unless K7E_STAYS, freed once out of range by box 8.
 * HOW: K7E_BOUNCE nonzero: down by 1; 16-bit (REP #$21: carry clear) the
 * player's y = its y - $25 - 1. Else ActorBouncesPlayerLong, bounced (Z
 * clear): SoundQueueAdd $12, K7E_BOUNCE 8. K7E_STAYS set: RTL; else X = 8
 * and on into ActorFreeOutOfRangeLong (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub ($83:92AC)
 * through the actor loop's wait table. Exit: M 8-bit; as
 * ActorFreeOutOfRangeLong leaves them, or A = K7E_STAYS.
 */
void abi_Kind7EWaitWork(void)
{
    a8_load(rd8(dpa(K7E_BOUNCE)));
    if ((uint8_t)cpu.a) {
        (void)dec8_at(dpa(K7E_BOUNCE));
        op_rep(0x21);
        a16_load(op_sbc16(dp16(ACTOR_Y + 1), 0x0025, &cpu.p));
        wr16(dba(PLAYER + ACTOR_Y + 1), cpu.a);
        op_sep(0x20);
    } else {
        abi_ActorBouncesPlayerLong();
        if (!(cpu.p & P_Z)) {
            a8_load(0x12);
            abi_SoundQueueAdd();
            a8_load(0x08);
            wr8(dpa(K7E_BOUNCE), 0x08);
        }
    }
    a8_load(rd8(dpa(K7E_STAYS)));
    if ((uint8_t)cpu.a)
        return;
    x8_load(0x08);
    abi_ActorFreeOutOfRangeLong();
}
