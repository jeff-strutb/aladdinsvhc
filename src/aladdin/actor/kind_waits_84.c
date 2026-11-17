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
#include "wram.h"

void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorHurtsPlayerOnTouchLong(void);
void abi_ActorBouncesPlayer(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_ActorBouncesPlayerLong(void);

#define K7E_BOUNCE 0x42u        /* frames left of a bounce (the player held on it) */
#define K7E_STAYS 0x47u         /* nonzero: never freed out of range */
#define K9A_BOUNCES 0x41u       /* nonzero: Aladdin bounces off it (else touching it hurts him) */
#define K9E_FRAME 0x27u         /* its animation frame: 7, 8, 9, $0C and $0D are drawn in front */

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
    abi_AnimStep();
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
    abi_AnimStep();
    abi_ActorHurtsPlayerOnTouchLong();
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
    abi_AnimStep();
    abi_ActorHurtsPlayerOnTouchLong();
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
    abi_AnimStep();
    a8_load(rd8(dpa(K9A_BOUNCES)));
    if (!(uint8_t)cpu.a) {
        abi_ActorHurtsPlayerOnTouchLong();
    } else {
        abi_ActorBouncesPlayerLong();
        if (!(cpu.p & P_Z)) {
            a8_load(0x12);
            abi_SoundQueueAdd();
            a8_load(0x18);
            abi_AnimStart();
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

    abi_AnimStep();
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
