/*
 * player_grab.c: whether Aladdin is close enough to an actor to take hold
 * of it.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

#define PLAYER_X_PIXELS (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXELS (PLAYER + ACTOR_Y + 1)
#define PLAYER_FLIP (PLAYER + ACTOR_FLIP)
#define PLAYER_SPEED_Y_WHOLE (PLAYER + ACTOR_SPEED_Y + 1) /* negative: he is going up (word) */
#define V_1427 0x001427u            /* nonzero: he can take hold whatever his state */
#define PLAYER_STATE_8C_COUNT 0x00141Bu /* counted up on entering state $8C */
#define V_1448 0x001448u            /* 7 to $0F: no taking hold */
#define V_141C 0x00141Cu            /* zero: no taking hold */
#define HOLDS 0x21u                 /* ActorHoldPlayer: nonzero while it holds him */
#define HOLD_PAUSE 0x4Fu            /* ActorHoldPlayer: frames before it can hold him again */
#define HOLD_AT 0x13u               /* ActorHoldPlayer: its offset for him (PlayerToActorAbove's A) */
#define HELD_RESUME 0xD99Bu         /* where his code resumes while he is held */

/*
 * @routine 83:882E ActorGrabbableByPlayer
 * WHAT IT DOES: Tells whether Aladdin can take hold of the actor: he is
 * close in front of it and either $1427 is set, or no state $8C is
 * counted, $1448 is not 7 to $0F, $141C is nonzero and he is not rising
 * faster than 2 pixels a frame.
 * HOW: PlayerFacesActorClose; then the tests in that order, C clear for
 * yes (CLC) and set for no (SEP #$21).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: M 8-bit; C clear for yes; A, X and the
 * other flags from the last test.
 */
void abi_PlayerFacesActorClose(void);
void abi_ActorGrabTest(void);
void abi_PlayerToActorAbove(void);
void abi_PlayerSpotAboveActor(void);
void abi_ActorHoldPlayer(void);

void abi_ActorGrabbableByPlayer(void)
{
    uint8_t v;
    uint16_t rise;

    abi_PlayerFacesActorClose();
    if (cpu.p & P_C)
        goto no;
    v = rd8(dba(V_1427));
    a8_load(v);
    if (v)
        goto yes;
    v = rd8(dba(PLAYER_STATE_8C_COUNT));
    a8_load(v);
    if (v)
        goto no;
    v = rd8(dba(V_1448));
    x8_load(v);
    cmp8_of(v, 0x07);
    if (v >= 0x07) {
        cmp8_of(v, 0x10);
        if (v < 0x10)
            goto no;
    }
    v = rd8(dba(V_141C));
    a8_load(v);
    if (!v)
        goto no;
    op_rep(P_M);
    rise = rd16(dba(PLAYER_SPEED_Y_WHOLE));
    a16_load(rise);
    if (!(rise & 0x8000))
        goto yes;
    cmp16_of(rise, 0xFFFE);               /* rising faster than 2 pixels a frame */
    if (rise < 0xFFFE)
        goto no;
yes:
    flag_set(P_C, 0);
    op_sep(P_M);
    return;
no:
    op_sep(0x21);
}

/*
 * @routine 83:8861 ActorGrabbableByPlayerLong
 * WHAT IT DOES: ActorGrabbableByPlayer for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorGrabbableByPlayer; called by JSL. Exit: as
 * ActorGrabbableByPlayer.
 */
void abi_ActorGrabbableByPlayerLong(void)
{
    abi_ActorGrabbableByPlayer();
}

/*
 * @routine 83:888C PlayerFacesActorClose
 * WHAT IT DOES: Tells whether Aladdin is close in front of the actor: his
 * x plus 22 (8 when he faces left) less the actor's below 30, and his y
 * plus 3 less the actor's below 26, unsigned.
 * HOW: Two subtractions and unsigned compares on the pixel words.
 * Entry: D = the actor; DB a bank whose $0000-$1FFF is work RAM; called by
 * JSL. Exit: C clear when he is; A = the last difference (16 bits); N,
 * Z from the compare that decided, V from the last subtraction; M 8-bit;
 * X = his flip byte (low byte, X 8-bit), Y unchanged.
 */
void abi_PlayerFacesActorClose(void)
{
    uint8_t flip = rd8(dba(PLAYER_FLIP));
    uint16_t d;

    op_rep(0x21);
    a16_load(0x0016);
    x8_load(flip);
    if (flip)
        a16_load(0x0008);
    /* across: his reach point (16 or 8 pixels ahead) within 30 of it */
    d = op_adc16((uint16_t)cpu.a, rd16(dba(PLAYER_X_PIXELS)), &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, dp16(ACTOR_X + 1), &cpu.p);
    cpu.a = d;
    cmp16_of(d, 0x001E);
    if (d < 0x001E) {
        /* down: his y plus 3 within 26 of it */
        d = op_adc16(rd16(dba(PLAYER_Y_PIXELS)), 0x0003, &cpu.p);
        flag_set(P_C, 1);
        d = op_sbc16(d, dp16(ACTOR_Y + 1), &cpu.p);
        cpu.a = d;
        cmp16_of(d, 0x001A);
    }
    op_sep(P_M);
}

/*
 * @routine 83:BAEF ActorHoldPlayerLong
 * WHAT IT DOES: ActorHoldPlayer for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorHoldPlayer; called by JSL. Exit: as ActorHoldPlayer.
 */
void abi_ActorHoldPlayerLong(void)
{
    abi_ActorHoldPlayer();
}

/*
 * @routine 83:BAF3 ActorHoldPlayer
 * WHAT IT DOES: A frame of an actor that takes hold of the player: when
 * ActorGrabTest finds him it holds him (HOLDS set) at its offset HOLD_AT;
 * while it holds him he stays there until $1427 is cleared, which lets
 * him go with a pause of 8 frames before it can hold him again.
 * HOW: HOLDS set: V_1427 clear, HOLDS = 0 and HOLD_PAUSE = 8; else
 * PlayerToActorAbove with A = HOLD_AT. HOLDS clear: HOLD_PAUSE nonzero,
 * one less; else ActorGrabTest; C clear: his resume address ($03-$04) =
 * HELD_RESUME, PlayerSpotAboveActor with A = HOLD_AT, HOLDS up, C clear;
 * else HOLDS = 0. The scripts run only that last path (HOLDS and
 * HOLD_PAUSE clear, the test failing).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit. Taken: C clear, HOLDS 1. Held: as
 * PlayerToActorAbove. Let go: A = 8. Else HOLDS 0, the rest as the pause
 * count or ActorGrabTest leaves them.
 */
void abi_ActorHoldPlayer(void)
{
    lda8(ea_dp(HOLDS));  /* 83BAF3 lda */
    if (IF_Z) goto L_83BB0A;  /* 83BAF5 beq */
    lda8(ea_abs(V_1427));  /* 83BAF7 lda */
    if (!IF_Z) goto L_83BB03;  /* 83BAFA bne */
    stz8(ea_dp(HOLDS));  /* 83BAFC stz */
    lda8_v(0x08);  /* 83BAFE lda */
    sta8(ea_dp(HOLD_PAUSE));  /* 83BB00 sta */
    return;  /* 83BB02 rts */
L_83BB03:
    lda8(ea_dp(HOLD_AT));  /* 83BB03 lda */
    abi_PlayerToActorAbove();  /* 83BB05 jsl */
    return;  /* 83BB09 rts */
L_83BB0A:
    lda8(ea_dp(HOLD_PAUSE));  /* 83BB0A lda */
    if (IF_Z) goto L_83BB12;  /* 83BB0C beq */
    MEM8(dec, ea_dp(HOLD_PAUSE));  /* 83BB0E dec */
    goto L_83BB2C;  /* 83BB10 bra */
L_83BB12:
    abi_ActorGrabTest();  /* 83BB12 jsl */
    if (IF_C) goto L_83BB2C;  /* 83BB16 bcs */
    lda8_v((uint8_t)HELD_RESUME);  /* 83BB18 lda */
    sta8(ea_abs(PLAYER + ACTOR_03));  /* 83BB1A sta */
    lda8_v((uint8_t)(HELD_RESUME >> 8));  /* 83BB1D lda */
    sta8(ea_abs(PLAYER + ACTOR_04));  /* 83BB1F sta */
    lda8(ea_dp(HOLD_AT));  /* 83BB22 lda */
    abi_PlayerSpotAboveActor();  /* 83BB24 jsl */
    MEM8(inc, ea_dp(HOLDS));  /* 83BB28 inc */
    clc();  /* 83BB2A clc */
    return;  /* 83BB2B rts */
L_83BB2C:
    stz8(ea_dp(HOLDS));  /* 83BB2C stz */
}
