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
void abi_ActorBouncesPlayer(void);
void abi_ActorBouncesOrHurtsPlayer(void);
void abi_ActorInRange(void);
void abi_ActorFreePlacedAtY(void);
void abi_ActorMoveX(void);
void abi_ActorEndIfLeftBehind(void);
void abi_ActorFreeOutOfRange(void);
void abi_LevelFlagFromMask(void);
void abi_ActorTestCarries(void);
void abi_ActorLiftPlayerStep(void);
void abi_ActorMoveY(void);
void abi_PlayerToActorBelow(void);

#define WAIT_HITS 0x42u  /* counted up each frame a shot does not down it */
#define KIND28_STOMPS 0x45u   /* kind $28: nonzero, it is stomped or hurts; zero, it bounces the player */
#define KIND28_RIGHT 0x46u    /* kind $28: counted up each frame it faces right */
#define FRAME_COUNT_32D 0x00032Du /* counted each frame (its bit 0: odd or even) */
#define KIND37_FREE 0x45u     /* kinds $37/$38: nonzero, it ends as other actors do */
#define KIND37_HIT 0x46u      /* 1 bounced, 2 downed by a shot */
#define KIND37_MAKER 0x4Cu    /* the record that made it (word) */
#define KIND37_PARTNER 0x4Eu  /* the record it is freed with (word) */
#define KIND37_MAKER_COUNT 0x004Bu /* in its maker's record: its count of these */
#define ACTOR_27 0x27u        /* the picture its animation shows, as AnimStep sets it */
#define KIND2A_BOB 0x49u      /* kind $2A: nonzero, it bobs with its animation */
#define KIND2A_FRAME 0x4Au    /* kind $2A: the frame it last bobbed on */
#define KIND2A_UP 0x4Bu       /* kind $2A: nonzero after frame 6, frame 5 then moves it down */
#define KIND2A_RIGHT 0x4Cu    /* kind $2A: counted when downed facing right */
#define KIND2A_STILL 0x47u    /* kind $2A: nonzero, no animation, touch or shot test */
#define KIND2A_HITS 0x48u     /* kind $2A: counted when downed by a shot */
#define KIND3B_REST_Y 0x4Au   /* kind $3B: the y pixels it rises back to (word) */
#define KIND3B_RISE 0x40u     /* kind $3B: how far below them it is (low byte) */
#define KIND3B_LIFT_STEP 0x44u /* kind $3B: ActorLiftPlayerStep's step */

/* Kind $69's end: what it sets in the escort's record, and its count */
#define ESCORT_CODE (ACTOR_ESCORT + ACTOR_03)  /* where the escort's code goes on (word) */
#define ESCORT_ON_CODE 0xE554u                 /* the escort's code once kind $69 is done */
#define KIND69_FRAMES 0x40u                    /* frames left of each of its two poses */
#define KIND3C_16 0x16u                        /* word: $D282 */
#define KIND3C_LEG 0x40u                       /* frames left before its vertical speed turns */
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
    jsl_frame(0x83AA47u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x83AA4Bu);
    abi_ActorPictureSend();
    cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0xAA51, abi_ActorStompOrHurt);
    call_frame(0xAA54, abi_ActorHitByShotCC);
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
    call_frame(0xB632, abi_ActorStompOrHurt);
    call_frame(0xB635, abi_ActorDownedByShot);
    if (!(cpu.p & P_C)) {
        uint8_t hits = (uint8_t)(rd8(dpa(WAIT_HITS)) + 1);
        wr8(dpa(WAIT_HITS), hits);
        nz8_of(hits);
    }
    jsl_frame(0x83B63Au);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x84952Bu);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(brk_at - 8); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* its JSL 8 bytes before the BRK */
    a8_load(0x20);
    wr8(dpa(KIND69_FRAMES), 0x20);
    do {
        actor_yield(brk_at);
        jsl_frame(brk_at + 2); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
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
 * then over and over: KIND3C_LEG $55 and ActorSpeedYTurn; each frame a
 * yield, ActorMoveY and the count down, until it is 0.
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
    jsl_frame(0x83B0E6u);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    y8_load(0x15);
    jsl_frame(0x83B0ECu);
    abi_ActorSpeedXSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    y8_load(0x42);
    jsl_frame(0x83B0F2u);
    abi_ActorSpeedYSet();
    cpu.s = (uint16_t)(cpu.s + 3);
    for (;;) {
        a8_load(0x55);
        wr8(dpa(KIND3C_LEG), 0x55);
        call_frame(0xB0FC, abi_ActorSpeedYTurn);
        do {
            actor_yield(0x83B0FDu);
            jsl_frame(0x83B0FFu);
            abi_ActorMoveY();
            cpu.s = (uint16_t)(cpu.s + 3);
        } while (dec8_at(dpa(KIND3C_LEG)));
    }
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

    jsl_frame(0x83EB24u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    riding = rd8(dpa(KIND69_RIDING));
    a8_load(riding);
    if (!riding)
        return;
    call_frame(0xEB2E, abi_ActorOnEscort);
    /* TRB: Z from A AND the byte, then the bit cleared */
    a8_load(0x08);
    bits = rd8(dba(ACTOR_ESCORT + ACTOR_08));
    flag_set(P_Z, !(bits & 0x08));
    wr8(dba(ACTOR_ESCORT + ACTOR_08), (uint8_t)(bits & ~0x08));
    call_frame(0xEB36, abi_ActorStopsShot);
    call_frame(0xEB39, abi_PlayerLandsOnActor);
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
    jsl_frame(0x83EB46u);
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
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

    jsl_frame(0x83ABD8u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
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
    jsl_frame(0x8495D4u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x8495D8u);
    abi_ActorHitsTestBoxIfPlayerFreeLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (!IF_C)
        { jsl_frame(0x8495DEu); abi_PlayerEnter8CFacingLong(); cpu.s = (uint16_t)(cpu.s + 3); }
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

    jsl_frame(0x84A7D4u);
    abi_ActorFlyAgainst();
    cpu.s = (uint16_t)(cpu.s + 3);
    top = rd8(dpa(ACTOR_SPEED_Y + 2));
    a8_load(top);
    if (!(top & 0x80))
        return;
    wr8(dpa(ACTOR_ACCEL_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y), 0);
    wr8(dpa(ACTOR_SPEED_Y + 1), 0);
    wr8(dpa(ACTOR_SPEED_Y + 2), 0);
}

/*
 * @routine 83:C569 Kind40Wait
 * WHAT IT DOES: What an actor of kind $40 runs each frame it waits: its
 * animation, then its end when it is out of range by box $18.
 * HOW: AnimStep; X = $18, then into ActorEndOutOfRange (a copy of
 * ActorWaitAnimRange18).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: as
 * ActorEndOutOfRange: in range it returns; out of range the actor ends.
 */
void abi_Kind40Wait(void)
{
    jsl_frame(0x83C569u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x18);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:C966 Kind62Wait
 * WHAT IT DOES: What an actor of kind $62 runs each frame it waits: the
 * test whether it stops Aladdin's shot, then its end when it is out of
 * range by box 0.
 * HOW: ActorStopsShot; X = 0, then into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: as
 * ActorEndOutOfRange: in range it returns; out of range the actor ends.
 */
void abi_Kind62Wait(void)
{
    call_frame(0xC968, abi_ActorStopsShot);
    x8_load(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:C5D5 Kind7BWait
 * WHAT IT DOES: What an actor of kind $7B runs each frame it waits: its
 * animation, then its end when it is out of range by box $60.
 * HOW: AnimStep; X = $60, then into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: as
 * ActorEndOutOfRange: in range it returns; out of range the actor ends.
 */
void abi_Kind7BWait(void)
{
    jsl_frame(0x83C5D5u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x60);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:FA45 Kind8AWait
 * WHAT IT DOES: What an actor of kind $8A runs each frame it waits: its
 * animation, then its end when it is out of range by box 0.
 * HOW: AnimStep; X = 0, then into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by JSR
 * through the actor loop's wait table (83:94D0). Exit: as
 * ActorEndOutOfRange: in range it returns; out of range the actor ends.
 */
void abi_Kind8AWait(void)
{
    jsl_frame(0x83FA45u);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:A1BD Kind28Wait
 * WHAT IT DOES: Kind $28's wait: animates; downed by shots; counts the
 * frames it faces right; stomped or hurting as touched, or with its $45
 * clear it bounces the player and, when it does (no script has it), it
 * is downed itself (state $8C, its per-frame routine off); ended when out
 * of range (box 8).
 * HOW: AnimStep; ActorDownedByShot; its facing not negative: $46 up. $45
 * set: ActorStompOrHurt; else ActorBouncesPlayer, Z clear: bit 7 of $09
 * cleared, state $8C. Then ActorEndOutOfRange with X = 8.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorEndOutOfRange.
 */
void abi_Kind28Wait(void)
{
    jsl_frame(0x83A1BDu);  /* 83A1BD jsl */
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0xA1C3, abi_ActorDownedByShot);  /* 83A1C1 jsr */
    lda8(ea_dp(ACTOR_FACING));  /* 83A1C4 lda */
    if (IF_N) goto L_83A1CA;  /* 83A1C6 bmi */
    MEM8(inc, ea_dp(KIND28_RIGHT));  /* 83A1C8 inc */
L_83A1CA:
    lda8(ea_dp(KIND28_STOMPS));  /* 83A1CA lda */
    if (IF_Z) goto L_83A1D3;  /* 83A1CC beq */
    call_frame(0xA1D0, abi_ActorStompOrHurt);  /* 83A1CE jsr */
    goto L_83A1E0;  /* 83A1D1 bra */
L_83A1D3:
    call_frame(0xA1D5, abi_ActorBouncesPlayer);  /* 83A1D3 jsr */
    if (IF_Z) goto L_83A1E0;  /* 83A1D6 beq */
    lda8_v(0x80);  /* 83A1D8 lda */
    trb8(ea_dp(ACTOR_09));  /* 83A1DA trb */
    lda8_v(0x8C);  /* 83A1DC lda */
    sta8(ea_dp(ACTOR_STATE));  /* 83A1DE sta */
L_83A1E0:
    ldx8_v(0x08);  /* 83A1E0 ldx */
    abi_ActorEndOutOfRange();  /* 83A1E2 jmp */
}

/*
 * @routine 83:B11C Kind3CWait
 * WHAT IT DOES: Kind $3C's wait: drifts across, turning round on odd
 * frames of $032D clear (no script has it); while high on the screen
 * (its y pixel low byte below $D4) it bounces the player, with sound $12,
 * and then turns its rise into a fall (no script has that); ended once
 * left behind.
 * HOW: $032D bit 0 clear: its facing flipped. ActorMoveX. Its $0F below
 * $D4: ActorBouncesPlayer, Z clear: SoundQueueAdd $12; its speed y high
 * byte negative: KIND3C_LEG = $55 less KIND3C_LEG, ActorSpeedYTurn. Then JML
 * ActorEndIfLeftBehind.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorEndIfLeftBehind.
 */
void abi_Kind3CWait(void)
{
    lda8(ea_abs(FRAME_COUNT_32D));  /* 83B11C lda */
    bit8_i(0x01);  /* 83B11F bit */
    if (!IF_Z) goto L_83B129;  /* 83B121 bne */
    lda8(ea_dp(ACTOR_FACING));  /* 83B123 lda */
    eor8_v(0x01);  /* 83B125 eor */
    sta8(ea_dp(ACTOR_FACING));  /* 83B127 sta */
L_83B129:
    jsl_frame(0x83B129u);  /* 83B129 jsl */
    abi_ActorMoveX();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(ACTOR_Y + 1));  /* 83B12D lda */
    cmp8_v(0xD4);  /* 83B12F cmp */
    if (IF_C) goto L_83B14C;  /* 83B131 bcs */
    call_frame(0xB135, abi_ActorBouncesPlayer);  /* 83B133 jsr */
    if (IF_Z) goto L_83B14C;  /* 83B136 beq */
    lda8_v(0x12);  /* 83B138 lda */
    jsl_frame(0x83B13Au);  /* 83B13A jsl */
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(ACTOR_SPEED_Y + 2));  /* 83B13E lda */
    if (!IF_N) goto L_83B14C;  /* 83B140 bpl */
    sec();  /* 83B142 sec */
    lda8_v(0x55);  /* 83B143 lda */
    sbc8_v(rd8(ea_dp(KIND3C_LEG)));  /* 83B145 sbc */
    sta8(ea_dp(KIND3C_LEG));  /* 83B147 sta */
    call_frame(0xB14B, abi_ActorSpeedYTurn);  /* 83B149 jsr */
L_83B14C:
    abi_ActorEndIfLeftBehind();  /* 83B14C jml */
}

/*
 * @routine 83:BD22 Kind37Wait
 * WHAT IT DOES: The wait of kinds $37 and $38: bounces or hurts the player
 * (bounced: it is downed, state $8C, no script has it); downed by shots
 * (KIND37_HIT = 2, no script has it); animates; kind $38, or kind $37 with
 * KIND37_FREE set, ends out of range (box $10); kind $37 with KIND37_FREE
 * clear, once out of range, frees its partner (KIND37_PARTNER) and counts
 * its maker's $4B down, then ends (no script has that).
 * HOW: The test of $07 bit 0 branches to the next instruction either way.
 * ActorBouncesOrHurtsPlayer; A = 1: state bits $8C set, bit 7 of $09
 * cleared, KIND37_HIT = 1. ActorDownedByShot, C clear: KIND37_HIT = 2.
 * AnimStep. Kind not $38: ActorEndOutOfRange with X = 0. Else X = $10;
 * KIND37_FREE set: ActorEndOutOfRange; clear: ActorInRange, in range
 * back; out: ActorFreePlacedAtY with Y = KIND37_PARTNER, its maker's
 * $4B ($4B,X with X = KIND37_MAKER) down by one; its return dropped and
 * into ActorEndPlaced (by the code at $83:96C6).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind
 * (also the start entry of kind $37 in the table at $83:8CAB). Exit: as
 * ActorEndOutOfRange; in range: as ActorInRange; ended: no return.
 */
void abi_Kind37Wait(void)
{
    lda8(ea_dp(ACTOR_07));  /* 83BD22 lda */
    and8_v(0x01);  /* 83BD24 and; 83BD26 bne to the next instruction */
    call_frame(0xBD2A, abi_ActorBouncesOrHurtsPlayer);  /* 83BD28 jsr */
    ACC8(dec);  /* 83BD2B dec */
    if (!IF_Z) goto L_83BD3A;  /* 83BD2C bne */
    lda8_v(0x8C);  /* 83BD2E lda */
    tsb8(ea_dp(ACTOR_STATE));  /* 83BD30 tsb */
    lda8_v(0x80);  /* 83BD32 lda */
    trb8(ea_dp(ACTOR_09));  /* 83BD34 trb */
    lda8_v(0x01);  /* 83BD36 lda */
    sta8(ea_dp(KIND37_HIT));  /* 83BD38 sta */
L_83BD3A:
    call_frame(0xBD3C, abi_ActorDownedByShot);  /* 83BD3A jsr */
    if (IF_C) goto L_83BD43;  /* 83BD3D bcs */
    lda8_v(0x02);  /* 83BD3F lda */
    sta8(ea_dp(KIND37_HIT));  /* 83BD41 sta */
L_83BD43:
    jsl_frame(0x83BD43u);  /* 83BD43 jsl */
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    lda8(ea_dp(ACTOR_06));  /* 83BD47 lda */
    cmp8_v(0x38);  /* 83BD49 cmp */
    if (IF_Z) goto L_83BD52;  /* 83BD4B beq */
    ldx8_v(0x00);  /* 83BD4D ldx */
    abi_ActorEndOutOfRange();  /* 83BD4F jmp */
    return;
L_83BD52:
    ldx8_v(0x10);  /* 83BD52 ldx */
    lda8(ea_dp(KIND37_FREE));  /* 83BD54 lda */
    if (IF_Z) goto L_83BD5B;  /* 83BD56 beq */
    abi_ActorEndOutOfRange();  /* 83BD58 jmp */
    return;
L_83BD5B:
    call_frame(0xBD5D, abi_ActorInRange);  /* 83BD5B jsr */
    if (IF_C) goto L_83BD61;  /* 83BD5E bcs */
    return;  /* 83BD60 rts */
L_83BD61:
    op_rep(0x10);  /* 83BD61 rep */
    ldy16(ea_dp(KIND37_PARTNER));  /* 83BD63 ldy */
    call_frame(0xBD67, abi_ActorFreePlacedAtY);  /* 83BD65 jsr */
    ldx16(ea_dp(KIND37_MAKER));  /* 83BD68 ldx */
    MEM8(dec, ea_absx(KIND37_MAKER_COUNT));  /* 83BD6A dec */
    op_sep(0x10);  /* 83BD6D sep */
    /* 83BD6F jmp $96C6: SEP #$30, the return dropped, into ActorEndPlaced */
    op_sep(0x30);
    pla8();
    pla8();
    abi_ActorEndPlaced();
}

/*
 * @routine 83:A413 Kind2AWait
 * WHAT IT DOES: Kind $2A's wait: while KIND2A_BOB is set it bobs with its
 * animation (3 down on frame 4, 2 up or down on frame 5, 3 up on frame
 * 6); unless KIND2A_STILL is set it animates, is stomped or hurts, and
 * when downed by a shot (no script has it) counts KIND2A_HITS, and
 * KIND2A_RIGHT when it faces right; ended when out of range (box 8).
 * HOW: KIND2A_BOB set: X 16-bit = its y pixels; its frame ($27) unlike
 * KIND2A_FRAME: kept, then frame 4: y + 3, KIND2A_UP = 0; frame 5: y - 2
 * with KIND2A_UP clear, else + 2; frame 6: y - 3, KIND2A_UP up; the y
 * stored. KIND2A_STILL clear: AnimStep, ActorStompOrHurt,
 * ActorDownedByShot; C clear: facing not negative, KIND2A_RIGHT up;
 * KIND2A_HITS up. Then ActorEndOutOfRange with X = 8.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorEndOutOfRange.
 */
void abi_Kind2AWait(void)
{
    lda8(ea_dp(KIND2A_BOB));  /* 83A413 lda */
    if (IF_Z) goto L_83A44A;  /* 83A415 beq */
    op_rep(0x10);  /* 83A417 rep */
    ldx16(ea_dp(ACTOR_Y + 1));  /* 83A419 ldx */
    lda8(ea_dp(ACTOR_27));  /* 83A41B lda */
    cmp8_v(rd8(ea_dp(KIND2A_FRAME)));  /* 83A41D cmp */
    if (IF_Z) goto L_83A448;  /* 83A41F beq */
    sta8(ea_dp(KIND2A_FRAME));  /* 83A421 sta */
    sec();  /* 83A423 sec */
    sbc8_v(0x04);  /* 83A424 sbc */
    if (!IF_Z) goto L_83A42F;  /* 83A426 bne */
    inx16();  /* 83A428 inx */
    inx16();  /* 83A429 inx */
    inx16();  /* 83A42A inx */
    stz8(ea_dp(KIND2A_UP));  /* 83A42B stz */
    goto L_83A446;  /* 83A42D bra */
L_83A42F:
    ACC8(dec);  /* 83A42F dec */
    if (!IF_Z) goto L_83A43E;  /* 83A430 bne */
    lda8(ea_dp(KIND2A_UP));  /* 83A432 lda */
    if (!IF_Z) goto L_83A43A;  /* 83A434 bne */
    dex16();  /* 83A436 dex */
    dex16();  /* 83A437 dex */
    goto L_83A446;  /* 83A438 bra */
L_83A43A:
    inx16();  /* 83A43A inx */
    inx16();  /* 83A43B inx */
    goto L_83A446;  /* 83A43C bra */
L_83A43E:
    ACC8(dec);  /* 83A43E dec */
    if (!IF_Z) goto L_83A448;  /* 83A43F bne */
    dex16();  /* 83A441 dex */
    dex16();  /* 83A442 dex */
    dex16();  /* 83A443 dex */
    MEM8(inc, ea_dp(KIND2A_UP));  /* 83A444 inc */
L_83A446:
    stx16(ea_dp(ACTOR_Y + 1));  /* 83A446 stx */
L_83A448:
    op_sep(0x10);  /* 83A448 sep */
L_83A44A:
    lda8(ea_dp(KIND2A_STILL));  /* 83A44A lda */
    if (!IF_Z) goto L_83A462;  /* 83A44C bne */
    jsl_frame(0x83A44Eu);  /* 83A44E jsl */
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0xA454, abi_ActorStompOrHurt);  /* 83A452 jsr */
    call_frame(0xA457, abi_ActorDownedByShot);  /* 83A455 jsr */
    if (IF_C) goto L_83A462;  /* 83A458 bcs */
    lda8(ea_dp(ACTOR_FACING));  /* 83A45A lda */
    if (IF_N) goto L_83A460;  /* 83A45C bmi */
    MEM8(inc, ea_dp(KIND2A_RIGHT));  /* 83A45E inc */
L_83A460:
    MEM8(inc, ea_dp(KIND2A_HITS));  /* 83A460 inc */
L_83A462:
    ldx8_v(0x08);  /* 83A462 ldx */
    abi_ActorEndOutOfRange();  /* 83A464 jmp */
}

/*
 * @routine 83:C21C Kind3BWait
 * WHAT IT DOES: Kind $3B's wait, by its $07: bit 4 set, it rises back to
 * KIND3B_REST_Y, carrying the player while it lifts him; bit 3 set, it
 * animates and, once out of range, sets its level flag; else it lifts
 * the player when he lands on it, and variant 2 stays while another
 * actor shares its tile slot; ended when out of range (box $18).
 * HOW: Bit 4: its y less KIND3B_REST_Y (below it: put at it, 0; no script
 * has that) in KIND3B_RISE; LIFT_STEP positive: below $10, speed entry
 * $5D, ActorMoveY and PlayerToActorBelow with A = 3; else KIND3B_RISE
 * nonzero: speed entry $5A, ActorMoveY. Then AnimStep and ActorFreeOut-
 * OfRange with X = $18. Bit 3: AnimStep, ActorFreeOutOfRange with X =
 * $18; freed: LevelFlagFromMask with X = $07 & 7. Else: $4C = $1E,
 * ActorTestCarries with X = 2, ActorLiftPlayerStep; variant 2 with its
 * tile slot's users not 1: back; else ActorFreeOutOfRange with X = $18.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorFreeOutOfRange, LevelFlagFromMask or ActorLiftPlayerStep.
 */
void abi_Kind3BWait(void)
{
    lda8(ea_dp(ACTOR_07));  /* 83C21C lda */
    bit8_i(0x10);  /* 83C21E bit */
    if (!IF_Z) goto L_83C25C;  /* 83C220 bne */
    bit8_i(0x08);  /* 83C222 bit */
    if (IF_Z) goto L_83C23A;  /* 83C224 beq */
    jsl_frame(0x83C226u); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83C226 jsl */
    ldx8_v(0x18);  /* 83C22A ldx */
    call_frame(0xC22E, abi_ActorFreeOutOfRange);  /* 83C22C jsr */
    if (!IF_C) return;  /* 83C22F bcc (83C239 rts) */
    lda8(ea_dp(ACTOR_07));  /* 83C231 lda */
    and8_v(0x07);  /* 83C233 and */
    tax8();  /* 83C235 tax */
    call_frame(0xC238, abi_LevelFlagFromMask);  /* 83C236 jsr */
    return;  /* 83C239 rts */
L_83C23A:
    lda8_v(0x1E);  /* 83C23A lda */
    sta8(ea_dp(ACTOR_4C));  /* 83C23C sta */
    ldx8_v(0x02);  /* 83C23E ldx */
    call_frame(0xC242, abi_ActorTestCarries);  /* 83C240 jsr */
    ldx8_v(0x02);  /* 83C243 ldx */
    call_frame(0xC247, abi_ActorLiftPlayerStep);  /* 83C245 jsr */
    lda8(ea_dp(ACTOR_07));  /* 83C248 lda */
    cmp8_v(0x02);  /* 83C24A cmp */
    if (!IF_Z) goto L_83C257;  /* 83C24C bne */
    ldy8(ea_dp(ACTOR_TILE_SLOT));  /* 83C24E ldy */
    lda8(ea_absy(ACTOR_TILE_SLOT_USERS));  /* 83C250 lda */
    ACC8(dec);  /* 83C253 dec */
    if (IF_Z) goto L_83C257;  /* 83C254 beq */
    return;  /* 83C256 rts */
L_83C257:
    ldx8_v(0x18);  /* 83C257 ldx */
    call_frame(0xC22E, abi_ActorFreeOutOfRange);  /* 83C259 jmp */
    return;
L_83C25C:
    op_rep(0x20);  /* 83C25C rep */
    sec();  /* 83C25E sec */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83C25F lda */
    sbc16_v(rd16(ea_dp(KIND3B_REST_Y)));  /* 83C261 sbc */
    if (IF_C) goto L_83C26C;  /* 83C263 bcs */
    lda16(ea_dp(KIND3B_REST_Y));  /* 83C265 lda */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83C267 sta */
    lda16_v(0x0000);  /* 83C269 lda */
L_83C26C:
    op_sep(0x20);  /* 83C26C sep */
    sta8(ea_dp(KIND3B_RISE));  /* 83C26E sta */
    lda8(ea_dp(KIND3B_LIFT_STEP));  /* 83C270 lda */
    if (IF_Z) goto L_83C28E;  /* 83C272 beq */
    if (IF_N) goto L_83C28E;  /* 83C274 bmi */
    lda8(ea_dp(KIND3B_RISE));  /* 83C276 lda */
    cmp8_v(0x10);  /* 83C278 cmp */
    if (IF_C) goto L_83C29C;  /* 83C27A bcs */
    ldy8_v(0x5D);  /* 83C27C ldy */
    jsl_frame(0x83C27Eu); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83C27E jsl */
    jsl_frame(0x83C282u); abi_ActorMoveY(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83C282 jsl */
    lda8_v(0x03);  /* 83C286 lda */
    jsl_frame(0x83C288u); abi_PlayerToActorBelow(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83C288 jsl */
    goto L_83C29C;  /* 83C28C bra */
L_83C28E:
    lda8(ea_dp(KIND3B_RISE));  /* 83C28E lda */
    if (IF_Z) goto L_83C29C;  /* 83C290 beq */
    ldy8_v(0x5A);  /* 83C292 ldy */
    jsl_frame(0x83C294u); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83C294 jsl */
    jsl_frame(0x83C298u); abi_ActorMoveY(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83C298 jsl */
L_83C29C:
    jsl_frame(0x83C29Cu); abi_AnimStep(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83C29C jsl */
    ldx8_v(0x18);  /* 83C2A0 ldx */
    abi_ActorFreeOutOfRange();  /* 83C2A2 jmp */
}
