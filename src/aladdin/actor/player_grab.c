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
