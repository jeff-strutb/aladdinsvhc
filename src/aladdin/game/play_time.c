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
    uint8_t n;

    n = (uint8_t)(rd8(dba(PLAY_FRAMES)) + 1);
    wr8(dba(PLAY_FRAMES), n);
    a8_load(n);
    cmp8_of(n, 60);
    if (n < 60)
        return;
    wr8(dba(PLAY_FRAMES), 0);
    wr8(dba(V_147D), (uint8_t)(rd8(dba(V_147D)) + 1));
    for (int i = 0; i < 5; i++) {        /* each digit, carrying into the next */
        uint16_t digit = (uint16_t)(PLAY_DIGITS + i);

        n = (uint8_t)(rd8(dba(digit)) + 1);
        wr8(dba(digit), n);
        a8_load(n);
        cmp8_of(n, limits[i]);
        if (n < limits[i])
            return;
        wr8(dba(digit), 0);
    }
    n = (uint8_t)(rd8(dba(PLAY_DIGITS + 5)) + 1);
    wr8(dba(PLAY_DIGITS + 5), n);
    nz8_of(n);
}
