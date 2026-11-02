/*
 * opening_story.c: the story told before the title screen: six scenes,
 * each a picture with its actors placed in it, held for a set number of
 * frames (Start skips the rest), faded in and out; then the title screen's
 * task. Its task runs in slot 0: the slot's parameter byte is the scene.
 */
#include "machine.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"
#include "wram_video.h"

void abi_ActorSpawnAtLong(void);
void abi_FrameEnd(void);
void abi_LevelTablesClear(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_SongPlay(void);
void abi_SoundQueueF6With1(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TaskSlotsFree1To6(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);
void abi_VramCopy(void);
void abi_VramCopy2(void);
void abi_VramFillMap(void);
void abi_Wait64Frames(void);
void abi_OpeningActorPlace(void);
void abi_ScreenFadeInStart(void);
void abi_ScreenFadeOutStart(void);
void abi_OpeningScene1Start(void);
void abi_OpeningScene2Start(void);
void abi_OpeningScene3Start(void);
void abi_OpeningScene4Start(void);
void abi_OpeningScene5Start(void);
void abi_OpeningScene6Start(void);

#define SLOT1_STATE (TASK_STATE + 24) /* slot 1's state: the fades run there */
#define SCENE TASK_PARAM            /* slot 0's parameter: the scene (0-5), $80 once Start skips */
#define V_0399 0x0399u              /* set to $17 while the story runs */
#define V_03B6 0x03B6u              /* cleared with HDMAEN */
#define V_1A08 0x1A08u              /* set to $FF */
#define STORY_LEVEL 0x35            /* the level number the story's set-up loads */
#define SPAWN_X 0x002Cu             /* where ActorSpawnAt places the actor: x, then y (words) */
#define SPAWN_Y 0x002Eu

/* In the bank DB names ($80), by scene: */
#define SCENE_GFX 0xDAA9u           /* its graphics set, 0: none of its own */
#define SCENE_COPY 0xDAAFu          /* its second-table VRAM copy */
#define SCENE_PALETTE 0xDAB5u       /* its palette */
/* 6 bytes an entry: kind, its byte $07, x, y (words) */
#define OPENING_ACTORS 0xDABBu

/* Code not yet filed */
/* OpeningSceneRun ($80:AD78): its assembly is filed, its C is not: A5
   cannot judge it while the frame update FrameEnd calls ($81:830D) has no
   C (each piece stops there, and the next resume reruns the C from its
   entry). */
#define SCENE_RUN 0x80AD78u
#define SCREEN_BLANK 0x8196B1u      /* forced blank on */
#define LAYER_POSITIONS_CLEAR 0x8196CBu /* the layers' scroll positions zeroed */
#define DISPLAY_SETUP 0x819865u     /* the PPU's display set-up */
#define LEVEL_LOAD 0x81825Bu        /* sets up the level LEVEL names */
#define GFX_LOAD 0x81BAF1u          /* unpacks graphics set A */
#define ACTORS_RESET 0x819D82u      /* every actor record freed */
#define SLOT_15F1_START 0x81EC8Au   /* starts entry X of the slots at $15F1 with A */

/* TaskCreate (or TaskExec) at site: its frame, as the JSL pushes it; the
   five (three) bytes after the JSL are its arguments. TaskCreate returns
   past them by its own RTL; the C goes on. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/* A JSR (its return address ret) to a routine that can wait: the frame the
   task's stack holds while it waits, pulled again by its RTS. */
static void jsr_waits(uint16_t ret, void (*f)(void))
{
    push16(ret);
    f();
    cpu.s = (uint16_t)(cpu.s + 2);
}

/* $80:9C79 TitleAfterFadeOut has only its assembly for now: no script runs
   it, so its C cannot be judged (asm/aladdin/game/opening_story.asm). */


/*
 * @routine 80:ADB6 OpeningScene1Start
 * WHAT IT DOES: Sets up the first scene of the opening story: its song,
 * and the task at $81:F332 that runs with it.
 * HOW: SongPlay $1C; the task at $81:F332 created in slot 4 with
 * parameter 0.
 * Entry: M 8-bit, X 8-bit; called by JSR; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: as TaskCreate leaves them.
 */
void abi_OpeningScene1Start(void)
{
    lda8_v(0x1C);
    abi_SongPlay();
    task_create(0x80ADBCu);
}






/*
 * @routine 80:AEA5 ScreenFadeInStart
 * WHAT IT DOES: Starts the screen fading in, over the next frames.
 * HOW: Creates the fade-in task ($81:9E8E) in slot 1, with parameter 4
 * (frames a step, doubled).
 * Entry: any widths; called by JSR; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: as TaskCreate leaves them.
 */
void abi_ScreenFadeInStart(void)
{
    task_create(0x80AEA5u);
}

/*
 * @routine 80:AEAF ScreenFadeOutStart
 * WHAT IT DOES: Starts the screen fading out, over the next frames.
 * HOW: Creates the fade-out task ($81:9EB9) in slot 1, with parameter 4.
 * Entry: any widths; called by JSR; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: as TaskCreate leaves them.
 */
void abi_ScreenFadeOutStart(void)
{
    task_create(0x80AEAFu);
}


/*
 * @routine 80:AEC0 OpeningActorPlace
 * WHAT IT DOES: Places one of the opening story's actors: entry X of its
 * table gives the kind, the byte its code reads at $07, and the place.
 * HOW: The entry's x and y in the spawn words (SPAWN_X, SPAWN_Y), its
 * kind in A's high byte and the byte in its low; ActorSpawnAtLong; 8-bit
 * registers after.
 * Entry: M 8-bit, X 8-bit; X = the entry's offset (6 bytes an entry); DB =
 * $80, the table's bank, whose $0000-$1FFF is work RAM. Exit: M and X
 * 8-bit (the high bytes of X and Y cleared); A, the low bytes of X and Y
 * and the flags but M and X as ActorSpawnAtLong leaves them.
 */
void abi_OpeningActorPlace(void)
{
    op_rep(0x20);
    lda16(ea_absx(OPENING_ACTORS + 2));
    sta16(ea_abs(SPAWN_X));
    lda16(ea_absx(OPENING_ACTORS + 4));
    sta16(ea_abs(SPAWN_Y));
    op_sep(0x20);
    lda8(ea_absx(OPENING_ACTORS));
    xba();
    lda8(ea_absx(OPENING_ACTORS + 1));
    abi_ActorSpawnAtLong();
    op_sep(0x30);
}
/*
 * @routine 80:ADC6 OpeningScene2Start
 * WHAT IT DOES: Sets up the second scene of the opening story: its song
 * and four actors.
 * HOW: SongPlay $1F; OpeningActorPlace with entries 0, 2, 3 and 1.
 * Entry: M 8-bit, X 8-bit; called by JSR; DB = $80, whose $0000-$1FFF is
 * work RAM. Exit: as OpeningActorPlace leaves them.
 */
void abi_OpeningScene2Start(void)
{
    lda8_v(0x1F);
    abi_SongPlay();
    ldx8_v(0x00);
    abi_OpeningActorPlace();
    ldx8_v(0x0C);
    abi_OpeningActorPlace();
    ldx8_v(0x12);
    abi_OpeningActorPlace();
    ldx8_v(0x06);
    abi_OpeningActorPlace();
}


/* The end three scenes share ($80:AE43): the task at $82:8000 created in
   slot 6 (parameter $18); a frame; palette $56; slot 0 at $15F1 started
   with $1F; the actor of entry 20. */
static void scene_end(void)
{
    task_create(0x80AE43u);
    jsr_waits(0xAE4E, abi_FrameEnd);
    lda8_v(0x56);
    abi_PaletteLoad();
    ldx8_v(0x00);
    lda8_v(0x1F);
    call_routine(SLOT_15F1_START, 1);
    ldx8_v(0x78);
    abi_OpeningActorPlace();
}



