/*
 * level_select.c: the level select (a development mode): L and R held
 * with a direction step the level part and stage, and the level text
 * shows them.
 */
#include "machine.h"
#include "ops65.h"
void abi_LevelNumberText(void);
void abi_LevelSelect(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 80:A0B1 LevelNumberText
 * WHAT IT DOES: Writes the level's part and stage into the text layer (or
 * a fixed message for part 6, and for part 5 stage 3), and has the NMI
 * upload it.
 * HOW: Part 6: message $5E; part 5 stage 3: message $60 (by $81:972D).
 * Else the two digits ($2031 + each) at $7E:DC1C and $7E:DC20, a '.'
 * between, spaces round; text_tilemap_upload up.
 * Entry: M 8-bit; DB = $80; called by JSR. Exit: M 8-bit.
 */
void abi_LevelNumberText(void)
{
    lda8(ea_abs(LEVEL_035C));  /* 80A0B1 lda */
    cmp8_v(0x06);  /* 80A0B4 cmp */
    if (IF_Z) goto L_80A0FB;  /* 80A0B6 beq */
    op_rep(0x20);  /* 80A0B8 rep */
    lda16(ea_abs(LEVEL_035C));  /* 80A0BA lda */
    cmp16_v(0x0305);  /* 80A0BD cmp */
    if (IF_Z) goto L_80A0F2;  /* 80A0C0 beq */
    and16_v(0x000F);  /* 80A0C2 and */
    clc();  /* 80A0C5 clc */
    adc16_v(0x2031);  /* 80A0C6 adc */
    sta16(0x7EDC1Cu);  /* 80A0C9 sta */
    lda16(ea_abs(0x035D));  /* 80A0CD lda */
    and16_v(0x000F);  /* 80A0D0 and */
    adc16_v(0x2031);  /* 80A0D3 adc */
    sta16(0x7EDC20u);  /* 80A0D6 sta */
    lda16_v(0x202E);  /* 80A0DA lda */
    sta16(0x7EDC1Eu);  /* 80A0DD sta */
    lda16_v(0x2020);  /* 80A0E1 lda */
    sta16(0x7EDC1Au);  /* 80A0E4 sta */
    sta16(0x7EDC22u);  /* 80A0E8 sta */
    op_sep(0x20);  /* 80A0EC sep */
    MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));  /* 80A0EE inc */
    return;  /* 80A0F1 rts */
L_80A0F2:
    op_sep(0x20);  /* 80A0F2 sep */
    lda8_v(0x60);  /* 80A0F4 lda */
    call_routine(0x81972Du, 1);  /* 80A0F6 jsl */
    return;  /* 80A0FA rts */
L_80A0FB:
    lda8_v(0x5E);  /* 80A0FB lda */
    call_routine(0x81972Du, 1);  /* 80A0FD jsl */
    return;  /* 80A101 rts */
}

/*
 * @routine 80:A05C LevelSelect
 * WHAT IT DOES: With L and R held, steps the level part (UP, DOWN) or its
 * stage (RIGHT, LEFT) within its limits and shows the new level: C clear;
 * C set when L and R are not held.
 * HOW: Pad 1 held L and R ($0377 bits 4 and 5) and a direction newly
 * pressed: UP: part up to 6, stage 0; DOWN: part down to 0, stage 0;
 * RIGHT: stage up to the part's last ($80:CE2E); LEFT: stage down to 0;
 * LevelNumberText after a stage change.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: C as above.
 */
void abi_LevelSelect(void)
{
    lda8(ea_abs(JOY_HELD));  /* 80A05C lda */
    and8_v(0x30);  /* 80A05F and */
    if (IF_Z) goto L_80A0AF;  /* 80A061 beq */
    lda8(ea_abs(0x037C));  /* 80A063 lda */
    and8_v(0x0F);  /* 80A066 and */
    if (IF_Z) goto L_80A0AF;  /* 80A068 beq */
    bit8_i(0x08);  /* 80A06A bit */
    if (IF_Z) goto L_80A07B;  /* 80A06C beq */
    ldx8(ea_abs(LEVEL_035C));  /* 80A06E ldx */
    cpx8_v(0x06);  /* 80A071 cpx */
    if (IF_Z) goto L_80A0AD;  /* 80A073 beq */
    MEM8(inc, ea_abs(LEVEL_035C));  /* 80A075 inc */
    stz8(ea_abs(0x035D));  /* 80A078 stz */
L_80A07B:
    bit8_i(0x04);  /* 80A07B bit */
    if (IF_Z) goto L_80A08A;  /* 80A07D beq */
    ldx8(ea_abs(LEVEL_035C));  /* 80A07F ldx */
    if (IF_Z) goto L_80A0AD;  /* 80A082 beq */
    MEM8(dec, ea_abs(LEVEL_035C));  /* 80A084 dec */
    stz8(ea_abs(0x035D));  /* 80A087 stz */
L_80A08A:
    bit8_i(0x01);  /* 80A08A bit */
    if (IF_Z) goto L_80A09E;  /* 80A08C beq */
    ldx8(ea_abs(LEVEL_035C));  /* 80A08E ldx */
    lda8(ea_abs(0x035D));  /* 80A091 lda */
    cmp8_v(rd8(ea_absx(0xCE2E)));  /* 80A094 cmp */
    if (IF_Z) goto L_80A0AD;  /* 80A097 beq */
    MEM8(inc, ea_abs(0x035D));  /* 80A099 inc */
    goto L_80A0AA;  /* 80A09C bra */
L_80A09E:
    bit8_i(0x02);  /* 80A09E bit */
    if (IF_Z) goto L_80A0AA;  /* 80A0A0 beq */
    ldx8(ea_abs(0x035D));  /* 80A0A2 ldx */
    if (IF_Z) goto L_80A0AD;  /* 80A0A5 beq */
    MEM8(dec, ea_abs(0x035D));  /* 80A0A7 dec */
L_80A0AA:
    call_routine(0x80A0B1u, 0);  /* 80A0AA jsr */
L_80A0AD:
    clc();  /* 80A0AD clc */
    return;  /* 80A0AE rts */
L_80A0AF:
    sec();  /* 80A0AF sec */
    return;  /* 80A0B0 rts */
}
