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
 * @routine 81:E730 PlayerEscortExit
 * @actor
 * WHAT IT DOES: Aladdin walks out of an escort level's exit (the scroll
 * passed its spot): facing right at a walk's speed, 32 frames, then the
 * screen fades out while he walks on (never past x $2060), and the
 * running task goes on at the level's end or at the story card.
 * HOW: ACTOR_FACING and ACTOR_FLIP 0; SecondPoolFreeAll; SCENE_ON and
 * V_1454 up; ActorSpeedXSetAccel with A $28, Y $57 when LEVEL_035C is 1,
 * else A $16, Y $45; VIEW_EDGE_RULE 4; PlayerMoveStart 0; FRAMES_40 $20
 * frames (BRK first): ActorAccelerateX, ActorMoveX. SoundQueueF6With1;
 * TaskCreate BrightnessDownTask in slot 2 with parameter 4. Each frame
 * (BRK first): ActorAccelerateX, ActorMoveX; his x pixels $2060 at most
 * (16-bit); until TASK2_STATE is 0. STAGE_GOES_ON by LEVEL (X 8-bit):
 * nonzero, TaskExec LevelEndTask; else TaskExec StoryCardTask.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; reached by
 * the actor loop's RTI where LevelExitCheck set his code (escort levels,
 * the table at $80:B96B). Exit: never returns.
 */
void abi_PlayerEscortExit(void)
{
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    jsl_call(0x81E734u, abi_SecondPoolFreeAll);
    MEM8(inc, ea_abs(SCENE_ON));
    MEM8(inc, ea_abs(V_1454));
    lda8(ea_abs(LEVEL_035C));
    ACC8(dec);
    if (IF_Z) {
        lda8_v(0x28);
        ldy8_v(0x57);
        jsl_call(0x81E748u, abi_ActorSpeedXSetAccel);
    } else {
        lda8_v(0x16);
        ldy8_v(0x45);
        jsl_call(0x81E752u, abi_ActorSpeedXSetAccel);
    }
    lda8_v(EXIT_VIEW_RULE);
    sta8(ea_abs(VIEW_EDGE_RULE));
    lda8_v(0x00);
    call_frame(0xE75F, abi_PlayerMoveStart);
    lda8_v(0x20);
    sta8(ea_dp(FRAMES_40));
    do {
        actor_yield(0x81E764u);
        jsl_call(0x81E766u, abi_ActorAccelerateX);
        jsl_call(0x81E76Au, abi_ActorMoveX);
        MEM8(dec, ea_dp(FRAMES_40));
    } while (!IF_Z);
    fade_start(0x81E772u);
    do {
        actor_yield(0x81E77Fu);
        jsl_call(0x81E781u, abi_ActorAccelerateX);
        jsl_call(0x81E785u, abi_ActorMoveX);
        op_rep(P_M);
        lda16_v(EXIT_X_MAX);
        cmp16_v(rd16(ea_dp(ACTOR_X + 1)));
        if (!IF_C)
            sta16(ea_dp(ACTOR_X + 1));
        op_sep(P_M);
        lda8(ea_abs(TASK2_STATE));
    } while (!IF_Z);
    stage_goes_on(0x81E7A3u, 0x81E7AAu);
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
