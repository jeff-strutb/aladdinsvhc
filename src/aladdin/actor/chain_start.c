/*
 * chain_start.c: the starts of the nine-piece chain's pieces (kinds $74
 * and $75), and the pattern its eighth piece runs for the whole chain: in
 * rounds, a sweep across at random speeds, or the head's animations with
 * the pieces' lists of children, by random picks per round.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"

void abi_ActorAnimBy40(void);
void abi_ActorListFindIdle(void);
void abi_ActorListStart(void);
void abi_ActorListStartAD(void);
void abi_ActorStartBy40(void);
void abi_ActorYieldForever(void);
void abi_RandomFromList(void);
void abi_SoundQueueAdd(void);
void abi_StepCountDown(void);
void abi_StepCountUp(void);

#define CHAIN_GO 0x1B18u        /* nonzero once the chain is built: the pieces start */
#define CHAIN_ROUNDS 0x1B1Au    /* counted up as the pattern starts */
#define CHAIN_PICK 0x1B1Bu      /* this phase's random pick (0 between phases) */
#define CHAIN_PHASE 0x1B1Cu     /* 0: the sweep; 1: the head's turn */
#define CHAIN_ROUND 0x1B1Du     /* 0-3, a step each two phases */
#define CHAIN_ROUND_OR 0x1B1Eu  /* ORed with the round for the pick lists */
#define HEAD_ANIM 0x1B20u       /* the head's animation (kind73.c's K73_ANIM) */
#define HEAD_CUE 0x1B22u        /* set when the head is done with its own code */
#define CHAIN_UNTOUCHABLE 0x1B2Cu /* nonzero: no touch tests */
#define HEAD_TURNS 0x1B33u      /* the head's own-code turns, four then the long one */
#define FIRST_SWEEPS 0x1B34u    /* sweeps left that take no random pick */
#define HEAD_POSE 0x1B35u       /* the head's touch pose (kind73.c's K73_POSE) */
#define CHAIN_HEAD 0x1B14u      /* the head's record (word) */
#define PIECE_NUMBER 0x40u      /* the piece's place in the chain (8: the one that runs the pattern) */
#define SWEEP_FRAMES 0x44u      /* frames left of a sweep, or of a pause */
#define SWEEP_SPEED 0x1Du       /* the sweep's speed entry; negative: still */
#define SWEEP_STEPS 0xD7B2u     /* (data bank) by SWEEP_SPEED: the x step (word) */
#define HEAD_OWN_CODE 0x8696u   /* where the head's code goes on for its own turn */
#define PIECE_PALETTE 0x7FE7E8u /* a kind $75 piece's look, from here (word), kept beside it */

/* ActorListStart with the list at $84:hi lo and the step a. */
static void list_start(uint8_t lo, uint8_t hi, uint8_t a)
{
    x8_load(lo);
    y8_load(hi);
    a8_load(a);
    abi_ActorListStart();
}

/* Each frame (BRK at brk_at) until ActorListFindIdle finds one idle. */
static void until_idle(uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        abi_ActorListFindIdle();
    } while (!(cpu.p & P_C));
}

static void pause(uint8_t n, uint32_t brk_at)
{
    a8_load(n);
    wr8(dpa(SWEEP_FRAMES), n);
    do
        actor_yield(brk_at);
    while (dec8_at(dpa(SWEEP_FRAMES)));
}

/* A random pick from list (CHAIN_ROUND | CHAIN_ROUND_OR) + base. */
static void pick(uint8_t base)
{
    flag_set(P_C, 0);
    a8_load(op_adc8((uint8_t)(rd8(dba(CHAIN_ROUND)) | rd8(dba(CHAIN_ROUND_OR))), base, &cpu.p));
    flag_set(P_C, cpu.a & 0x80);
    a8_load((uint8_t)(cpu.a << 1));
    x8_load((uint8_t)cpu.a);
    abi_RandomFromList();
}

/* Phase 0: the sweep across, SWEEP_FRAMES ($FF) frames at a random speed,
   kept between x $80 and $F0, stopped when Aladdin is to its right. */
static void sweep(void)
{
    uint8_t s, x;

    wr8(dba(HEAD_ANIM), 0);
    a8_load(rd8(dba(FIRST_SWEEPS)));
    if ((uint8_t)cpu.a) {
        (void)dec8_at(dba(FIRST_SWEEPS));
    } else {
        pick(0x0C);
        wr8(dba(CHAIN_PICK), (uint8_t)cpu.a);
    }
    flag_set(P_C, 0);
    a8_load(op_adc8(rd8(dba(CHAIN_ROUND)), 0x21, &cpu.p));
    flag_set(P_C, cpu.a & 0x80);
    a8_load((uint8_t)(cpu.a << 1));
    x8_load((uint8_t)cpu.a);
    abi_RandomFromList();
    wr8(dpa(SWEEP_SPEED), (uint8_t)cpu.a);
    a8_load(0xFF);
    wr8(dpa(SWEEP_FRAMES), 0xFF);
    do {
        actor_yield(0x84887Au);
        a8_load(rd8(dba((uint16_t)(PLAYER + ACTOR_X + 1))));
        cmp8_of((uint8_t)cpu.a, rd8(dpa(ACTOR_X + 1)));
        if (cpu.p & P_C)
            wr8(dpa(SWEEP_SPEED), 0);
        s = rd8(dpa(SWEEP_SPEED));
        x8_load(s);
        if (s & 0x80)
            continue;
        op_rep(0x20);
        flag_set(P_C, 0);
        a16_load(op_adc16(rd16(dba((uint16_t)(SWEEP_STEPS + s))), dp16(ACTOR_X), &cpu.p));
        dp16_set(ACTOR_X, cpu.a);
        op_sep(0x20);
        op_xba();
        x = (uint8_t)cpu.a;
        x8_load(x);
        a8_load(rd8(dpa(SWEEP_SPEED)));
        if (!(uint8_t)cpu.a) {
            cmp8_of(x, 0xF0);
            if (x >= 0xF0) {
                a8_load(0xF0);
                wr8(dpa(ACTOR_X + 1), 0xF0);
                wr8(dpa(ACTOR_X), 0);
                a8_load(0x02);
                wr8(dpa(SWEEP_SPEED), 0x02);
            }
        }
        cmp8_of(x, 0x80);
        if (x < 0x80) {                 /* $84:88AF, which no script meets */
            a8_load(0x80);
            wr8(dpa(ACTOR_X + 1), 0x80);
            wr8(dpa(ACTOR_X), 0);
            wr8(dpa(SWEEP_SPEED), 0);
        }
    } while (dec8_at(dpa(SWEEP_FRAMES)));
}

/* The head's long turn ($84:88D7): its animations 1 and 6 with the
   pieces' children, sound $49, and back. */
static void head_long_turn(void)
{
    wr8(dba(HEAD_TURNS), 0);
    a8_load(0x01);
    wr8(dba(HEAD_ANIM), 0x01);
    list_start(0xD5, 0x89, 0x01);
    a8_load(0x01);
    wr8(dpa(ACTOR_18), 0x01);
    do {
        actor_yield(0x8488ECu);
        abi_StepCountDown();
    } while (!(cpu.p & P_Z));
    until_idle(0x8488F3u);
    pause(0x08, 0x8488FEu);
    a8_load(0x06);
    wr8(dba(HEAD_ANIM), 0x06);
    list_start(0x2E, 0x8A, 0x10);
    a8_load(0x49);
    abi_SoundQueueAdd();
    a8_load(0x04);
    wr8(dba(HEAD_POSE), 0x04);
    do {
        actor_yield(0x84891Du);
        abi_StepCountUp();
    } while (!(cpu.p & P_Z));
    until_idle(0x848924u);
    a8_load(0x06);
    wr8(dba(HEAD_ANIM), 0x06);
    pause(0x3F, 0x848934u);
    a8_load(0x00);
    wr8(dba(HEAD_ANIM), 0x00);
    wr8(dba(HEAD_POSE), 0);
    list_start(0xD1, 0x89, 0x02);
    until_idle(0x84894Bu);
    abi_ActorListStartAD();
}

/* Phase 1: by a random pick, the head's long turn, or (three times in
   four) the head's own code ($84:8696) until it sets HEAD_CUE. */
static void head_turn(void)
{
    uint16_t head;

    pick(0x14);
    x8_load((uint8_t)cpu.a);
    if ((uint8_t)cpu.x != 0x02) {       /* the table at $84:88D1: 0 and 4 the long turn */
        head_long_turn();
        return;
    }
    a8_load(inc8_at(dba(HEAD_TURNS)));
    cmp8_of((uint8_t)cpu.a, 0x04);
    if ((uint8_t)cpu.a == 0x04) {       /* $84:8958, which no script meets */
        head_long_turn();
        return;
    }
    wr8(dba(HEAD_CUE), 0);
    op_rep(0x30);
    head = rd16(dba(CHAIN_HEAD));
    cpu.x = head;
    nz16_of(head);
    a16_load(HEAD_OWN_CODE);
    wr16(dba((uint16_t)(head + ACTOR_03)), HEAD_OWN_CODE);
    wr16(dba((uint16_t)(head + ACTOR_35)), HEAD_OWN_CODE);
    op_sep(0x30);
    do {
        actor_yield(0x848978u);
        a8_load(rd8(dba(HEAD_CUE)));
    } while (!(uint8_t)cpu.a);
}

/* $84:87EC: the eighth piece runs the pattern; the others stay. */
static void piece_on(void)
{
    uint8_t phase;

    actor_yield(0x8487ECu);
    a8_load(rd8(dpa(PIECE_NUMBER)));
    cmp8_of((uint8_t)cpu.a, 0x08);
    if ((uint8_t)cpu.a != 0x08) {
        abi_ActorYieldForever();
        return;
    }
    wr8(dba(CHAIN_PHASE), 0);
    wr8(dba(CHAIN_ROUND), 0);
    a8_load(0x02);
    wr8(dba(FIRST_SWEEPS), 0x02);
    list_start(0xA0, 0x89, 0x10);
    until_idle(0x84880Bu);
    wr8(dba(CHAIN_UNTOUCHABLE), 0);
    abi_ActorListStartAD();
    (void)inc8_at(dba(CHAIN_ROUNDS));
    wr8(dba(CHAIN_PICK), 0);
    for (;;) {
        phase = rd8(dba(CHAIN_PHASE));
        a8_load((uint8_t)(phase << 1));
        x8_load((uint8_t)(phase << 1));
        if (phase == 0)
            sweep();
        else
            head_turn();
        /* $84:8826: the next phase; each two, the next round (0-3) */
        (void)inc8_at(dba(CHAIN_PHASE));
        wr8(dba(CHAIN_PICK), 0);
        a8_load((uint8_t)(rd8(dba(CHAIN_PHASE)) & 0x01));
        wr8(dba(CHAIN_PHASE), (uint8_t)cpu.a);
        if ((uint8_t)cpu.a)
            continue;
        a8_load(inc8_at(dba(CHAIN_ROUND)));
        cmp8_of((uint8_t)cpu.a, 0x04);
        if ((uint8_t)cpu.a == 0x04)     /* $84:8840, which no script meets */
            wr8(dba(CHAIN_ROUND), 0);
    }
}

/* Each frame until the chain is built, then started and animated by its
   number. */
static void piece_wait(uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        a8_load(rd8(dba(CHAIN_GO)));
    } while (!(uint8_t)cpu.a);
    abi_ActorStartBy40();
    abi_ActorAnimBy40();
}

/*
 * @routine 84:87C2 Kind74Start
 * @actor
 * WHAT IT DOES: Actor kind $74, one of the five rear pieces of the
 * nine-piece chain: once the chain is built it starts and animates by its
 * number, and the eighth piece runs the chain's pattern for good: rounds
 * of a sweep across at random speeds and the head's turn (its long turn
 * with the pieces' children, or the head's own code at $84:8696), by
 * random picks from lists by the round; the others stay.
 * HOW: Each frame (BRK) until CHAIN_GO; ActorStartBy40, ActorAnimBy40;
 * then (at $84:87EC, in Kind75Start's code) BRK; PIECE_NUMBER 8:
 * CHAIN_PHASE and CHAIN_ROUND 0, FIRST_SWEEPS 2, ActorListStart $89A0
 * with $10, BRK until ActorListFindIdle, CHAIN_UNTOUCHABLE 0,
 * ActorListStartAD, CHAIN_ROUNDS up; then through the table at $84:8845
 * by CHAIN_PHASE: the sweep (SWEEP_SPEED from RandomFromList, $FF frames
 * of x + SWEEP_STEPS, held from $80 to $F0) or the head's turn (the
 * table at $84:88D1 by a pick: the long turn, or HEAD_OWN_CODE written
 * as the head's resume address and BRK until HEAD_CUE, four times before
 * the long turn); the phase flipped, each two the round up (0-3). Not
 * the eighth: ActorYieldForever.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds the tables (bank $84); reached by
 * the actor loop's jump for kind $74 in state $0C (the table at $83:8CAB,
 * through the stub at $83:9267). Exit: never returns.
 */
void abi_Kind74Start(void)
{
    piece_wait(0x8487C2u);
    piece_on();
}

/*
 * @routine 84:87D1 Kind75Start
 * @actor
 * WHAT IT DOES: Actor kind $75, one of the three front pieces of the
 * nine-piece chain: as Kind74Start, with its look taken from
 * PIECE_PALETTE (and kept beside it) once it starts; its code holds the
 * pattern the eighth piece runs ($84:87EC).
 * HOW: Each frame (BRK) until CHAIN_GO; ActorStartBy40, ActorAnimBy40;
 * ACTOR_LOOK and PIECE_PALETTE + 2 = PIECE_PALETTE (16-bit); then on as
 * Kind74Start.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds the tables (bank $84); reached by
 * the actor loop's jump for kind $75 in state $0C (the table at $83:8CAB,
 * through the stub at $83:9274). Exit: never returns.
 */
void abi_Kind75Start(void)
{
    piece_wait(0x8487D1u);
    op_rep(0x20);
    a16_load(rd16(PIECE_PALETTE));
    dp16_set(ACTOR_LOOK, cpu.a);
    wr16(PIECE_PALETTE + 2, cpu.a);
    op_sep(0x20);
    piece_on();
}
