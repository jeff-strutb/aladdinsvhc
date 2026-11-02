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
