/*
 * children_start.c: an actor that starts two children at its spawn points, the free list put
 * in order first. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorCreateChild8(void);
void abi_ActorDownedByShot(void);
void abi_ActorEndOutOfRange(void);
void abi_ActorFreePlaced(void);
void abi_ActorHitsTestBoxIfPlayerFree(void);
void abi_ActorStompOrHurt(void);
void abi_ActorYByFrame(void);
void abi_AnimStep(void);
void abi_BackdropActorShow(void);
void abi_BackdropPictureStep(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_PlayerOnFirmGround(void);
void abi_PlayerOnFirmGroundProbe4(void);
void abi_PlayerOnFirmGroundProbe8(void);
void abi_SoundQueueAdd(void);


#define V_035A 0x00035Au  /* bits the actors that use it set */
#define V_036C 0x00036Cu

/*
 * @routine 83:D078 ActorVariantChildStep
 * WHAT IT DOES: For an actor with a variant in bits 2-4 of $07: makes it
 * variant 8 plus that, and starts a child (kind $30 at spawn point $3E),
 * counting $14 up for it; no variant: sets its bit ($41) in $035A and
 * counts $036C up.
 * HOW: Variant: $07 = bits 2-4 shifted down OR 8; $14 up; spawn point $3E;
 * ActorCreateChild8 with kind $30; result zero: its bit set in $035A and
 * $14 back down. None: its bit set in $035A, $036C up.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * (83:D071). Exit: M 8-bit; as ActorCreateChild8, or after the count.
 */
void abi_ActorVariantChildStep(void)
{
    uint8_t variant = rd8(dpa(ACTOR_07)) & 0x1C, n;

    a8_load(variant);
    if (!variant) {
        a8_load(rd8(dpa(ACTOR_41)));
        tsb_at(dba(V_035A), (uint8_t)cpu.a);
        n = (uint8_t)(rd8(dba(V_036C)) + 1);
        wr8(dba(V_036C), n);
        nz8_of(n);
        return;
    }
    /* a kind $30 child at spawn point $3E, its variant in $07 */
    a8_load((uint8_t)((variant >> 2) | 0x08));
    flag_set(P_C, 0);
    wr8(dpa(ACTOR_07), (uint8_t)cpu.a);
    wr8(dpa(ACTOR_FACING), (uint8_t)(rd8(dpa(ACTOR_FACING)) + 1));
    a8_load(0x3E);
    wr8(dpa(ACTOR_SPAWN_AT), 0x3E);
    a8_load(0x30);
    abi_ActorCreateChild8();
    if (!(cpu.p & P_Z))
        return;
    a8_load(rd8(dpa(ACTOR_41)));
    tsb_at(dba(V_035A), (uint8_t)cpu.a);
    n = (uint8_t)(rd8(dpa(ACTOR_FACING)) - 1);
    wr8(dpa(ACTOR_FACING), n);
    nz8_of(n);
}

