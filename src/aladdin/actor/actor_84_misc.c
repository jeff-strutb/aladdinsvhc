/*
 * actor_84_misc.c: bank $84 actor helpers: a child spawned with a hit box
 * of $A2, an actor freed far out of range telling its parent, a chain of
 * followers, a clamp above the floor, the player's fall test, and a grab
 * test.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
void abi_ActorAnimFreeFar(void);
void abi_ActorClampAboveFloor(void);
void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorGrabTest(void);
void abi_ActorHitsTestBoxIfActiveLong(void);
void abi_ActorMoveAlong(void);
void abi_ActorSpawn(void);
void abi_ActorSpawnA2(void);
void abi_ActorStompOrHurtLong(void);
void abi_ActorsFollowSteps(void);
void abi_AnimStep(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_PlayerFallCheck(void);
void abi_ActorListStart(void);
void abi_ActorPriorityByHeight(void);
void abi_PlayerClampAboveFloor(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define SPAWN_A2_ADD 0x002Cu        /* ActorSpawnA2's A, added to the child's $07 (scratch, data bank) */
#define FOLLOW_HEAD 0x1B16u         /* the record that heads the followers' list (word) */
#define FOLLOW_STEP 0x1B19u         /* added to each follower's $41 for its step */
#define FALL_OFF 0x1B2Bu            /* nonzero: the player's fall check is off */
#define PLAYER_STATE_8C_COUNT 0x141Bu /* counted up on entering state $8C */
#define NO_BOUNCE 0x145Au           /* negative: no bounce */
#define FLOOR_LINE 0x7EDFC0u        /* the floor's height by x: 32 words, 9-bit signed, one per 8 pixels from x $200 */
#define FLOOR_AT 0x000Eu            /* the floor's y found here (scratch word, data bank) */
#define FLOOR_CLEARANCE 0xD7C4u     /* taken from the floor's y when an actor is put on it (word, data bank) */
#define HIT_BOXES 0xC29Au           /* the hit boxes: 4 bytes each, the half-width first (data bank) */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)  /* the player's x, whole pixels (word) */
#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1)  /* the player's y, whole pixels (word) */
#define PLAYER_FLIP_AT (PLAYER + ACTOR_FLIP) /* nonzero when he faces right */
#define ACTOR_43 0x43u
#define PLAYER_FLOOR_HITS 0x1428u   /* counted up (word) each frame the floor line holds the player up */
#define FOLLOW_LIST 0x89ADu         /* the list ActorListStartAD starts (pair X, Y) */
#define FOLLOW_WHEN 0x40u           /* ActorsFollowIf8: the followers move while this is 8 */

/*
 * @routine 84:8145 ActorSpawnA2
 * WHAT IT DOES: Starts a child with hit box $A2 at the spawner's spawn
 * point $82, telling it its parent and a $07, and counts it in the
 * spawner's $42.
 * HOW: A kept at $002C; ActorSpawn; none free: back. The child's $07 =
 * the spawner's $07 less 3 plus A when that $07 is 3 or more, else the
 * spawner's $44; the child's $4C word = the spawner's record (D); the
 * spawner's $42 up.
 * Entry: M 8-bit, X 8-bit; A = the byte for the child's $07 sum; D = the
 * spawner; DB = $80; called by JSR. Exit: M 8-bit, X 8-bit; spawned: X =
 * the child's low byte; else as ActorSpawn.
 */
void abi_ActorSpawnA2(void)
{
    uint16_t child;
    uint8_t v;

    wr8(dba(SPAWN_A2_ADD), (uint8_t)cpu.a);
    a8_load(0x82);
    wr8(dpa(ACTOR_SPAWN_AT), 0x82);
    a8_load(0xA2);
    jsl_frame(0x84814Eu);
    abi_ActorSpawn();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (cpu.p & P_N)
        return;                                  /* none free */
    child = cpu.x;
    flag_set(P_C, 1);
    v = op_sbc8(rd8(dpa(ACTOR_07)), 3, &cpu.p);
    if (cpu.p & P_C) {
        flag_set(P_C, 0);
        v = op_adc8(v, rd8(dba(SPAWN_A2_ADD)), &cpu.p);
    } else {
        v = rd8(dpa(ACTOR_44));
    }
    wr8(dba((uint16_t)(child + ACTOR_07)), v);
    /* the child's $4C word: its parent, the spawner */
    wr8(dba((uint16_t)(child + ACTOR_4C)), (uint8_t)cpu.d);
    wr8(dba((uint16_t)(child + ACTOR_4D)), (uint8_t)(cpu.d >> 8));
    cpu.a = (uint16_t)(cpu.d << 8 | cpu.d >> 8);
    (void)inc8_at(dpa(ACTOR_42));
    op_sep(P_X);
}

/*
 * @routine 84:832B ActorAnimFreeFar
 * WHAT IT DOES: Plays the actor's animation, lets it hurt or be stomped
 * by the player, and ends it when out of range by box $58, then counting
 * it off its parent's $42.
 * HOW: AnimStep, ActorStompOrHurtLong, ActorFreeOutOfRangeLong with X =
 * $58; ended (C set): the parent's record (the actor's $4C word) has its
 * $42 stepped down.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL. Exit:
 * M 8-bit, X 8-bit.
 */
void abi_ActorAnimFreeFar(void)
{
    jsl_frame(0x84832Bu);
    abi_AnimStep();
    cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x84832Fu);
    abi_ActorStompOrHurtLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x58);
    jsl_frame(0x848335u);
    abi_ActorFreeOutOfRangeLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (cpu.p & P_C) {
        /* ended: one fewer counted in its parent's $42 */
        uint16_t parent = dp16(ACTOR_4C);
        uint8_t n = (uint8_t)(rd8(dba((uint16_t)(parent + ACTOR_42))) - 1);

        cpu.x = parent;
        a8_load(n);
        wr8(dba((uint16_t)(parent + ACTOR_42)), n);
        op_sep(P_X);
    }
}

/*
 * @routine 84:8D04 ActorsFollowSteps
 * WHAT IT DOES: Moves each actor of the list from the head at $1B16 to
 * where the one before it is, then on along its own direction by the step
 * $41 + $1B19 (at least 1).
 * HOW: With 16-bit registers, from the head: each next record (the word
 * at $37 of the one before, 0 ends) becomes D; its x and y (the bytes at
 * $0B-$10) copied from the record before; its direction ($42 and $1FF) in
 * Y and the step in A (0 or negative taken as 1); ActorMoveAlong. D kept
 * on the stack.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M 8-bit, X
 * 8-bit; D as at entry.
 */
void abi_ActorsFollowSteps(void)
{
    uint16_t before = rd16(dba(FOLLOW_HEAD)), rec, dir;
    uint8_t step;

    op_push8((uint8_t)(cpu.d >> 8));
    op_push8((uint8_t)cpu.d);
    op_rep(0x30);
    cpu.x = before;
    for (;;) {
        rec = rd16(dba((uint16_t)(before + ACTOR_NEXT)));
        a16_load(rec);
        if (!rec)
            break;
        /* this one goes where the one before it is, then steps on */
        cpu.d = rec;
        dp16_set(ACTOR_X, rd16(dba((uint16_t)(before + ACTOR_X))));
        dp16_set(ACTOR_X + 2, rd16(dba((uint16_t)(before + ACTOR_X + 2))));
        dp16_set(ACTOR_Y + 1, rd16(dba((uint16_t)(before + ACTOR_Y + 1))));
        dir = (uint16_t)(dp16(ACTOR_42) & 0x01FF);
        cpu.a = dir;
        cpu.y = dir;
        op_sep(0x20);
        flag_set(P_C, 0);
        step = rd8(dpa(ACTOR_41));
        step = op_adc8(step, rd8(dba(FOLLOW_STEP)), &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | step);
        if (!step || (step & 0x80))
            a8_load(0x01);
        jsl_frame(0x848D33u);
        abi_ActorMoveAlong();
        cpu.s = (uint16_t)(cpu.s + 3);
        op_rep(0x30);
        before = cpu.d;
        a16_load(before);
        cpu.x = before;
    }
    op_sep(0x30);
    {
        uint8_t lo = op_pull8();                 /* PLD: low byte first */
        uint8_t hi = op_pull8();

        cpu.d = (uint16_t)(lo | hi << 8);
    }
    nz16_of(cpu.d);
}

/*
 * @routine 84:8F3C ActorClampAboveFloor
 * WHAT IT DOES: Keeps the actor's y plus offset A above the floor line at
 * its x: C set (and y moved up to the floor) when it reached it.
 * HOW: The floor: the word for the actor's x from the table at $7E:DFC0
 * ((x - $200) / 4, in words), sign-extended from 9 bits, negated, plus
 * $B0. The actor's y + A: negative, C clear; below the floor, y = the
 * floor less the word at $D7C4 (Y is 0), C set; else C clear.
 * Entry: M 16-bit, X 8-bit; A = the offset; D = the actor; DB a bank
 * whose $0000-$1FFF is work RAM; called by JSR. Exit: M 16-bit, X 8-bit;
 * Y = 0; C as above.
 */
void abi_ActorClampAboveFloor(void)
{
    uint16_t off = cpu.a, raw, floor, at;
    uint8_t col;

    wr16(dba(SCRATCH), off);
    cpu.y = 0;
    col = (uint8_t)(((uint16_t)(dp16(ACTOR_X + 1) - 0x0200) >> 2) & 0x3E);
    cpu.x = col;
    raw = (uint16_t)(rd16(FLOOR_LINE + col) & 0x01FF);
    if (raw >= 0x0100)
        raw |= 0xFF00;
    floor = (uint16_t)(0xB0 - raw);
    wr16(dba(FLOOR_AT), floor);
    flag_set(P_C, 0);
    at = op_adc16(dp16(ACTOR_Y + 1), off, &cpu.p);
    cpu.a = at;
    if (at & 0x8000) {
        flag_set(P_C, 0);
        return;
    }
    cmp16_of(at, floor);
    if (at < floor)
        return;
    /* on the floor: put there, less its clearance */
    cpu.a = op_sbc16(floor, rd16(dba(FLOOR_CLEARANCE)), &cpu.p);
    dp16_set(ACTOR_Y + 1, cpu.a);
    flag_set(P_C, 1);
}

/*
 * @routine 84:8F81 PlayerFallCheck
 * WHAT IT DOES: When $1B2B is clear and the player is at or below y $B8,
 * puts him into state $8C facing as he is (the fall).
 * HOW: $1B2B nonzero: nothing; the player's y negative: nothing; at or
 * past $B8: PlayerEnter8CFacingLong.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M 8-bit; as PlayerEnter8CFacingLong when it ran.
 */
void abi_PlayerFallCheck(void)
{
    uint8_t off = rd8(dba(FALL_OFF));
    uint16_t y;

    a8_load(off);
    if (off)
        return;
    op_rep(0x20);
    y = rd16(dba(PLAYER_Y_AT));
    a16_load(y);
    if (y & 0x8000) {
        op_sep(0x20);
        return;
    }
    cmp16_of(y, 0x00B8);
    op_sep(0x20);
    if (y >= 0x00B8)
        { jsl_frame(0x848F94u); abi_PlayerEnter8CFacingLong(); cpu.s = (uint16_t)(cpu.s + 3); }
}

/*
 * @routine 82:88CF ActorGrabTest
 * WHAT IT DOES: Tells whether the actor can grab the player: C clear when
 * he may be grabbed, touches it, is below it and is within its reach on
 * the side he faces.
 * HOW: $145A negative, $141B nonzero or $141C zero: C set.
 * ActorHitsTestBoxIfActiveLong: no touch, C set. The actor's y at or
 * below the player's: C set. Left of him: he must face right and its x +
 * its hit box's width - 15 reach his x; right of him: he must face left
 * and its x - width + 15 reach it; else C set.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80, the hit boxes' bank;
 * called by JSL. Exit: M 8-bit, X 8-bit (C clear, or C set at $82:8931 or
 * by the test).
 */
void abi_ActorGrabTest(void)
{
    uint16_t ax, px, reach, faces;
    uint8_t v;

    v = rd8(dba(NO_BOUNCE));
    a8_load(v);
    if (v & 0x80)
        goto no;
    v = rd8(dba(PLAYER_STATE_8C_COUNT));
    a8_load(v);
    if (v)
        goto no;
    v = rd8(dba(V_141C & 0xFFFF));
    a8_load(v);
    if (!v)
        goto no;
    jsl_frame(0x8288DEu);
    abi_ActorHitsTestBoxIfActiveLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (cpu.p & P_C)
        return;                                  /* no touch */
    op_rep(0x30);
    a16_load(dp16(ACTOR_Y + 1));
    cmp16_of(cpu.a, rd16(dba(PLAYER_Y_AT)));
    if (cpu.p & P_C)
        goto out;                                /* not above him */
    cpu.a = (uint16_t)((dp16(ACTOR_06) & 0x00FF) << 2);
    cpu.y = cpu.a;
    ax = dp16(ACTOR_X + 1);
    px = rd16(dba(PLAYER_X_AT));
    flag_set(P_C, 1);
    cpu.a = op_sbc16(ax, px, &cpu.p);
    faces = (uint16_t)(rd16(dba(PLAYER_FLIP_AT)) & 0x00FF);
    if (!(cpu.p & P_C)) {
        /* it is left of him: he must face right, its reach past his x */
        a16_load(faces);
        if (!faces)
            goto out;
        flag_set(P_C, 1);
        reach = op_sbc16(rd16(dba((uint16_t)(HIT_BOXES + cpu.y))), 0x000F, &cpu.p);
        flag_set(P_C, 0);
        cpu.a = op_adc16(reach, ax, &cpu.p);
        cmp16_of(cpu.a, px);
        if (cpu.p & P_C)
            goto out;
    } else {
        /* right of him: he must face left, its reach up to his x */
        a16_load(faces);
        if (faces)
            goto out;
        flag_set(P_C, 1);
        reach = op_sbc16(ax, rd16(dba((uint16_t)(HIT_BOXES + cpu.y))), &cpu.p);
        flag_set(P_C, 0);
        cpu.a = op_adc16(reach, 0x000F, &cpu.p);
        cmp16_of(cpu.a, px);
        if (!(cpu.p & P_C))
            goto out;
    }
    op_sep(0x30);
    flag_set(P_C, 0);
    return;
out:
    op_sep(0x31);
    return;
no:
    flag_set(P_C, 1);
}

/*
 * @routine 84:8992 ActorYieldForever
 * @actor
 * WHAT IT DOES: An actor that has nothing more to do: it yields every
 * frame for good.
 * HOW: BRK, BRA back.
 * Entry: D = the actor's record; reached by jumps from actor code in this
 * bank. Exit: never returns.
 */
void abi_ActorYieldForever(void)
{
    for (;;)
        actor_yield(0x848992u);
}

/*
 * @routine 84:8996 ActorListStartAD
 * WHAT IT DOES: Starts every actor in this actor's list with the pair
 * $AD, $89 and values stepping by 8.
 * HOW: ActorListStart with X = $AD, Y = $89, A = 8.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSR. Exit: as
 * ActorListStart.
 */
void abi_ActorListStartAD(void)
{
    x8_load((uint8_t)FOLLOW_LIST);
    y8_load((uint8_t)(FOLLOW_LIST >> 8));
    a8_load(0x08);
    call_frame(0x899E, abi_ActorListStart);
}

/*
 * @routine 84:8CC5 ActorsFollowIf8
 * WHAT IT DOES: While the actor's $40 is 8, moves its followers on and
 * sets its sprite priority by its height.
 * HOW: FOLLOW_WHEN against 8: equal, ActorsFollowSteps and
 * ActorPriorityByHeight.
 * Entry: M 8-bit, X 8-bit; D = the actor; called by JSR. Exit: M and X
 * 8-bit; as ActorPriorityByHeight when it ran, else the compare's flags.
 */
void abi_ActorsFollowIf8(void)
{
    uint8_t when = rd8(dpa(FOLLOW_WHEN));

    a8_load(when);
    cmp8_of(when, 0x08);
    if (when == 0x08) {
        call_frame(0x8CCDu, abi_ActorsFollowSteps);   /* its JSR at $84:8CCB: the C pushes */
        abi_ActorPriorityByHeight();
    }
}

/*
 * @routine 84:8EF0 Kind88Work
 * WHAT IT DOES: What actors of kind $88 do each frame they wait: keep
 * Aladdin above the floor line, and put him into his fall below it.
 * HOW: PlayerClampAboveFloor, PlayerFallCheck.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSL from the kind's stub ($83:931D) through the actor loop's
 * wait table at $83:904F. Exit: as PlayerFallCheck.
 */
void abi_Kind88Work(void)
{
    call_frame(0x8EF2, abi_PlayerClampAboveFloor);
    call_frame(0x8EF5, abi_PlayerFallCheck);
}

/*
 * @routine 84:8EF7 PlayerClampAboveFloor
 * WHAT IT DOES: Keeps Aladdin above the floor line once he is at x $200
 * or more, by an offset of 30 pixels (24 with no state $8C counted and
 * $141C set); not while he rises, except with no state $8C counted and
 * $141C clear.
 * HOW: D = his record. His x's high byte below 2: nothing. With
 * PLAYER_STATE_8C_COUNT set: rising (the high byte of his vertical speed
 * negative), nothing, else $1E. With it zero: $141C zero, $1E; else
 * rising, nothing, else $18. ActorClampAboveFloor; C set:
 * PLAYER_FLOOR_HITS up.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: M and X 8-bit; D as at entry, N and Z from it.
 */
void abi_PlayerClampAboveFloor(void)
{
    uint8_t v;
    uint16_t off = 0x001E;

    op_rep(0x20);
    op_push8((uint8_t)(cpu.d >> 8));
    op_push8((uint8_t)cpu.d);
    a16_load((uint16_t)PLAYER);
    cpu.d = (uint16_t)PLAYER;
    v = rd8(dba((uint16_t)(PLAYER + ACTOR_X + 2)));
    x8_load(v);
    cmp8_of(v, 0x02);
    if (v < 0x02)
        goto out;
    v = rd8(dba(PLAYER_STATE_8C_COUNT));
    x8_load(v);
    if (v) {
        x8_load(rd8(dba((uint16_t)(PLAYER + ACTOR_SPEED_Y + 2))));
        if (cpu.x & 0x80)
            goto out;                            /* rising */
    } else {
        v = rd8(dba(V_141C & 0xFFFF));
        x8_load(v);
        if (v) {
            x8_load(rd8(dba((uint16_t)(PLAYER + ACTOR_SPEED_Y + 2))));
            if (cpu.x & 0x80)
                goto out;                        /* rising */
            off = 0x0018;
        }
    }
    a16_load(off);
    call_frame(0x8F25, abi_ActorClampAboveFloor);
    if (cpu.p & P_C)
        (void)inc16_at(dba(PLAYER_FLOOR_HITS));
out:
    op_sep(0x30);
    {
        uint8_t lo = op_pull8();                 /* PLD: low byte first */
        uint8_t hi = op_pull8();

        cpu.d = (uint16_t)(lo | hi << 8);
    }
    nz16_of(cpu.d);
}

/*
 * @routine 84:8F2F ActorClampAboveFloor12
 * WHAT IT DOES: Keeps the actor's y plus 12 above the floor line.
 * HOW: ActorClampAboveFloor with A = $000C (16-bit), Y = 2.
 * Entry: X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work
 * RAM; called by JSR. Exit: M and X 8-bit; C as ActorClampAboveFloor.
 */
void abi_ActorClampAboveFloor12(void)
{
    op_rep(0x20);
    y8_load(0x02);
    a16_load(0x000C);
    call_frame(0x8F38, abi_ActorClampAboveFloor);
    op_sep(0x30);
}
