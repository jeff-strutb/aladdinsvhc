/*
 * sound_test.c: the options screen's sound test: its number stepped up or
 * down, wrapping, and shown.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    uint8_t n;

    if (!(cpu.p & P_C)) {                 /* down, from the first to the last */
        n = (uint8_t)(rd8(dba(SOUND_TEST)) - 1);
        wr8(dba(SOUND_TEST), n);
        nz8_of(n);
        if (n & 0x80) {
            a8_load(SOUND_TEST_LAST);
            wr8(dba(SOUND_TEST), SOUND_TEST_LAST);
        }
    } else {                              /* up, from the last to the first */
        n = (uint8_t)(rd8(dba(SOUND_TEST)) + 1);
        wr8(dba(SOUND_TEST), n);
        a8_load(n);
        cmp8_of(n, SOUND_TEST_LAST + 1);
        if (n >= SOUND_TEST_LAST + 1)
            wr8(dba(SOUND_TEST), 0);
    }
    abi_SoundQueueF2();
    abi_SoundTestShow();
}
