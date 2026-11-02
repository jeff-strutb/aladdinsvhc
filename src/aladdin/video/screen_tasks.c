/*
 * screen_tasks.c: tasks that change the screen over many frames (fades,
 * palette sequences, layer shakes), and runs of text-layer words.
 */
#include "machine.h"
#include "ops65.h"
void abi_TextLayoutDraw(void);
void abi_BrightnessUpTask(void);
void abi_BrightnessDownTask(void);
void abi_TextLayerCopyWords(void);
void abi_TextLayerBlankWords(void);
void abi_RandomFromList(void);
void abi_TaskWait(void);
void abi_TaskExit(void);
void abi_ShakeUp(void);
void abi_ShakeDown(void);
void abi_ScreenFullBright(void);
void abi_PaletteLoad(void);
void abi_ColourFadeStep(void);
void abi_ColourFadeBegin(void);
void abi_RandomNext(void);
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:9774 TextLayerCopyWords
 * WHAT IT DOES: Copies a run of words into the text layer's tilemap, or
 * repeats one word along it.
 * HOW: Scratch $0C words: each word at the data bank's Y goes to the
 * tilemap at X; X steps a word each time, Y only while C is clear (C set:
 * the same word all along).
 * Entry: M 16-bit, X 16-bit; X = the tilemap's byte offset; Y = the
 * source; C set to repeat; scratch $0C = the count (nonzero); called by
 * JSR. Exit: X, Y past the run; scratch $0C = 0; Z set, N clear; A = the
 * last word; C unchanged.
 */
void abi_TextLayerCopyWords(void)
{
L_819774:
    lda16(ea_absy(0x0000));  /* 819774 lda */
    sta16(ea_longx(TEXT_TILEMAP));  /* 819777 sta */
    if (IF_C) goto L_81977F;  /* 81977B bcs */
    iny16();  /* 81977D iny */
    iny16();  /* 81977E iny */
L_81977F:
    inx16();  /* 81977F inx */
    inx16();  /* 819780 inx */
    MEM16(dec, ea_abs(SCRATCH));  /* 819781 dec */
    if (!IF_Z) goto L_819774;  /* 819784 bne */
    return;  /* 819786 rts */
}

/*
 * @routine 81:9787 TextLayerBlankWords
 * WHAT IT DOES: Blanks a run of the text layer's tilemap.
 * HOW: Scratch $0C words of $0020 (the blank tile) from X; X steps a word
 * each time, Y too while C is clear.
 * Entry: M 16-bit, X 16-bit; X = the tilemap's byte offset; scratch $0C =
 * the count (nonzero); called by JSR. Exit: X past the run; Y stepped as
 * X when C is clear; scratch $0C = 0; A = $0020; Z set, N clear; C
 * unchanged.
 */
void abi_TextLayerBlankWords(void)
{
L_819787:
    lda16_v(0x0020);  /* 819787 lda */
    sta16(ea_longx(TEXT_TILEMAP));  /* 81978A sta */
    if (IF_C) goto L_819792;  /* 81978E bcs */
    iny16();  /* 819790 iny */
    iny16();  /* 819791 iny */
L_819792:
    inx16();  /* 819792 inx */
    inx16();  /* 819793 inx */
    MEM16(dec, ea_abs(SCRATCH));  /* 819794 dec */
    if (!IF_Z) goto L_819787;  /* 819797 bne */
    return;  /* 819799 rts */
}





/*
 * @routine 81:AE8A RandomFromList
 * WHAT IT DOES: Picks a random entry from list X.
 * HOW: The list (its offset at $DCE7 + X) starts with a mask; RandomNext
 * masked by it picks the entry, a byte after the mask.
 * Entry: X 8-bit; X = the list's number * 2; DB = $81. Exit: M and X
 * 8-bit; Y = A low byte = the entry; A high byte = the byte after it;
 * scratch $1C = the masked random (word); N, Z from the entry; C, V from
 * the ADC.
 */
void abi_RandomFromList(void)
{
    op_rep(0x30);  /* 81AE8A rep */
    lda16(ea_absx(0xDCE7));  /* 81AE8C lda */
    tay16();  /* 81AE8F tay */
    op_sep(0x20);  /* 81AE90 sep */
    lda8(ea_absy(0xDCE7));  /* 81AE92 lda */
    sta8(ea_abs(0x001C));  /* 81AE95 sta */
    abi_RandomNext();  /* 81AE98 jsl */
    and8_v(rd8(ea_abs(0x001C)));  /* 81AE9C and */
    op_rep(0x20);  /* 81AE9F rep */
    and16_v(0x00FF);  /* 81AEA1 and */
    sta16(ea_abs(0x001C));  /* 81AEA4 sta */
    tya16();  /* 81AEA7 tya */
    clc();  /* 81AEA8 clc */
    adc16_v(rd16(ea_abs(0x001C)));  /* 81AEA9 adc */
    tay16();  /* 81AEAC tay */
    lda16(ea_absy(0xDCE8));  /* 81AEAD lda */
    op_sep(0x30);  /* 81AEB0 sep */
    tay8();  /* 81AEB2 tay */
    return;  /* 81AEB3 rtl */
}

/*
 * @routine 81:9734 TextLayoutDraw
 * WHAT IT DOES: Draws text layout A (low 7 bits) into the text layer's
 * tilemap, or blanks the places it covers (bit 7 set).
 * HOW: With DB = $82, the layout's offset from the word table at $B575;
 * then runs until a 0 byte: a count byte (low 6 bits; bit 6 set: one word
 * repeated), the tilemap offset (word), and the words (one when
 * repeated), copied by TextLayerCopyWords or blanked by
 * TextLayerBlankWords; A is kept at scratch $0F for the blank test.
 * Entry: M 8-bit, X 8-bit; A low byte = the layout; called by JSR. Exit:
 * M and X 8-bit; DB as at entry, N and Z from it (PLB); X, Y as the last
 * run left them (low bytes); scratch $0C = 0.
 */
void abi_TextLayoutDraw(void)
{
    sta8(ea_abs(0x000F));  /* 819734 sta */
    ACC8(asl);  /* 819737 asl */
    ACC8(lsr);  /* 819738 lsr */
    tax8();  /* 819739 tax */
    phb();  /* 81973A phb */
    lda8_v(0x82);  /* 81973B lda */
    pha8();  /* 81973D pha */
    plb();  /* 81973E plb */
    op_rep(0x10);  /* 81973F rep */
    ldy16(ea_absx(0xB575));  /* 819741 ldy */
L_819744:
    op_sep(0x20);  /* 819744 sep */
    lda8(ea_absy(0x0000));  /* 819746 lda */
    if (IF_Z) goto L_819770;  /* 819749 beq */
    iny16();  /* 81974B iny */
    pha8();  /* 81974C pha */
    ACC8(rol);  /* 81974D rol */
    ACC8(rol);  /* 81974E rol */
    pla8();  /* 81974F pla */
    op_rep(0x20);  /* 819750 rep */
    and16_v(0x003F);  /* 819752 and */
    sta16(ea_abs(SCRATCH));  /* 819755 sta */
    ldx16(ea_absy(0x0000));  /* 819758 ldx */
    iny16();  /* 81975B iny */
    iny16();  /* 81975C iny */
    lda16(ea_abs(0x000E));  /* 81975D lda */
    if (!IF_N) goto L_819767;  /* 819760 bpl */
    abi_TextLayerBlankWords();  /* 819762 jsr */
    goto L_81976A;  /* 819765 bra */
L_819767:
    abi_TextLayerCopyWords();  /* 819767 jsr */
L_81976A:
    if (!IF_C) goto L_819744;  /* 81976A bcc */
    iny16();  /* 81976C iny */
    iny16();  /* 81976D iny */
    if (IF_C) goto L_819744;  /* 81976E bcs */
L_819770:
    op_sep(0x30);  /* 819770 sep */
    plb();  /* 819772 plb */
    return;  /* 819773 rts */
}

/*
 * @routine 81:9E8E BrightnessUpTask
 * WHAT IT DOES: A task that raises the screen's brightness a step at a
 * time to full, then ends.
 * HOW: The brightness shadow's low 4 bits kept; at 15 already it ends;
 * else the parameter is halved (at least 1) and, for each step, TaskWait
 * for that many frames, then the brightness up by one, until 15; then
 * TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_BrightnessUpTask(void)
{
    lda8(ea_abs(INIDISP_SHADOW));  /* 819E8E lda */
    and8_v(0x0F);  /* 819E91 and */
    sta8(ea_abs(INIDISP_SHADOW));  /* 819E93 sta */
    cmp8_v(0x0F);  /* 819E96 cmp */
    if (IF_Z) goto L_819EB5;  /* 819E98 beq */
    lda8(ea_absy(TASK_PARAM));  /* 819E9A lda */
    ACC8(lsr);  /* 819E9D lsr */
    if (!IF_Z) goto L_819EA1;  /* 819E9E bne */
    ACC8(inc);  /* 819EA0 inc */
L_819EA1:
    sta8(ea_absy(TASK_PARAM));  /* 819EA1 sta */
L_819EA4:
    lda8(ea_absy(TASK_PARAM));  /* 819EA4 lda */
    jsl_frame(0x819EA7u); abi_TaskWait();  /* 819EA7 jsl */
    MEM8(inc, ea_abs(INIDISP_SHADOW));  /* 819EAB inc */
    lda8(ea_abs(INIDISP_SHADOW));  /* 819EAE lda */
    cmp8_v(0x0F);  /* 819EB1 cmp */
    if (!IF_Z) goto L_819EA4;  /* 819EB3 bne */
L_819EB5:
    abi_TaskExit(); return;  /* 819EB5 jml */
}

/*
 * @routine 81:9EB9 BrightnessDownTask
 * WHAT IT DOES: A task that lowers the screen's brightness a step at a
 * time to 0 (dark), then ends.
 * HOW: The brightness shadow's low 4 bits kept; at 0 already it ends;
 * else the parameter is halved (at least 1) and, for each step, TaskWait
 * for that many frames, then the brightness down by one, until 0; then
 * TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_BrightnessDownTask(void)
{
    lda8(ea_abs(INIDISP_SHADOW));  /* 819EB9 lda */
    and8_v(0x0F);  /* 819EBC and */
    sta8(ea_abs(INIDISP_SHADOW));  /* 819EBE sta */
    if (IF_Z) goto L_819ED9;  /* 819EC1 beq */
    lda8(ea_absy(TASK_PARAM));  /* 819EC3 lda */
    ACC8(lsr);  /* 819EC6 lsr */
    if (!IF_Z) goto L_819ECA;  /* 819EC7 bne */
    ACC8(inc);  /* 819EC9 inc */
L_819ECA:
    sta8(ea_absy(TASK_PARAM));  /* 819ECA sta */
L_819ECD:
    lda8(ea_absy(TASK_PARAM));  /* 819ECD lda */
    jsl_frame(0x819ED0u); abi_TaskWait();  /* 819ED0 jsl */
    MEM8(dec, ea_abs(INIDISP_SHADOW));  /* 819ED4 dec */
    if (!IF_Z) goto L_819ECD;  /* 819ED7 bne */
L_819ED9:
    abi_TaskExit(); return;  /* 819ED9 jml */
}
