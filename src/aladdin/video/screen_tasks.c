/*
 * screen_tasks.c: tasks that change the screen over many frames (fades,
 * palette sequences, layer shakes), and runs of text-layer words.
 */
#include "machine.h"
#include "ops65.h"
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
