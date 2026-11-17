/*
 * guard_states.c: the bank $A5 guard's (actor kind $08) wait: each frame
 * its place kept for others, its animation and picture, then the step of
 * the state it is in: calm or chasing, facing the view, idle, holding back
 * while the player is to one side, or running off to the right.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"

void abi_ActorPictureSend(void);
void abi_AnimStep(void);
void abi_GuardChase(void);
void guard_chase_from(uint16_t at);
void abi_GuardFaceView(void);
void abi_GuardHoldLeft(void);
void abi_GuardHoldRight(void);
void abi_GuardIdle(void);
void abi_GuardRunOff(void);

#define GUARD_STATE 0x4Du       /* its state: the entry of GuardWait's table */
#define GUARD_HELD 0x41u        /* nonzero: it does not chase */
#define GUARD_GONE 0x45u        /* counted up when it runs off for good */
#define GUARD_CODE 0x03u        /* where its code goes on (word) */
#define GUARD_CODE_SET 0x146Bu  /* which code it was last sent to: bit 7 set, it chases at once */
#define GUARD_X_KEPT 0x1471u    /* its x, kept for others (word) */
#define GUARD_Y_KEPT 0x1473u    /* its y, kept for others (word) */
#define RANGE_X 0x1A44u         /* the x of the point actors are kept in range of (word) */
#define STATES 0xF03Du          /* GuardWait's table (bank $A5) */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)
#define CODE_CHASE 0xE812u      /* its code for a chase */
#define CODE_WALK 0xE807u       /* its code once the player has no health left */

#define CHASE_SPEED_UP 0xF127u  /* GuardChase's speed-up step */
#define CHASE_CLOSE 0xF172u     /* GuardChase's close step */

/* GUARD_CODE_SET's bits 0-6 cleared (the guard held). */
static void held(void)
{
    a8_load(0x7F);
    trb_at(dba(GUARD_CODE_SET), 0x7F);
}

/* GuardWait's state 1: held, or sent to its chase, or its walk once the
   player has no health, or a chase step. */
static void calm(void)
{
    uint8_t set;

    a8_load(rd8(dpa(GUARD_HELD)));
    if ((uint8_t)cpu.a) {
        held();
        return;
    }
    set = rd8(dba(GUARD_CODE_SET));
    a8_load(set);
    if (set & 0x80) {
        op_rep(0x20);
        a16_load(CODE_CHASE);
        dp16_set(GUARD_CODE, CODE_CHASE);
    } else if (a8_load(rd8(dba(HEALTH))), !(uint8_t)cpu.a) {
        op_rep(0x20);
        a16_load(CODE_WALK);
        dp16_set(GUARD_CODE, CODE_WALK);
    } else {
        op_rep(0x20);
        push16(0xF074);                          /* JSR GuardChase */
        abi_GuardChase();
        cpu.s = (uint16_t)(cpu.s + 2);
    }
    op_sep(0x30);
}

/*
 * @routine A5:F020 GuardWait
 * WHAT IT DOES: What the guard runs each frame it waits: its place kept
 * for others, its animation and picture sent; then by its state: 0
 * nothing; 1 calm or chasing; 2 facing the view; 3 nothing; 4 and 5
 * holding back while the player is to its left or right; 6 running off
 * to the right.
 * HOW: GUARD_X_KEPT, GUARD_Y_KEPT = its x, y; AnimStep, ActorPictureSend;
 * JMP through the table at $A5:F03D by GUARD_STATE: 0 RTL ($A5:F04B); 1
 * ($A5:F04C): GUARD_HELD set: GUARD_CODE_SET bits 0-6 cleared; else
 * GUARD_CODE_SET bit 7 set: code word CODE_CHASE; else no health: code
 * word CODE_WALK; else GuardChase; 2 GuardFaceView; 3 GuardIdle; 4
 * GuardHoldLeft; 5 GuardHoldRight; 6 GuardRunOff. States 4, 5 and 6 and 3
 * no script meets: written from their bytes.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the actor loop's stub for
 * kind $08 ($83:9209). Exit: M 8-bit, X 8-bit (state 1); else as the
 * state's step leaves them.
 */
void abi_GuardWait(void)
{
    uint8_t state;

    op_rep(0x20);
    a16_load(dp16(ACTOR_X + 1));
    wr16(dba(GUARD_X_KEPT), cpu.a);
    a16_load(dp16(ACTOR_Y + 1));
    wr16(dba(GUARD_Y_KEPT), cpu.a);
    op_sep(0x20);
    abi_AnimStep();
    abi_ActorPictureSend();
    state = rd8(dpa(GUARD_STATE));
    a8_load((uint8_t)(state << 1));             /* ASL */
    flag_set(P_C, state & 0x80);
    x8_load((uint8_t)cpu.a);                    /* TAX */
    switch (state & 0x7F) {
    case 0:
        return;
    case 1:
        calm();
        return;
    case 2:
        abi_GuardFaceView();
        return;
    case 3:
        abi_GuardIdle();
        return;
    case 4:
        abi_GuardHoldLeft();
        return;
    case 5:
        abi_GuardHoldRight();
        return;
    case 6:
        abi_GuardRunOff();
        return;
    default:                                     /* past the table's 7 words: no state names one */
        jump_routine(0xA50000u | rd16(0xA50000u | (uint16_t)(STATES + cpu.x)));
        return;
    }
}

/* GuardHoldLeft and GuardHoldRight: with the player on that side, it stops
   when it faces him (or away, for the right), and keeps pace unless held;
   else it goes calm. */
static void hold(int right)
{
    uint16_t px = rd16(dba(PLAYER_X_AT)), x = dp16(ACTOR_X + 1);
    int level;

    op_rep(0x20);
    if (right) {
        a16_load(x);
        cmp16_of(x, px);
        level = x >= px;
    } else {
        a16_load(px);
        cmp16_of(px, x);
        level = px >= x;
    }
    if (level) {
        op_sep(0x20);
        a8_load(0x01);
        wr8(dpa(GUARD_STATE), 0x01);
        calm();
        return;
    }
    a16_load(dp16(ACTOR_FACING));
    if (right ? !cpu.a : cpu.a) {
        dp16_set(ACTOR_SPEED_X, 0);
        dp16_set(ACTOR_SPEED_X + 1, 0);
        dp16_set(ACTOR_SPEED_X + 2, 0);
    }
    a16_load(dp16(GUARD_HELD));
    if (!cpu.a)
        guard_chase_from(CHASE_CLOSE);
    op_sep(0x20);
}

/*
 * @routine A5:F07A GuardHoldLeft
 * WHAT IT DOES: The guard's state 4: while the player is to its left, it
 * stops if it faces left and, unless held, keeps pace with him; once he
 * is level with it or to its right, it goes calm (state 1) at once.
 * HOW: 16-bit: the player's x at or past its own: GUARD_STATE 1 and on as
 * GuardWait's state 1 ($A5:F04C). Else its facing and flip (word)
 * nonzero: speed 0 (STZ of three words from ACTOR_SPEED_X); GUARD_HELD
 * and the byte after it (word) zero: GuardChase from its close step
 * ($A5:F172).
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
 */
void abi_GuardHoldLeft(void)
{
    hold(0);
}

/*
 * @routine A5:F09F GuardHoldRight
 * WHAT IT DOES: The guard's state 5: as GuardHoldLeft with the sides
 * swapped: while the player is to its right, it stops if it faces right
 * and, unless held, keeps pace with him; once he is level with it or to
 * its left, it goes calm (state 1) at once.
 * HOW: 16-bit: its x at or past the player's: GUARD_STATE 1 and on as
 * GuardWait's state 1. Else its facing and flip (word) zero: speed 0;
 * GUARD_HELD (word) zero: GuardChase from its close step ($A5:F172).
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
 */
void abi_GuardHoldRight(void)
{
    hold(1);
}

/*
 * @routine A5:F0C4 GuardRunOff
 * WHAT IT DOES: The guard's state 6: unless held, it runs off to the
 * right at speed, and once it is more than 192 pixels right of the
 * player it is gone: counted, state 0, and moved far off.
 * HOW: GUARD_HELD set: GUARD_CODE_SET bits 0-6 cleared. Else 16-bit: the
 * player's x + $C0 below its x: GUARD_GONE up, GUARD_STATE 0, the high
 * bytes of its x and y $3F. Else GuardChase from its speed-up step
 * ($A5:F127, which leaves M 8-bit); ACTOR_SPEED_X's middle byte 5;
 * facing and flip 0.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
 */
void abi_GuardRunOff(void)
{
    uint16_t reach;

    a8_load(rd8(dpa(GUARD_HELD)));
    if ((uint8_t)cpu.a) {
        held();
        return;
    }
    op_rep(0x20);
    flag_set(P_C, 0);
    reach = op_adc16(rd16(dba(PLAYER_X_AT)), 0x00C0, &cpu.p);
    a16_load(reach);
    cmp16_of(reach, dp16(ACTOR_X + 1));
    if (reach < dp16(ACTOR_X + 1)) {
        op_sep(0x20);
        (void)inc8_at(dpa(GUARD_GONE));
        wr8(dpa(GUARD_STATE), 0x00);
        a8_load(0x3F);
        wr8(dpa(ACTOR_X + 2), 0x3F);
        wr8(dpa(ACTOR_Y + 2), 0x3F);
        return;
    }
    guard_chase_from(CHASE_SPEED_UP);           /* leaves M 8-bit */
    a8_load(0x05);
    wr8(dpa(ACTOR_SPEED_X + 1), 0x05);
    wr8(dpa(ACTOR_FACING), 0x00);
    wr8(dpa(ACTOR_FLIP), 0x00);
}

/*
 * @routine A5:F2E5 GuardFaceView
 * WHAT IT DOES: The guard's state 2: it faces toward the point actors are
 * kept in range of: left when it is more than 96 pixels right of it,
 * right when it is more than 96 pixels left of it.
 * HOW: 16-bit: RANGE_X + $60 less its x: a borrow: facing and flip 1;
 * else 8-bit, $C0 or more: facing and flip 0.
 * Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
 */
void abi_GuardFaceView(void)
{
    uint16_t d;

    op_rep(0x20);
    flag_set(P_C, 0);
    d = op_adc16(rd16(dba(RANGE_X)), 0x0060, &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, dp16(ACTOR_X + 1), &cpu.p);
    a16_load(d);
    if (!(cpu.p & P_C)) {
        op_sep(0x20);
        a8_load(0x01);
        wr8(dpa(ACTOR_FACING), 0x01);
        wr8(dpa(ACTOR_FLIP), 0x01);
        return;
    }
    cmp16_of(d, 0x00C0);
    op_sep(0x20);
    if (d >= 0x00C0) {
        wr8(dpa(ACTOR_FACING), 0x00);
        wr8(dpa(ACTOR_FLIP), 0x00);
    }
}

/*
 * @routine A5:F308 GuardIdle
 * WHAT IT DOES: The guard's state 3: nothing (no script meets it).
 * HOW: RTL.
 * Entry: any; reached by GuardWait's JMP. Exit: as entered.
 */
void abi_GuardIdle(void)
{
}
