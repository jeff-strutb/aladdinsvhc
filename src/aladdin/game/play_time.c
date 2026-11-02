/*
 * play_time.c: the time the game has been played, counted each frame in
 * decimal digits.
 */
#include "machine.h"
#include "ops65.h"
void abi_PlayTimeTick(void);
#include "wram.h"

/*
 * @routine 81:85CE PlayTimeTick
 * WHAT IT DOES: Counts one more frame of play time.
 * HOW: Frames to 60 make a second; the seconds' units carry at 10, tens at
 * 6, minutes' units at 10, tens at 6, hours' units at 10 into the tens.
 * The seconds count at $147D steps too, without carrying.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: A low byte = the last digit compared; N, Z, C from its compare; X,
 * Y, V unchanged.
 */
void abi_PlayTimeTick(void)
{
    MEM8(inc, ea_abs(0x147C));  /* 8185CE inc */
    lda8(ea_abs(0x147C));  /* 8185D1 lda */
    cmp8_v(0x3C);  /* 8185D4 cmp */
    if (!IF_C) goto L_818622;  /* 8185D6 bcc */
    stz8(ea_abs(0x147C));  /* 8185D8 stz */
    MEM8(inc, ea_abs(0x147D));  /* 8185DB inc */
    MEM8(inc, ea_abs(0x147E));  /* 8185DE inc */
    lda8(ea_abs(0x147E));  /* 8185E1 lda */
    cmp8_v(0x0A);  /* 8185E4 cmp */
    if (!IF_C) goto L_818622;  /* 8185E6 bcc */
    stz8(ea_abs(0x147E));  /* 8185E8 stz */
    MEM8(inc, ea_abs(0x147F));  /* 8185EB inc */
    lda8(ea_abs(0x147F));  /* 8185EE lda */
    cmp8_v(0x06);  /* 8185F1 cmp */
    if (!IF_C) goto L_818622;  /* 8185F3 bcc */
    stz8(ea_abs(0x147F));  /* 8185F5 stz */
    MEM8(inc, ea_abs(0x1480));  /* 8185F8 inc */
    lda8(ea_abs(0x1480));  /* 8185FB lda */
    cmp8_v(0x0A);  /* 8185FE cmp */
    if (!IF_C) goto L_818622;  /* 818600 bcc */
    stz8(ea_abs(0x1480));  /* 818602 stz */
    MEM8(inc, ea_abs(0x1481));  /* 818605 inc */
    lda8(ea_abs(0x1481));  /* 818608 lda */
    cmp8_v(0x06);  /* 81860B cmp */
    if (!IF_C) goto L_818622;  /* 81860D bcc */
    stz8(ea_abs(0x1481));  /* 81860F stz */
    MEM8(inc, ea_abs(0x1482));  /* 818612 inc */
    lda8(ea_abs(0x1482));  /* 818615 lda */
    cmp8_v(0x0A);  /* 818618 cmp */
    if (!IF_C) goto L_818622;  /* 81861A bcc */
    stz8(ea_abs(0x1482));  /* 81861C stz */
    MEM8(inc, ea_abs(0x1483));  /* 81861F inc */
L_818622:
    return;  /* 818622 rts */
}
