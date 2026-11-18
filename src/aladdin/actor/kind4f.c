/*
 * kind4f.c: actor kind $4F, a thing that opens once when struck: unless
 * the flag its $07 names in V_035A is already set (then it shows open),
 * it waits to be struck, then sounds, opens and lets out its children.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorSpawnRing(void);
void abi_ActorVariantChildStep(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define K4F_FRAMES 0x40u        /* frames left of a pause */
#define K4F_FLAG 0x41u          /* the V_035A bit its $07 names */
#define K4F_OPEN 0x42u          /* set when it is struck (by its wait), or at once when already open */
#define V_035A 0x035Au          /* flags of things already opened */
#define K4F_FLAGS 0xD366u       /* by $07 AND 3: the bit (four bytes of kind $52's code at $83:D366, read as data) */

static void pause(uint32_t brk_at)
{
    a8_load(0x05);
    wr8(dpa(K4F_FRAMES), 0x05);
    do
        actor_yield(brk_at);
    while (dec8_at(dpa(K4F_FRAMES)));
}

static void stay(void)
{
    for (;;)
        actor_yield(0x83D074u);
}

/*
 * @routine 83:D01A Kind4FStart
 * @actor
 * WHAT IT DOES: Actor kind $4F: shown closed (pose 9) until struck, then
 * sound $12, 5 frames, open (pose $0A), a ring of children, 5 frames, its
 * variant's child, and it stays open; when the flag its $07 names is
 * already set in V_035A it is shown open at once.
 * HOW: ACTOR_08 bits 0-1, ACTOR_09 bits $90 and ACTOR_LOOK's high byte
 * bit 1 set (TSB); K4F_OPEN 0; K4F_FLAG = K4F_FLAGS by $07 AND 3; ANDed
 * with V_035A nonzero: K4F_OPEN up, AnimStart $0A, stays. Else AnimStart
 * 9; BRK until K4F_OPEN; SoundQueueAdd $12, 5 frames (BRK); AnimStart
 * $0A, ActorSpawnRing; 5 frames; ActorVariantChildStep; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83 (the flag
 * bytes' bank) with work RAM at $0000-$1FFF; reached by the actor loop's
 * jump for kind $4F in state $0C (the table at $83:8CAB). Exit: never
 * returns.
 */
void abi_Kind4FStart(void)
{
    uint8_t flag;

    a8_load(0x03);
    tsb_at(dpa(ACTOR_08), 0x03);
    a8_load(0x90);
    tsb_at(dpa(ACTOR_09), 0x90);
    a8_load(0x02);
    tsb_at(dpa(ACTOR_LOOK + 1), 0x02);
    wr8(dpa(K4F_OPEN), 0);
    x8_load((uint8_t)(rd8(dpa(ACTOR_07)) & 0x03));
    flag = rd8(dba((uint16_t)(K4F_FLAGS + cpu.x)));
    a8_load(flag);
    wr8(dpa(K4F_FLAG), flag);
    a8_load((uint8_t)(flag & rd8(dba(V_035A))));
    if ((uint8_t)cpu.a) {
        /* already open */
        (void)inc8_at(dpa(K4F_OPEN));
        a8_load(0x0A);
        jsl_frame(0x83D03Bu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        stay();
    }
    a8_load(0x09);
    jsl_frame(0x83D044u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    do {
        actor_yield(0x83D048u);
        a8_load(rd8(dpa(K4F_OPEN)));
    } while (!(uint8_t)cpu.a);
    a8_load(0x05);
    wr8(dpa(K4F_FRAMES), 0x05);
    a8_load(0x12);
    jsl_frame(0x83D054u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);
    do
        actor_yield(0x83D058u);
    while (dec8_at(dpa(K4F_FRAMES)));
    a8_load(0x0A);
    jsl_frame(0x83D060u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0xD066, abi_ActorSpawnRing);
    pause(0x83D06Bu);
    call_frame(0xD073, abi_ActorVariantChildStep);
    stay();
}
