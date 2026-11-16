/*
 * cue_task_actors.c: actor kinds $24 and $9C, which wait for a cue and
 * then start a task (and show a pose, a sound, a palette cycle).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_TaskCreate(void);
void abi_PaletteCycleStartLong(void);

#define CUE_D7 0x00D7u      /* kind $24 starts its task once this reaches $18 */
#define CUE_B7 0x00B7u      /* then shows its second pose once this is 0 */
#define V_0399 0x0399u      /* set to $10 as kind $24 shows its second pose */
#define CUE_CF 0x00CFu      /* kind $9C starts its task once this is 0 */

/* TaskCreate at `site`, its five argument bytes after the JSL: its frame
   as the JSL pushes it; TaskCreate returns past them by its own RTL and
   the C goes on. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/*
 * @routine 84:8000 Kind24Start
 * @actor
 * WHAT IT DOES: Actor kind $24: shown facing right in its first pose
 * until CUE_D7 reaches $18; then it starts its task, and once CUE_B7 is 0
 * shows its second pose with sound $1C, and stays.
 * HOW: actor_08 bit 1 and actor_09 bits $B0 set; facing and flip 0;
 * AnimStart 0. Each frame (BRK first) until CUE_D7 is $18 or more;
 * TaskCreate (slot 3, parameter 1, KIND24_TASK); each frame until CUE_B7
 * is 0; V_0399 = $10, AnimStart 1, SoundQueueAdd $1C; then BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $24
 * in state $0C (the table at $83:8CAB, through the stub at $83:9233).
 * Exit: never returns.
 */
void abi_Kind24Start(void)
{
    uint8_t v;

    v = (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02);
    wr8(dpa(ACTOR_08), v);
    v = (uint8_t)(rd8(dpa(ACTOR_09)) | 0xB0);
    wr8(dpa(ACTOR_09), v);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    a8_load(0x00);
    abi_AnimStart();
    do {
        actor_yield(0x848016u);
        v = rd8(dba(CUE_D7));
        a8_load(v);
        cmp8_of(v, 0x18);
    } while (v < 0x18);
    task_create(0x84801Fu);
    do {
        actor_yield(0x848028u);
        v = rd8(dba(CUE_B7));
        a8_load(v);
    } while (v);
    a8_load(0x10);
    wr8(dba(V_0399), 0x10);
    a8_load(0x01);
    abi_AnimStart();
    a8_load(0x1C);
    abi_SoundQueueAdd();
    for (;;)
        actor_yield(0x848040u);
}

/*
 * @routine 84:A7E5 Kind9CStart
 * @actor
 * WHAT IT DOES: Actor kind $9C: after 16 frames, waits until CUE_CF is
 * 0, then 128 frames more; it starts its task, palette cycle $24 and its
 * pose, and stays.
 * HOW: actor_08 bit 1 and actor_09 bit 4 set (TSB); COP $10; each frame
 * (BRK first) until CUE_CF is 0; COP $80; TaskCreate (slot 4, parameter
 * 5, KIND9C_TASK); PaletteCycleStartLong with X = 0, A = $24; actor_09
 * bit 7 set; AnimStart 0; then BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $9C
 * in state $0C (the table at $83:8DAB, through the stub at $83:93EE).
 * Exit: never returns.
 */
void abi_Kind9CStart(void)
{
    uint8_t v;

    a8_load(0x02);
    tsb_at(dpa(ACTOR_08), 0x02);
    a8_load(0x10);
    tsb_at(dpa(ACTOR_09), 0x10);
    a8_load(0x10);
    actor_wait(0x84A7EFu);
    do {
        actor_yield(0x84A7F1u);
        v = rd8(dba(CUE_CF));
        a8_load(v);
    } while (v);
    a8_load(0x80);
    actor_wait(0x84A7FAu);
    task_create(0x84A7FCu);
    x8_load(0x00);
    a8_load(0x24);
    abi_PaletteCycleStartLong();
    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
    a8_load(0x00);
    abi_AnimStart();
    for (;;)
        actor_yield(0x84A817u);
}
