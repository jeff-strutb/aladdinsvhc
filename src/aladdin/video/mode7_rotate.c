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
