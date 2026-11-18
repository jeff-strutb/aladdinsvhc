/*
 * kind6a.c: actor kind $6A, which stage a3 s1 places as spawners: a
 * spawner ($07 0-3) makes a child of its own kind with $07 4 more, again
 * and again after a pause by its $07; a child ($07 4-7) moves one of four
 * ways for good.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorSpeedYSetGravity(void);
void abi_ActorAccelerateYBack(void);
void abi_ActorMoveY(void);
void abi_ActorSpeedXSetAccel(void);
void abi_ActorSpeedYSet(void);
void abi_ActorSpeedUpMove(void);
void abi_ActorSpeedXSet(void);
void abi_ActorMove(void);
void abi_ActorSpeedXUpFlyAgainst(void);
void abi_Kind6ASpawnChild(void);
void abi_Kind6AWay4(void);
void abi_Kind6AWay5(void);
void abi_Kind6AWay6(void);
void abi_Kind6AWay7(void);

#define K6A_PAUSES 0xD65Bu          /* ($80) by $07: the spawner's frames between children */
#define K6A_SPAWNED 0x7EFF00u       /* by kind: children spawned */
#define K6A_FLY_BOX 0x002Cu         /* ActorSpeedXUpFlyAgainst's box ($08) */
#define K6A_WAYS 0x83EB80u          /* Kind6AStart's jump table by $07 * 2 */
#define ACTOR_END_PLACED 0x8396CAu

/* JSL at site to C that may push: its return on the stack while it runs. */
static void jsl_at(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:EB5E Kind6AStart
 * @actor
 * WHAT IT DOES: Actor kind $6A: by its $07, a spawner (0-3) that makes a
 * child of its own kind with $07 4 more and waits its pause, again and
 * again; or a child (4-7), shown in animation 0 facing right, that moves
 * one of four ways for good.
 * HOW: $07 4 or more: ACTOR_08 bit 1 and ACTOR_09 bits $90 set, flip 0,
 * AnimStart 0. JMP (K6A_WAYS,X) by $07 * 2: 0-3 the spawner:
 * Kind6ASpawnChild, COP with K6A_PAUSES by $07, and again; 4-7
 * Kind6AWay4-Kind6AWay7.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (K6A_PAUSES
 * there); reached by the actor loop's jump for kind $6A in state $0C (the
 * table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind6AStart(void)
{
    uint8_t way, v;

    way = rd8(dpa(ACTOR_07));
    a8_load(way);
    flag_set(P_C, 1);
    cmp8_of(way, 0x04);
    if (way >= 0x04) {
        v = (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02);
        a8_load(v);
        wr8(dpa(ACTOR_08), v);
        wr8(dpa(ACTOR_FLIP), 0);
        v = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90);
        a8_load(v);
        wr8(dpa(ACTOR_09), v);
        a8_load(0x00);
        jsl_at(0x83EB75u, abi_AnimStart);
    }
    way = rd8(dpa(ACTOR_07));
    flag_set(P_C, way & 0x80);
    a8_load((uint8_t)(way << 1));            /* ASL */
    x8_load((uint8_t)cpu.a);                 /* TAX */
    switch (cpu.x) {
    case 0: case 2: case 4: case 6:
        for (;;) {                           /* the spawner, 83:EB90 */
            call_frame(0xEB92u, abi_Kind6ASpawnChild);
            x8_load(rd8(dpa(ACTOR_07)));
            a8_load(rd8(dba((uint16_t)(K6A_PAUSES + cpu.x))));
            actor_wait(0x83EB98u);
        }
    case 8: abi_Kind6AWay4(); return;
    case 10: abi_Kind6AWay5(); return;
    case 12: abi_Kind6AWay6(); return;
    case 14: abi_Kind6AWay7(); return;
    default:                                 /* past the table: its next bytes */
        jump_routine(0x830000u | rd16(K6A_WAYS + cpu.x));
        return;
    }
}

/*
 * @routine 83:EB9C Kind6ASpawnChild
 * WHAT IT DOES: A kind $6A spawner makes a child like itself with $07 4
 * more, at its spawn point 0, and counts it for its kind.
 * HOW: $07 kept (PHA); ACTOR_SPAWN_AT 0; $07 + 4; ActorSpawnSameLook with
 * A = ACTOR_06; made (N clear): K6A_SPAWNED by ACTOR_06 up. M 8-bit; $07
 * back (PLA).
 * Entry: M 8-bit, X 8-bit; D = the spawner; called by JSR. Exit: M 8-bit;
 * A = $07, N and Z from it; X as the spawn or the count left it.
 */
void abi_Kind6ASpawnChild(void)
{
    uint8_t v;

    a8_load(rd8(dpa(ACTOR_07)));
    pha8();
    wr8(dpa(ACTOR_SPAWN_AT), 0);
    v = rd8(dpa(ACTOR_07));
    flag_set(P_C, 0);
    v = op_adc8(v, 0x04, &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(dpa(ACTOR_07), v);
    a8_load(rd8(dpa(ACTOR_06)));
    jsl_at(0x83EBAAu, abi_ActorSpawnSameLook);
    if (!(cpu.p & P_N)) {
        x8_load(rd8(dpa(ACTOR_06)));
        v = rd8(K6A_SPAWNED + cpu.x);
        a8_load((uint8_t)(v + 1));           /* LDA, INC */
        wr8(K6A_SPAWNED + cpu.x, (uint8_t)cpu.a);
    }
    op_sep(0x20);
    pla8();
    wr8(dpa(ACTOR_07), (uint8_t)cpu.a);
}

/*
 * @routine 83:EBC1 Kind6AWay4
 * @actor
 * WHAT IT DOES: A kind $6A child with $07 4: thrown up with gravity $0C,
 * then each frame slowed and moved up and down.
 * HOW: ActorSpeedYSetGravity with A = $0C, Y = 0; each frame (BRK first)
 * ActorAccelerateYBack, ActorMoveY.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * Kind6AStart's jump. Exit: never returns.
 */
void abi_Kind6AWay4(void)
{
    a8_load(0x0C);
    y8_load(0x00);
    jsl_at(0x83EBC5u, abi_ActorSpeedYSetGravity);
    for (;;) {
        actor_yield(0x83EBC9u);
        jsl_at(0x83EBCBu, abi_ActorAccelerateYBack);
        jsl_at(0x83EBCFu, abi_ActorMoveY);
    }
}

/*
 * @routine 83:EBD5 Kind6AWay5
 * @actor
 * WHAT IT DOES: A kind $6A child with $07 5: speed $36 across (gaining
 * 6), $60 up or down, and each frame faster.
 * HOW: ActorSpeedXSetAccel A = 6, Y = $36; ActorSpeedYSet Y = $60; each
 * frame (BRK first) ActorSpeedUpMove.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * Kind6AStart's jump. Exit: never returns.
 */
void abi_Kind6AWay5(void)
{
    a8_load(0x06);
    y8_load(0x36);
    jsl_at(0x83EBD9u, abi_ActorSpeedXSetAccel);
    y8_load(0x60);
    jsl_at(0x83EBDFu, abi_ActorSpeedYSet);
    for (;;) {
        actor_yield(0x83EBE3u);
        jsl_at(0x83EBE5u, abi_ActorSpeedUpMove);
    }
}

/*
 * @routine 83:EBEB Kind6AWay6
 * @actor
 * WHAT IT DOES: A kind $6A child with $07 6: speed $39 across, $60 up or
 * down, moved each frame.
 * HOW: ActorSpeedXSet Y = $39; ActorSpeedYSet Y = $60; each frame (BRK
 * first) ActorMove.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * Kind6AStart's jump. Exit: never returns.
 */
void abi_Kind6AWay6(void)
{
    y8_load(0x39);
    jsl_at(0x83EBEDu, abi_ActorSpeedXSet);
    y8_load(0x60);
    jsl_at(0x83EBF3u, abi_ActorSpeedYSet);
    for (;;) {
        actor_yield(0x83EBF7u);
        jsl_at(0x83EBF9u, abi_ActorMove);
    }
}

/*
 * @routine 83:EBFF Kind6AWay7
 * @actor
 * WHAT IT DOES: A kind $6A child with $07 7: speed $3C across (gaining 8),
 * $60 with gravity 8, and each frame faster across, flown against box 8.
 * HOW: ActorSpeedXSetAccel A = 8, Y = $3C; ActorSpeedYSetGravity A = 8,
 * Y = $60; each frame (BRK first) K6A_FLY_BOX 8, ActorSpeedXUpFlyAgainst.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
 * Kind6AStart's jump. Exit: never returns.
 */
void abi_Kind6AWay7(void)
{
    a8_load(0x08);
    y8_load(0x3C);
    jsl_at(0x83EC03u, abi_ActorSpeedXSetAccel);
    a8_load(0x08);
    y8_load(0x60);
    jsl_at(0x83EC0Bu, abi_ActorSpeedYSetGravity);
    for (;;) {
        actor_yield(0x83EC0Fu);
        a8_load(0x08);
        wr8(dba(K6A_FLY_BOX), 0x08);
        jsl_at(0x83EC16u, abi_ActorSpeedXUpFlyAgainst);
    }
}

/*
 * @routine 83:EC1C Kind6ASecond
 * @actor
 * WHAT IT DOES: Kind $6A's second state: it ends.
 * HOW: JML ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $6A with state bit 7 set (the table at $83:8DFD).
 * Exit: never returns.
 */
void abi_Kind6ASecond(void)
{
    actor_leave(ACTOR_END_PLACED);
}
