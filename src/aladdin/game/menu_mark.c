/*
 * menu_mark.c: the selection mark of a menu drawn in the text layer: taken
 * away from the choice left; and the title menu's cursor moved. Drafted
 * by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"

void abi_SoundQueueAdd(void);

/*
 * @routine 81:EF15 MenuMarkErase
 * WHAT IT DOES: Takes the selection mark away from a menu choice: blanks
 * its seven tiles in the text layer's tilemap.
 * HOW: Tile word $0020 (a blank) stored at the choice's offset plus 2 (row
 * 0), $40, $42, $44 (row 1) and $80, $82, $84 (row 2), with 16-bit A; P
 * kept.
 * Entry: X 16-bit = the choice's byte offset in the tilemap; called by JSL
 * (from the menu's cursor move, 81:EE04, which draws the mark at the new
 * choice by 81:EF39). Exit: P as at entry; A = $0020.
 */
void abi_MenuMarkErase(void)
{
    php();  /* 81EF15 php */
    op_rep(0x20);  /* 81EF16 rep */
    lda16_v(0x0020);  /* 81EF18 lda */
    sta16(ea_longx(0x7ED802u));  /* 81EF1B sta */
    sta16(ea_longx(0x7ED840u));  /* 81EF1F sta */
    sta16(ea_longx(0x7ED842u));  /* 81EF23 sta */
    sta16(ea_longx(0x7ED844u));  /* 81EF27 sta */
    sta16(ea_longx(0x7ED880u));  /* 81EF2B sta */
    sta16(ea_longx(0x7ED882u));  /* 81EF2F sta */
    sta16(ea_longx(0x7ED884u));  /* 81EF33 sta */
    plp();  /* 81EF37 plp */
    return;  /* 81EF38 rtl */
}

/*
 * @routine 81:EDA6 TitleMenuCursor
 * WHAT IT DOES: Moves the title screen menu's cursor on Up or Down
 * (wrapping round the three choices), with its mark, picture and sound;
 * anything else, the menu's other step.
 * HOW: Pad 1's newly pressed high byte: Up, the choice ($0849) down by 1
 * (wrapping to 2); Down, up by 1 (wrapping to 0); neither, the step at
 * 81:EE55 only. Moved: the mark moved (81:EE04), the step at 81:EE55, the
 * call at 81:972D with the old choice's byte from $CE25, $0847-$0848
 * cleared, the old choice kept ($084A), $0343 raised to at least $04EC,
 * sound $0C.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR (81:EDA2). Exit: M
 * 8-bit; as SoundQueueAdd, or as the step at 81:EE55.
 */
void abi_TitleMenuCursor(void)
{
    lda8(ea_abs(0x037C));  /* 81EDA6 lda */
    bit8_i(0x08);  /* 81EDA9 bit */
    if (!IF_Z) goto L_81EDC4;  /* 81EDAB bne */
    bit8_i(0x04);  /* 81EDAD bit */
    if (!IF_Z) goto L_81EDB5;  /* 81EDAF bne */
    call_routine(0x81EE55u, 0);  /* 81EDB1 jsr */
    return;  /* 81EDB4 rts */
L_81EDB5:
    MEM8(inc, ea_abs(0x0849));  /* 81EDB5 inc */
    lda8(ea_abs(0x0849));  /* 81EDB8 lda */
    cmp8_v(0x03);  /* 81EDBB cmp */
    if (!IF_Z) goto L_81EDD1;  /* 81EDBD bne */
    stz8(ea_abs(0x0849));  /* 81EDBF stz */
    goto L_81EDD1;  /* 81EDC2 bra */
L_81EDC4:
    MEM8(dec, ea_abs(0x0849));  /* 81EDC4 dec */
    lda8(ea_abs(0x0849));  /* 81EDC7 lda */
    if (!IF_N) goto L_81EDD1;  /* 81EDCA bpl */
    lda8_v(0x02);  /* 81EDCC lda */
    sta8(ea_abs(0x0849));  /* 81EDCE sta */
L_81EDD1:
    call_routine(0x81EE04u, 1);  /* 81EDD1 jsl */
    call_routine(0x81EE55u, 0);  /* 81EDD5 jsr */
    ldy8(ea_abs(0x084A));  /* 81EDD8 ldy */
    lda8(ea_absy(0xCE25));  /* 81EDDB lda */
    call_routine(0x81972Du, 1);  /* 81EDDE jsl */
    stz8(ea_abs(0x0847));  /* 81EDE2 stz */
    stz8(ea_abs(0x0848));  /* 81EDE5 stz */
    lda8(ea_abs(0x0849));  /* 81EDE8 lda */
    sta8(ea_abs(0x084A));  /* 81EDEB sta */
    op_rep(0x20);  /* 81EDEE rep */
    lda16_v(0x04EC);  /* 81EDF0 lda */
    cmp16_v(rd16(ea_abs(V_0343)));  /* 81EDF3 cmp */
    if (!IF_C) goto L_81EDFB;  /* 81EDF6 bcc */
    sta16(ea_abs(V_0343));  /* 81EDF8 sta */
L_81EDFB:
    op_sep(0x20);  /* 81EDFB sep */
    lda8_v(0x0C);  /* 81EDFD lda */
    abi_SoundQueueAdd();  /* 81EDFF jsl */
    return;  /* 81EE03 rts */
}

