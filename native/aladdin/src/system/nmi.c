/*
 * nmi.c: what the original's vertical-blank interrupt did for the game,
 * run in each update (plan, "The loop"): its game work before the tasks'
 * turns, its picture work (video/'s video_vblank) after them. The hardware
 * it drove is gone.
 */
#include "system/state.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

/*
 * @routine 80:8111 NmiHandler
 * WHAT IT DOES: The vertical blank's game work, each frame the NMI is on:
 * the frame count up; every waiting task counted down, one whose count runs
 * out made ready; the next sound command sent. Its picture work comes at
 * the end of the update before (NmiPictureWork).
 * HOW: The original's second count ($032A) told its scheduler to scan from
 * slot 0 again after the NMI; each update's scheduler pass starts there.
 */
void NmiHandler(void)
{
    if (!g.nmi_on)
        return;
    g.frame_count++;
    tasks_count_down();
    SoundCommandSend();
}

/*
 * The NMI's picture work (80:8111, from ScrollRegistersWrite to
 * Mode7MatrixWrite): what the tasks queued, written to the engine's objects,
 * and the shadows the picture reads (video_vblank). It also makes the table
 * moves of 80:8721 WramMovesRun, which only video/'s window effects queue.
 * The IRQ (80:8332) is the split in draw (video/).
 */
void NmiPictureWork(void)
{
    if (g.nmi_on)
        video_vblank();
}
