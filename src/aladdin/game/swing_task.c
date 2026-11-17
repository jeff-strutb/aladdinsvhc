/*
 * swing_task.c: a task that moves the 24-bit value at $18CC up, then down
 * at a slowing speed, over 48 frames. Drafted by cdraft.py; named and
 * described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"

void abi_TaskExit(void);
void abi_TaskWait(void);

#define SWING_VALUE 0x18CCu       /* the value moved (24 bits) */
#define V_1A10 0x1A10u            /* counted up as the task starts */
#define V_1A12 0x1A12u            /* nonzero: no camera follow up and down */
#define SWING_COUNT 0x00A5u       /* slot 2's own bytes: frames left in a stretch */
#define SWING_STEP 0x00A6u        /* the speed's fall each frame */
#define SWING_SPEED 0x00A7u       /* the speed (word) */

/*
 * @routine 81:EE8B SwingTask
 * WHAT IT DOES: A task that moves the 24-bit scroll value at $18CC:
 * counts up $1A10 and $1A12, sets its middle byte to $3E, waits
 * 16 frames, takes $000408 off it each frame for 16 frames, then adds a
 * speed to it each frame for 16 frames, the speed starting at $0258 and
 * falling by $49 a frame; then ends.
 * HOW: SWING_COUNT, SWING_STEP and SWING_SPEED are the task's own bytes in
 * slot 2 ($A5-$A8); the 24-bit sums byte by byte (SBC, then ADC, with the
 * carry between bytes); TaskWait 1 each frame; TaskExit. No script runs it
 * (nothing found creates it): written from its bytes.
 * Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
 * RAM. Exit: none: the task ends.
 */
void abi_SwingTask(void)
{
    inc8_at(dba(V_1A10));
    inc8_at(dba(V_1A12));
    a8_load(0x3E);
    wr8(dba(SWING_VALUE + 1), 0x3E);
    a8_load(0x10);
    jsl_frame(0x81EE98u);
    abi_TaskWait();
    a8_load(0x10);
    wr8(dba(SWING_COUNT), 0x10);
    do {                                  /* 16 frames less $000408 */
        a8_load(0x01);
        jsl_frame(0x81EEA3u);
        abi_TaskWait();
        sec();
        lda8(ea_abs(SWING_VALUE));
        sbc8_v(0x08);
        sta8(ea_abs(SWING_VALUE));
        lda8(ea_abs(SWING_VALUE + 1));
        sbc8_v(0x04);
        sta8(ea_abs(SWING_VALUE + 1));
        lda8(ea_abs(SWING_VALUE + 2));
        sbc8_v(0x00);
        sta8(ea_abs(SWING_VALUE + 2));
    } while (dec8_at(dba(SWING_COUNT)));
    a8_load(0x10);
    wr8(dba(SWING_COUNT), 0x10);
    a8_load(0x49);
    wr8(dba(SWING_STEP), 0x49);
    a8_load(0x58);
    wr8(dba(SWING_SPEED), 0x58);
    a8_load(0x02);
    wr8(dba(SWING_SPEED + 1), 0x02);
    do {                                  /* 16 frames by the slowing speed */
        a8_load(0x01);
        jsl_frame(0x81EEDBu);
        abi_TaskWait();
        sec();
        lda8(ea_abs(SWING_SPEED));
        sbc8_v(rd8(ea_abs(SWING_STEP)));
        sta8(ea_abs(SWING_SPEED));
        lda8(ea_abs(SWING_SPEED + 1));
        sbc8_v(0x00);
        sta8(ea_abs(SWING_SPEED + 1));
        clc();
        lda8(ea_abs(SWING_VALUE));
        adc8_v(rd8(ea_abs(SWING_SPEED)));
        sta8(ea_abs(SWING_VALUE));
        lda8(ea_abs(SWING_VALUE + 1));
        adc8_v(rd8(ea_abs(SWING_SPEED + 1)));
        sta8(ea_abs(SWING_VALUE + 1));
        lda8(ea_abs(SWING_VALUE + 2));
        adc8_v(0x00);
        sta8(ea_abs(SWING_VALUE + 2));
    } while (dec8_at(dba(SWING_COUNT)));
    abi_TaskExit();
}
