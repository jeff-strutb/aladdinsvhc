/*
 * kind7d_wait.c: actor kind $7D's wait: a spot that takes hold of the
 * player. Each frame it follows its kept height, steps its state (take
 * hold, keep him there or put him back, a pause), and ends once it is far
 * from the point actors are kept in range of.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"

void abi_ActorGrabbableByPlayerLong(void);
void abi_GroundInFrontLong(void);
void abi_Kind7DStep(void);
void abi_Kind7DWait(void);

#define K7D_Y_BASE 0x40u        /* its height before the scroll (word, kept from its start) */
#define K7D_SCROLL_Y 0x1A2Cu    /* subtracted from it for its y (word) */
#define K7D_STATE 0x13u         /* its state: the entry of Kind7DStep's table */
#define K7D_PROBE_SIDE 0x18u    /* GroundInFrontLong's side: 0, then 4 */
#define K7D_PAUSE 0x42u         /* frames left in state 2 */
#define K7D_RANGE_X 0x1A44u     /* the x of the point actors are kept in range of (word) */
#define K7D_PUT_BACK 0x1458u    /* counted up each time the player is put back */
#define K7D_KEEP 0x1427u        /* nonzero: the player is kept at the spot */
#define K7D_HELD_CODE 0xD6F8u   /* the player's code word while held */
#define K7D_STATES 0x9127u      /* Kind7DStep's table (bank $84) */
#define PLAYER_X_BEFORE 0x143Bu /* the player's x before this frame's move (word) */
#define PLAYER_Y_BEFORE 0x143Eu /* his y before it (word) */
#define PLAYER_RESUME (PLAYER + ACTOR_03) /* the player's resume address (word) */
#define ACTOR_END_PLACED 0x8396CAu

/*
 * @routine 83:929F ActorKind7DWait
 * WHAT IT DOES: The routine actors of kind $7D run each frame they wait
 * (not the last), when bit 7 of their byte at $09 is set: Kind7DWait.
 * HOW: JSL Kind7DWait ($84:90F4), then RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
 * (its low 7 bits); called by JSR through the actor loop's table by kind.
 * Exit: as Kind7DWait leaves them (it may end the actor: never returns).
 */
void abi_ActorKind7DWait(void)
{
    /* the JSL's return address, as Kind7DWait finds it: it drops it (and
       the loop's) when it ends the actor */
    jsl_frame(0x83929Fu);
    abi_Kind7DWait();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 84:90F4 Kind7DWait
 * WHAT IT DOES: What actors of kind $7D do each frame they wait: follow
 * their kept height as the view scrolls, take a step of their state, and
 * end once more than 224 pixels from the point actors are kept in range
 * of.
 * HOW: 16-bit: its y = K7D_Y_BASE - K7D_SCROLL_Y; Kind7DStep (JSR).
 * 16-bit: its x - K7D_RANGE_X + $E0 below $1C0 (within 224 either way):
 * RTL. Else its JSL's and the loop's JSR return addresses dropped (five
 * pulls) and on into ActorEndPlaced at the loop's level.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL from the kind's stub ($83:929F)
 * through the actor loop's wait table at $83:8F4F. Exit: M 8-bit; in
 * range: C clear, N and Z from the difference; else never returns.
 */
void abi_Kind7DWait(void)
{
    uint16_t d;
    uint8_t got = 0;

    op_rep(0x20);
    flag_set(P_C, 1);
    d = dp16(K7D_Y_BASE);
    a16_load(op_sbc16(d, rd16(dba(K7D_SCROLL_Y)), &cpu.p));
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(0x20);
    call_frame(0x9102, abi_Kind7DStep);
    op_rep(0x20);
    flag_set(P_C, 1);
    d = dp16(ACTOR_X + 1);
    d = op_sbc16(d, rd16(dba(K7D_RANGE_X)), &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, 0x00E0, &cpu.p);
    a16_load(d);
    cmp16_of(d, 0x01C0);
    op_sep(0x20);
    if (d < 0x01C0)
        return;
    /* out of range: out of its call and the loop's, ended */
    for (int i = 0; i < 5; i++)
        got = op_pull8();
    a8_load(got);
    actor_leave(ACTOR_END_PLACED);
}

/* GroundInFrontLong on one side (K7D_PROBE_SIDE): C set when ground is
   in front of it. */
static int ground_in_front(uint8_t side)
{
    if (side) {
        a8_load(side);
        wr8(dpa(K7D_PROBE_SIDE), side);
    } else
        wr8(dpa(K7D_PROBE_SIDE), 0x00);
    abi_GroundInFrontLong();
    return (cpu.p & P_C) != 0;
}

/* State 0: once the player can take hold of it and no ground is in front
   of it either side, he is held (his code word and the spot kept). */
static void take_hold(void)
{
    abi_ActorGrabbableByPlayerLong();
    if (cpu.p & P_C)
        return;
    if (ground_in_front(0x00) || ground_in_front(0x04))
        return;
    op_rep(0x20);
    a16_load(K7D_HELD_CODE);
    wr16(dba(PLAYER_RESUME), K7D_HELD_CODE);
    a16_load(dp16(ACTOR_X + 1));
    wr16(dba(PLAYER_POS_SAVED), cpu.a);
    a16_load(dp16(ACTOR_Y + 1));
    wr16(dba((uint16_t)(PLAYER_POS_SAVED + 2)), cpu.a);
    op_sep(0x20);
    (void)inc8_at(dpa(K7D_STATE));
}

/* State 1: ground in front of it either side puts the player back where
   he was; else, kept, he is held at its place (and it stays in state 1). */
static void hold(void)
{
    if (ground_in_front(0x00) || ground_in_front(0x04)) {
        op_rep(0x20);
        a16_load(rd16(dba(PLAYER_X_BEFORE)));
        wr16(dba(PLAYER + ACTOR_X + 1), cpu.a);
        a16_load(rd16(dba(PLAYER_Y_BEFORE)));
        wr16(dba(PLAYER + ACTOR_Y + 1), cpu.a);
        op_sep(0x20);
        (void)inc8_at(dba(K7D_PUT_BACK));
    } else {
        a8_load(rd8(dba(K7D_KEEP)));
        if ((uint8_t)cpu.a) {
            op_rep(0x20);
            a16_load(dp16(ACTOR_X + 1));
            wr16(dba(PLAYER + ACTOR_X + 1), cpu.a);
            a16_load(dp16(ACTOR_Y + 1));
            wr16(dba(PLAYER + ACTOR_Y + 1), cpu.a);
            op_sep(0x20);
            return;
        }
    }
    (void)inc8_at(dpa(K7D_STATE));
    a8_load(0x10);
    wr8(dpa(K7D_PAUSE), 0x10);
}

/*
 * @routine 84:9120 Kind7DStep
 * WHAT IT DOES: Kind $7D's state step: 0 waits for the player to step
 * onto it and takes hold of him; 1 keeps him there, or puts him back
 * where he was when ground is in front of it; 2 a 16-frame pause, then 0
 * again.
 * HOW: JMP through the table at $84:9127 by K7D_STATE. 0 ($84:912D):
 * ActorGrabbableByPlayerLong C set, or GroundInFrontLong on either side
 * (K7D_PROBE_SIDE 0, then 4) C set: RTS. Else 16-bit: the player's code
 * word K7D_HELD_CODE, PLAYER_POS_SAVED = its x and y; K7D_STATE up. 1
 * ($84:915C): GroundInFrontLong on either side C set: the player back at
 * his x and y before this frame's move, K7D_PUT_BACK up, then to state 2.
 * Else K7D_KEEP set: the player at its x and y, RTS; clear: to state 2
 * (K7D_STATE up, K7D_PAUSE $10). 2 ($84:919E): K7D_PAUSE down; at 0,
 * K7D_STATE 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR from Kind7DWait. Exit: M 8-bit.
 */
void abi_Kind7DStep(void)
{
    uint8_t state = rd8(dpa(K7D_STATE));

    a8_load((uint8_t)(state << 1));             /* ASL */
    flag_set(P_C, state & 0x80);
    x8_load((uint8_t)cpu.a);                    /* TAX */
    switch (state & 0x7F) {
    case 0:
        take_hold();
        return;
    case 1:
        hold();
        return;
    case 2:
        if (!dec8_at(dpa(K7D_PAUSE)))
            wr8(dpa(K7D_STATE), 0x00);
        return;
    default:                                    /* past the table's 3 words: no state names one */
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(K7D_STATES + cpu.x)));
        return;
    }
}
