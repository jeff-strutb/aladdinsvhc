/*
 * player_landing.c: Aladdin coming down on top of an actor: he is stood on
 * it, his code goes on at his landing, and an actor of kind $11 starts at
 * his feet; and the pad he lands on and presses down, which stands him
 * on itself.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "actor_anim.h"

void abi_ActorHitsTestBoxIfActive(void);
void abi_ScratchBelowBy10(void);
void abi_ActorAllocBLong(void);
void abi_PlayerStandOnActor(void);
void abi_ActorSpawnKind11AtPlayer(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define V_1448 0x001448u           /* $3C: no landing on actors */
#define V_0367 0x000367u           /* counted down on entering state $8C; 0: no landing */
#define PLAYER_STATE_8C_COUNT 0x00141Bu /* counted up on entering state $8C */
#define V_141C 0x00141Cu           /* 0: no landing */
#define V_145A 0x00145Au           /* bit 7 set: no landing */
#define PLAYER_SPEED_Y_PIXELS (PLAYER + ACTOR_SPEED_Y + 1) /* his vertical speed's whole part (word) */
#define PLAYER_X_PIXELS (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define PLAYER_CODE (PLAYER + ACTOR_03) /* where his code goes on (word, his bank) */
#define PLAYER_LANDED_CODE 0xD77Cu /* his code once landed on an actor */
#define PLAYER_LANDED 0x00144Eu    /* 1: he has landed on an actor */
#define KIND_11 0x11

/* The pad's own fields */
#define PAD_REST 0x40u             /* frames before it can be landed on again */
#define PAD_PRESSED 0x41u          /* frames it shows pressed; at 1 it rises again */
#define PAD_LAND_ANIM 0x13         /* its animation at its start and at each landing */
#define PAD_AFTER_ANIM 0x14        /* its animation from 9 frames after a landing */
#define PAD_SOUND 0x12             /* the sound of a landing on it */
#define PAD_TOP 0x24               /* his y less its y, as he lands: he is above it */
#define PAD_STAND 0x2B             /* he stands this many pixels above its y, less 1 (taken with C clear) */
/* A byte of work RAM it puts in PLAYER_LANDED (an absolute $0001 where
   the other landings put an immediate 1) */
#define V_0001 0x000001u
/* Not yet filed (no script reaches it): pushes the player out of the
   actor's box sideways. */
#define PLAYER_PUSH_ASIDE 0x8380C3u
/* Not yet filed (no script reaches it): the pad met from the side, or
   with a state $8C counted: PLAYER_PUSH_ASIDE, then its next frame. */
#define PAD_FROM_SIDE 0x83FA40u

/*
 * @routine 83:84D5 PlayerStandOnActor
 * WHAT IT DOES: Stands Aladdin on the actor he has come down on: his code
 * goes on at his landing ($D77C of his bank) and his y is the actor's top.
 * HOW: Unless a state $8C is counted ($141B): $D77C as his code address,
 * his y pixels = the actor's less scratch $0010 (the boxes' half-heights
 * less 4, from ScratchBelowBy10), and $144E = 1. With $141B nonzero it
 * pulls its own return address and returns from its caller (M and X
 * 8-bit), code the retail game never runs: each of its five callers
 * ($83:8382, $83F6, $843E, $847E, $84BC) has just found $141B zero.
 * Entry: M 16-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: M 16-bit, X 8-bit; A = his y pixels; X =
 * 1; N, Z from X; C, V from the subtraction; Y unchanged.
 */
void abi_PlayerStandOnActor(void)
{
    uint16_t back;
    uint8_t stomped = rd8(dba(PLAYER_STATE_8C_COUNT));

    cpu.a = PLAYER_LANDED_CODE;
    x8_load(stomped);
    if (stomped) {
        op_sep(0x30);
        pla8();                                /* PLA, PLA: its own return */
        pla8();
        back = pull16();                       /* RTS through its caller's */
        cpu.pc = 0x830000u | (uint16_t)(back + 1);
        cpu.unwound = 1;
        return;
    }
    wr16(dba(PLAYER_CODE), PLAYER_LANDED_CODE);
    flag_set(P_C, 1);
    cpu.a = op_sbc16(dp16(ACTOR_Y + 1), rd16(dba(SCRATCH + 4)), &cpu.p);
    wr16(dba(PLAYER_Y_PIXELS), cpu.a);
    x8_load(0x01);
    wr8(dba(PLAYER_LANDED), 0x01);
}

/*
 * @routine 83:848D PlayerLandsOnActor
 * WHAT IT DOES: Tells whether Aladdin has come down on top of the actor,
 * and if so stands him on it, his code going on at his landing, with an
 * actor of kind $11 at his feet.
 * HOW: No when $1448 is $3C, when the actor misses the test box
 * (ActorHitsTestBoxIfActive), when the boxes overlap by 10 or more
 * pixels of height (ScratchBelowBy10), when he is rising, or when $0367
 * is 0, a state $8C is counted, $141C is 0 or bit 7 of $145A is set. Yes:
 * $83:84D5 stands him on it, then ActorSpawnKind11AtPlayer.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the hit boxes' bank. Exit: Z clear when he landed: A = 1, N
 * clear, X, Y, V and C as ActorSpawnKind11AtPlayer leaves them. Not: Z
 * set; C set when the $1448 test, the box test or the height test said
 * no, else clear; N, A, X and Y as the test that said no leaves them. M
 * and X 8-bit either way.
 */
void abi_PlayerLandsOnActor(void)
{
    uint8_t v;
    uint16_t fall;

    v = rd8(dba(V_1448));
    a8_load(v);
    cmp8_of(v, 0x3C);
    if (v == 0x3C)
        goto no;
    abi_ActorHitsTestBoxIfActive();
    if (cpu.p & P_C)
        goto no;
    op_rep(P_M);
    abi_ScratchBelowBy10();
    if (cpu.p & P_C)
        goto no;
    fall = rd16(dba(PLAYER_SPEED_Y_PIXELS));
    a16_load(fall);
    if (fall & 0x8000)                    /* rising */
        goto no;
    v = rd8(dba(V_0367));
    x8_load(v);
    if (!v)
        goto no;
    v = rd8(dba(PLAYER_STATE_8C_COUNT));
    x8_load(v);
    if (v)
        goto no;
    v = rd8(dba(V_141C));
    x8_load(v);
    if (!v)
        goto no;
    v = rd8(dba(V_145A));
    x8_load(v);
    if (v & 0x80)
        goto no;
    abi_PlayerStandOnActor();
    op_sep(0x30);
    abi_ActorSpawnKind11AtPlayer();
    a8_load(0x01);
    return;
no:
    op_sep(0x32);
}
/*
 * @routine 83:84F4 ActorSpawnKind11AtPlayer
 * WHAT IT DOES: Starts an actor of kind $11 at Aladdin's feet: at his x,
 * 16 pixels below his y, in a record of the second free list.
 * HOW: The record taken through $81:EA3D; none free: nothing. Its state
 * $0C (to start), kind $11, byte $07 zero, byte $08 $80, and its x and y
 * pixels from his.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: none free: N set, Y = the list's top offset (negative), A, X and
 * the other flags unchanged. Started: X = the record (low byte); Y = the
 * list's old top offset; A = the new y (16 bits); N, V, Z and C from
 * adding 16. M and X 8-bit either way.
 */
void abi_ActorSpawnKind11AtPlayer(void)
{
    abi_ActorAllocBLong();
    if (!(cpu.p & P_N)) {
        uint32_t rec = dba(0) + cpu.x;

        a8_load(0x0C);
        wr8(rec + ACTOR_STATE, 0x0C);
        a8_load(KIND_11);
        wr8(rec + ACTOR_06, KIND_11);
        wr8(rec + ACTOR_07, 0);
        a8_load(0x80);
        wr8(rec + ACTOR_08, 0x80);
        op_rep(P_M);
        wr16(rec + ACTOR_X + 1, rd16(dba(PLAYER_X_PIXELS)));
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(dba(PLAYER_Y_PIXELS)), 0x0010, &cpu.p);   /* at his feet */
        wr16(rec + ACTOR_Y + 1, cpu.a);
    }
    op_sep(0x30);
}

/* TSB: Z from A AND the byte, then the byte ORed with A. */
static void tsb_dp(uint16_t off, uint8_t bits)
{
    uint8_t v = rd8(dpa(off));

    a8_load(bits);
    flag_set(P_Z, !(v & bits));
    wr8(dpa(off), (uint8_t)(v | bits));
}

/*
 * @routine 83:F9BA LandingPad
 * @actor
 * WHAT IT DOES: An actor (through the stub at $83:9315): a pad Aladdin
 * can land on. Landed on from above, it plays sound $12, starts its
 * animation $13 again and $14 nine frames later, stands him on its top at
 * his landing code, and cannot be landed on again for 20 frames.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; $2C = 6; animation $13;
 * PAD_REST and PAD_PRESSED 0. Each frame (BRK first): PAD_PRESSED counts
 * down, and the animation $14 starts as it goes from 1 to 0; with
 * PAD_REST nonzero it counts down and the frame ends. Else with the test
 * box met (ActorHitsTestBoxIfActive) and $145A not negative: when his y
 * plus $24 is below its y, animation $13 and $141C = 1; then with no
 * state $8C counted, PAD_REST 20, PAD_PRESSED 10, SoundQueueAdd $12, his
 * code $D77C, a yield, his y = its y less $2C (SBC #$2B with the C its
 * REP #$21 cleared, kept through the yield), his x = its x, and
 * PLAYER_LANDED = the byte at $0001. Met from the side or with a state
 * $8C counted it goes on at PAD_FROM_SIDE, which no script reaches yet:
 * its C is not written, and it runs as the original's (jump_routine).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump through the stub at $83:9315. Exit: never
 * returns.
 */
void abi_LandingPad(void)
{
    tsb_dp(ACTOR_08, 0x02);
    tsb_dp(ACTOR_09, 0x90);
    wr8(dpa(ACTOR_2C), 0x06);
    a8_load(PAD_LAND_ANIM);
    abi_AnimStart();
    wr8(dpa(PAD_REST), 0);
    wr8(dpa(PAD_PRESSED), 0);
    for (;;) {
        uint8_t pressed, rest, block;
        uint16_t feet;

        actor_yield(0x83F9D0u);
        /* the pressed count runs out: animation $14 */
        pressed = rd8(dpa(PAD_PRESSED));
        a8_load(pressed);
        if (pressed) {
            a8_load((uint8_t)(pressed - 1));
            if (pressed == 1) {
                a8_load(PAD_AFTER_ANIM);
                abi_AnimStart();
            }
            pressed = (uint8_t)(rd8(dpa(PAD_PRESSED)) - 1);
            wr8(dpa(PAD_PRESSED), pressed);
            nz8_of(pressed);
        }
        rest = rd8(dpa(PAD_REST));
        a8_load(rest);
        if (rest) {
            rest--;
            wr8(dpa(PAD_REST), rest);
            nz8_of(rest);
            continue;
        }
        abi_ActorHitsTestBoxIfActive();
        if (cpu.p & P_C)
            continue;
        block = rd8(dba(V_145A));
        a8_load(block);
        if (block & 0x80)
            continue;
        /* met: from above (his feet above its y) it takes him */
        op_rep(0x20);
        feet = op_adc16(rd16(dba(PLAYER_Y_PIXELS)), PAD_TOP, &cpu.p);
        cpu.a = feet;
        cmp16_of(feet, dp16(ACTOR_Y + 1));
        op_sep(0x20);
        if (cpu.p & P_C)
            break;
        a8_load(PAD_LAND_ANIM);
        abi_AnimStart();
        a8_load(0x01);
        wr8(dba(V_141C), 0x01);
        uint8_t stomped = rd8(dba(PLAYER_STATE_8C_COUNT));
        a8_load(stomped);
        if (stomped)
            break;
        wr8(dpa(PAD_REST), 20);
        wr8(dpa(PAD_PRESSED), 10);
        a8_load(PAD_SOUND);
        abi_SoundQueueAdd();
        op_rep(0x21);
        a16_load(PLAYER_LANDED_CODE);
        wr16(dba(PLAYER_CODE), PLAYER_LANDED_CODE);
        actor_yield(0x83FA27u);
        /* he stands on its top (the SBC with the C the REP cleared) */
        cpu.a = op_sbc16(dp16(ACTOR_Y + 1), PAD_STAND, &cpu.p);
        wr16(dba(PLAYER_Y_PIXELS), cpu.a);
        a16_load(dp16(ACTOR_X + 1));
        wr16(dba(PLAYER_X_PIXELS), cpu.a);
        op_sep(0x20);
        uint8_t landed = rd8(dba(V_0001));
        a8_load(landed);
        wr8(dba(PLAYER_LANDED), landed);
    }
    jump_routine(PAD_FROM_SIDE);
}
