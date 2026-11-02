/*
 * mode7_rotate.c: mode 7's matrix set for a rotation by an angle, from
 * the sine table in bank $82; and a task zooming the picture in.
 */
#include "machine.h"
#include "ops65.h"
void abi_Mode7Rotate(void);
void abi_RotateSine(void);
void abi_Mode7ZoomInTask(void);
void abi_TaskWait(void);
void abi_TaskExit(void);

#define M7A_SHADOW 0x038Du         /* mode 7's matrix as the NMI writes it: A and D (words) */
#define M7D_SHADOW 0x0393u
#define M7_CENTRE_Y 0x0397u        /* mode 7's centre, its y (word) */
#define V_146B 0x146Bu

/*
 * @routine 81:B157 Mode7ZoomInTask
 * WHAT IT DOES: A task that zooms the mode 7 picture in from a speck to
 * full size, the centre drifting up and back, then ends.
 * HOW: After 24 frames, A and D of the matrix (the scale) = $0FF0 and the
 * centre's y = $80; each frame the scale drops by $20 and the centre's y
 * by 1, until the scale's high byte is 1; then each frame the scale drops
 * by $20 and the centre's y rises by 1, until the scale is below $0100;
 * then the scale = $0100 (full size), the centre's y = $40, $146B = $80
 * (purpose not yet established), and TaskExit.
 * Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: none: the task ends.
 */
void abi_Mode7ZoomInTask(void)
{
    lda8_v(0x18);  /* 81B157 lda */
    jsl_frame(0x81B159u); abi_TaskWait();  /* 81B159 jsl */
    op_rep(0x20);  /* 81B15D rep */
    lda16_v(0x0FF0);  /* 81B15F lda */
    sta16(ea_abs(M7A_SHADOW));  /* 81B162 sta */
    sta16(ea_abs(M7D_SHADOW));  /* 81B165 sta */
    lda16_v(0x0080);  /* 81B168 lda */
    sta16(ea_abs(M7_CENTRE_Y));  /* 81B16B sta */
    op_sep(0x20);  /* 81B16E sep */
L_81B170:
    lda8_v(0x01);  /* 81B170 lda */
    jsl_frame(0x81B172u); abi_TaskWait();  /* 81B172 jsl */
    clc();  /* 81B176 clc */
    lda8(ea_abs(M7_CENTRE_Y));  /* 81B177 lda */
    adc8_v(0xFF);  /* 81B17A adc */
    sta8(ea_abs(M7_CENTRE_Y));  /* 81B17C sta */
    sec();  /* 81B17F sec */
    lda8(ea_abs(M7A_SHADOW));  /* 81B180 lda */
    sbc8_v(0x20);  /* 81B183 sbc */
    sta8(ea_abs(M7A_SHADOW));  /* 81B185 sta */
    sta8(ea_abs(M7D_SHADOW));  /* 81B188 sta */
    if (IF_C) goto L_81B170;  /* 81B18B bcs */
    lda8(ea_abs(M7A_SHADOW + 1));  /* 81B18D lda */
    ACC8(dec);  /* 81B190 dec */
    sta8(ea_abs(M7A_SHADOW + 1));  /* 81B191 sta */
    sta8(ea_abs(M7D_SHADOW + 1));  /* 81B194 sta */
    cmp8_v(0x01);  /* 81B197 cmp */
    if (IF_Z) goto L_81B19D;  /* 81B199 beq */
    goto L_81B170;  /* 81B19B bra */
L_81B19D:
    lda8_v(0x01);  /* 81B19D lda */
    jsl_frame(0x81B19Fu); abi_TaskWait();  /* 81B19F jsl */
    clc();  /* 81B1A3 clc */
    lda8(ea_abs(M7_CENTRE_Y));  /* 81B1A4 lda */
    adc8_v(0x01);  /* 81B1A7 adc */
    sta8(ea_abs(M7_CENTRE_Y));  /* 81B1A9 sta */
    sec();  /* 81B1AC sec */
    lda8(ea_abs(M7A_SHADOW));  /* 81B1AD lda */
    sbc8_v(0x20);  /* 81B1B0 sbc */
    sta8(ea_abs(M7A_SHADOW));  /* 81B1B2 sta */
    sta8(ea_abs(M7D_SHADOW));  /* 81B1B5 sta */
    if (IF_C) goto L_81B19D;  /* 81B1B8 bcs */
    stz8(ea_abs(M7A_SHADOW));  /* 81B1BA stz */
    stz8(ea_abs(M7D_SHADOW));  /* 81B1BD stz */
    lda8_v(0x40);  /* 81B1C0 lda */
    sta8(ea_abs(M7_CENTRE_Y));  /* 81B1C2 sta */
    lda8_v(0x80);  /* 81B1C5 lda */
    sta8(ea_abs(V_146B));  /* 81B1C7 sta */
    abi_TaskExit(); return;  /* 81B1CA jmp */
}

/*
 * @routine 81:B1CD Mode7Rotate
 * WHAT IT DOES: Sets mode 7's matrix to a rotation by angle A (256 to a
 * turn): A and D the cosine, B the sine and C its negative.
 * HOW: The angle's place in its quarter gives the table entry of one value
 * (X) and of the other (Y, from the quarter's far end); the quarter (A's
 * top two bits) picks which is the sine and which signs they take.
 * Entry: M 8-bit, X 8-bit; A = the angle; DB = $00-$3F or $80-$BF; called
 * by JSL. Exit: M 8-bit; A as the last store left it (16 bits); X, Y the
 * two entries.
 */
void abi_Mode7Rotate(void)
{
    pha8();  /* 81B1CD pha */
    and8_v(0x3F);  /* 81B1CE and */
    ACC8(asl);  /* 81B1D0 asl */
    tax8();  /* 81B1D1 tax */
    eor8_v(0x7F);  /* 81B1D2 eor */
    ACC8(inc);  /* 81B1D4 inc */
    tay8();  /* 81B1D5 tay */
    pla8();  /* 81B1D6 pla */
    cmp8_v(0xC0);  /* 81B1D7 cmp */
    if (IF_C) goto L_81B23F;  /* 81B1D9 bcs */
    cmp8_v(0x80);  /* 81B1DB cmp */
    if (IF_C) goto L_81B21F;  /* 81B1DD bcs */
    cmp8_v(0x40);  /* 81B1DF cmp */
    if (IF_C) goto L_81B1FF;  /* 81B1E1 bcs */
    op_rep(0x20);  /* 81B1E3 rep */
    abi_RotateSine();  /* 81B1E5 jsr */
    sta16(ea_abs(0x038F));  /* 81B1E8 sta */
    eor16_v(0xFFFF);  /* 81B1EB eor */
    ACC16(inc);  /* 81B1EE inc */
    sta16(ea_abs(0x0391));  /* 81B1EF sta */
    tyx8();  /* 81B1F2 tyx */
    abi_RotateSine();  /* 81B1F3 jsr */
    sta16(ea_abs(0x038D));  /* 81B1F6 sta */
    sta16(ea_abs(0x0393));  /* 81B1F9 sta */
    op_sep(0x20);  /* 81B1FC sep */
    return;  /* 81B1FE rtl */
L_81B1FF:
    op_rep(0x20);  /* 81B1FF rep */
    abi_RotateSine();  /* 81B201 jsr */
    eor16_v(0xFFFF);  /* 81B204 eor */
    ACC16(inc);  /* 81B207 inc */
    sta16(ea_abs(0x038D));  /* 81B208 sta */
    sta16(ea_abs(0x0393));  /* 81B20B sta */
    tyx8();  /* 81B20E tyx */
    abi_RotateSine();  /* 81B20F jsr */
    sta16(ea_abs(0x038F));  /* 81B212 sta */
    eor16_v(0xFFFF);  /* 81B215 eor */
    ACC16(inc);  /* 81B218 inc */
    sta16(ea_abs(0x0391));  /* 81B219 sta */
    op_sep(0x20);  /* 81B21C sep */
    return;  /* 81B21E rtl */
L_81B21F:
    op_rep(0x20);  /* 81B21F rep */
    abi_RotateSine();  /* 81B221 jsr */
    sta16(ea_abs(0x0391));  /* 81B224 sta */
    eor16_v(0xFFFF);  /* 81B227 eor */
    ACC16(inc);  /* 81B22A inc */
    sta16(ea_abs(0x038F));  /* 81B22B sta */
    tyx8();  /* 81B22E tyx */
    abi_RotateSine();  /* 81B22F jsr */
    eor16_v(0xFFFF);  /* 81B232 eor */
    ACC16(inc);  /* 81B235 inc */
    sta16(ea_abs(0x038D));  /* 81B236 sta */
    sta16(ea_abs(0x0393));  /* 81B239 sta */
    op_sep(0x20);  /* 81B23C sep */
    return;  /* 81B23E rtl */
L_81B23F:
    op_rep(0x20);  /* 81B23F rep */
    abi_RotateSine();  /* 81B241 jsr */
    sta16(ea_abs(0x038D));  /* 81B244 sta */
    sta16(ea_abs(0x0393));  /* 81B247 sta */
    tyx8();  /* 81B24A tyx */
    abi_RotateSine();  /* 81B24B jsr */
    sta16(ea_abs(0x0391));  /* 81B24E sta */
    eor16_v(0xFFFF);  /* 81B251 eor */
    ACC16(inc);  /* 81B254 inc */
    sta16(ea_abs(0x038F));  /* 81B255 sta */
    op_sep(0x20);  /* 81B258 sep */
    return;  /* 81B25A rtl */
}

/*
 * @routine 81:B25B RotateSine
 * WHAT IT DOES: Entry X of the quarter-turn sine table, divided by 16.
 * HOW: The word at $82:87D3 + X, shifted right four times.
 * Entry: M 16-bit; X = the entry's byte offset; called by JSR. Exit: A the
 * value; N clear, Z from it; C the last bit shifted out.
 */
void abi_RotateSine(void)
{
    lda16(ea_longx(0x8287D3u));  /* 81B25B lda */
    ACC16(lsr);  /* 81B25F lsr */
    ACC16(lsr);  /* 81B260 lsr */
    ACC16(lsr);  /* 81B261 lsr */
    ACC16(lsr);  /* 81B262 lsr */
    return;  /* 81B263 rts */
}
