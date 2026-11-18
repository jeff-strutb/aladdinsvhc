/*
 * opening_story.c: the story told before the title screen: six scenes,
 * each a picture with its actors placed in it, held for a set number of
 * frames (Start skips the rest), faded in and out; then the title screen's
 * task. Its task runs in slot 0: the slot's parameter byte is the scene.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "hw.h"
#include "wram.h"
#include "wram_video.h"

void abi_ActorSpawnAtLong(void);
void abi_FrameEnd(void);
void abi_PaletteCycleStartLong(void);
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
void abi_ScreenBlank(void);
void abi_UnpackSet(void);
void abi_ScrollStateClear(void);
void abi_DisplayDefaults(void);
void abi_LevelRamSetupLong(void);
void abi_ActorsFreeAll(void);
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
void abi_OpeningSceneRun(void);
void abi_TitleTask(void);
void abi_TaskSlotsFree1To6(void);

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

#define SCENE_FRAMES 0xDA9Du        /* in the bank DB names: by scene, the frames it runs (words) */
#define SCENE_FRAMES_LEFT 0x0076u   /* slot 0's own data: frames the scene still runs (word) */
#define V_0079 0x0079u              /* slot 0's own data, cleared as a scene starts (word) */
#define V_007B 0x007Bu              /* the same (word) */
#define PAD1_PRESSED_HIGH (JOY_PRESSED + 1) /* bit 4: Start newly pressed */
#define SCENE_STARTS 0xADAAu        /* in bank $80: the six scenes' set-ups (words) */
#define SCREEN_BLANK 0x8196B1u      /* forced blank on */
#define LAYER_POSITIONS_CLEAR 0x8196CBu /* the layers' scroll positions zeroed */
#define DISPLAY_SETUP 0x819865u     /* the PPU's display set-up */
#define LEVEL_LOAD 0x81825Bu        /* sets up the level LEVEL names */
#define GFX_LOAD 0x81BAF1u          /* unpacks graphics set A */
#define ACTORS_RESET 0x819D82u      /* every actor record freed */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

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
    a8_load(0x1C);
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
    uint32_t entry = dba(OPENING_ACTORS) + cpu.x;
    uint16_t y = rd16(entry + 4);

    wr16(dba(SPAWN_X), rd16(entry + 2));
    wr16(dba(SPAWN_Y), y);
    cpu.a = y;
    a8_load(rd8(entry));                  /* the kind, into A's high byte */
    op_xba();
    a8_load(rd8(entry + 1));              /* its $07 */
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
    static const uint8_t entries[4] = { 0x00, 0x0C, 0x12, 0x06 };

    a8_load(0x1F);
    abi_SongPlay();
    for (int i = 0; i < 4; i++) {
        x8_load(entries[i]);
        abi_OpeningActorPlace();
    }
}


/* The end three scenes share ($80:AE43): the task at $82:8000 created in
   slot 6 (parameter $18); a frame; palette $56; slot 0 at $15F1 started
   with $1F; the actor of entry 20. */
static void scene_end(void)
{
    task_create(0x80AE43u);
    jsr_waits(0xAE4E, abi_FrameEnd);
    a8_load(0x56);
    jsl_call(0x80AE51u, abi_PaletteLoad);
    x8_load(0x00);
    a8_load(0x1F);
    jsl_call(0x80AE59u, abi_PaletteCycleStartLong);
    x8_load(0x78);
    call_frame(0xAE61, abi_OpeningActorPlace);
}

/*
 * @routine 80:AE63 OpeningScene5Start
 * WHAT IT DOES: Sets up the fifth scene of the opening story: more
 * graphics, an actor, the sprite task and two of the slots at $15F1.
 * HOW: With the NMI off and the screen blanked, graphics set $27 and the
 * second table's copy $54; the actor of entry 19; the task at
 * $81:F332 in slot 4 with parameter 2; slot 0 at $15F1 started with $28
 * and slot $0E with $29.
 * Entry: M 8-bit, X 8-bit; called by JSR; DB = $80, whose $0000-$1FFF is
 * work RAM. Exit: A low = $29, X = $0E; N and Z from the INC of $15F1+X;
 * the rest as that routine leaves them.
 */
void abi_OpeningScene5Start(void)
{
    abi_NmiDisable();
    abi_ScreenBlank();
    lda8_v(0x27);
    abi_UnpackSet();
    ldx8_v(0x54);
    abi_VramCopy2();
    abi_NmiEnable();
    ldx8_v(0x72);
    abi_OpeningActorPlace();
    task_create(0x80AE80u);
    ldx8_v(0x00);
    lda8_v(0x28);
    abi_PaletteCycleStartLong();
    ldx8_v(0x0E);
    lda8_v(0x29);
    abi_PaletteCycleStartLong();
}

/*
 * @routine 80:AE17 OpeningScene6Start
 * WHAT IT DOES: Sets up the sixth scene of the opening story: seven
 * actors, the sprite task, then the end it shares with the third and
 * fourth scenes.
 * HOW: OpeningActorPlace with entries 16, 17, 18, 5, 6, 14 and 15; the
 * task at $81:F332 in slot 4 with parameter 5; then from $80:AE43 the
 * task at $82:8000 in slot 6, a frame (FrameEnd), palette $56, slot 0 at
 * $15F1 started with $1F and the actor of entry 20.
 * Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB = $80,
 * whose $0000-$1FFF is work RAM. Exit: as OpeningActorPlace leaves them.
 */
void abi_OpeningScene6Start(void)
{
    static const uint8_t at[7] = { 0x60, 0x66, 0x6C, 0x1E, 0x24, 0x54, 0x5A };
    static const uint16_t ret[7] = { 0xAE1B, 0xAE20, 0xAE25, 0xAE2A, 0xAE2F, 0xAE34, 0xAE39 };
    int i;

    for (i = 0; i < 7; i++) {
        ldx8_v(at[i]);
        call_frame(ret[i], abi_OpeningActorPlace);
    }
    task_create(0x80AE3Au);
    scene_end();
}
/*
 * @routine 80:AC87 OpeningStory
 * WHAT IT DOES: The opening story: after the screen fades out, six scenes
 * one after another (Start skips to the end), then the title screen.
 * HOW: A frame's wait, a sound command ($F6, 1), the fade-out and 64
 * frames; the screen set up for the story (level $35's set-up, graphics
 * set 1, palette $5A). For each scene (the slot's parameter, 0 up): the
 * screen blanked and cleared, the text and two tilemaps emptied, the
 * story's common graphics and palette loaded, then the scene's own (when
 * it has a graphics set); the fade-in, OpeningSceneRun ($80:AD78), the
 * fade-out and 64 frames. After the sixth (or a skip): the sound command
 * again, and the task starts over at $80:9C91.
 * Entry: M 8-bit, X 8-bit; called by JSL from the task in slot 0; DB = $80,
 * whose $0000-$1FFF is work RAM. Exit: never returns: TaskExec.
 */
void abi_OpeningStory(void)
{
    lda8_v(0x01);
    jsl_frame(0x80AC89u);
    abi_TaskWait();
    lda8_v(0x01);
    xba();
    jsl_call(0x80AC90u, abi_SoundQueueF6With1);
    call_frame(0xAC96, abi_ScreenFadeOutStart);
    jsr_waits(0xAC99, abi_Wait64Frames);
    jsl_call(0x80AC9Au, abi_NmiDisable);
    jsl_call(0x80AC9Eu, abi_ScreenBlank);
    jsl_call(0x80ACA2u, abi_ScrollStateClear);
    jsl_call(0x80ACA6u, abi_OamBufferClear);
    jsl_call(0x80ACAAu, abi_DisplayDefaults);
    lda8_v(STORY_LEVEL);
    sta8(ea_abs(LEVEL));
    jsl_call(0x80ACB3u, abi_LevelRamSetupLong);
    lda8_v(0xFF);
    sta8(ea_abs(V_1A08));
    lda8_v(0x01);
    jsl_call(0x80ACBEu, abi_UnpackSet);
    ldx8_v(0x00);
    jsl_call(0x80ACC4u, abi_VramCopy);
    lda8_v(0x5A);
    jsl_call(0x80ACCAu, abi_PaletteLoad);
    jsl_call(0x80ACCEu, abi_NmiEnable);
    lda8_v(0x17);
    sta8(ea_abs(V_0399));
    stz8(ea_abs(SCENE));
    do {
        jsl_call(0x80ACDAu, abi_NmiDisable);
        jsl_call(0x80ACDEu, abi_ScreenBlank);
        jsl_call(0x80ACE2u, abi_OamBufferClear);
        jsl_call(0x80ACE6u, abi_ScrollStateClear);
        jsl_call(0x80ACEAu, abi_LevelTablesClear);
        jsl_call(0x80ACEEu, abi_ActorsFreeAll);
        stz8(ea_abs(V_03B6));
        stz8(ea_abs(HDMAEN));
        op_rep(0x10);
        ldx16_v(0x4000);
        ldy16_v(0x0000);
        jsl_call(0x80AD00u, abi_VramFillMap);
        ldx16_v(0x4800);
        ldy16_v(0x0000);
        jsl_call(0x80AD0Au, abi_VramFillMap);
        op_sep(0x10);
        jsl_call(0x80AD10u, abi_TextLayerClear);
        lda8_v(0x0E);
        jsl_call(0x80AD16u, abi_UnpackSet);
        ldx8_v(0x7E);
        jsl_call(0x80AD1Cu, abi_VramCopy);
        lda8_v(0x31);
        jsl_call(0x80AD22u, abi_PaletteLoad);
        ldx8(ea_abs(SCENE));
        lda8(ea_absx(SCENE_GFX));
        if (!IF_Z) {
            jsl_call(0x80AD2Eu, abi_UnpackSet);
            ldx8_v(0x00);
            jsl_call(0x80AD34u, abi_VramCopy2);
            ldx8(ea_abs(SCENE));
            lda8(ea_absx(SCENE_COPY));
            tax8();
            jsl_call(0x80AD3Fu, abi_VramCopy2);
            ldx8(ea_abs(SCENE));
            lda8(ea_absx(SCENE_PALETTE));
            jsl_call(0x80AD49u, abi_PaletteLoad);
        }
        jsl_call(0x80AD4Du, abi_NmiEnable);
        call_frame(0xAD53, abi_ScreenFadeInStart);
        call_frame(0xAD56, abi_OpeningSceneRun);
        call_frame(0xAD59, abi_ScreenFadeOutStart);
        jsr_waits(0xAD5C, abi_Wait64Frames);
        MEM8(inc, ea_abs(SCENE));
        lda8(ea_abs(SCENE));
        cmp8_v(0x06);
    } while (!IF_C);
    lda8_v(0x01);
    xba();
    jsl_call(0x80AD6Du, abi_SoundQueueF6With1);
    jsl_frame(0x80AD71u);
    abi_TaskExec();
}
/*
 * @routine 80:ADE1 OpeningScene3Start
 * WHAT IT DOES: Sets up the third scene of the opening story: one actor,
 * then the scenes' common end.
 * HOW: OpeningActorPlace with entry 7, then the end at $80:AE43 (in
 * OpeningScene6Start): the task at $82:8000 in slot 6, a frame
 * (FrameEnd), palette $56, slot 0 at $15F1 started with $1F, the actor of
 * entry 20.
 * Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB = $80,
 * whose $0000-$1FFF is work RAM. Exit: as OpeningActorPlace leaves them.
 */
void abi_OpeningScene3Start(void)
{
    ldx8_v(0x2A);
    call_frame(0xADE5, abi_OpeningActorPlace);
    scene_end();
}
/*
 * @routine 80:ADE8 OpeningScene4Start
 * WHAT IT DOES: Sets up the fourth scene of the opening story: nine
 * actors, then the scenes' common end.
 * HOW: OpeningActorPlace with entries 10, 11, 13, 12, 4, 6, 8, 21 and 9,
 * then the end at $80:AE43 as OpeningScene3Start.
 * Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB = $80,
 * whose $0000-$1FFF is work RAM. Exit: as OpeningActorPlace leaves them.
 */
void abi_OpeningScene4Start(void)
{
    static const uint8_t at[9] = { 0x3C, 0x42, 0x4E, 0x48, 0x18, 0x24, 0x30, 0x7E, 0x36 };
    static const uint16_t ret[9] = { 0xADEC, 0xADF1, 0xADF6, 0xADFB, 0xAE00, 0xAE05, 0xAE0A, 0xAE0F, 0xAE14 };
    int i;

    for (i = 0; i < 9; i++) {
        ldx8_v(at[i]);
        call_frame(ret[i], abi_OpeningActorPlace);
    }
    scene_end();
}

/*
 * @routine 80:AD78 OpeningSceneRun
 * WHAT IT DOES: Runs one scene of the opening story: sets it up, then
 * lets it play for its frames, or until Start is pressed, which skips the
 * rest of the story.
 * HOW: The scene's frame count from its table; its set-up routine through
 * a table of six; then FrameEnd and the count down each frame. Start:
 * the scene number becomes $80, past the last.
 * Entry: M 8-bit, X 8-bit; called by JSR from the story's task (slot 0);
 * DB = $80, whose $0000-$1FFF is work RAM. Exit: M 8-bit, X 8-bit. Ran
 * out: Z set from the count; A, X, Y as the last FrameEnd leaves them.
 * Start: A low = $80, N set, Z clear.
 */
void abi_OpeningSceneRun(void)
{
    lda8(ea_abs(SCENE));
    ACC8(asl);
    tax8();
    op_rep(0x20);
    stz16(ea_abs(V_007B));
    stz16(ea_abs(V_0079));
    lda16(ea_absx(SCENE_FRAMES));
    sta16(ea_abs(SCENE_FRAMES_LEFT));
    op_sep(0x20);
    /* JSR (SCENE_STARTS,X) at $80:AD8D: its return address on the stack
       while the set-up runs */
    op_push8(0xAD);
    op_push8(0x8F);
    call_routine(0x800000u | rd16(0x800000u | (uint16_t)(SCENE_STARTS + cpu.x)), 0);
    cpu.s = (uint16_t)(cpu.s + 2);
    for (;;) {                                   /* each frame of the scene */
        lda8(ea_abs(PAD1_PRESSED_HIGH));
        bit8_i(0x10);
        if (!IF_Z) {                             /* Start: past the last scene */
            lda8_v(0x80);
            sta8(ea_abs(SCENE));
            return;
        }
        call_frame(0xAD99, abi_FrameEnd);
        op_rep(0x20);
        MEM16(dec, ea_abs(SCENE_FRAMES_LEFT));
        op_sep(0x20);
        if (IF_Z)
            return;
    }
}

/*
 * @routine 80:9C79 TitleAfterFadeOut
 * WHAT IT DOES: A task's entry: ends the other tasks, fades the screen
 * out and waits for the fade to end, then goes on as the title screen's
 * task.
 * HOW: TaskSlotsFree1To6; the fade-out task created in slot 1; a frame's
 * wait at a time until slot 1 is free; then on into $80:9C91 (TitleTask).
 * Entry: started by the scheduler: M 8-bit, X 8-bit, D = 0; DB a bank
 * whose $0000-$1FFF is work RAM. Exit: as TitleTask (never returns).
 */
void abi_TitleAfterFadeOut(void)
{
    jsl_call(0x809C79u, abi_TaskSlotsFree1To6);
    task_create(0x809C7Du);
    do {
        lda8_v(0x01);
        jsl_frame(0x809C88u);
        abi_TaskWait();
        lda8(ea_abs(SLOT1_STATE));
    } while (!IF_Z);
    abi_TitleTask();                             /* on into the title screen's task */
}
