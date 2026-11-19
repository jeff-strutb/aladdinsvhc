/*
 * player_exit.c: Aladdin's code at a stage's end: the exit walk of an
 * escort level (LevelExitCheck sets it) and the wait after a level's end
 * scene; each fades the screen out, waits for the fade's task and starts
 * the running task again at what follows the stage. Actor code (it yields
 * by BRK, docs/aladdin/ACTORS.md) reached where his resume address was
 * set. Written instruction for instruction from the assembly
 * (player_exit.asm), every JSR and JSL with its return frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_video.h"
#include "actor.h"

void abi_ActorAccelerateX(void);
void abi_ActorMoveX(void);
void abi_ActorSpeedXSetAccel(void);
void abi_PlayerMoveStart(void);
void abi_SecondPoolFreeAll(void);
void abi_SoundQueueF6With1(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);

#define SCENE_ON 0x03FAu            /* counted up as a stage ends */
#define V_1454 0x1454u              /* counted up as a stage ends */
#define VIEW_EDGE_RULE 0x145Bu      /* the rule the view's edges follow */
#define EXIT_VIEW_RULE 0x04         /* the rule while he walks out */
#define TASK2_STATE 0x009Fu         /* task slot 2's state: 0 when free */
#define EXIT_X_MAX 0x2060u          /* the escort walk's x pixels go no further */
#define FRAMES_40 ACTOR_40          /* frames left of the walk's first part */
#define STAGE_GOES_ON 0xC6F2u       /* by level: nonzero, the level's end follows; 0 the next part */

/* A JSL at `site`: its return address on the stack while the callee runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* The fade's start: SoundQueueF6With1 (its JSL at `site`) and TaskCreate
   (the next JSL, its five bytes after it), which returns past them. */
static void fade_start(uint32_t site)
{
    jsl_call(site, abi_SoundQueueF6With1);
    jsl_frame(site + 4);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/* STAGE_GOES_ON by LEVEL: nonzero, TaskExec (its JSL at `site`) to the
   level's end; else the one at `other`. Never returns. */
static void stage_goes_on(uint32_t site, uint32_t other)
{
    ldx8(ea_abs(LEVEL));
    lda8(ea_absx(STAGE_GOES_ON));
    if (IF_Z)
        site = other;
    jsl_frame(site);
    abi_TaskExec();
}

/*
 * @routine 81:E7B1 PlayerStageEnd
 * @actor
 * WHAT IT DOES: Aladdin at a level's end once its end scene is over: the
 * screen fades out, and the running task goes on at the level's end (or
 * at the next part, for a level the table marks so).
 * HOW: ACTOR_FACING and ACTOR_FLIP 0; SecondPoolFreeAll; SCENE_ON and
 * V_1454 up; SoundQueueF6With1; TaskCreate BrightnessDownTask in slot 2
 * with parameter 4. Each frame (BRK first) until TASK2_STATE is 0.
 * STAGE_GOES_ON by LEVEL (X 8-bit): nonzero, TaskExec LevelEndTask; else
 * (no script's level) TaskExec NextPartTask.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where the end scene's task (80:A4C1) set his code.
 * Exit: never returns.
 */
void abi_PlayerStageEnd(void)
{
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    jsl_call(0x81E7B5u, abi_SecondPoolFreeAll);
    MEM8(inc, ea_abs(SCENE_ON));
    MEM8(inc, ea_abs(V_1454));
    fade_start(0x81E7BFu);
    do {
        actor_yield(0x81E7CCu);
        lda8(ea_abs(TASK2_STATE));
    } while (!IF_Z);
    stage_goes_on(0x81E7DBu, 0x81E7E2u);
}
