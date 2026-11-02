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

/* FrameEndTimes ($80:B2D3), X frames each ended so: its assembly is
   filed, its C is not, for the same reason (A5 stops at $81:830D, which
   has no C, and reruns the C from its entry at the next resume). */





