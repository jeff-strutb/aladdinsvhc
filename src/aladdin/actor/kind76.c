/*
 * kind76.c: actor kind $76's start: a figure that poses, waits, then
 * fades its palette away under colour math and cues the scene on.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_AnimStart(void);
void abi_PaletteFadeStep(void);
void abi_PaletteSave(void);
void abi_SoundQueueAdd(void);

#define K76_28 0x28u            /* $FF */
#define K76_29 0x29u            /* word: $891B */
#define K76_FADE 0x40u          /* the fade's step: 0 down to -$1F */
#define SUB_LAYERS 0x00039Au    /* layers kept for the subscreen while the fade runs */
#define CUE_1B1F 0x001B1Fu      /* the scene goes on once this is nonzero (kind81_scene.c) */

static void pose(uint8_t n)
{
    a8_load(n);
    abi_AnimStart();
}

/*
 * @routine 84:8FBA Kind76Start
 * @actor
 * WHAT IT DOES: Actor kind $76: poses (2) for $BD frames, then (1) for
 * $16, queues sound $46, and fades its own sprite palette down to black
 * over 31 frames with colour math on, then cues the scene on (CUE_1B1F)
 * and ends.
 * HOW: ACTOR_08 = 3, ACTOR_09 = $80, K76_29 = $891B, flip 1, K76_28 =
 * $FF; AnimStart 2, COP $BD, AnimStart 1, COP $16; SoundQueueAdd $46;
 * ACTOR_09 = $20; PaletteSave of one palette, 8 + its palette bits
 * (ACTOR_LOOK's high byte AND $0E, halved); K76_FADE 0, PaletteFadeStep;
 * CGWSEL 2, CGADSUB $12, SUB_LAYERS $10; each frame (BRK) K76_FADE down
 * by one and PaletteFadeStep of one palette by it, until it is $E1; then
 * CGWSEL $30, CGADSUB and SUB_LAYERS 0, CUE_1B1F counted up,
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $76
 * in state $0C (the table at $83:8CAB, through the stub at $83:9281).
 * Exit: never returns; ends in ActorEndPlaced.
 */
void abi_Kind76Start(void)
{
    uint8_t step;

    a8_load(0x03);
    wr8(dpa(ACTOR_08), 0x03);
    a8_load(0x80);
    wr8(dpa(ACTOR_09), 0x80);
    op_rep(0x20);
    a16_load(0x891B);
    dp16_set(K76_29, 0x891B);
    op_sep(0x20);
    a8_load(0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    a8_load(0xFF);
    wr8(dpa(K76_28), 0xFF);
    pose(0x02);
    a8_load(0xBD);
    actor_wait(0x848FDBu);
    pose(0x01);
    a8_load(0x16);
    actor_wait(0x848FE5u);
    a8_load(0x46);
    jsl_frame(0x848FE9u);
    abi_SoundQueueAdd();
    cpu.s = (uint16_t)(cpu.s + 3);

    a8_load(0x20);
    wr8(dpa(ACTOR_09), 0x20);
    a8_load((uint8_t)(((rd8(dpa(ACTOR_LOOK + 1)) & 0x0E) >> 1) | 0x08));
    flag_set(P_C, 0);                   /* the LSR's carry: bit 0 of an even value */
    y8_load(0x01);
    jsl_frame(0x848FFAu);
    abi_PaletteSave();
    cpu.s = (uint16_t)(cpu.s + 3);
    a8_load(0x00);
    wr8(dpa(K76_FADE), 0x00);
    y8_load(0x01);
    abi_PaletteFadeStep();
    a8_load(0x02);
    wr8(dba(CGWSEL_SHADOW), 0x02);
    a8_load(0x12);
    wr8(dba(CGADSUB_SHADOW), 0x12);
    a8_load(0x10);
    wr8(dba(SUB_LAYERS), 0x10);
    do {
        actor_yield(0x849017u);
        step = (uint8_t)(rd8(dpa(K76_FADE)) - 1);
        wr8(dpa(K76_FADE), step);
        a8_load(step);
        y8_load(0x01);
        abi_PaletteFadeStep();
        step = rd8(dpa(K76_FADE));
        a8_load(step);
        cmp8_of(step, 0xE1);
    } while (step != 0xE1);

    a8_load(0x30);
    wr8(dba(CGWSEL_SHADOW), 0x30);
    wr8(dba(CGADSUB_SHADOW), 0);
    wr8(dba(SUB_LAYERS), 0);
    (void)inc8_at(dba(CUE_1B1F));
    abi_ActorEndPlaced();
}
