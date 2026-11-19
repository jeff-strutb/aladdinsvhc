/*
 * kind2c.c: actor kind $2C's start, a ride: when Aladdin comes into its
 * zone it takes him (his code sent to $D992) and flies off right and down,
 * carrying him, until it reaches the x its $07 gives; there it lets him go
 * (his code sent to $CDD2) and stays. Its wait is Kind2CWait
 * (kind_waits_rest.c), which plays its sound while it carries him.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_ActorMotionPreset(void);
void abi_ActorAccelerateX(void);
void abi_ActorAccelerateY(void);
void abi_ActorMoveRight(void);
void abi_ActorStepY(void);
void abi_Kind2CCarry(void);

#define K2C_END_X 0x40u         /* the x pixels where the ride ends (word) */
#define K2C_FRAMES 0x43u        /* frames to its wait's next sound, from 6 */
#define K2C_SOUNDS 0x44u        /* nonzero while it carries him: its wait's sound */
#define K2C_END_FAR 0x0F93u     /* K2C_END_X with $07 nonzero */
#define K2C_END_NEAR 0x0CF3u    /* and with $07 0 */
#define K2C_ZONE 0x74u          /* ActorZoneHasTestPoint's zone */
#define K2C_RIDER_KIND 0x146Fu  /* the kind of the actor carrying Aladdin */
#define K2C_PLAYER_FLAGS 0x0347u /* bits 1-7 set while it carries him */
#define K2C_STATE_8C_COUNT 0x141Bu /* counted up as Aladdin enters state $8C */
#define K2C_V_145A 0x145Au      /* negative: no ride */
#define K2C_RIDE_CODE 0xD992u   /* Aladdin's code while it carries him */
#define K2C_OFF_CODE 0xCDD2u    /* and once it lets him go */
#define K2C_PRESET 0x2Du        /* ActorMotionPreset's preset for its flight */
#define K2C_ACCEL_X 0x0Cu       /* its horizontal acceleration */
#define K2C_TOP_X 0x04u         /* the horizontal speed's middle byte it stops speeding up at */
#define K2C_RIDER_Y 0x001Au     /* Aladdin's y below it while it carries him */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:A758 Kind2CStart
 * @actor
 * WHAT IT DOES: Actor kind $2C: a ride. It waits until Aladdin comes into
 * its zone ($145A not negative), then takes him (his code to $D992, its
 * wait's sound on) and flies off: speeding up right and down to a speed
 * of 4 across, carrying him $1A below it, until it reaches K2C_END_X
 * ($0CF3, $0F93 with $07 set); there it puts him and itself at that x and
 * lets him go (his code to $CDD2), and stays for good.
 * HOW: K2C_END_X by $07; $08 bit 1 and $09 bits 7 and 4 set; AnimStart 0;
 * K2C_SOUNDS 0, K2C_FRAMES 6. Each frame (BRK first) until $145A is not
 * negative and ActorZoneHasTestPointLong with zone $74 gives C clear.
 * Then K2C_RIDER_KIND its kind, K2C_SOUNDS up, $0347 bits 1-7 set, $08
 * bit 3 cleared, his facing and flip 0, his resume address $D992;
 * ActorMotionPreset $2D, $1C $0C. Each frame (BRK first): its speed's
 * middle byte other than 4: ActorAccelerateX, ActorAccelerateY;
 * ActorMoveRight, ActorStepY, Kind2CCarry; until its x is K2C_END_X or
 * more. Then $0347 bits 1-7 cleared, $141B 0, $08 bit 3 set, its x and
 * his = K2C_END_X, his speed's middle and top bytes ours, his resume
 * address $CDD2, K2C_SOUNDS 0; a BRK each frame for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $2C in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind2CStart(void)
{
    op_rep(0x20);
    lda16_v(K2C_END_FAR);
    ldy8(ea_dp(ACTOR_07));
    if (IF_Z)
        lda16_v(K2C_END_NEAR);
    sta16(ea_dp(K2C_END_X));
    op_sep(0x20);
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x00);
    jsl_call(0x83A772u, abi_AnimStart);
    stz8(ea_dp(K2C_SOUNDS));
    lda8_v(0x06);
    sta8(ea_dp(K2C_FRAMES));
    for (;;) {
        actor_yield(0x83A77Cu);
        lda8(ea_abs(K2C_V_145A));
        if (IF_N)
            continue;
        ldy8_v(K2C_ZONE);
        jsl_call(0x83A785u, abi_ActorZoneHasTestPointLong);
        if (!IF_C)
            break;
    }
    lda8(ea_dp(ACTOR_06));
    sta8(ea_abs(K2C_RIDER_KIND));
    MEM8(inc, ea_dp(K2C_SOUNDS));
    lda8_v(0xFE);
    tsb8(ea_abs(K2C_PLAYER_FLAGS));
    lda8_v(0x08);
    trb8(ea_dp(ACTOR_08));
    stz8(ea_abs(PLAYER + ACTOR_FACING));
    stz8(ea_abs(PLAYER + ACTOR_FLIP));
    lda8_v(K2C_RIDE_CODE & 0xFF);
    sta8(ea_abs(PLAYER + ACTOR_03));
    lda8_v(K2C_RIDE_CODE >> 8);
    sta8(ea_abs(PLAYER + ACTOR_03 + 1));
    ldy8_v(K2C_PRESET);
    jsl_call(0x83A7ADu, abi_ActorMotionPreset);
    lda8_v(K2C_ACCEL_X);
    sta8(ea_dp(ACTOR_ACCEL_X));
    do {
        actor_yield(0x83A7B5u);
        lda8(ea_dp(ACTOR_SPEED_X + 1));
        cmp8_v(K2C_TOP_X);
        if (!IF_Z) {
            jsl_call(0x83A7BDu, abi_ActorAccelerateX);
            jsl_call(0x83A7C1u, abi_ActorAccelerateY);
        }
        jsl_call(0x83A7C5u, abi_ActorMoveRight);
        jsl_call(0x83A7C9u, abi_ActorStepY);
        call_frame(0xA7CF, abi_Kind2CCarry);
        op_rep(0x20);
        lda16(ea_dp(ACTOR_X + 1));
        cmp16_v(rd16(ea_dp(K2C_END_X)));
        op_sep(0x20);
    } while (!IF_C);
    lda8_v(0xFE);
    trb8(ea_abs(K2C_PLAYER_FLAGS));
    stz8(ea_abs(K2C_STATE_8C_COUNT));
    lda8_v(0x08);
    tsb8(ea_dp(ACTOR_08));
    op_rep(0x20);
    lda16(ea_dp(K2C_END_X));
    sta16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(PLAYER + ACTOR_X + 1));
    lda16(ea_dp(ACTOR_SPEED_X + 1));
    sta16(ea_abs(PLAYER + ACTOR_SPEED_X + 1));
    op_sep(0x20);
    lda8_v(K2C_OFF_CODE & 0xFF);
    sta8(ea_abs(PLAYER + ACTOR_03));
    lda8_v(K2C_OFF_CODE >> 8);
    sta8(ea_abs(PLAYER + ACTOR_03 + 1));
    stz8(ea_dp(K2C_SOUNDS));
    for (;;)
        actor_yield(0x83A802u);
}

/*
 * @routine 83:A806 Kind2CCarry
 * WHAT IT DOES: Keeps Aladdin with the ride: his x its x, his y $1A below
 * its y.
 * HOW: 16-bit, carry cleared: his x pixels = its x pixels; his y pixels
 * = its y pixels + K2C_RIDER_Y.
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: M 8-bit; A = his y (16 bits); C from the
 * addition.
 */
void abi_Kind2CCarry(void)
{
    op_rep(0x21);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(PLAYER + ACTOR_X + 1));
    lda16(ea_dp(ACTOR_Y + 1));
    adc16_v(K2C_RIDER_Y);
    sta16(ea_abs(PLAYER + ACTOR_Y + 1));
    op_sep(0x20);
}
