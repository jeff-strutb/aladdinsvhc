/*
 * kind_waits.c: the routines some actor kinds run each frame they wait (by the kind
 * table at $83:8F4F, or a kind's stub): animation, shots, touches and
 * their range. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram_state.h"
#include "wram_video.h"
#include "wram.h"
#include "actor.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFreePlaced(void);
void abi_ActorHitsTestBoxLong(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_ActorDownedByShot(void);
void abi_ActorStompOrHurt(void);
void abi_ActorTakesShot(void);
void abi_ActorStopsShot(void);
void abi_ActorZoneHasTestBox(void);
void abi_PlayerEnter8CFacing(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorHurtsPlayerOnTouchLong(void);
void abi_ActorStopsShotLong(void);

void abi_ActorCreateChild8(void);
void abi_ActorHitsTestBoxIfPlayerFree(void);
void abi_ActorYByFrame(void);
void abi_BackdropActorShow(void);
void abi_BackdropPictureStep(void);
void abi_PlayerOnFirmGround(void);
void abi_PlayerOnFirmGroundProbe4(void);
void abi_PlayerOnFirmGroundProbe8(void);

#define ACTOR_43 0x43u
#define ACTOR_4E 0x4Eu   /* word: the record of its holder (kind $53) */

/*
 * @routine 83:E645 ActorCount42Down
 * WHAT IT DOES: Counts the actor's word at $42 down by 1, not below 0.
 * HOW: A 16-bit subtraction; negative becomes 0.
 * Entry: M 16-bit; D = the actor's record; called by JSR (83:E60C,
 * 83:E633). Exit: M 16-bit; A = the new count; C from the subtraction.
 */
void abi_ActorCount42Down(void)
{
    flag_set(P_C, 1);
    cpu.a = op_sbc16(dp16(ACTOR_42), 0x0001, &cpu.p);
    if (cpu.a & 0x8000)                   /* not below 0 */
        a16_load(0x0000);
    dp16_set(ACTOR_42, cpu.a);
}


/*
 * @routine 83:A012 Kind27Wait
 * WHAT IT DOES: Kind $27's wait: animates; once its $42 is set, touching
 * the player it plays sound $4F, counts $036C up and ends; it ends at once
 * when $0367 is 0, or when out of range.
 * HOW: AnimStep; $42 clear returns. $0367 zero: ActorEndOutOfRange with
 * box $58. Else ActorHitsTestBoxLong with box $24: touched, SoundQueueAdd
 * $4F, $036C up, and into ActorEndOutOfRange past its range test (its
 * return dropped, freed); not touched, ActorEndOutOfRange with box $58.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Its $42 is set only by its untraced start for $07 bit 3 (83:9E58). Exit:
 * $42 clear: as AnimStep. Else in range: as ActorEndOutOfRange; ended:
 * does not return.
 */
void abi_Kind27Wait(void)
{
    abi_AnimStep();
    a8_load(rd8(dpa(ACTOR_42)));
    if (!(uint8_t)cpu.a)
        return;
    a8_load(rd8(dba(HEALTH)));
    if ((uint8_t)cpu.a) {
        y8_load(0x24);
        abi_ActorHitsTestBoxLong();
        if (!(cpu.p & P_C)) {             /* touched: sound $4F, counted, it ends */
            a8_load(0x4F);
            abi_SoundQueueAdd();
            inc8_at(dba(0x036C));
            op_sep(0x30);                 /* $83:96C6 */
            pla8();
            pla8();
            abi_ActorFreePlaced();
            actor_leave(0x8394BFu);
            return;
        }
    }
    x8_load(0x58);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 83:AEEC Kind31Wait
 * WHAT IT DOES: Kind $31's wait: with its $42 set it turns round at x
 * $0F00 and beyond in levels below 7, and may be downed by a shot; it
 * animates and is ended when out of range.
 * HOW: $42 set: level below 7 and x pixels $0F00 or more, facing and flip
 * toggled; ActorDownedByShot, downed: ActorStompOrHurt. Then AnimStep and
 * ActorEndOutOfRange with box $38 (by JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Called by JSR: it ends by JML to ActorEndOutOfRange, which returns to
 * the loop. Exit: as ActorEndOutOfRange.
 */
void abi_Kind31Wait(void)
{
    uint8_t level;
    uint16_t x;

    a8_load(rd8(dpa(ACTOR_42)));
    if ((uint8_t)cpu.a) {
        /* before level 7, past x $0F00 it turns */
        level = rd8(dba(LEVEL));
        a8_load(level);
        cmp8_of(level, 0x07);
        if (level < 0x07) {
            x = dp16(ACTOR_X + 1);
            cpu.a = x;
            cmp16_of(x, 0x0F00);
            if (x >= 0x0F00) {
                a8_load((uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01));
                wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
                wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
            }
        }
        abi_ActorDownedByShot();
        if (cpu.p & P_C)
            abi_ActorStompOrHurt();
    }
    abi_AnimStep();
    x8_load(0x38);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 83:B94A Kind30Wait
 * WHAT IT DOES: Kind $30's wait: animates; variant 7 (low three bits of
 * $07) notes the first shot that reaches it ($43); ended when out of
 * range.
 * HOW: AnimStep; variant 7 with $43 clear: ActorTakesShot, hit: $43 up.
 * Then ActorEndOutOfRange with box $18.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorEndOutOfRange.
 */
void abi_Kind30Wait(void)
{
    uint8_t variant;

    abi_AnimStep();
    variant = rd8(dpa(ACTOR_07)) & 0x07;
    a8_load(variant);
    cmp8_of(variant, 0x07);
    if (variant == 0x07) {                /* variant 7: one shot counts */
        a8_load(rd8(dpa(ACTOR_43)));
        if (!(uint8_t)cpu.a) {
            abi_ActorTakesShot();
            if (!(cpu.p & P_C))
                inc8_at(dpa(ACTOR_43));
        }
    }
    x8_load(0x18);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 83:C94A Kind46Wait
 * WHAT IT DOES: Kind $46's wait: a shot it stops sets its $43; with $07
 * bit 1 it is ended when out of range.
 * HOW: ActorStopsShot: stopped (C clear), $43 = 1. $07 bit 1:
 * ActorEndOutOfRange with box 0; else returns.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: M 8-bit; A low = $07 AND 2; or as ActorEndOutOfRange.
 */
void abi_Kind46Wait(void)
{
    abi_ActorStopsShot();
    if (!(cpu.p & P_C)) {
        a8_load(0x01);
        wr8(dpa(ACTOR_43), 0x01);
    }
    a8_load(rd8(dpa(ACTOR_07)) & 0x02);
    if (!(uint8_t)cpu.a)
        return;
    x8_load(0x00);
    abi_ActorEndOutOfRange();
}




/*
 * @routine 83:D64A Kind53Wait
 * WHAT IT DOES: Kind $53's wait: stomped or hurting as touched; sets its
 * $45 once the record its $4E names has $46 set; animates.
 * HOW: ActorStompOrHurt; $45 clear: the byte at $46 of the record at $4E
 * (16-bit X) nonzero, $45 = 1. AnimStep.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as AnimStep.
 */
void abi_Kind53Wait(void)
{
    abi_ActorStompOrHurt();
    a8_load(rd8(dpa(ACTOR_45)));
    if (!(uint8_t)cpu.a) {                /* its holder's let-go copied in */
        op_rep(P_X);
        cpu.x = dp16(ACTOR_4E);
        a8_load(rd8(dba(ACTOR_46) + cpu.x));
        op_sep(P_X);
        if ((uint8_t)cpu.a) {
            a8_load(0x01);
            wr8(dpa(ACTOR_45), 0x01);
        }
    }
    abi_AnimStep();
}


/*
 * @routine 83:E679 Kind67Wait
 * WHAT IT DOES: Kind $67's wait: puts the player into state $8C while the
 * player is across x $04F8-$0E47 and the test box is within its zone (from
 * $1A2C) up and down.
 * HOW: The player's x pixels less $09A0 plus $04A8 below $0950; then the
 * test box's y against $C0 less $1A2C, by the test box's half-height plus
 * $4C (in $000C, doubled in $000E): inside, PlayerEnter8CFacingLong.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: M 8-bit, X 8-bit; as PlayerEnter8CFacingLong, or flags from the
 * comparison.
 */
void abi_Kind67Wait(void)
{
    uint16_t d, half;

    /* the player within $04A8 of x $09A0, and his test box (by its half
       height) at the backdrop's height: he is caught */
    op_rep(P_M);
    flag_set(P_C, 1);
    d = op_sbc16(rd16(dba(PLAYER + ACTOR_X + 1)), 0x09A0, &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, 0x04A8, &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x0950);
    if (d < 0x0950) {
        flag_set(P_C, 0);
        half = op_adc16(rd16(dba(0x148A)), 0x004C, &cpu.p);
        wr16(dba(SCRATCH), half);
        flag_set(P_C, half & 0x8000);
        wr16(dba(SCRATCH + 2), (uint16_t)(half << 1));
        d = (uint16_t)((rd16(dba(0x1A2C)) ^ 0xFFFF) + 1);
        flag_set(P_C, 0);
        d = op_adc16(d, 0x00C0, &cpu.p);
        flag_set(P_C, 1);
        d = op_sbc16(d, rd16(dba(0x1486)), &cpu.p);
        flag_set(P_C, 0);
        d = op_adc16(d, rd16(dba(SCRATCH)), &cpu.p);
        cpu.a = d;
        cmp16_of(d, rd16(dba(SCRATCH + 2)));
        if (d < rd16(dba(SCRATCH + 2))) {
            op_sep(0x30);
            abi_PlayerEnter8CFacingLong();
            return;
        }
    }
    op_sep(0x30);
}







/*
 * @routine 83:F1D5 Kind6EWait
 * WHAT IT DOES: Kind $6E's wait: a sound ($33) every 63 frames while its
 * $49 is clear; its backdrop picture stepped and shown; touching the test
 * box (moved by its variant's offset for the test) while the player can be
 * hit, the player enters state $8C.
 * HOW: $49 clear: $48 down; at 0, back to $3F and SoundQueueAdd $33.
 * BackdropPictureStep, BackdropActorShow. Its x pixels kept at $4C and the
 * variant's offset (word at $D6AB) added; ActorHitsTestBoxIfPlayerFree;
 * met: PlayerEnter8CFacingLong. Its x put back.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Its $09 has bit 6 clear in play, so the touch test never meets: the hurt
 * is reached only by unit_kind6e_touch.box. Exit: M 8-bit; A = its x
 * pixels.
 */
void abi_Kind6EWait(void)
{
    uint16_t x;

    a8_load(rd8(dpa(ACTOR_49)));
    if (!(uint8_t)cpu.a && !dec8_at(dpa(ACTOR_48))) {   /* its sound every $3F frames */
        a8_load(0x3F);
        wr8(dpa(ACTOR_48), 0x3F);
        a8_load(0x33);
        abi_SoundQueueAdd();
    }
    abi_BackdropPictureStep();
    abi_BackdropActorShow();
    /* its touch tested at its x moved by its variant's offset */
    cpu.x = (uint8_t)(rd8(dpa(ACTOR_07)) << 1);
    op_rep(P_M);
    x = dp16(ACTOR_X + 1);
    dp16_set(ACTOR_4C, x);
    flag_set(P_C, 0);
    cpu.a = op_adc16(x, rd16(dba(0xD6AB) + cpu.x), &cpu.p);
    dp16_set(ACTOR_X + 1, cpu.a);
    op_sep(P_M);
    abi_ActorHitsTestBoxIfPlayerFree();
    if (!(cpu.p & P_C))
        abi_PlayerEnter8CFacingLong();
    op_rep(P_M);
    a16_load(dp16(ACTOR_4C));
    dp16_set(ACTOR_X + 1, cpu.a);
    op_sep(P_M);
}

