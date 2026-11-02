/*
 * progress.c: the state a new game starts with, and the level the game's
 * progress leads to.
 */
#include "machine.h"
#include "ops65.h"
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
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte =
 * $0360's new value; X = the demo; Y = the demo * 2; N, Z from the last
 * load; C from the doubling; V unchanged.
 */
void abi_DemoStart(void)
{
    lda8_v(0x02);  /* 818066 lda */
    sta8(ea_abs(PLAY_MODE));  /* 818068 sta */
    lda8_v(0x01);  /* 81806B lda */
    sta8(ea_abs(DEMO_INPUT_FRAMES));  /* 81806D sta */
    ldx8(ea_abs(DEMO_NUMBER));  /* 818070 ldx */
    txa8();  /* 818073 txa */
    ACC8(asl);  /* 818074 asl */
    tay8();  /* 818075 tay */
    lda8(ea_absy(DEMO_INPUTS));  /* 818076 lda */
    sta8(ea_abs(DEMO_INPUT_AT));  /* 818079 sta */
    lda8(ea_absy(DEMO_INPUTS + 1));  /* 81807C lda */
    sta8(ea_abs(DEMO_INPUT_AT + 1));  /* 81807F sta */
    lda8(ea_absx(DEMO_SEQUENCES));  /* 818082 lda */
    sta8(ea_abs(LEVEL_035C));  /* 818085 sta */
    lda8(ea_absx(DEMO_STAGES));  /* 818088 lda */
    sta8(ea_abs(0x035D));  /* 81808B sta */
    lda8(ea_absx(DEMO_0360));  /* 81808E lda */
    sta8(ea_abs(0x0360));  /* 818091 sta */
    return;  /* 818094 rts */
}

/*
 * @routine 81:80E0 PlayStartCounts
 * WHAT IT DOES: Sets what play starts with: the random number's seed and
 * full health; with the cheat flag at $0009 set, 10 health marks and 99
 * at $0369.
 * HOW: Random = $C3, $032C = 1, $032D = 0; when $0009 is set, health
 * marks = 10, $036E = 1 and $0369 = $99; health = the health marks;
 * $0347, $036C and $035A cleared.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: A low byte =
 * the health; N, Z from it; X, Y, C, V unchanged.
 */
void abi_PlayStartCounts(void)
{
    lda8_v(0xC3);  /* 8180E0 lda */
    sta8(ea_abs(RANDOM));  /* 8180E2 sta */
    lda8_v(0x01);  /* 8180E5 lda */
    sta8(ea_abs(0x032C));  /* 8180E7 sta */
    stz8(ea_abs(0x032D));  /* 8180EA stz */
    lda8(ea_abs(CHEAT_0009));  /* 8180ED lda */
    if (IF_Z) goto L_818101;  /* 8180F0 beq */
    lda8_v(0x0A);  /* 8180F2 lda */
    sta8(ea_abs(HEALTH_MARKS));  /* 8180F4 sta */
    lda8_v(0x01);  /* 8180F7 lda */
    sta8(ea_abs(0x036E));  /* 8180F9 sta */
    lda8_v(0x99);  /* 8180FC lda */
    sta8(ea_abs(0x0369));  /* 8180FE sta */
L_818101:
    lda8(ea_abs(HEALTH_MARKS));  /* 818101 lda */
    sta8(ea_abs(HEALTH));  /* 818104 sta */
    stz8(ea_abs(0x0347));  /* 818107 stz */
    stz8(ea_abs(0x036C));  /* 81810A stz */
    stz8(ea_abs(0x035A));  /* 81810D stz */
    stz8(ea_abs(0x036C));  /* 818110 stz */
    return;  /* 818113 rts */
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
    abi_LevelCountersClear();  /* 818118 jsl */
    abi_LevelFromProgress();  /* 81811C jsr */
    lda8_v(0x03);  /* 81811F lda */
    sta8(ea_abs(0x0362));  /* 818121 sta */
    sta8(ea_abs(0x0363));  /* 818124 sta */
    sta8(ea_abs(0x0364));  /* 818127 sta */
    lda8_v(0x03);  /* 81812A lda */
    sta8(ea_abs(0x0365));  /* 81812C sta */
    sta8(ea_abs(0x0366));  /* 81812F sta */
    sta8(ea_abs(0x0367));  /* 818132 sta */
    lda8_v(0x10);  /* 818135 lda */
    sta8(ea_abs(0x0369));  /* 818137 sta */
    sta8(ea_abs(0x0368));  /* 81813A sta */
    stz8(ea_abs(0x036B));  /* 81813D stz */
    stz8(ea_abs(0x036A));  /* 818140 stz */
    stz8(ea_abs(0x036E));  /* 818143 stz */
    stz8(ea_abs(0x036D));  /* 818146 stz */
    stz8(ea_abs(0x0358));  /* 818149 stz */
    stz8(ea_abs(0x036C));  /* 81814C stz */
    stz8(ea_abs(0x035A));  /* 81814F stz */
    return;  /* 818152 rts */
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
    ldx8(ea_abs(LEVEL_035C));  /* 818157 ldx */
    clc();  /* 81815A clc */
    lda8(ea_absx(0xB73E));  /* 81815B lda */
    adc8_v(rd8(ea_abs(0x035D)));  /* 81815E adc */
    tax8();  /* 818161 tax */
    lda8(ea_absx(0xB73E));  /* 818162 lda */
    sta8(ea_abs(LEVEL));  /* 818165 sta */
    return;  /* 818168 rts */
}
