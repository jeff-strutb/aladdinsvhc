/*
 * actor_waits_small.c: small actor routines: two routines actors run each
 * frame they wait (hit tests, then their end out of range), kind $82's
 * start (shown, and stays), a turn of the vertical speed, a place on
 * the escort actor, kind $69's wait and its end riding the escort, kind $3C's
 * start (it drifts up and down), two more wait routines, and a flight
 * frame that stops rising. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorDownedByShot(void);
void abi_ActorEndOutOfRange(void);
void abi_ActorHitByShotCC(void);
void abi_ActorPictureSend(void);
void abi_ActorStompOrHurt(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorOnEscort(void);
void abi_ActorEndPlaced(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedYSet(void);
void abi_ActorSpeedYTurn(void);
void abi_ActorStopsShot(void);
void abi_PlayerLandsOnActor(void);
void abi_SoundQueueAdd(void);
void abi_ActorHitsTestBoxIfPlayerFreeLong(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_ActorFlyAgainst(void);

#define WAIT_HITS 0x42u  /* counted up each frame a shot does not down it */

/* Kind $69's end: what it sets in the escort's record, and its count */
#define ESCORT_CODE (ACTOR_ESCORT + ACTOR_03)  /* where the escort's code goes on (word) */
#define ESCORT_ON_CODE 0xE554u                 /* the escort's code once kind $69 is done */
#define KIND69_FRAMES 0x40u                    /* frames left of each of its two poses */
#define KIND3C_16 0x16u                        /* word: $D282 */
#define KIND3C_LEG 0x40u                       /* frames left before its vertical speed turns */
/* Not yet filed (no script reaches it): kind $3C's frames after its
   first, each ActorMoveY, its vertical speed turned every $55. */
#define KIND3C_DRIFT 0x83B0FFu
#define KIND69_RIDING 0x42u                    /* nonzero: kind $69 rides on the escort */
#define V_00BD 0x0000BDu                       /* counted up as each such end finishes; a camera trigger task clears it */

/*
 * @routine 83:AA47 ActorWaitHitsRange8
 * WHAT IT DOES: What an actor (entry $2D of the actor loop's wait table at
 * $83:8F4F) runs each frame it waits: its animation and picture, the
 * stomp-or-hurt and shot tests, then its end when it is out of range by
 * box 8.
 * HOW: AnimStep, ActorPictureSend, ActorStompOrHurt, ActorHitByShotCC; X =
 * 8, then into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: as
 * ActorEndOutOfRange: in range it returns; out of range the actor ends.
 */
void abi_ActorWaitHitsRange8(void)
{
    abi_AnimStep();
    abi_ActorPictureSend();
    abi_ActorStompOrHurt();
    abi_ActorHitByShotCC();
    x8_load(0x08);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 83:B630 ActorWaitHitsRange0
 * WHAT IT DOES: What an actor (entry $2F of the actor loop's wait table at
 * $83:8F4F) runs each frame it waits: the stomp-or-hurt and shot tests
 * ($42 counted up when no shot downs it), its animation, then its end when
 * it is out of range by box 0.
 * HOW: ActorStompOrHurt; ActorDownedByShot, C clear: $42 up; AnimStep; X =
 * 0, then into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0), or from 83:1FE0. Exit: as
 * ActorEndOutOfRange.
 */
void abi_ActorWaitHitsRange0(void)
{
    abi_ActorStompOrHurt();
    abi_ActorDownedByShot();
    if (!(cpu.p & P_C)) {
        uint8_t hits = (uint8_t)(rd8(dpa(WAIT_HITS)) + 1);
        wr8(dpa(WAIT_HITS), hits);
        nz8_of(hits);
    }
    abi_AnimStep();
    x8_load(0x00);
    abi_ActorEndOutOfRange();
}


/*
 * @routine 84:951D Kind82Show
 * @actor
 * WHAT IT DOES: Actor kind $82: shown with animation 0 (bits $B0 of $09,
 * bit 1 of $08 set) and stays.
 * HOW: $08 bit 1 set, $09 bits $B0 set, AnimStart 0, BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $82 in state $0C (through the stub at
 * $83:92CF). Exit: never returns.
 */
void abi_Kind82Show(void)
{
    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0xB0));
    a8_load(0x00);
    abi_AnimStart();
    for (;;)
        actor_yield(0x84952Fu);
}


/*
 * @routine 83:B109 ActorSpeedYTurn
 * WHAT IT DOES: Turns the actor's vertical speed round: the 24-bit speed
 * negated (its high byte inverted, the low word negated).
 * HOW: The speed's top byte EOR $FF; the low word EOR $FFFF plus 1
 * (16-bit); M 8-bit.
 * Entry: M 8-bit; D = the actor's record; called by JSR (83:B0FA). Exit: M
 * 8-bit; A = the new low word; N and Z from it.
 */
void abi_ActorSpeedYTurn(void)
{
    uint16_t low;

    wr8(dpa(ACTOR_SPEED_Y + 2), (uint8_t)(rd8(dpa(ACTOR_SPEED_Y + 2)) ^ 0xFF));
    low = (uint16_t)((dp16(ACTOR_SPEED_Y) ^ 0xFFFF) + 1);
    dp16_set(ACTOR_SPEED_Y, low);
    a16_load(low);
}


/*
 * @routine 83:EB4B ActorOnEscort
 * WHAT IT DOES: Puts the actor on the escort actor: its x the escort's,
 * its y 4 above the escort's.
 * HOW: 16-bit copies of the escort's x and of its y less 4; M 8-bit.
 * Entry: M 8-bit; D = the actor's record; called by JSR (83:EB00, 83:EB17,
 * 83:EB2C). Exit: M 8-bit; A = the new y pixels.
 */
void abi_ActorOnEscort(void)
{
    dp16_set(ACTOR_X + 1, rd16(dba(ACTOR_ESCORT + ACTOR_X + 1)));
    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(dba(ACTOR_ESCORT + ACTOR_Y + 1)), 0xFFFC, &cpu.p);
    dp16_set(ACTOR_Y + 1, cpu.a);
}

/* Kind $69's pose on the escort: animation `anim`, then 32 frames on it. */
static void ride_escort(uint8_t anim, uint32_t brk_at)
{
    uint8_t frames;

    a8_load(anim);
    abi_AnimStart();
    a8_load(0x20);
    wr8(dpa(KIND69_FRAMES), 0x20);
    do {
        actor_yield(brk_at);
        abi_AnimStep();
        abi_ActorOnEscort();
        frames = (uint8_t)(rd8(dpa(KIND69_FRAMES)) - 1);
        wr8(dpa(KIND69_FRAMES), frames);
        nz8_of(frames);
    } while (frames);
}

/*
 * @routine 83:EADC Kind69EscortEnd
 * @actor
 * WHAT IT DOES: Actor kind $69's code in state $8C: it sets the escort
 * going on at $E554 of its bank, rides on the escort through two
 * animations of 32 frames each, counts itself in $00BD and ends.
 * HOW: The escort's $08 bit 3 set, its $07 = 6, its code word $E554;
 * animation 2, then each frame (BRK first) AnimStep and ActorOnEscort,
 * 32 times; animation 3, the same 32 times; V_00BD up; ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $69 in state $8C (the table at
 * $83:8DFD). Exit: never returns.
 */
void abi_Kind69EscortEnd(void)
{
    uint8_t bits = rd8(dba(ACTOR_ESCORT + ACTOR_08));
    uint8_t done;

    wr8(dba(ACTOR_ESCORT + ACTOR_08), (uint8_t)(bits | 0x08));
    wr8(dba(ACTOR_ESCORT + ACTOR_07), 0x06);
    wr16(dba(ESCORT_CODE), ESCORT_ON_CODE);
    cpu.a = ESCORT_ON_CODE;
    ride_escort(0x02, 0x83EAFAu);
    ride_escort(0x03, 0x83EB11u);
    done = (uint8_t)(rd8(dba(V_00BD)) + 1);
    wr8(dba(V_00BD), done);
    nz8_of(done);
    abi_ActorEndPlaced();
}

/*
 * @routine 83:B0CB Kind3CDrift
 * @actor
 * WHAT IT DOES: Actor kind $3C: shown facing right, given the speeds of
 * entries $15 (horizontal) and $42 (vertical), it drifts up and down: its
 * vertical speed turns every $55 frames.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; KIND3C_16 = $D282;
 * facing and flip 0; animation 0; ActorSpeedXSet $15, ActorSpeedYSet $42;
 * KIND3C_LEG $55 and ActorSpeedYTurn, then a yield. What it does after
 * that (each frame ActorMoveY, the count down, at 0 again from $55 and a
 * turn) is at KIND3C_DRIFT, which no script reaches: its C is not
 * written, and it runs as the original's (jump_routine).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $3C in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind3CDrift(void)
{
    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    dp16_set(KIND3C_16, 0xD282);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    cpu.a = 0xD200;
    nz8_of(0x00);
    abi_AnimStart();
    y8_load(0x15);
    abi_ActorSpeedXSet();
    y8_load(0x42);
    abi_ActorSpeedYSet();
    a8_load(0x55);
    wr8(dpa(KIND3C_LEG), 0x55);
    abi_ActorSpeedYTurn();
    actor_yield(0x83B0FDu);
    jump_routine(KIND3C_DRIFT);
}

/*
 * @routine 83:EB24 Kind69Wait
 * WHAT IT DOES: What actor kind $69 runs each frame it waits: its
 * animation; while it rides the escort it stays on it, can stop the
 * player's shots, and when Aladdin lands on it, it goes into state $8C
 * (its end, Kind69EscortEnd) with sound $12.
 * HOW: AnimStep; KIND69_RIDING zero: nothing more. Else ActorOnEscort,
 * the escort's $08 bit 3 cleared, ActorStopsShot, PlayerLandsOnActor;
 * landed (Z clear): bit 7 of its $09 cleared, its state $8C,
 * SoundQueueAdd $12.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: M 8-bit; not
 * riding: as AnimStep; else as PlayerLandsOnActor or SoundQueueAdd.
 */
void abi_Kind69Wait(void)
{
    uint8_t riding, bits;

    abi_AnimStep();
    riding = rd8(dpa(KIND69_RIDING));
    a8_load(riding);
    if (!riding)
        return;
    abi_ActorOnEscort();
    /* TRB: Z from A AND the byte, then the bit cleared */
    a8_load(0x08);
    bits = rd8(dba(ACTOR_ESCORT + ACTOR_08));
    flag_set(P_Z, !(bits & 0x08));
    wr8(dba(ACTOR_ESCORT + ACTOR_08), (uint8_t)(bits & ~0x08));
    abi_ActorStopsShot();
    abi_PlayerLandsOnActor();
    if (cpu.p & P_Z)
        return;
    /* Aladdin landed on it: its end, with sound $12 */
    a8_load(0x80);
    bits = rd8(dpa(ACTOR_09));
    flag_set(P_Z, !(bits & 0x80));
    wr8(dpa(ACTOR_09), (uint8_t)(bits & ~0x80));
    a8_load(0x8C);
    wr8(dpa(ACTOR_STATE), 0x8C);
    a8_load(0x12);
    abi_SoundQueueAdd();
}

/*
 * @routine 83:ABD8 ActorWaitEndLowKinds
 * WHAT IT DOES: What an actor runs each frame it waits: its animation; an
 * actor whose $07 has a low nibble below 2 ends when it is out of range by
 * box 0.
 * HOW: AnimStep; ACTOR_07 AND $0F against 2: 2 or more, RTS; below, X = 0
 * and on into ActorEndOutOfRange (JMP).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: as AnimStep with A
 * low = the nibble and C set; or as ActorEndOutOfRange.
 */
void abi_ActorWaitEndLowKinds(void)
{
    uint8_t low;

    abi_AnimStep();
    low = rd8(dpa(ACTOR_07)) & 0x0F;
    a8_load(low);
    cmp8_of(low, 0x02);
    if (low >= 0x02)
        return;
    x8_load(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 84:95D4 ActorWaitGrabsPlayer
 * WHAT IT DOES: The wait work the actor loop's stub at $83:92DC calls each
 * frame: the actor's animation, and when the player, free to be caught,
 * meets its box, he goes into state $8C facing it.
 * HOW: AnimStep; ActorHitsTestBoxIfPlayerFreeLong: met (C clear),
 * PlayerEnter8CFacingLong.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL. Exit: as
 * the test (C set) or as PlayerEnter8CFacingLong.
 */
void abi_ActorWaitGrabsPlayer(void)
{
    abi_AnimStep();
    abi_ActorHitsTestBoxIfPlayerFreeLong();
    if (!IF_C)
        abi_PlayerEnter8CFacingLong();
}

/*
 * @routine 84:A7D4 ActorFlyAgainstNoRise
 * WHAT IT DOES: A frame of flight with the vertical pull reversed, which
 * stops the actor's vertical motion once its vertical speed has gone
 * negative (upward).
 * HOW: ActorFlyAgainst; with the speed's top byte negative, the
 * acceleration ($20) and the 24-bit vertical speed cleared.
 * Entry: M 8-bit; D = the actor; called by JSR (84:A630, A6B0, A6D9,
 * A74E). Exit: M 8-bit; as ActorFlyAgainst, or A low = the speed's top
 * byte, N set.
 */
void abi_ActorFlyAgainstNoRise(void)
{
    uint8_t top;

    abi_ActorFlyAgainst();
    top = rd8(dpa(ACTOR_SPEED_Y + 2));
    a8_load(top);
    if (!(top & 0x80))
        return;
    wr8(dpa(ACTOR_ACCEL_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y + 1), 0);
    wr8(dpa(ACTOR_SPEED_Y + 2), 0);
}
