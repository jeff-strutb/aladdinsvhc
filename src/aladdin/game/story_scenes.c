/*
 * story_scenes.c: steps of the story that runs at $80:AF89, six scenes by
 * slot 0's parameter (each set up by an entry of the table at $80:B092),
 * faded in and out, with 64 frames between scenes. A scene may leave a
 * sound command for its end.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
void abi_ActorSpawnAtLong(void);
void abi_StoryActorPlace(void);
void abi_SongPlay(void);
void abi_PaletteCycleStartLong(void);
void abi_Mode7ScreenSetup(void);
#include "wram.h"
#include "wram_video.h"
#include "wram_player.h"
#include "wram_state.h"
#include "hw.h"

void abi_TaskCreate(void);
void abi_TaskWait(void);
void abi_ActorsFreeAll(void);
void abi_DisplayDefaults(void);
void abi_FrameStepLong(void);
void abi_LevelRamSetupLong(void);
void abi_LevelTablesClear(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_ScreenBlank(void);
void abi_ScrollStateClear(void);
void abi_SoundQueueF6(void);
void abi_SoundQueueF6With1(void);
void abi_StoryFadeInStart(void);
void abi_StoryFadeOutStart(void);
void abi_StoryWait64Frames(void);
void abi_TaskExec(void);
void abi_TextLayerClear(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);
void abi_VramCopy2(void);
void abi_VramFillMap(void);
void abi_StoryFrameEnd(void);
void abi_StorySceneRun(void);
void abi_StoryScene5Start(void);

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* Slot 0's own data, read as the scene ends ($80:B02F): when nonzero, the
   parameter of sound command $F6 queued then, and the frames waited after
   it. */
#define SCENE_END_SOUND 0x007Cu
#define SCENE_END_WAIT 0x007Du
#define SLOT1_STATE (TASK_STATE + 24) /* slot 1's state: the fades run there */
#define V_0399 0x0399u        /* set to $17 while the story runs; purpose not yet established */
#define STORY_FRAMES_LEFT 0x0076u /* slot 0's own data: frames the scene still runs (word) */
#define V_0078 0x0078u        /* slot 0's own data, cleared as the fifth scene starts */
#define V_0079 0x0079u
#define V_007A 0x007Au
#define V_007B 0x007Bu
/* In the bank DB names ($80), by scene: */
#define STORY_FRAMES 0xDB3Fu  /* the frames it runs (words) */
#define STORY_GFX 0xDB4Bu     /* its graphics set, 0: none of its own */
#define STORY_COPY 0xDB51u    /* its second-table VRAM copy */
#define STORY_PALETTE 0xDB57u /* its palette */
#define STORY_SCENE_STARTS 0xB092u /* in bank $80: the six scenes' set-ups (words) */

/* TaskCreate at site: its frame, as the JSL pushes it; the five bytes
   after the JSL are its arguments. TaskCreate returns past them by its
   own RTL; the C goes on. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

#define STORY_ACTORS 0xDB5Du  /* 6 bytes each: kind (0: the player), $07, x, y (in the bank DB names) */
#define PLACE_X 0x002Cu       /* where ActorSpawnAt places the actor: x, then y (words) */
#define PLACE_Y 0x002Eu

/*
 * @routine 80:B09E StoryScene1Start
 * WHAT IT DOES: Sets up the story's first scene: song $32, and the scene's
 * task at $81:F344 started.
 * HOW: SongPlay $32; the task created in slot 4 with parameter 0.
 * Entry: M 8-bit, X 8-bit; called by JSR (through the table at
 * $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as
 * TaskCreate leaves them.
 */
void abi_StoryScene1Start(void)
{
    a8_load(0x32);
    jsl_frame(0x80B0A0u);           /* SongPlay pushes D and calls on: its JSL's return under them */
    abi_SongPlay();
    (void)rd8((uint16_t)(cpu.s + 1)); /* its RTL pulls the return: low, high, bank */
    (void)rd8((uint16_t)(cpu.s + 2));
    (void)rd8((uint16_t)(cpu.s + 3));
    cpu.s = (uint16_t)(cpu.s + 3);
    task_create(0x80B0A4u);
}

/*
 * @routine 80:B0AE StoryScene2Start
 * WHAT IT DOES: Sets up the story's second scene: sound command $F6 with
 * parameter 2 to be queued as it ends, then a wait of 64 frames, and the
 * scene's task at $81:F344 started.
 * HOW: The two slot 0 bytes; the task created in slot 4 with parameter 3.
 * Entry: M 8-bit, X 8-bit; called by JSR (through the table at
 * $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as
 * TaskCreate leaves them.
 */
void abi_StoryScene2Start(void)
{
    a8_load(0x02);
    wr8(dba(SCENE_END_SOUND), 0x02);
    a8_load(0x40);
    wr8(dba(SCENE_END_WAIT), 0x40);
    task_create(0x80B0B8u);
}

/*
 * @routine 80:B0C2 StoryScene3Start
 * WHAT IT DOES: Sets up the story's third scene: song $34, its actors
 * from the first entry, the scene's task with scene 4, and palette cycle
 * $14 in slot 0.
 * HOW: SongPlay $34; StoryActorPlace with X = 0; TaskCreate (slot 4,
 * parameter 4, $81:F344); PaletteCycleStartLong with X = 0, A = $14.
 * Entry: M 8-bit, X 8-bit; called by JSR (through the table at
 * $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as
 * PaletteCycleStartLong leaves them.
 */
void abi_StoryScene3Start(void)
{
    a8_load(0x34);
    jsl_frame(0x80B0C4u);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x00);
    call_frame(0xB0CC, abi_StoryActorPlace);
    task_create(0x80B0CDu);
    x8_load(0x00);
    a8_load(0x14);
    jsl_frame(0x80B0DAu);
    abi_PaletteCycleStartLong();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 80:B0DF StoryScene4Start
 * WHAT IT DOES: Sets up the story's fourth scene: song $23, its actors
 * from entry $24, a frame, the Mode 7 screen (set $20), the spinning zoom
 * task, and the first two palette bytes $2048 uploaded.
 * HOW: SongPlay $23; StoryActorPlace with X = $24; TaskWait 1;
 * Mode7ScreenSetup $20; TaskCreate (slot 4, parameter 0,
 * SceneSpinZoomTask); the palette buffer's first word's bytes $48, $20;
 * PALETTE_UPLOAD up.
 * Entry: M 8-bit, X 8-bit; called by JSR (through the table at $80:B092)
 * from the story's task; DB a bank whose $0000-$1FFF is work RAM. Exit:
 * when the task has resumed; N and Z from PALETTE_UPLOAD.
 */
void abi_StoryScene4Start(void)
{
    a8_load(0x23);
    jsl_frame(0x80B0E1u);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x24);
    call_frame(0xB0E9, abi_StoryActorPlace);
    a8_load(0x01);
    jsl_frame(0x80B0ECu);
    abi_TaskWait();
    a8_load(0x20);
    jsl_frame(0x80B0F2u);
    abi_Mode7ScreenSetup();
    cpu.s = (uint16_t)(cpu.s + 3);
    task_create(0x80B0F6u);
    a8_load(0x48);
    wr8(PALETTE_BUFFER, 0x48);
    a8_load(0x20);
    wr8(PALETTE_BUFFER + 1, 0x20);
    (void)inc8_at(dba(PALETTE_UPLOAD));
}

/*
 * @routine 80:B160 StoryScene6Start
 * WHAT IT DOES: Sets up the story's sixth scene: song $13 and the scene's
 * task with scene $13.
 * HOW: SongPlay $13; TaskCreate (slot 4, parameter $13, $81:F344).
 * Entry: M 8-bit, X 8-bit; called by JSR (through the table at
 * $80:B092); DB a bank whose $0000-$1FFF is work RAM. Exit: as TaskCreate
 * leaves them.
 */
void abi_StoryScene6Start(void)
{
    a8_load(0x13);
    jsl_frame(0x80B162u);
    abi_SongPlay();
    cpu.s = (uint16_t)(cpu.s + 3);
    task_create(0x80B166u);
}

/*
 * @routine 80:B17B StoryFadeInStart
 * WHAT IT DOES: Starts the screen fading in, over the next frames, as a
 * scene of the story begins.
 * HOW: Creates the fade-in task ($81:9E8E) in slot 1, with parameter 4.
 * Entry: any widths; called by JSR from the story's task; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: as TaskCreate leaves them.
 */
void abi_StoryFadeInStart(void)
{
    task_create(0x80B17Bu);
}

/*
 * @routine 80:B185 StoryFadeOutStart
 * WHAT IT DOES: Starts the screen fading out, over the next frames, as a
 * scene of the story ends.
 * HOW: Creates the fade-out task ($81:9EB9) in slot 1, with parameter 4.
 * Entry: any widths; called by JSR from the story's task; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: as TaskCreate leaves them.
 */
void abi_StoryFadeOutStart(void)
{
    task_create(0x80B185u);
}

/*
 * @routine 80:B18F StoryWait64Frames
 * WHAT IT DOES: Makes the story's task wait 64 frames between scenes,
 * letting the other tasks run.
 * HOW: TaskWait with A = 64.
 * Entry: M 8-bit; called by JSR from the story's task. Exit: when the
 * task resumes, the registers as the wait restores them (A low byte =
 * 64).
 */
void abi_StoryWait64Frames(void)
{
    a8_load(0x40);
    jsl_frame(0x80B191u);
    abi_TaskWait();
}

/*
 * @routine 80:B196 StoryActorPlace
 * WHAT IT DOES: Places entry X of the story scenes' actors ($80:DB5D): a
 * new actor of its kind, or, for kind 0, the player.
 * HOW: The entry's x and y into the spawn words. A kind: in A's high
 * byte, the $07 byte in its low, ActorSpawnAtLong. Kind 0: the player's
 * $06 cleared, its $07 the byte, the low bytes of x and y its whole
 * pixels' low bytes with their fractions cleared, and its state $0C (new).
 * Entry: M 8-bit, X 8-bit; X = the entry's offset; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit.
 */
void abi_StoryActorPlace(void)
{
    uint32_t entry = dba(STORY_ACTORS) + cpu.x;
    uint16_t y = rd16(entry + 4);
    uint8_t kind, b07;

    wr16(dba(PLACE_X), rd16(entry + 2));
    wr16(dba(PLACE_Y), y);
    cpu.a = y;
    kind = rd8(entry);
    a8_load(kind);
    if (kind) {
        op_xba();
        a8_load(rd8(entry + 1));
        jsl_frame(0x80B1AFu);
        abi_ActorSpawnAtLong();
        cpu.s = (uint16_t)(cpu.s + 3);
        op_sep(0x30);
        return;
    }
    /* kind 0: the player, set there to start */
    wr8(dba(PLAYER + ACTOR_06), 0);
    b07 = rd8(entry + 1);
    a8_load(b07);
    wr8(dba(PLAYER + ACTOR_07), b07);
    x8_load(rd8(dba(PLACE_X)));
    wr8(dba(PLAYER + ACTOR_X + 1), (uint8_t)cpu.x);
    wr8(dba(PLAYER + ACTOR_X), 0);
    x8_load(rd8(dba(PLACE_Y)));
    wr8(dba(PLAYER + ACTOR_Y + 1), (uint8_t)cpu.x);
    wr8(dba(PLAYER + ACTOR_Y), 0);
    a8_load(0x0C);
    wr8(dba(PLAYER + ACTOR_STATE), 0x0C);
    op_sep(0x30);
}

/*
 * @routine 80:AF42 EndingStory
 * WHAT IT DOES: The ending's story: after the screen fades out, six scenes
 * one after another, each set up by StorySceneStarts and run for its
 * frames (a scene may leave a sound command for its end), faded in and
 * out; then the iris task, a sound command, the fade-out, and the ending's
 * stages task.
 * HOW: Sound $F6 with 1; StoryFadeOutStart, StoryWait64Frames; the screen
 * blanked and cleared, level $39 set up, V_1A08 = $FF, graphics set 1,
 * palette $5A. For each scene (slot 0's parameter, 0 up): V_0399 = $17;
 * the screen blanked and cleared, the actors freed, HDMA off, both
 * tilemaps filled, the text cleared, the first colour black; set $0E,
 * palette $31; the scene's graphics set (STORY_GFX: 0 none, else copies 0,
 * $4D and STORY_COPY, palette STORY_PALETTE, SCENE_END_SOUND and
 * SCENE_END_WAIT cleared); StoryFadeInStart, StorySceneRun; after it the
 * end sound and wait when set, StoryFadeOutStart, StoryWait64Frames. After
 * the sixth: IrisCloseTask in slot 3; sound $F6 with 1; the fade-out task;
 * frames until slot 1 is free; TaskExec EndingStagesTask.
 * Entry: M 8-bit, X 8-bit; a task's start; DB = $80, whose $0000-$1FFF is
 * work RAM. Exit: never returns.
 */
void abi_EndingStory(void)
{
    lda8_v(0x01);  /* 80AF42 lda */
    jsl_call(0x80AF44u, abi_SoundQueueF6);  /* 80AF44 jsl */
    push16(0xAF4A); abi_StoryFadeOutStart(); cpu.s = (uint16_t)(cpu.s + 2);  /* 80AF48 jsr */
    push16(0xAF4D); abi_StoryWait64Frames(); cpu.s = (uint16_t)(cpu.s + 2);  /* 80AF4B jsr */
    jsl_call(0x80AF4Eu, abi_NmiDisable);  /* 80AF4E jsl */
    jsl_call(0x80AF52u, abi_ScreenBlank);  /* 80AF52 jsl */
    jsl_call(0x80AF56u, abi_ScrollStateClear);  /* 80AF56 jsl */
    jsl_call(0x80AF5Au, abi_OamBufferClear);  /* 80AF5A jsl */
    jsl_call(0x80AF5Eu, abi_DisplayDefaults);  /* 80AF5E jsl */
    lda8_v(0x39);  /* 80AF62 lda */
    sta8(ea_abs(LEVEL));  /* 80AF64 sta */
    jsl_frame(0x80AF67u); abi_LevelRamSetupLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AF67 jsl */
    lda8_v(0xFF);  /* 80AF6B lda */
    sta8(ea_abs(V_1A08));  /* 80AF6D sta */
    lda8_v(0x01);  /* 80AF70 lda */
    jsl_call(0x80AF72u, abi_UnpackSet);  /* 80AF72 jsl */
    ldx8_v(0x00);  /* 80AF76 ldx */
    jsl_call(0x80AF78u, abi_VramCopy);  /* 80AF78 jsl */
    lda8_v(0x5A);  /* 80AF7C lda */
    jsl_call(0x80AF7Eu, abi_PaletteLoad);  /* 80AF7E jsl */
    jsl_call(0x80AF82u, abi_NmiEnable);  /* 80AF82 jsl */
    stz8(ea_abs(TASK_PARAM));  /* 80AF86 stz */
L_80AF89:
    lda8_v(0x17);  /* 80AF89 lda */
    sta8(ea_abs(V_0399));  /* 80AF8B sta */
    jsl_call(0x80AF8Eu, abi_NmiDisable);  /* 80AF8E jsl */
    jsl_call(0x80AF92u, abi_ScreenBlank);  /* 80AF92 jsl */
    jsl_call(0x80AF96u, abi_OamBufferClear);  /* 80AF96 jsl */
    jsl_call(0x80AF9Au, abi_ScrollStateClear);  /* 80AF9A jsl */
    jsl_call(0x80AF9Eu, abi_LevelTablesClear);  /* 80AF9E jsl */
    jsl_call(0x80AFA2u, abi_ActorsFreeAll);  /* 80AFA2 jsl */
    jsl_call(0x80AFA6u, abi_DisplayDefaults);  /* 80AFA6 jsl */
    stz8(ea_abs(HDMAEN_SHADOW));  /* 80AFAA stz */
    stz8(ea_abs(HDMAEN));  /* 80AFAD stz */
    op_rep(0x10);  /* 80AFB0 rep */
    ldx16_v(0x4000);  /* 80AFB2 ldx */
    ldy16_v(0x0000);  /* 80AFB5 ldy */
    jsl_call(0x80AFB8u, abi_VramFillMap);  /* 80AFB8 jsl */
    ldx16_v(0x4800);  /* 80AFBC ldx */
    ldy16_v(0x0000);  /* 80AFBF ldy */
    jsl_call(0x80AFC2u, abi_VramFillMap);  /* 80AFC2 jsl */
    op_sep(0x10);  /* 80AFC6 sep */
    jsl_call(0x80AFC8u, abi_TextLayerClear);  /* 80AFC8 jsl */
    lda8_v(0x00);  /* 80AFCC lda */
    sta8(PALETTE_BUFFER);  /* 80AFCE sta */
    sta8((PALETTE_BUFFER + 1));  /* 80AFD2 sta */
    lda8_v(0x0E);  /* 80AFD6 lda */
    jsl_call(0x80AFD8u, abi_UnpackSet);  /* 80AFD8 jsl */
    ldx8_v(0x7E);  /* 80AFDC ldx */
    jsl_call(0x80AFDEu, abi_VramCopy);  /* 80AFDE jsl */
    lda8_v(0x31);  /* 80AFE2 lda */
    jsl_call(0x80AFE4u, abi_PaletteLoad);  /* 80AFE4 jsl */
    ldx8(ea_abs(TASK_PARAM));  /* 80AFE8 ldx */
    lda8(ea_absx(STORY_GFX));  /* 80AFEB lda */
    if (IF_Z) goto L_80B01B;  /* 80AFEE beq */
    jsl_call(0x80AFF0u, abi_UnpackSet);  /* 80AFF0 jsl */
    ldx8_v(0x00);  /* 80AFF4 ldx */
    jsl_call(0x80AFF6u, abi_VramCopy2);  /* 80AFF6 jsl */
    ldx8_v(0x4D);  /* 80AFFA ldx */
    jsl_call(0x80AFFCu, abi_VramCopy2);  /* 80AFFC jsl */
    ldx8(ea_abs(TASK_PARAM));  /* 80B000 ldx */
    lda8(ea_absx(STORY_COPY));  /* 80B003 lda */
    tax8();  /* 80B006 tax */
    jsl_call(0x80B007u, abi_VramCopy2);  /* 80B007 jsl */
    ldx8(ea_abs(TASK_PARAM));  /* 80B00B ldx */
    lda8(ea_absx(STORY_PALETTE));  /* 80B00E lda */
    jsl_call(0x80B011u, abi_PaletteLoad);  /* 80B011 jsl */
    stz8(ea_abs(SCENE_END_SOUND));  /* 80B015 stz */
    stz8(ea_abs(SCENE_END_WAIT));  /* 80B018 stz */
L_80B01B:
    jsl_call(0x80B01Bu, abi_NmiEnable);  /* 80B01B jsl */
    push16(0xB021); abi_StoryFadeInStart(); cpu.s = (uint16_t)(cpu.s + 2);  /* 80B01F jsr */
    call_frame(0xB024, abi_StorySceneRun);  /* 80B022 jsr */
    MEM8(inc, ea_abs(TASK_PARAM));  /* 80B025 inc */
    lda8(ea_abs(TASK_PARAM));  /* 80B028 lda */
    cmp8_v(0x06);  /* 80B02B cmp */
    if (IF_C) goto L_80B048;  /* 80B02D bcs */
    lda8(ea_abs(SCENE_END_SOUND));  /* 80B02F lda */
    if (IF_Z) goto L_80B03F;  /* 80B032 beq */
    jsl_call(0x80B034u, abi_SoundQueueF6);  /* 80B034 jsl */
    lda8(ea_abs(SCENE_END_WAIT));  /* 80B038 lda */
    jsl_frame(0x80B03Bu); abi_TaskWait();  /* 80B03B jsl */
L_80B03F:
    push16(0xB041); abi_StoryFadeOutStart(); cpu.s = (uint16_t)(cpu.s + 2);  /* 80B03F jsr */
    push16(0xB044); abi_StoryWait64Frames(); cpu.s = (uint16_t)(cpu.s + 2);  /* 80B042 jsr */
    goto L_80AF89;  /* 80B045 jmp */
L_80B048:
    jsl_frame(0x80B048u); abi_TaskCreate(); cpu.unwound = 0;  /* 80B048 jsl */
    jsl_call(0x80B051u, abi_SoundQueueF6With1);  /* 80B051 jsl */
    jsl_frame(0x80B055u); abi_TaskCreate(); cpu.unwound = 0;  /* 80B055 jsl */
L_80B05E:
    lda8_v(0x01);  /* 80B05E lda */
    jsl_frame(0x80B060u); abi_TaskWait();  /* 80B060 jsl */
    lda8(ea_abs(SLOT1_STATE));  /* 80B064 lda */
    if (!IF_Z) goto L_80B05E;  /* 80B067 bne */
    jsl_frame(0x80B069u); abi_TaskExec(); return;  /* 80B069 jsl */
}

/*
 * @routine 80:B10F StoryScene5Start
 * WHAT IT DOES: Sets up the story's fifth scene: five actors, the window
 * effect task, song $24, a frame, palette $5B, the scene's text task, and
 * the sound and wait at its end.
 * HOW: V_0078-V_007B cleared; StoryActorPlace with entries 1-5;
 * WindowEffectTask in slot 6 with parameter $18; SongPlay $24;
 * StoryFrameEnd; palette $5B; StoryTextTask1 in slot 4 with parameter 6;
 * SCENE_END_SOUND = 2, SCENE_END_WAIT = $40.
 * Entry: M 8-bit, X 8-bit; called by JSR (through StorySceneStarts); DB a
 * bank whose $0000-$1FFF is work RAM. Exit: A low = $40.
 */
void abi_StoryScene5Start(void)
{
    stz8(ea_abs(V_0078));  /* 80B10F stz */
    stz8(ea_abs(V_0079));  /* 80B112 stz */
    stz8(ea_abs(V_007A));  /* 80B115 stz */
    stz8(ea_abs(V_007B));  /* 80B118 stz */
    ldx8_v(0x06);  /* 80B11B ldx */
    call_frame(0xB11F, abi_StoryActorPlace);  /* 80B11D jsr */
    ldx8_v(0x0C);  /* 80B120 ldx */
    call_frame(0xB124, abi_StoryActorPlace);  /* 80B122 jsr */
    ldx8_v(0x12);  /* 80B125 ldx */
    call_frame(0xB129, abi_StoryActorPlace);  /* 80B127 jsr */
    ldx8_v(0x18);  /* 80B12A ldx */
    call_frame(0xB12E, abi_StoryActorPlace);  /* 80B12C jsr */
    ldx8_v(0x1E);  /* 80B12F ldx */
    call_frame(0xB133, abi_StoryActorPlace);  /* 80B131 jsr */
    jsl_frame(0x80B134u); abi_TaskCreate(); cpu.unwound = 0;  /* 80B134 jsl */
    lda8_v(0x24);  /* 80B13D lda */
    jsl_call(0x80B13Fu, abi_SongPlay);  /* 80B13F jsl */
    call_frame(0xB145, abi_StoryFrameEnd);  /* 80B143 jsr */
    lda8_v(0x5B);  /* 80B146 lda */
    jsl_call(0x80B148u, abi_PaletteLoad);  /* 80B148 jsl */
    jsl_frame(0x80B14Cu); abi_TaskCreate(); cpu.unwound = 0;  /* 80B14C jsl */
    lda8_v(0x02);  /* 80B155 lda */
    sta8(ea_abs(SCENE_END_SOUND));  /* 80B157 sta */
    lda8_v(0x40);  /* 80B15A lda */
    sta8(ea_abs(SCENE_END_WAIT));  /* 80B15C sta */
    return;  /* 80B15F rts */
}

/*
 * @routine 80:B073 StorySceneRun
 * WHAT IT DOES: Runs one scene of the ending's story: its set-up, then a
 * frame at a time for its frames.
 * HOW: STORY_FRAMES_LEFT from STORY_FRAMES by the scene (slot 0's
 * parameter); the set-up through StorySceneStarts; StoryFrameEnd until the
 * count is 0.
 * Entry: M 8-bit, X 8-bit; called by JSR from EndingStory; DB = $80, whose
 * $0000-$1FFF is work RAM. Exit: Z set; as the last StoryFrameEnd leaves
 * A, X and Y.
 */
void abi_StorySceneRun(void)
{
    lda8(ea_abs(TASK_PARAM));  /* 80B073 lda */
    ACC8(asl);  /* 80B076 asl */
    tax8();  /* 80B077 tax */
    op_rep(0x20);  /* 80B078 rep */
    lda16(ea_absx(STORY_FRAMES));  /* 80B07A lda */
    sta16(ea_abs(STORY_FRAMES_LEFT));  /* 80B07D sta */
    op_sep(0x20);  /* 80B080 sep */
    /* JSR (STORY_SCENE_STARTS,X) at $80:B082: its return address on the stack while the set-up runs */
    op_push8(0xB0);
    op_push8(0x84);
    call_routine(0x800000u | rd16(0x800000u | (uint16_t)(STORY_SCENE_STARTS + cpu.x)), 0);
    cpu.s = (uint16_t)(cpu.s + 2);
L_80B085:
    call_frame(0xB087, abi_StoryFrameEnd);  /* 80B085 jsr */
    op_rep(0x20);  /* 80B088 rep */
    MEM16(dec, ea_abs(STORY_FRAMES_LEFT));  /* 80B08A dec */
    op_sep(0x20);  /* 80B08D sep */
    if (!IF_Z) goto L_80B085;  /* 80B08F bne */
    return;  /* 80B091 rts */
}

/*
 * @routine 80:B170 StoryFrameEnd
 * WHAT IT DOES: Ends a frame of the ending's story: the frame's update,
 * then the wait for the next frame.
 * HOW: FrameStepLong; TaskWait with A = 1.
 * Entry: M 8-bit, X 8-bit; called by JSR from the story's task; DB a bank
 * whose $0000-$1FFF is work RAM. Exit: as the wait restores the
 * registers (A low = 1).
 */
void abi_StoryFrameEnd(void)
{
    jsl_call(0x80B170u, abi_FrameStepLong);  /* 80B170 jsl */
    lda8_v(0x01);  /* 80B174 lda */
    jsl_frame(0x80B176u); abi_TaskWait();  /* 80B176 jsl */
    return;  /* 80B17A rts */
}
