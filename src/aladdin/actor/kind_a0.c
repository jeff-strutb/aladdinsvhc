/*
 * kind_a0.c: actor kind $A0, a ledge that gives way: once Aladdin stands
 * on it or hangs from it, it shakes for 32 frames, then falls, carrying
 * him down while he stands on it.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"

void abi_ActorFall(void);
void abi_ActorHoldPlayerLong(void);
void abi_ActorSolidForPlayerLong(void);
void abi_ActorSpeedYSetGravity(void);
void abi_AnimStart(void);

#define KA0_ON 0x40u            /* the fall: nonzero while Aladdin stands on it */
#define KA0_FRAMES 0x41u        /* the shake's frames left */
#define KA0_13 0x13u            /* 7 */
#define PLAYER_380 0x000380u    /* bit 2 set: Aladdin cannot hang (no hold test) */
#define KEPT_Y (SCRATCH + 0)    /* its y (pixels, two bytes) before the frame's fall */
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)
#define SOLID_BOX 0x30u         /* ActorSolidForPlayer's box */

/* Stood on or held: ActorSolidForPlayer, then (unless PLAYER_380 bit 2)
   ActorHoldPlayer; C clear when one of them says so. */
static int stood_or_held(int hold_test)
{
    y8_load(SOLID_BOX);
    abi_ActorSolidForPlayerLong();
    if (hold_test && !(cpu.p & P_C))
        return 1;
    a8_load(rd8(PLAYER_380));
    flag_set(P_Z, !((uint8_t)cpu.a & 0x04));
    if (!((uint8_t)cpu.a & 0x04)) {
        abi_ActorHoldPlayerLong();
        if (hold_test)
            return !(cpu.p & P_C);
    }
    return 0;
}

/*
 * @routine 84:A8E0 KindA0Start
 * @actor
 * WHAT IT DOES: Actor kind $A0, a ledge that gives way: solid for
 * Aladdin (and to hang from) in pose 0; once he stands on it or hangs
 * from it, 32 frames more as it is, then pose 1 and it falls (gravity
 * $0A), moving him down with it while he stands on it; it stays solid
 * and holdable as it falls.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; AnimStart 0; KA0_13 7.
 * Each frame (BRK): ActorSolidForPlayerLong with Y = $30, C clear: on;
 * PLAYER_380 bit 2 clear: ActorHoldPlayerLong, C clear: on. Then
 * KA0_FRAMES $20 frames of BRK, the same tests (whatever they say).
 * AnimStart 1; ActorSpeedYSetGravity with A = $0A, Y = 0. Each frame
 * (BRK): KA0_ON 0; ActorSolidForPlayerLong, C clear: KA0_ON up and its
 * y kept at KEPT_Y; ActorFall; KA0_ON set: Aladdin's y + (its y less
 * KEPT_Y); PLAYER_380 bit 2 clear: ActorHoldPlayerLong. Only the first
 * wait runs in the scripts; the rest is written from its bytes
 * ($84:A90C-A973).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $A0
 * in state $0C (the table at $83:8DAB, through the stub at $83:9400).
 * Exit: never returns.
 */
void abi_KindA0Start(void)
{
    a8_load(0x02);
    tsb_at(dpa(ACTOR_08), 0x02);
    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    a8_load(0x00);
    abi_AnimStart();
    a8_load(0x07);
    wr8(dpa(KA0_13), 0x07);
    do
        actor_yield(0x84A8F2u);
    while (!stood_or_held(1));

    /* it shakes for 32 frames */
    a8_load(0x20);
    wr8(dpa(KA0_FRAMES), 0x20);
    do {
        actor_yield(0x84A910u);
        (void)stood_or_held(0);
    } while (dec8_at(dpa(KA0_FRAMES)));

    /* then falls, carrying him */
    a8_load(0x01);
    abi_AnimStart();
    a8_load(0x0A);
    y8_load(0x00);
    abi_ActorSpeedYSetGravity();
    for (;;) {
        actor_yield(0x84A936u);
        wr8(dpa(KA0_ON), 0);
        y8_load(SOLID_BOX);
        abi_ActorSolidForPlayerLong();
        if (!(cpu.p & P_C)) {
            (void)inc8_at(dpa(KA0_ON));
            a8_load(rd8(dpa(ACTOR_Y + 1)));
            wr8(dba(KEPT_Y), (uint8_t)cpu.a);
            a8_load(rd8(dpa(ACTOR_Y + 2)));
            wr8(dba(KEPT_Y + 1), (uint8_t)cpu.a);
        }
        abi_ActorFall();
        a8_load(rd8(dpa(KA0_ON)));
        if ((uint8_t)cpu.a) {
            op_rep(0x20);
            a16_load(dp16(ACTOR_Y + 1));
            flag_set(P_C, 1);
            a16_load(op_sbc16(cpu.a, rd16(dba(KEPT_Y)), &cpu.p));
            flag_set(P_C, 0);
            a16_load(op_adc16(cpu.a, rd16(dba((uint16_t)PLAYER_Y)), &cpu.p));
            wr16(dba((uint16_t)PLAYER_Y), cpu.a);
            op_sep(0x20);
        }
        a8_load(rd8(PLAYER_380));
        flag_set(P_Z, !((uint8_t)cpu.a & 0x04));
        if (!((uint8_t)cpu.a & 0x04))
            abi_ActorHoldPlayerLong();
    }
}
