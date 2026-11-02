/*
 * progress.c: the state a new game starts with, and the level the game's
 * progress leads to.
 */
#include "machine.h"
#include "ops65.h"
void abi_NewGameState(void);
void abi_LevelFromProgress(void);
void abi_LevelCountersClear(void);
#include "wram.h"
#include "wram_video.h"

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
