/*
 * frame_steps.c: a task's frame end: the frame's update committed, then a
 * frame's wait; once or X times.
 */
#include "machine.h"
#include "ops65.h"

void abi_TaskWait(void);

/* Not yet filed: the frame's update (the player's position kept, the
   sprites and blocks for the NMI). */
#define FRAME_UPDATE 0x81830Du

/*
 * @routine 80:AE9A FrameEnd
 * WHAT IT DOES: Ends a task's frame: the frame's update, then the wait
 * for the next frame.
 * HOW: The update at $81:830D, then TaskWait with A = 1.
 * Entry: M 8-bit, X 8-bit; called by JSR from a task; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: when the task resumes, the registers as
 * the wait restores them (A low = 1).
 */
void abi_FrameEnd(void)
{
    call_routine(FRAME_UPDATE, 1);
    lda8_v(0x01);
    jsl_frame(0x80AEA0u);
    abi_TaskWait();
}

/*
 * @routine 80:B2D3 FrameEndTimes
 * WHAT IT DOES: Lets X frames go by, with the frame's update after each.
 * HOW: X kept on the stack across a frame's wait (TaskWait, A = 1) and
 * the update at $81:830D, then counted down.
 * Entry: M 8-bit, X 8-bit; X = the frames (0: 256); called by JSR from a
 * task; DB a bank whose $0000-$1FFF is work RAM. Exit: X = 0, Z set, N
 * clear; A, Y and the other flags as the last update leaves them.
 */
void abi_FrameEndTimes(void)
{
    do {
        phx8();
        lda8_v(0x01);
        jsl_frame(0x80B2D6u);
        abi_TaskWait();
        call_routine(FRAME_UPDATE, 1);
        plx8();
        dex8();
    } while (!IF_Z);
}
