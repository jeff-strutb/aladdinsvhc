/*
 * screen_tasks.c: tasks that change the screen over many frames (fades,
 * palette sequences, layer shakes), and runs of text-layer words.
 */
#include "machine.h"
#include "ops65.h"
void abi_TextLayerCopyWords(void);
void abi_TextLayerBlankWords(void);
void abi_PaletteSequenceTask(void);
void abi_FadeInTask(void);
void abi_FadeOutTask(void);
void abi_ShakeTask(void);
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
 * @routine 81:9E75 PaletteSequenceTask
 * WHAT IT DOES: A task that loads six palette sets in turn, each held for
 * its own number of frames, and then ends.
 * HOW: For steps 5 down to 0: PaletteLoad of the step's set, then
 * TaskWait for its frames; then TaskExit.
 * Entry: a task's entry; M 8-bit, X 8-bit; DB = $81. Exit: never returns
 * (TaskExit).
 */
void abi_PaletteSequenceTask(void)
{
    ldy8_v(0x05);  /* 819E75 ldy */
L_819E77:
    lda8(ea_absy(0xBA6A));  /* 819E77 lda */
    phy8();  /* 819E7A phy */
    abi_PaletteLoad();  /* 819E7B jsl */
    ply8();  /* 819E7F ply */
    lda8(ea_absy(0xBA70));  /* 819E80 lda */
    jsl_frame(0x819E83u); abi_TaskWait();  /* 819E83 jsl */
    dey8();  /* 819E87 dey */
    if (!IF_N) goto L_819E77;  /* 819E88 bpl */
    abi_TaskExit(); return;  /* 819E8A jml */
}

/*
 * @routine 81:9F0E FadeInTask
 * WHAT IT DOES: A task that fades the screen in from black, then ends.
 * HOW: ColourFadeBegin; one step at grey 31 (black), a frame, the screen
 * on at full brightness; then steps from grey 31 down to 0, the task's
 * frames apart; the colour window off; TaskExit.
 * Entry: a task's entry; M 8-bit, X 8-bit; Y = its slot; DB = $81. Exit:
 * never returns (TaskExit).
 */
void abi_FadeInTask(void)
{
    abi_ColourFadeBegin();  /* 819F0E jsr */
    ldx8_v(0x1F);  /* 819F11 ldx */
    push16(0x9F15); abi_ColourFadeStep();  /* 819F13 jsr */
    lda8_v(0x01);  /* 819F16 lda */
    jsl_frame(0x819F18u); abi_TaskWait();  /* 819F18 jsl */
    abi_ScreenFullBright();  /* 819F1C jsl */
    ldx8_v(0x1F);  /* 819F20 ldx */
L_819F22:
    push16(0x9F24); abi_ColourFadeStep();  /* 819F22 jsr */
    dex8();  /* 819F25 dex */
    if (!IF_N) goto L_819F22;  /* 819F26 bpl */
    goto L_819F5F;  /* 819F28 bra */
L_819F5F:
    stz8(ea_abs(WOBJSEL_SHADOW));  /* 819F5F stz */
    abi_TaskExit(); return;  /* 819F62 jml */
}

/*
 * @routine 81:9F52 FadeOutTask
 * WHAT IT DOES: A task that fades the screen out to black, then ends.
 * HOW: ColourFadeBegin; steps from grey 0 up to 31, the task's frames
 * apart; the colour window off; TaskExit.
 * Entry: a task's entry; M 8-bit, X 8-bit; Y = its slot; DB = $81. Exit:
 * never returns (TaskExit).
 */
void abi_FadeOutTask(void)
{
    abi_ColourFadeBegin();  /* 819F52 jsr */
    ldx8_v(0x00);  /* 819F55 ldx */
L_819F57:
    push16(0x9F59); abi_ColourFadeStep();  /* 819F57 jsr */
    inx8();  /* 819F5A inx */
    cpx8_v(0x20);  /* 819F5B cpx */
    if (!IF_Z) goto L_819F57;  /* 819F5D bne */
    stz8(ea_abs(WOBJSEL_SHADOW));  /* 819F5F stz */
    abi_TaskExit(); return;  /* 819F62 jml */
}

/*
 * @routine 81:A1D5 ShakeTask
 * WHAT IT DOES: A task that shakes a layer up and down, then ends.
 * HOW: With its slot as the direct page, its parameter picks a shake
 * preset whose four bytes (layer, sways, frames a half-sway, step) go to
 * its record; per sway, ShakeUp then ShakeDown; then TaskExit.
 * Entry: a task's entry; M 8-bit, X 8-bit; Y = its slot; DB = $81. Exit:
 * never returns (TaskExit).
 */
void abi_ShakeTask(void)
{
    lda8_v(0x00);  /* 81A1D5 lda */
    xba();  /* 81A1D7 xba */
    tya8();  /* 81A1D8 tya */
    tcd();  /* 81A1D9 tcd */
    ldx8(ea_dp(0x75));  /* 81A1DA ldx */
    lda8(ea_absx(0xBAA3));  /* 81A1DC lda */
    sta8(ea_dp(0x75));  /* 81A1DF sta */
    lda8(ea_absx(0xBAA4));  /* 81A1E1 lda */
    sta8(ea_dp(0x76));  /* 81A1E4 sta */
    lda8(ea_absx(0xBAA5));  /* 81A1E6 lda */
    sta8(ea_dp(0x77));  /* 81A1E9 sta */
    lda8(ea_absx(0xBAA6));  /* 81A1EB lda */
    sta8(ea_dp(0x78));  /* 81A1EE sta */
L_81A1F0:
    push16(0xA1F2); abi_ShakeUp();  /* 81A1F0 jsr */
    push16(0xA1F5); abi_ShakeDown();  /* 81A1F3 jsr */
    MEM8(dec, ea_dp(0x76));  /* 81A1F6 dec */
    if (!IF_Z) goto L_81A1F0;  /* 81A1F8 bne */
    abi_TaskExit(); return;  /* 81A1FA jml */
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
