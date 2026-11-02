/*
 * player_landing.c: Aladdin coming down on top of an actor: he is stood on
 * it, his code goes on at his landing, and an actor of kind $11 starts at
 * his feet.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

void abi_ActorHitsTestBoxIfActive(void);
void abi_ScratchBelowBy10(void);
void abi_ActorAllocBLong(void);
void abi_PlayerStandOnActor(void);

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

    lda16_v(PLAYER_LANDED_CODE);
    ldx8(ea_abs(PLAYER_STATE_8C_COUNT));
    if (!IF_Z) {
        op_sep(0x30);
        pla8();                                /* PLA, PLA: its own return */
        pla8();
        back = pull16();                       /* RTS through its caller's */
        cpu.pc = 0x830000u | (uint16_t)(back + 1);
        cpu.unwound = 1;
        return;
    }
    sta16(ea_abs(PLAYER_CODE));
    sec();
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(rd16(ea_abs(SCRATCH + 4)));
    sta16(ea_abs(PLAYER_Y_PIXELS));
    ldx8_v(0x01);
    stx8(ea_abs(PLAYER_LANDED));
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
    lda8(ea_abs(V_1448));
    cmp8_v(0x3C);
    if (IF_Z) goto no;
    abi_ActorHitsTestBoxIfActive();
    if (IF_C) goto no;
    op_rep(0x20);
    abi_ScratchBelowBy10();
    if (IF_C) goto no;
    lda16(ea_abs(PLAYER_SPEED_Y_PIXELS));
    if (IF_N) goto no;
    ldx8(ea_abs(V_0367));
    if (IF_Z) goto no;
    ldx8(ea_abs(PLAYER_STATE_8C_COUNT));
    if (!IF_Z) goto no;
    ldx8(ea_abs(V_141C));
    if (IF_Z) goto no;
    ldx8(ea_abs(V_145A));
    if (IF_N) goto no;
    abi_PlayerStandOnActor();
    op_sep(0x30);
    abi_ActorSpawnKind11AtPlayer();
    lda8_v(0x01);
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
    if (!IF_N) {
        lda8_v(0x0C);
        sta8(ea_absx(ACTOR_STATE));
        lda8_v(KIND_11);
        sta8(ea_absx(ACTOR_06));
        stz8(ea_absx(ACTOR_07));
        lda8_v(0x80);
        sta8(ea_absx(ACTOR_08));
        op_rep(0x20);
        lda16(ea_abs(PLAYER_X_PIXELS));
        sta16(ea_absx(ACTOR_X + 1));
        lda16(ea_abs(PLAYER_Y_PIXELS));
        clc();
        adc16_v(0x0010);
        sta16(ea_absx(ACTOR_Y + 1));
    }
    op_sep(0x30);
}
