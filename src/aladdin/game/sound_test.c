/*
 * sound_test.c: the options screen's sound test: its number stepped up or
 * down, wrapping, and shown.
 */
#include "machine.h"
#include "ops65.h"

void abi_SoundQueueF2(void);
void abi_SoundTestShow(void);

#define SOUND_TEST 0x1B15u         /* the sound test's number, less one (0-34) */
#define SOUND_TEST_LAST 0x22       /* the last number (less one) */

/*
 * @routine 83:8B7E SoundTestStep
 * WHAT IT DOES: Steps the sound test's number up (C set) or down,
 * wrapping within 1-35, queues sound command $F2 and shows the number.
 * HOW: INC or DEC of the number less one: past $22 it becomes 0, below 0
 * it becomes $22; SoundQueueF2, then on into SoundTestShow.
 * Entry: M 8-bit, X 8-bit; C = the direction; DB a bank whose
 * $0000-$1FFF is work RAM and whose $4200-$421F are the CPU registers.
 * Exit: as SoundTestShow.
 */
void abi_SoundTestStep(void)
{
    if (IF_C) goto up;
    MEM8(dec, ea_abs(SOUND_TEST));
    if (!IF_N) goto show;
    lda8_v(SOUND_TEST_LAST);
    sta8(ea_abs(SOUND_TEST));
    goto show;
up:
    MEM8(inc, ea_abs(SOUND_TEST));
    lda8(ea_abs(SOUND_TEST));
    cmp8_v(SOUND_TEST_LAST + 1);
    if (!IF_C) goto show;
    stz8(ea_abs(SOUND_TEST));
show:
    abi_SoundQueueF2();
    abi_SoundTestShow();
}
