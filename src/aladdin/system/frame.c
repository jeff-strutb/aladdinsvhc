/*
 * frame.c: a task's steps from one frame to the next: fixed waits. (The
 * play screen's frame step, $80:AE9A, comes when the frame update it
 * calls, $81:830D, has C.)
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_TaskWait(void);

/*
 * @routine 80:AEB9 Wait64Frames
 * WHAT IT DOES: Makes the running task wait 64 frames (a little over a
 * second), letting the other tasks run.
 * HOW: TaskWait with A = 64.
 * Entry: M 8-bit; called by JSR from a task. Exit: when the task resumes,
 * the registers as the wait restores them (A low byte = 64).
 */
void abi_Wait64Frames(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 64);
    cpu.p = nz8(cpu.p, 64);
    jsl_frame(0x80AEBB);
    abi_TaskWait();
}
