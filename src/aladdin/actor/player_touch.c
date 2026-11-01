/*
 * player_touch.c: what an actor does when Aladdin touches it: hurts him,
 * or, when he comes down on it from above, he bounces off it (and may
 * stomp it).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorHitsTestBoxIfActive(void);
void abi_ActorHurtsPlayerOnTouch(void);
void abi_PlayerEnter8CFacing(void);
void abi_ScratchBelowBy10(void);

#define ACTOR_11 0x11u              /* taken one from as the player stomps it */
#define HEALTH 0x0367u              /* the player's health */
#define PLAYER_SPEED_Y_WHOLE (PLAYER + ACTOR_SPEED_Y + 1) /* negative: he is going up (word) */
#define V_141B 0x141Bu              /* nonzero: no bounce */
#define V_141C 0x141Cu              /* zero: no bounce */
#define V_145A 0x145Au              /* negative: no bounce */
#define V_1448 0x1448u              /* $3C: the bouncing actor is not touched */
#define STOMPED 0x8Cu               /* the state a stomped actor goes into */

/* Not yet filed */
#define PLAYER_BOUNCE 0x8384D5u     /* the player bounces up off the actor */
#define STOMP_MARK 0x8384F4u        /* an actor of kind $11 started at the player's place */
#define TOUCH_8362 0x838362u        /* a touch test (ActorHitsTestBoxIfActive first) */
#define TOUCH_844A 0x83844Au        /* a touch test (ActorHitsTestBoxIfActive first) */

/*
 * @routine 83:80A1 ActorHurtsPlayerOnTouchLong
 * WHAT IT DOES: ActorHurtsPlayerOnTouch for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorHurtsPlayerOnTouch; called by JSL. Exit: as
 * ActorHurtsPlayerOnTouch.
 */
void abi_ActorHurtsPlayerOnTouchLong(void)
{
    abi_ActorHurtsPlayerOnTouch();
}

/*
 * @routine 83:835E ActorTouch8362Long
 * WHAT IT DOES: The touch test at $83:8362 for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as $83:8362: M 8-bit, X 8-bit; D = the actor; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL. Exit: as $83:8362.
 */
void abi_ActorTouch8362Long(void)
{
    call_routine(TOUCH_8362, 0);
}

/*
 * @routine 83:83C9 ActorStompOrHurt
 * WHAT IT DOES: When Aladdin touches the actor: from the side or below he
 * is hurt; coming down on it from above (and able to bounce) he bounces
 * off and stomps it: it goes into state $8C, loses one from its $11, and
 * a small actor marks the stomp.
 * HOW: ActorHitsTestBoxIfActive; ScratchBelowBy10 tells above (C clear)
 * from not; not above: PlayerEnter8CFacing. Above: the bounce needs
 * health, $141B zero, $141C nonzero, $145A and the player's vertical
 * speed not negative; then the bounce at $83:84D5, bit 7 of the actor's
 * $09 cleared, the state, the DEC and the mark at $83:84F4.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the hit boxes' bank. Exit: M and X 8-bit. Hurt: A low = 1, N
 * and Z clear; X and C as PlayerEnter8CFacing leaves them. Otherwise
 * (stomped, or no touch): Z set; A, X, Y and the other flags from the
 * last test or call.
 */
void abi_ActorStompOrHurt(void)
{
    abi_ActorHitsTestBoxIfActive();
    if (IF_C)
        goto none;
    op_rep(0x20);
    abi_ScratchBelowBy10();
    if (IF_C) {
        op_sep(0x30);
        abi_PlayerEnter8CFacing();
        lda8_v(0x01);
        return;
    }
    ldx8(ea_abs(HEALTH));
    if (IF_Z) goto none;
    ldx8(ea_abs(V_141B));
    if (!IF_Z) goto none;
    ldx8(ea_abs(V_141C));
    if (IF_Z) goto none;
    ldx8(ea_abs(V_145A));
    if (IF_N) goto none;
    lda16(ea_abs(PLAYER_SPEED_Y_WHOLE));
    if (IF_N) goto none;
    call_routine(PLAYER_BOUNCE, 0);
    op_sep(0x30);
    MEM8(asl, ea_dp(ACTOR_09));
    MEM8(lsr, ea_dp(ACTOR_09));
    lda8_v(STOMPED);
    sta8(ea_dp(ACTOR_STATE));
    MEM8(dec, ea_dp(ACTOR_11));
    call_routine(STOMP_MARK, 0);
none:
    op_sep(0x32);
}

/*
 * @routine 83:83C5 ActorStompOrHurtLong
 * WHAT IT DOES: ActorStompOrHurt for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorStompOrHurt; called by JSL. Exit: as ActorStompOrHurt.
 */
void abi_ActorStompOrHurtLong(void)
{
    abi_ActorStompOrHurt();
}

/*
 * @routine 83:840F ActorBouncesPlayer
 * WHAT IT DOES: When Aladdin comes down on the actor from above (and can
 * bounce), he bounces up off it; the actor is unharmed.
 * HOW: Nothing when $1448 is $3C; ActorHitsTestBoxIfActive;
 * ScratchBelowBy10 must say above (C clear) and the player's vertical
 * speed not be negative; then health, $141B zero, $141C nonzero, $145A
 * not negative; the bounce at $83:84D5.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the hit boxes' bank. Exit: M and X 8-bit. Bounced: A low = 1,
 * N and Z clear. Otherwise: Z set; A, X, Y and the other flags from the
 * last test.
 */
void abi_ActorBouncesPlayer(void)
{
    lda8(ea_abs(V_1448));
    cmp8_v(0x3C);
    if (IF_Z) goto none;
    abi_ActorHitsTestBoxIfActive();
    if (IF_C) goto none;
    op_rep(0x20);
    abi_ScratchBelowBy10();
    if (IF_C) goto none;
    lda16(ea_abs(PLAYER_SPEED_Y_WHOLE));
    if (IF_N) goto none;
    ldx8(ea_abs(HEALTH));
    if (IF_Z) goto none;
    ldx8(ea_abs(V_141B));
    if (!IF_Z) goto none;
    ldx8(ea_abs(V_141C));
    if (IF_Z) goto none;
    ldx8(ea_abs(V_145A));
    if (IF_N) goto none;
    call_routine(PLAYER_BOUNCE, 0);
    op_sep(0x30);
    lda8_v(0x01);
    return;
none:
    op_sep(0x32);
}

/*
 * @routine 83:8446 ActorTouch844ALong
 * WHAT IT DOES: The touch test at $83:844A for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as $83:844A: M 8-bit, X 8-bit; D = the actor; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSL. Exit: as $83:844A.
 */
void abi_ActorTouch844ALong(void)
{
    call_routine(TOUCH_844A, 0);
}
