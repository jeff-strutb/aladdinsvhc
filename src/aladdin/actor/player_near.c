/*
 * player_near.c: tests of where Aladdin stands against an actor: just in
 * front of it, and close enough to take hold of it.
 */
#include "machine.h"
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
    abi_PlayerFacesActorClose();
    if (IF_C) goto no;
    lda8(ea_abs(V_1427));
    if (!IF_Z) goto yes;
    lda8(ea_abs(PLAYER_STATE_8C_COUNT));
    if (!IF_Z) goto no;
    ldx8(ea_abs(V_1448));
    cpx8_v(0x07);
    if (IF_C) {
        cpx8_v(0x10);
        if (!IF_C) goto no;
    }
    lda8(ea_abs(V_141C));
    if (IF_Z) goto no;
    op_rep(0x20);
    lda16(ea_abs(PLAYER_SPEED_Y_WHOLE));
    if (!IF_N) goto yes;
    cmp16_v(0xFFFE);
    if (!IF_C) goto no;
yes:
    clc();
    op_sep(0x20);
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
 * @routine 83:8865 PlayerFacesActorNear
 * WHAT IT DOES: Tells whether Aladdin stands just in front of the actor:
 * his x plus 18 (8 when he faces left) less the actor's below 26, and his
 * y plus 2 less the actor's below 31, unsigned.
 * HOW: Two subtractions and unsigned compares on the pixel words.
 * Entry: D = the actor; DB a bank whose $0000-$1FFF is work RAM; called by
 * JSL. Exit: C clear when he does; A = the last difference (16 bits); N,
 * Z from the compare that decided, V from the last subtraction; M 8-bit;
 * X = his flip byte (low byte, X 8-bit), Y unchanged.
 */
void abi_PlayerFacesActorNear(void)
{
    op_rep(0x21);
    lda16_v(0x0012);
    ldx8(ea_abs(PLAYER_FLIP));
    if (!IF_Z)
        lda16_v(0x0008);
    adc16_v(rd16(ea_abs(PLAYER_X_PIXELS)));
    sec();
    sbc16_v(rd16(ea_dp(ACTOR_X + 1)));
    cmp16_v(0x001A);
    if (!IF_C) {
        lda16(ea_abs(PLAYER_Y_PIXELS));
        adc16_v(0x0002);
        sec();
        sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));
        cmp16_v(0x001F);
    }
    op_sep(0x20);
}

/*
 * @routine 83:888C PlayerFacesActorClose
 * WHAT IT DOES: Tells whether Aladdin is close in front of the actor: his
 * x plus 22 (8 when he faces left) less the actor's below 30, and his y
 * plus 3 less the actor's below 26, unsigned.
 * HOW: As PlayerFacesActorNear, with the other offsets and sizes.
 * Entry: as PlayerFacesActorNear. Exit: as PlayerFacesActorNear.
 */
void abi_PlayerFacesActorClose(void)
{
    op_rep(0x21);
    lda16_v(0x0016);
    ldx8(ea_abs(PLAYER_FLIP));
    if (!IF_Z)
        lda16_v(0x0008);
    adc16_v(rd16(ea_abs(PLAYER_X_PIXELS)));
    sec();
    sbc16_v(rd16(ea_dp(ACTOR_X + 1)));
    cmp16_v(0x001E);
    if (!IF_C) {
        lda16(ea_abs(PLAYER_Y_PIXELS));
        adc16_v(0x0003);
        sec();
        sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));
        cmp16_v(0x001A);
    }
    op_sep(0x20);
}
