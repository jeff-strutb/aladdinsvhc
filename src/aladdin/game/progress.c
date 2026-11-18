/*
 * progress.c: the state a new game starts with, and the level the game's
 * progress leads to.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_GameStartTask(void);
void abi_LevelPlayStartTask(void);
void abi_NewGameState(void);
void abi_LevelFromProgress(void);
void abi_PlayLoopStart(void);
void abi_DemoPlayStart(void);
void abi_RegionCheck(void);
void abi_TextCardScreen(void);
void abi_SoundSetsLoad(void);
void abi_LogoScreen(void);
void abi_NmiEnable(void);
void abi_TitleStartTask(void);
void abi_LevelPlayStartTask(void);
void abi_TaskExec(void);
void abi_TaskCreate(void);
void abi_TaskWait(void);
void abi_SoundQueueF1(void);
void abi_NextPartTask(void);
void abi_LevelCountersClear(void);
void abi_DemoStart(void);
void abi_PlayStartCounts(void);
void abi_NewGameStateLong(void);
void abi_DemoStartLong(void);
void abi_PlayStartCountsLong(void);
#include "wram.h"
#include "wram_video.h"
#include "wram_state.h"

/* Per demo: its pad stream's offset from $84:F700 (words); its progress
   sequence, stage and $0360 (bytes). */
#define DEMO_INPUTS 0xB724u
#define DEMO_SEQUENCES 0xB6FDu
#define DEMO_STAGES 0xB70Au
#define DEMO_0360 0xB717u
#define LEVEL_BY_PROGRESS 0xB73Eu  /* by sequence its first entry's offset; then the levels (in the bank DB names) */

/*
 * @routine 81:8000 BootTask
 * WHAT IT DOES: The first task (slot 0, made by PowerOn): the text
 * upload's VRAM address and size set, then the region check, the text
 * card, the sound sets, the logo screen, the NMI on, and on to the title.
 * HOW: text_upload_vram = $5C00, text_upload_size = $0800; DB = $80;
 * TaskWait a frame; RegionCheck; TextCardScreen; SoundSetsLoad;
 * LogoScreen; NmiEnable; then on at TitleStartTask (JML).
 * Entry: the task's start (M 8-bit, X 8-bit). Exit: none: it goes on as
 * the title's task.
 */
void abi_BootTask(void)
{
    op_rep(0x20);
    lda16_v(0x5C00);
    sta16(ea_abs((uint16_t)TEXT_UPLOAD_VRAM));
    lda16_v(0x0800);
    sta16(ea_abs((uint16_t)TEXT_UPLOAD_SIZE));
    op_sep(0x20);
    lda8_v(0x80);
    pha8();
    plb();
    lda8_v(0x01);
    jsl_frame(0x818016u); abi_TaskWait();
    jsl_frame(0x81801Au); abi_RegionCheck(); cpu.s = (uint16_t)(cpu.s + 3);
    call_frame(0x8020, abi_TextCardScreen);
    jsl_frame(0x818021u); abi_SoundSetsLoad(); cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x818025u); abi_LogoScreen(); cpu.s = (uint16_t)(cpu.s + 3);
    jsl_frame(0x818029u); abi_NmiEnable(); cpu.s = (uint16_t)(cpu.s + 3);
    abi_TitleStartTask();  /* 81802D jml */
}

/*
 * @routine 81:8031 DemoTask
 * WHAT IT DOES: The demo's task: the screen darkened, the demo's state
 * set, the next demo's number for next time, the demo's length, then play
 * (DemoPlayStart).
 * HOW: TaskCreate (slot 1, parameter 4, BrightnessDownTask); TaskWait a
 * frame at a time until slot 1 is free; DemoStart; demo_number counted up,
 * back to 0 at 2; demo_frames = $04EC; then on at DemoPlayStart.
 * Entry: the task's start (M 8-bit, X 8-bit); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: it goes on as play.
 */
void abi_DemoTask(void)
{
    /* BrightnessDownTask, slot 1, parameter 4 */
    jsl_frame(0x818031u); abi_TaskCreate(); cpu.unwound = 0;
    do {
        lda8_v(0x01);
        jsl_frame(0x81803Cu); abi_TaskWait();
        lda8(ea_abs((uint16_t)(TASK_STATE + 1 * 24)));
    } while (!IF_Z);
    call_frame(0x8047, abi_DemoStart);
    MEM8(inc, ea_abs((uint16_t)DEMO_NUMBER));
    lda8(ea_abs((uint16_t)DEMO_NUMBER));
    cmp8_v(0x02);
    if (IF_Z)
        stz8(ea_abs((uint16_t)DEMO_NUMBER));
    op_rep(0x20);
    lda16_v(0x04EC);
    sta16(ea_abs(0x0341));  /* the demo's frames left */
    op_sep(0x20);
    abi_DemoPlayStart();  /* 81805F jmp */
}

/*
 * @routine 81:8062 DemoStartLong
 * WHAT IT DOES: Sets a demo up to play, for callers in other banks.
 * HOW: DemoStart.
 * Entry: as DemoStart; called by JSL. Exit: as DemoStart.
 */
void abi_DemoStartLong(void)
{
    abi_DemoStart();  /* 818062 jsr */
    return;  /* 818065 rtl */
}



/*
 * @routine 81:8095 GameStartTask
 * WHAT IT DOES: The game task's start of play from the title: a new game
 * when none has begun ($036F clear), else play goes on with the game's
 * progress.
 * HOW: $0347 cleared. None begun: NewGameState, $036F set, and on into
 * LevelPlayStartTask. Else: LevelFromProgress, $0364 = $0362, $0347,
 * $036B, $036E, $0359 and $035A cleared, PlayStartCounts, then the play
 * loop's start ($81:8191).
 * Entry: the task's turn (TaskExec from the title): M 8-bit, X 8-bit; DB
 * = $80. Exit: none: it goes on as the play loop.
 */
void abi_GameStartTask(void)
{
    stz8(ea_abs(0x0347));  /* 818095 stz */
    lda8(ea_abs(0x036F));  /* 818098 lda */
    if (IF_Z) {
        call_frame(0x809F, abi_NewGameState);  /* 81809D jsr */
        MEM8(inc, ea_abs(0x036F));  /* 8180A0 inc */
        abi_LevelPlayStartTask();  /* on into it */
        return;
    }
    /* 8180B5: a game begun, play goes on from the progress */
    call_frame(0x80B7, abi_LevelFromProgress);  /* 8180B5 jsr */
    lda8(ea_abs(0x0362));  /* 8180B8 lda */
    sta8(ea_abs(0x0364));  /* 8180BB sta */
    stz8(ea_abs(0x0347));  /* 8180BE stz */
    stz8(ea_abs(0x036B));  /* 8180C1 stz */
    stz8(ea_abs(0x036E));  /* 8180C4 stz */
    stz8(ea_abs(0x0359));  /* 8180C7 stz */
    stz8(ea_abs(0x035A));  /* 8180CA stz */
    call_frame(0x80CF, abi_PlayStartCounts);  /* 8180CD jsr */
    abi_PlayLoopStart();  /* 8180D0 jmp */
}

/*
 * @routine 81:80A3 LevelPlayStartTask
 * WHAT IT DOES: The game task's start of play at the level the game's
 * progress gives (also after a level ends).
 * HOW: LevelFromProgress, PlayStartCounts, $0359, $035A and $036C
 * cleared, then the play loop's start ($81:8191).
 * Entry: the task's turn (TaskExec): M 8-bit, X 8-bit; DB = $80. Exit:
 * none: it goes on as the play loop.
 */
void abi_LevelPlayStartTask(void)
{
    call_frame(0x80A5, abi_LevelFromProgress);  /* 8180A3 jsr */
    call_frame(0x80A8, abi_PlayStartCounts);  /* 8180A6 jsr */
    stz8(ea_abs(0x0359));  /* 8180A9 stz */
    stz8(ea_abs(0x035A));  /* 8180AC stz */
    stz8(ea_abs(0x036C));  /* 8180AF stz */
    abi_PlayLoopStart();  /* 8180B2 jmp */
}

/*
 * @routine 81:80D3 DemoPlayStart
 * WHAT IT DOES: The demo's start of play: a new game's state and its
 * counts, then the play loop.
 * HOW: NewGameState, PlayStartCounts, then on at PlayLoopStart.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * reached by a jump from DemoTask ($81:805F). Exit: none: it goes on as
 * the play loop.
 */
void abi_DemoPlayStart(void)
{
    call_frame(0x80D5, abi_NewGameState);  /* 8180D3 jsr */
    call_frame(0x80D8, abi_PlayStartCounts);  /* 8180D6 jsr */
    abi_PlayLoopStart();  /* 8180D9 jmp */
}

/*
 * @routine 81:80DC PlayStartCountsLong
 * WHAT IT DOES: Sets what play starts with, for callers in other banks.
 * HOW: PlayStartCounts.
 * Entry: as PlayStartCounts; called by JSL. Exit: as PlayStartCounts.
 */
void abi_PlayStartCountsLong(void)
{
    abi_PlayStartCounts();  /* 8180DC jsr */
    return;  /* 8180DF rtl */
}

/*
 * @routine 81:8066 DemoStart
 * WHAT IT DOES: Sets a demo up to play: demo $033F's level, and its pad
 * stream from the first entry.
 * HOW: The play mode = 2 (the pad read from the demo's stream) with 1
 * frame left on the current entry; the stream's offset from the word table
 * at $B724 (the demo * 2); the progress sequence, the stage and $0360
 * from the byte tables at $B6FD, $B70A and $B717.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: A low byte =
 * $0360's new value; X = the demo; Y = the demo * 2; N, Z from the last
 * load; C from the doubling; V unchanged.
 */
void abi_DemoStart(void)
{
    uint8_t demo = rd8(dba(DEMO_NUMBER)), v;

    a8_load(0x02);
    wr8(dba(PLAY_MODE), 0x02);
    a8_load(0x01);
    wr8(dba(DEMO_INPUT_FRAMES), 0x01);
    /* the demo's inputs, and its sequence, stage and $0360 */
    x8_load(demo);
    flag_set(P_C, demo & 0x80);
    cpu.y = (uint8_t)(demo << 1);
    wr16(dba(DEMO_INPUT_AT), rd16(dba(DEMO_INPUTS) + cpu.y));
    wr8(dba(LEVEL_035C), rd8(dba(DEMO_SEQUENCES) + demo));
    wr8(dba(0x035D), rd8(dba(DEMO_STAGES) + demo));
    v = rd8(dba(DEMO_0360) + demo);
    a8_load(v);
    wr8(dba(0x0360), v);
}

/*
 * @routine 81:80E0 PlayStartCounts
 * WHAT IT DOES: Sets what play starts with: the random number's seed and
 * full health; with the cheat flag at $0009 set, 10 health marks and 99
 * at $0369.
 * HOW: Random = $C3, $032C = 1, $032D = 0; when $0009 is set, health
 * marks = 10, $036E = 1 and $0369 = $99; health = the health marks;
 * $0347, $036C and $035A cleared.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: A low byte =
 * the health; N, Z from it; X, Y, C, V unchanged.
 */
void abi_PlayStartCounts(void)
{
    uint8_t cheat, health;

    wr8(dba(RANDOM), 0xC3);
    wr8(dba(0x032C), 0x01);
    wr8(dba(0x032D), 0x00);
    cheat = rd8(dba(CHEAT_0009));
    a8_load(cheat);
    if (cheat) {                          /* 10 marks, the 2x2 mark, 99 apples */
        wr8(dba(HEALTH_MARKS), 0x0A);
        wr8(dba(0x036E), 0x01);
        wr8(dba(0x0369), 0x99);
    }
    health = rd8(dba(HEALTH_MARKS));
    a8_load(health);
    wr8(dba(HEALTH), health);
    wr8(dba(0x0347), 0);
    wr8(dba(0x036C), 0);
    wr8(dba(0x035A), 0);
}

/*
 * @routine 81:8114 NewGameStateLong
 * WHAT IT DOES: Sets the counts a new game starts with, and the first
 * level, for callers in other banks.
 * HOW: NewGameState.
 * Entry: as NewGameState; called by JSL. Exit: as NewGameState.
 */
void abi_NewGameStateLong(void)
{
    abi_NewGameState();  /* 818114 jsr */
    return;  /* 818117 rtl */
}

/*
 * @routine 81:8118 NewGameState
 * WHAT IT DOES: Sets the counts a new game starts with, and the first
 * level.
 * HOW: LevelCountersClear; LevelFromProgress; three bytes from $0362 and
 * three from $0365 = 3, $0368-$0369 = $10, and $036A-$036E, $0358 and
 * $035A cleared.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: A low byte =
 * $10; X = the level; N, Z from the last load; Y, C, V as LevelCountersClear
 * and LevelFromProgress leave them.
 */
void abi_NewGameState(void)
{
    static const uint16_t threes[6] = { 0x0362, 0x0363, 0x0364, 0x0365, 0x0366, 0x0367 };
    static const uint16_t zeros[7] = { 0x036B, 0x036A, 0x036E, 0x036D, 0x0358, 0x036C, 0x035A };

    abi_LevelCountersClear();
    abi_LevelFromProgress();
    for (int i = 0; i < 6; i++)           /* 3 lives, health 3, both shown */
        wr8(dba(threes[i]), 0x03);
    wr8(dba(0x0369), 0x10);               /* 10 apples, shown */
    wr8(dba(0x0368), 0x10);
    for (int i = 0; i < 7; i++)
        wr8(dba(zeros[i]), 0x00);
    a8_load(0x10);
}

/*
 * @routine 81:8157 LevelFromProgress
 * WHAT IT DOES: Sets the level from the game's progress: the stage of the
 * level sequence it has reached.
 * HOW: The sequence's offset in the table at $B73E plus the stage indexes
 * the table again; that byte is the level.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: A low byte = the
 * level; X = its index in the table; N, Z from the level;
 * C, V from the ADC; Y unchanged.
 */
void abi_LevelFromProgress(void)
{
    uint8_t at, level, ofs;

    x8_load(rd8(dba(LEVEL_035C)));
    flag_set(P_C, 0);
    ofs = rd8(dba(LEVEL_BY_PROGRESS) + cpu.x);
    at = op_adc8(ofs, rd8(dba(0x035D)), &cpu.p);
    cpu.x = at;
    level = rd8(dba(LEVEL_BY_PROGRESS) + at);
    a8_load(level);
    wr8(dba(LEVEL), level);
}

/*
 * @routine 81:8169 NextPartFadeTask
 * WHAT IT DOES: The game task's next part of play after a fade: the sound
 * faded (SoundQueueF1), the screen darkened by BrightnessDownTask in slot
 * 1, and once that has ended, on into NextPartTask. Nothing in the game
 * starts it.
 * HOW: SoundQueueF1; TaskCreate (slot 1, parameter 4, BrightnessDownTask);
 * TaskWait a frame at a time until slot 1's state is 0; then on into
 * NextPartTask.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; the
 * game task's code. Exit: never returns.
 */
void abi_NextPartFadeTask(void)
{
    jsl_frame(0x818169u); abi_SoundQueueF1(); cpu.s = (uint16_t)(cpu.s + 3);
    /* BrightnessDownTask, slot 1, parameter 4 */
    jsl_frame(0x81816Du); abi_TaskCreate(); cpu.unwound = 0;
    do {
        lda8_v(0x01);
        jsl_frame(0x818178u); abi_TaskWait();
        lda8(ea_abs((uint16_t)(TASK_STATE + 1 * 24)));
    } while (!IF_Z);
    abi_NextPartTask();
}

/*
 * @routine 81:8181 NextPartTask
 * WHAT IT DOES: The game task's start of the next part of play (the story
 * card's TaskExec, $81:F2D3): the level from the game's progress, then the
 * play loop from its start.
 * HOW: LevelFromProgress; $0359 and $035A cleared; TaskExec with the entry
 * in the three bytes after its JSL (the play loop's start, $81:8191).
 * Entry: M 8-bit, X 8-bit; the task's start (TaskExec). Exit: never
 * returns.
 */
void abi_NextPartTask(void)
{
    call_frame(0x8183, abi_LevelFromProgress);  /* 818181 jsr */
    wr8(dba(0x0359), 0x00);
    wr8(dba(0x035A), 0x00);
    jsl_frame(0x81818Au);
    abi_TaskExec();
}
