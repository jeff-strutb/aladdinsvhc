/*
 * kind51.c: actor kind $51, a thing that rises and settles (Aladdin can
 * land on it once it has): its start and cycle, what it does in its second
 * state, its variant with $07 bit 1 set, which plays once and is gone, and
 * the jump its code ends by.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorEndPlaced(void);
void abi_Kind51End(void);
void abi_Kind51Cycle(void);
void abi_Kind51Variant(void);
void abi_SoundQueueAdd(void);
void abi_RandomFromList(void);
void abi_ActorSpawnPair(void);
void abi_ActorSideOfPlayer(void);

#define KIND51_FRAMES 0x40u     /* frames left of the present step */
#define KIND51_SOLID 0x41u      /* nonzero: Aladdin can land on it (its wait tests it) */
#define V_08F9 0x08F9u          /* read (a word) and not used by its second state */

/* KIND51_FRAMES = n, then each frame (BRK at brk_at) the step if any, until it counts down. */
static void for_frames(uint8_t n, uint32_t brk_at, void (*step)(void))
{
    a8_load(n);
    wr8(dpa(KIND51_FRAMES), n);
    do {
        actor_yield(brk_at);
        if (step)
            step();
    } while (dec8_at(dpa(KIND51_FRAMES)));
}

/*
 * @routine 83:D239 Kind51Start
 * @actor
 * WHAT IT DOES: Actor kind $51's start: drawn in front and with its own
 * frame parity bits; with $07 bit 1 set it is the variant that plays once
 * (Kind51Variant), else it cycles (Kind51Cycle).
 * HOW: ACTOR_08 bits 0-1 and ACTOR_09 bits $90 set (TSB); $07 AND 2:
 * Kind51Variant (JMP), else on into Kind51Cycle.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $51 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind51Start(void)
{
    uint8_t v;

    a8_load(0x03);
    tsb_at(dpa(ACTOR_08), 0x03);
    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    v = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x02);
    a8_load(v);
    if (v)
        abi_Kind51Variant();
    else
        abi_Kind51Cycle();
}

/*
 * @routine 83:D24A Kind51Cycle
 * @actor
 * WHAT IT DOES: Kind $51's cycle, for good: down (animation 0, not to be
 * landed on) for 64 frames, or 144 with $07 bit 0 set; then up (animation
 * 1, Aladdin can land on it) for 128.
 * HOW: KIND51_SOLID 0, AnimStart 0; KIND51_FRAMES $40 (or $90) frames of
 * BRK; AnimStart 1, KIND51_FRAMES $80, KIND51_SOLID up, $80 frames of
 * BRK; again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached from
 * Kind51Start and by a jump from Kind51Second (83:D32C). Exit: never
 * returns.
 */
void abi_Kind51Cycle(void)
{
    uint8_t frames;

    for (;;) {
        wr8(dpa(KIND51_SOLID), 0);
        a8_load(0x00);
        abi_AnimStart();
        frames = 0x40;
        x8_load(frames);
        a8_load((uint8_t)(rd8(dpa(ACTOR_07)) & 0x01));
        if ((uint8_t)cpu.a) {
            frames = 0x90;
            x8_load(frames);
        }
        for_frames(frames, 0x83D25Fu, 0);
        a8_load(0x01);
        abi_AnimStart();
        a8_load(0x80);
        wr8(dpa(KIND51_FRAMES), 0x80);
        (void)inc8_at(dpa(KIND51_SOLID));
        do
            actor_yield(0x83D271u);
        while (dec8_at(dpa(KIND51_FRAMES)));
    }
}

/*
 * @routine 83:D2BE Kind51Second
 * @actor
 * WHAT IT DOES: Kind $51 in its second state: animation 2 with sound $0F
 * for 65 frames, then by a random pick animation 3, or animation 4 with a
 * pair of actors started; 96 frames turned away from Aladdin; then back
 * to its start state and its cycle.
 * HOW: AnimStart 2, SoundQueueAdd $0F; KIND51_FRAMES $20; a BRK and
 * AnimStep, the word at V_08F9 read; $20 frames, then $20 more, of BRK
 * and AnimStep. RandomFromList list $0C: 0, AnimStart 3; else
 * ActorSpawnPair, AnimStart 4. KIND51_FRAMES $60: each frame facing and
 * flip = ActorSideOfPlayer EOR 1, BRK, AnimStep. ACTOR_09 bit 7 and state
 * bits $0C set; facing and flip 0; Kind51Cycle (JMP).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $51 with state bit 7 set (the table at
 * $83:8DFD). Exit: never returns.
 */
void abi_Kind51Second(void)
{
    a8_load(0x02);
    abi_AnimStart();
    a8_load(0x0F);
    abi_SoundQueueAdd();
    a8_load(0x20);
    wr8(dpa(KIND51_FRAMES), 0x20);
    actor_yield(0x83D2CEu);
    abi_AnimStep();
    op_rep(0x20);
    a16_load(rd16(dba(V_08F9)));
    op_sep(0x20);
    do {
        actor_yield(0x83D2DBu);
        abi_AnimStep();
    } while (dec8_at(dpa(KIND51_FRAMES)));
    for_frames(0x20, 0x83D2E9u, abi_AnimStep);
    x8_load(0x0C);
    abi_RandomFromList();
    if (cpu.p & P_Z) {
        a8_load(0x03);
    } else {
        abi_ActorSpawnPair();
        a8_load(0x04);
    }
    abi_AnimStart();
    a8_load(0x60);
    wr8(dpa(KIND51_FRAMES), 0x60);
    do {
        abi_ActorSideOfPlayer();
        a8_load((uint8_t)(cpu.a ^ 0x01));
        wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
        wr8(dpa(ACTOR_FLIP), (uint8_t)cpu.a);
        actor_yield(0x83D316u);
        abi_AnimStep();
    } while (dec8_at(dpa(KIND51_FRAMES)));
    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
    a8_load(0x0C);
    tsb_at(dpa(ACTOR_STATE), 0x0C);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    abi_Kind51Cycle();
}

/*
 * @routine 83:D279 Kind51Variant
 * @actor
 * WHAT IT DOES: Kind $51 with $07 bit 1 set: after a pause when $07 bit 2
 * is set, plays its animation 5 for $58 frames, then is gone for good.
 * HOW: KIND51_SOLID cleared; $07 bit 2: KIND51_FRAMES = $0A frames
 * (BRK each). AnimStart 5; KIND51_FRAMES = $58 frames of BRK and
 * AnimStep; then Kind51End.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by a
 * jump from its start (83:D247). Exit: never returns: as Kind51End.
 */
void abi_Kind51Variant(void)
{
    uint8_t pause;

    wr8(dpa(KIND51_SOLID), 0);
    pause = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x04);
    a8_load(pause);
    if (pause) {
        a8_load(0x0A);
        wr8(dpa(KIND51_FRAMES), 0x0A);
        do
            actor_yield(0x83D285u);
        while (dec8_at(dpa(KIND51_FRAMES)));
    }
    a8_load(0x05);
    abi_AnimStart();
    a8_load(0x58);
    wr8(dpa(KIND51_FRAMES), 0x58);
    do {
        actor_yield(0x83D295u);
        abi_AnimStep();
    } while (dec8_at(dpa(KIND51_FRAMES)));
    abi_Kind51End();
}

/*
 * @routine 83:D32F Kind51End
 * @actor
 * WHAT IT DOES: Where kind $51's code ends: the actor gone for good.
 * HOW: JMP to ActorEndPlaced.
 * Entry: D = the actor's record; reached by a jump from Kind51Variant
 * (83:D29F). Exit: never returns: as ActorEndPlaced.
 */
void abi_Kind51End(void)
{
    abi_ActorEndPlaced();
}
