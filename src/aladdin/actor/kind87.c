/*
 * kind87.c: actor kind $87, a catcher: when Aladdin's test box comes
 * into its zone it takes hold of him, shakes him about, lets him go,
 * drops, and breaks into five of its children before it ends; a child
 * ($07 nonzero) drops to the ground, waits its turn, and floats away.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_ActorFall(void);
void abi_ActorMotionReset(void);
void abi_ActorOutOfGround(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorStepYBack(void);
void abi_ActorZoneHasTestPointLong(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_Kind87HoldAtIt(void);
void abi_Kind87PlayerToIt(void);

#define K87_CAUGHT 0x43u
#define K87_HELD 0x45u
#define K87_DONE 0x42u
#define K87_CHILD 0x44u
#define K87_FRAMES 0x41u
#define K87_STEP 0x40u
#define K87_SLOT 0x21u
#define K87_ZONE 0x7C
#define K87_PROBES 0xD852u
#define K87_CHILD_PROBES 0xD856u
#define K87_SHAKE_PICKS 0xD843u
#define K87_SHAKE_X 0xD84Cu
#define K87_CHILD_WAITS 0xD859u
#define K87_COUNTS 0x7EFF00u
#define K87_HOLD_CODE 0xD6F8u
#define K87_FREED_CODE 0xCDD2u
#define K87_PLAYER_08F0 0x08F0u
#define K87_PLAYER_TAKEN 0x1427u
#define K87_145A 0x145Au
#define K87_141B 0x141Bu
#define K87_HOLD_AT 0x145Du

/* a child ($83:F894): drops, waits its turn, floats away */
static void k87_child(void)
{
    stz8(ea_dp(K87_CHILD));
    lda8_v(K87_CHILD_PROBES & 0xFF);
    sta8(ea_dp(0x16));
    lda8_v(K87_CHILD_PROBES >> 8);
    sta8(ea_dp(0x17));
    for (;;) {
        abi_ActorOutOfGround();
        if (!IF_Z)
            break;
        op_rep(0x21);
        lda16(ea_dp(ACTOR_Y + 1));
        adc16_v(0x0008);
        sta16(ea_dp(ACTOR_Y + 1));
        op_sep(0x20);
    }
    abi_ActorSnapToGround();
    ldx8(ea_dp(ACTOR_07));
    lda8(ea_absx(K87_CHILD_WAITS));
    if (!IF_Z)
        actor_wait(0x83F8BCu);
    MEM8(inc, ea_dp(K87_CHILD));
    lda8_v(0x02);
    abi_AnimStart();
    lda8_v(0x18);
    actor_wait(0x83F8C8u);
    stz8(ea_dp(ACTOR_SPEED_Y));
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_SPEED_Y + 1));
    stz8(ea_dp(ACTOR_SPEED_Y + 2));
    lda8_v(0x0C);
    sta8(ea_dp(K87_FRAMES));
    do {
        actor_yield(0x83F8D6u);
        abi_ActorStepYBack();
        MEM8(dec, ea_dp(K87_FRAMES));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}

/*
 * @routine 83:F77F Kind87Start
 * @actor
 * WHAT IT DOES: Actor kind $87. $07 0, the catcher: shown and hittable,
 * it waits until Aladdin's test box is in its zone (not while $145A is
 * negative or he is hurt); then it holds him ($30 frames), shakes him
 * through 8 steps of its table, lets him go (his code $CDD2 if it still
 * held him), drops to the ground with sound $3D, makes five children of
 * its kind ($07 1-5, counted by kind at $7E:FF00), blinks 11 frames and
 * ends. With K87_DONE set it skips the catch. $07 nonzero, a child: it
 * drops to the ground, waits by its $07, rises 12 frames and ends.
 * HOW: ACTOR_08 bit 1, ACTOR_09 $90; K87_CAUGHT, K87_HELD 0. Catcher:
 * K87_DONE 0, probes $D852, AnimStart 3; each frame (BRK) unless
 * K87_DONE: $145A negative or $141B: again; ActorZoneHasTestPointLong $7C
 * C set: again. Caught: K87_CAUGHT up, facing 0 (1 when $08F0 is 0);
 * Aladdin's code $D6F8, K87_HELD up, Kind87HoldAtIt, BRK,
 * Kind87PlayerToIt; $30 frames (BRK), K87_HELD 0 once $1427 is 0. Shake:
 * 8 steps down: its x + K87_SHAKE_X by the step's pick; unless K87_DONE,
 * $1427 and K87_HELD set: Aladdin's x the same, else K87_HELD 0 when
 * $1427 is 0; COP 2. Then $1427 and K87_HELD: his code $CDD2. Flip =
 * facing; AnimStart 1, ActorMotionReset $38; ActorFall each frame (BRK)
 * until ActorOutOfGround; SoundQueueAdd $3D, ActorSnapToGround; 5 times:
 * $07 up, slot ($72) up 2, ActorSpawnSameLook with its $06, made: its
 * count up. ACTOR_09 bit 7 cleared; $26 and K87_FRAMES $0B; each frame
 * (BRK) ACTOR_08 bit 3 flipped, 11 frames; ActorEndPlaced (JML). Child:
 * K87_CHILD 0, probes $D856; while not ActorOutOfGround its y + 8;
 * ActorSnapToGround; COP by K87_CHILD_WAITS (when nonzero); K87_CHILD up,
 * AnimStart 2, COP $18; vertical speed $000200; 12 frames (BRK) of
 * ActorStepYBack; ActorEndPlaced (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $87 in state $0C (through its stub at
 * $83:930C). Exit: never returns.
 */
void abi_Kind87Start(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    stz8(ea_dp(K87_CAUGHT));
    stz8(ea_dp(K87_HELD));
    lda8(ea_dp(ACTOR_07));
    if (!IF_Z) {
        k87_child();
        return;
    }
    stz8(ea_dp(K87_DONE));
    lda8_v(K87_PROBES & 0xFF);
    sta8(ea_dp(0x16));
    lda8_v(K87_PROBES >> 8);
    sta8(ea_dp(0x17));
    lda8_v(0x03);
    abi_AnimStart();
    for (;;) {
        actor_yield(0x83F7A2u);
        lda8(ea_dp(K87_DONE));
        if (!IF_Z)
            goto shake;
        lda8(ea_abs(K87_145A));
        if (IF_N)
            continue;
        lda8(ea_abs(K87_141B));
        if (!IF_Z)
            continue;
        ldy8_v(K87_ZONE);
        abi_ActorZoneHasTestPointLong();
        if (!IF_C)
            break;
    }
    MEM8(inc, ea_dp(K87_CAUGHT));
    stz8(ea_dp(ACTOR_FACING));
    lda8(ea_abs(K87_PLAYER_08F0));
    if (IF_Z)
        MEM8(inc, ea_dp(ACTOR_FACING));
    lda8_v(K87_HOLD_CODE & 0xFF);
    sta8(ea_abs(PLAYER + ACTOR_03));
    lda8_v(K87_HOLD_CODE >> 8);
    sta8(ea_abs(PLAYER + ACTOR_03 + 1));
    MEM8(inc, ea_dp(K87_HELD));
    abi_Kind87HoldAtIt();
    actor_yield(0x83F7D4u);
    abi_Kind87PlayerToIt();
    lda8_v(0x30);
    sta8(ea_dp(K87_FRAMES));
    do {
        lda8(ea_abs(K87_PLAYER_TAKEN));
        if (IF_Z)
            stz8(ea_dp(K87_HELD));
        actor_yield(0x83F7E4u);
        MEM8(dec, ea_dp(K87_FRAMES));
    } while (!IF_Z);
shake:
    lda8_v(0x08);
    sta8(ea_dp(K87_STEP));
    do {
        ldx8(ea_dp(K87_STEP));
        ldy8(ea_absx(K87_SHAKE_PICKS));
        op_rep(0x21);
        lda16(ea_dp(ACTOR_X + 1));
        adc16_v(rd16(ea_absy(K87_SHAKE_X)));
        sta16(ea_dp(ACTOR_X + 1));
        ldx8(ea_dp(K87_DONE));
        if (IF_Z) {
            ldx8(ea_abs(K87_PLAYER_TAKEN));
            if (IF_Z) {
                ldx8_v(0x00);
                stx8(ea_dp(K87_HELD));
            } else {
                ldx8(ea_dp(K87_HELD));
                if (!IF_Z)
                    sta16(ea_abs(PLAYER + ACTOR_X + 1));
            }
        }
        op_sep(0x20);
        lda8_v(0x02);
        actor_wait(0x83F816u);
        MEM8(dec, ea_dp(K87_STEP));
    } while (!IF_Z);
    ldx8(ea_abs(K87_PLAYER_TAKEN));
    if (!IF_Z) {
        ldx8(ea_dp(K87_HELD));
        if (!IF_Z) {
            lda8_v(K87_FREED_CODE & 0xFF);
            sta8(ea_abs(PLAYER + ACTOR_03));
            lda8_v(K87_FREED_CODE >> 8);
            sta8(ea_abs(PLAYER + ACTOR_03 + 1));
        }
    }
    lda8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    lda8_v(0x01);
    abi_AnimStart();
    lda8_v(0x38);
    abi_ActorMotionReset();
    do {
        actor_yield(0x83F83Fu);
        abi_ActorFall();
        abi_ActorOutOfGround();
    } while (IF_Z);
    lda8_v(0x3D);
    abi_SoundQueueAdd();
    abi_ActorSnapToGround();
    lda8_v(0x05);
    sta8(ea_dp(K87_FRAMES));
    lda8_v(0x72);
    sta8(ea_dp(K87_SLOT));
    do {
        MEM8(inc, ea_dp(ACTOR_07));
        MEM8(inc, ea_dp(K87_SLOT));
        MEM8(inc, ea_dp(K87_SLOT));
        lda8(ea_dp(ACTOR_06));
        abi_ActorSpawnSameLook();
        if (!IF_N) {
            ldx8(ea_dp(ACTOR_06));
            lda8(ea_longx(K87_COUNTS));
            ACC8(inc);
            sta8(ea_longx(K87_COUNTS));
        }
        MEM8(dec, ea_dp(K87_FRAMES));
    } while (!IF_Z);
    lda8_v(0x80);
    trb8(ea_dp(ACTOR_09));
    lda8_v(0x0B);
    sta8(ea_dp(K87_FRAMES));
    sta8(ea_dp(0x26));
    do {
        actor_yield(0x83F884u);
        lda8(ea_dp(ACTOR_08));
        eor8_v(0x08);
        sta8(ea_dp(ACTOR_08));
        MEM8(dec, ea_dp(K87_FRAMES));
    } while (!IF_Z);
    abi_ActorEndPlaced();
}

/*
 * @routine 83:F8E4 Kind87PlayerToIt
 * WHAT IT DOES: Puts Aladdin at actor kind $87's place: its x, and its y
 * less 8.
 * HOW: 16-bit with C clear: its x pixels to the player's; its y pixels
 * less 7 (less the clear carry: 8) to the player's.
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: M 8-bit; A = the player's new y (16
 * bits); N, V, Z, C from the subtraction.
 */
void abi_Kind87PlayerToIt(void)
{
    op_rep(0x21);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(PLAYER + ACTOR_X + 1));
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(0x0007);
    sta16(ea_abs(PLAYER + ACTOR_Y + 1));
    op_sep(0x20);
}

/*
 * @routine 83:F8F6 Kind87HoldAtIt
 * WHAT IT DOES: Sets where Aladdin is held (K87_HOLD_AT) to actor kind
 * $87's place: its x, and its y less 8.
 * HOW: As Kind87PlayerToIt, into K87_HOLD_AT and the word after it.
 * Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: as Kind87PlayerToIt.
 */
void abi_Kind87HoldAtIt(void)
{
    op_rep(0x21);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(K87_HOLD_AT));
    lda16(ea_dp(ACTOR_Y + 1));
    sbc16_v(0x0007);
    sta16(ea_abs((uint16_t)(K87_HOLD_AT + 2)));
    op_sep(0x20);
}
