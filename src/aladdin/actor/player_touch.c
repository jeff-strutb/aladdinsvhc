/*
 * player_touch.c: what an actor does when Aladdin touches it: hurts him,
 * or, when he comes down on it from above, he bounces off it (and may
 * stomp it).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorHitsTestBoxIfActive(void);
void abi_ActorHurtsPlayerOnTouch(void);
void abi_PlayerEnter8CFacing(void);
void abi_ScratchBelowBy10(void);
void abi_PlayerStandOnActor(void);
void abi_SoundQueueAdd(void);
void abi_ActorStompOrHurt(void);

#define ACTOR_11 0x11u              /* taken one from as the player stomps it */
#define HEALTH 0x0367u              /* the player's health */
#define PLAYER_SPEED_Y_WHOLE (PLAYER + ACTOR_SPEED_Y + 1) /* negative: he is going up (word) */
#define V_141B 0x141Bu              /* nonzero: no bounce */
#define V_141C 0x141Cu              /* zero: no bounce */
#define V_145A 0x145Au              /* negative: no bounce */
#define V_1448 0x1448u              /* $3C: the bouncing actor is not touched */
#define STOMPED 0x8Cu               /* the state a stomped actor goes into */
#define STOMP_SOUND 0x0F            /* the sound of a stomp */

/* Not yet filed */
#define STOMP_MARK 0x8384F4u        /* an actor of kind $11 started at the player's place */

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
 * @routine 83:83C5 ActorStompOrHurtLong
 * WHAT IT DOES: ActorStompOrHurt for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorStompOrHurt; called by JSL. Exit: as ActorStompOrHurt.
 */
void abi_ActorStompOrHurtLong(void)
{
    abi_ActorStompOrHurt();
}
/* The player's tests, each as its LDX (or LDA) leaves the registers. */
static uint8_t x_from(uint16_t addr)
{
    uint8_t v = rd8(dba(addr));

    x8_load(v);
    return v;
}

static int rising(void)
{
    uint16_t v = rd16(dba(PLAYER_SPEED_Y_WHOLE));

    a16_load(v);
    return (v & 0x8000) != 0;
}

/* Not the bouncing actor ($1448 $3C), touching the test box, from above:
   the tests the bounce routines make first. 0 when one says no. */
static int touch_from_above(int skip_1448)
{
    if (!skip_1448) {
        uint8_t v = rd8(dba(V_1448));

        a8_load(v);
        cmp8_of(v, 0x3C);
        if (v == 0x3C)
            return 0;
    }
    abi_ActorHitsTestBoxIfActive();
    if (cpu.p & P_C)
        return 0;
    op_rep(P_M);
    return 1;
}

/* He can stand on it: alive, not counting state $8C, $141C set, $145A not
   negative. */
static int can_stand(void)
{
    return x_from(HEALTH) && !x_from(V_141B) && x_from(V_141C) && !(x_from(V_145A) & 0x80);
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
    if (!touch_from_above(0))
        goto none;
    abi_ScratchBelowBy10();
    if ((cpu.p & P_C) || rising() || !can_stand())
        goto none;
    abi_PlayerStandOnActor();
    op_sep(0x30);
    a8_load(0x01);
    return;
none:
    op_sep(0x32);
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
    if (!touch_from_above(1))
        goto none;
    abi_ScratchBelowBy10();
    if (cpu.p & P_C) {                    /* from the side: he is hurt */
        op_sep(0x30);
        abi_PlayerEnter8CFacing();
        a8_load(0x01);
        return;
    }
    if (!can_stand() || rising())
        goto none;
    /* stomped: it can no longer be hit, goes into state $8C, one hit
       less, a mark at his feet */
    abi_PlayerStandOnActor();
    op_sep(0x30);
    nz8_of((uint8_t)(rd8(dpa(ACTOR_09)) & 0x7F));
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) & 0x7F));
    flag_set(P_C, 0);
    a8_load(STOMPED);
    wr8(dpa(ACTOR_STATE), STOMPED);
    dec8_at(dpa(ACTOR_11));
    call_routine(STOMP_MARK, 0);
none:
    op_sep(0x32);
}

/*
 * @routine 83:8362 ActorStompedByPlayer
 * WHAT IT DOES: When Aladdin touches the actor: from the side or below he
 * is hurt; coming down on it from above (and able to land) he is stood on
 * it and it is stomped: its code address kept at $35, state $8C, one taken
 * from its $11, sound $0F.
 * HOW: ActorHitsTestBoxIfActive; ScratchBelowBy10 tells above (C clear)
 * from not; not above: PlayerEnter8CFacing. Above: $145A not negative,
 * $141C nonzero, the player's vertical speed not negative and $141B zero;
 * then PlayerStandOnActor, its $03 word to its $35, bit 7 of its $09
 * cleared and of its $08 set, the state, the DEC and SoundQueueAdd.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the hit boxes' bank; called by JSR. Exit: M and X 8-bit. A
 * landing refused: Z and C set. Hurt or stomped, or no touch: as
 * PlayerEnter8CFacing, SoundQueueAdd or ActorHitsTestBoxIfActive leaves
 * the rest.
 */
void abi_ActorStompedByPlayer(void)
{
    uint16_t code;

    if (!touch_from_above(1))
        goto out;
    abi_ScratchBelowBy10();
    if (cpu.p & P_C) {                    /* from the side: he is hurt */
        op_sep(0x30);
        abi_PlayerEnter8CFacing();
        return;
    }
    if ((x_from(V_145A) & 0x80) || !x_from(V_141C) || rising() || x_from(V_141B))
        goto refused;
    /* stomped: its code kept, it can no longer be hit, state $8C, one hit
       less, the stomp's sound */
    abi_PlayerStandOnActor();
    code = dp16(ACTOR_03);
    dp16_set(ACTOR_35, code);
    a16_load(code);
    op_sep(0x30);
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) & 0x7F));
    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x80));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load(STOMPED);
    wr8(dpa(ACTOR_STATE), STOMPED);
    dec8_at(dpa(ACTOR_11));
    a8_load(STOMP_SOUND);
    abi_SoundQueueAdd();
out:
    op_sep(0x30);
    return;
refused:
    op_sep(0x32);
}

/*
 * @routine 83:835E ActorStompedByPlayerLong
 * WHAT IT DOES: ActorStompedByPlayer for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorStompedByPlayer; called by JSL. Exit: as
 * ActorStompedByPlayer.
 */
void abi_ActorStompedByPlayerLong(void)
{
    abi_ActorStompedByPlayer();
}

/*
 * @routine 83:844A ActorBouncesOrHurtsPlayer
 * WHAT IT DOES: When Aladdin touches the actor: from the side or below he
 * is hurt; coming down on it from above (and able to bounce) he is stood
 * on it; the actor is unharmed either way.
 * HOW: Nothing when $1448 is $3C; ActorHitsTestBoxIfActive;
 * ScratchBelowBy10 tells above (C clear) from not; not above:
 * PlayerEnter8CFacing, A = 2. Above: health, $141B zero, $141C nonzero,
 * $145A and the player's vertical speed not negative; then
 * PlayerStandOnActor, A = 1.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the hit boxes' bank; called by JSR. Exit: M and X 8-bit. Hurt:
 * A low = 2; stood: A low = 1; N and Z clear either way. Otherwise: Z
 * set; C set when the $1448 test or the box test said no; A, X, Y and the
 * other flags from the last test.
 */
void abi_ActorBouncesOrHurtsPlayer(void)
{
    if (!touch_from_above(0))
        goto none;
    abi_ScratchBelowBy10();
    if (cpu.p & P_C) {                    /* from the side: he is hurt */
        op_sep(0x30);
        abi_PlayerEnter8CFacing();
        a8_load(0x02);
        return;
    }
    if (!can_stand() || rising())
        goto none;
    abi_PlayerStandOnActor();
    op_sep(0x30);
    a8_load(0x01);
    return;
none:
    op_sep(0x32);
}

/*
 * @routine 83:8446 ActorBouncesOrHurtsPlayerLong
 * WHAT IT DOES: ActorBouncesOrHurtsPlayer for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorBouncesOrHurtsPlayer; called by JSL. Exit: as
 * ActorBouncesOrHurtsPlayer.
 */
void abi_ActorBouncesOrHurtsPlayerLong(void)
{
    abi_ActorBouncesOrHurtsPlayer();
}
