/*
 * frame.c: a task's steps from one frame to the next: the play screen's
 * frame step, and fixed waits.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_TaskWait(void);

/* The per-frame update of the play screen (camera, objects, sprites), not
   yet filed. */
#define FRAME_UPDATE 0x81830Du

/*
 * @routine 80:AE9A FrameStep
 * WHAT IT DOES: Runs one frame's update of the play screen, then waits for
 * the next frame, letting the other tasks run.
 * HOW: The frame update, then a wait of one frame (TaskWait).
 * Entry: as the frame update; M 8-bit at its return; called by JSR from a
 * task. Exit: when the task resumes, the registers as the wait restores
 * them (as they were after the frame update, A low byte = 1).
 */
void abi_FrameStep(void)
{
    call_routine(FRAME_UPDATE, 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 1);
    cpu.p = nz8(cpu.p, 1);
    jsl_frame(0x80AEA0);
    abi_TaskWait();
}

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
