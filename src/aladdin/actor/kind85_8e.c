/*
 * kind85_8e.c: actor kinds $85 (a brief pose, with a sound unless its $07
 * bit 4 is set) and $8E (by its $07: a pose for 32 frames, or a burst of
 * copies of itself, each moved off for two frames).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_ActorMoveByVector(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define K8E_FRAMES 0x40u        /* frames left */
#define K8E_COPIES 0x41u        /* copies left to make (set by its spawner) */

/*
 * @routine 84:9602 Kind85Start
 * @actor
 * WHAT IT DOES: Actor kind $85: drawn in front, sound $36 unless its $07
 * bit 4 is set ($84:9614, which no script meets), pose 0 for 16 frames,
 * then it ends.
 * HOW: ACTOR_08 bit 1, ACTOR_09 bit 7 set; $07 bit 4 clear:
 * SoundQueueAdd $36; AnimStart 0; COP $10; ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
 * loop's jump for kind $85 in state $0C (the table at $83:8DAB, through
 * the stub at $83:92F2). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_Kind85Start(void)
{
    uint8_t v;

    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x80));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    v = rd8(dpa(ACTOR_07));
    a8_load(v);
    flag_set(P_Z, !(v & 0x10));
    if (!(v & 0x10)) {
        a8_load(0x36);
        jsl_frame(0x849616u);
        abi_SoundQueueAdd();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
    a8_load(0x00);
    jsl_frame(0x84961Cu);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x10);
    actor_wait(0x849622u);
    abi_ActorEndPlaced();
}

/* Kind $8E's pose: pose n for 32 frames, animating, then it ends. */
static void k8e_pose(uint8_t n, uint32_t brk_at)
{
    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x10));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    a8_load(n);
    abi_AnimStart();
    a8_load(0x20);
    wr8(dpa(K8E_FRAMES), 0x20);
    do {
        actor_yield(brk_at);
        abi_AnimStep();
    } while (dec8_at(dpa(K8E_FRAMES)));
    abi_ActorEndPlaced();
}

/*
 * @routine 84:9B6B Kind8EStart
 * @actor
 * WHAT IT DOES: Actor kind $8E, by its $07: 0 and 1 a pose (0 or 1) for
 * 32 frames, then it ends; 2 or more a burst: K8E_COPIES times a copy of
 * itself ($07 0) made and the actor moved off by vector set $20 for two
 * frames, then it ends. Only $07 = 1 is met by a script ($84:9B72-9B98,
 * $84:9BBD-9BE2 written from their bytes).
 * HOW: $07 0: ACTOR_08 bit 1, ACTOR_09 bit 4 set, AnimStart 0, K8E_FRAMES
 * $20 frames of BRK and AnimStep, ActorEndPlaced; $07 1 the same with
 * AnimStart 1. Else (JMP $84:9BBD): $07 and ACTOR_SPAWN_AT 0; each copy:
 * ActorCreateChild8 with A = its kind ($06), K8E_FRAMES 2, two frames of
 * BRK, ActorMoveByVector with X = $20 and the facing written back;
 * K8E_COPIES down; ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $8E in state $0C (the table at $83:8DAB,
 * through the stub at $83:9351). Exit: never returns; ends in
 * ActorEndPlaced.
 */
void abi_Kind8EStart(void)
{
    uint8_t v = rd8(dpa(ACTOR_07));

    a8_load(v);
    if (v == 0) {
        k8e_pose(0x00, 0x849B8Bu);
        return;
    }
    a8_load((uint8_t)(v - 1));
    if (v == 1) {
        k8e_pose(0x01, 0x849BAFu);
        return;
    }
    /* the burst ($84:9BBD) */
    wr8(dpa(ACTOR_07), 0);
    wr8(dpa(ACTOR_SPAWN_AT), 0);
    do {
        a8_load(rd8(dpa(ACTOR_06)));
        jsl_frame(0x849BC3u);
        abi_ActorCreateChild8();
        cpu.s = (uint16_t)(cpu.s + 3);
        a8_load(0x02);
        wr8(dpa(K8E_FRAMES), 0x02);
        do {
            actor_yield(0x849BCBu);
            x8_load(0x20);
            jsl_frame(0x849BCFu);
            abi_ActorMoveByVector();
            cpu.s = (uint16_t)(cpu.s + 3);
            a8_load(rd8(dpa(ACTOR_FACING)));
            wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
        } while (dec8_at(dpa(K8E_FRAMES)));
    } while (dec8_at(dpa(K8E_COPIES)));
    abi_ActorEndPlaced();
}
