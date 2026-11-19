/*
 * play_time.c: the time the game has been played, counted each frame in
 * decimal digits.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_PlayTimeTick(void);
#include "wram.h"

#define PLAY_FRAMES 0x147Cu    /* frames of the second, 0-59 */
#define V_147D 0x147Du         /* counted up each second, unbounded */
#define PLAY_DIGITS 0x147Eu    /* the time digits from the seconds up, to the hours tens */

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
    static const uint8_t limits[5] = { 10, 6, 10, 6, 10 };   /* s, 10 s, min, 10 min, h */

    MEM8(inc, dba(PLAY_FRAMES));          /* INC, then LDA: two reads */
    lda8(dba(PLAY_FRAMES));
    cmp8_v(0x3C);
    if (!IF_C)
        return;
    stz8(dba(PLAY_FRAMES));
    MEM8(inc, dba(V_147D));
    for (int i = 0; i < 5; i++) {        /* each digit, carrying into the next */
        uint16_t digit = (uint16_t)(PLAY_DIGITS + i);

        MEM8(inc, dba(digit));
        lda8(dba(digit));
        cmp8_v(limits[i]);
        if (!IF_C)
            return;
        stz8(dba(digit));
    }
    MEM8(inc, dba(PLAY_DIGITS + 5));
}
