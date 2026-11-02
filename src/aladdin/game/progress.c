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
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte =
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
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte = the
 * level; X = its index in the table; N, Z from the level;
 * C, V from the ADC; Y unchanged.
 */
void abi_LevelFromProgress(void)
{
    uint8_t at, level;

    x8_load(rd8(dba(LEVEL_035C)));
    flag_set(P_C, 0);
    at = op_adc8(rd8(dba(LEVEL_BY_PROGRESS) + cpu.x), rd8(dba(0x035D)), &cpu.p);
    cpu.x = at;
    level = rd8(dba(LEVEL_BY_PROGRESS) + at);
    a8_load(level);
    wr8(dba(LEVEL), level);
}
