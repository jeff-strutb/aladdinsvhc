/*
 * brightness_wait.c: the screen brightened or darkened by a task of its
 * own in slot 1, the caller's task waiting until it has ended. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_TaskCreate(void);
void abi_TaskWait(void);

#define FADE_SLOT_STATE (TASK_STATE + 24)   /* slot 1's state: 0 once its task has ended */

/* From $81:F325: TaskWait 1 until slot 1 is free. */
static void brightness_task_wait(void)
{
    do {
        a8_load(0x01);
        jsl_frame(0x81F327u);
        abi_TaskWait();
        a8_load(rd8(dba(FADE_SLOT_STATE)));
    } while ((uint8_t)cpu.a);
}

/*
 * @routine 81:F311 BrightnessUpAndWait
 * WHAT IT DOES: Brings the screen up to full brightness, a step every 2
 * frames, and waits until it is done.
 * HOW: TaskCreate (slot 1, parameter 4, BrightnessUpTask); then TaskWait
 * 1 until slot 1's state is 0, at BrightnessDownAndWait's loop.
 * Entry: M 8-bit; called by JSR from a task (the task at
 * $81:F10D); DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit; A
 * low = 0, Z set; the rest as the last wait restores them.
 */
void abi_BrightnessUpAndWait(void)
{
    jsl_frame(0x81F311u);
    abi_TaskCreate();
    cpu.unwound = 0;
    brightness_task_wait();
}

/*
 * @routine 81:F31C BrightnessDownAndWait
 * WHAT IT DOES: Takes the screen down to dark, a step every 2 frames, and
 * waits until it is done.
 * HOW: TaskCreate (slot 1, parameter 4, BrightnessDownTask); then TaskWait
 * 1 until slot 1's state is 0.
 * Entry: M 8-bit; called by JSR from a task (the task at
 * $81:F10D); DB a bank whose $0000-$1FFF is work RAM. Exit: M 8-bit; A
 * low = 0, Z set; the rest as the last wait restores them.
 */
void abi_BrightnessDownAndWait(void)
{
    jsl_frame(0x81F31Cu);
    abi_TaskCreate();
    cpu.unwound = 0;
    brightness_task_wait();
}
