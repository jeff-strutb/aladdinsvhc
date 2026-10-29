/*
 * task.c: the game's cooperative tasks.
 *
 * The game runs its screens and modes as up to 8 tasks, each with a slot
 * record and a stack of its own. A task runs until it waits for a number of
 * frames, exits, or starts again elsewhere; then the scheduler runs the
 * next task that is ready, and the NMI counts the waits down
 * (docs/aladdin/TASKS.md).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

/* The scheduler's loop, where a task call hands over, on its own stack */
#define TASK_SCHEDULER_NEXT 0x81BC7Fu
#define TASK_SCHEDULER_STACK 0x0326u

enum { TASK_FREE = 0x00, TASK_WAITING = 0x01 };

static void push8(uint8_t v)
{
    wr8(cpu.s, v);
    cpu.s--;
}

/* LDA #$03, XBA, LDA #$26, TCS, JMP $BC7F: onto the scheduler's stack and
   into its loop, the A, N and Z the four leave behind. */
static void task_to_scheduler(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (TASK_SCHEDULER_STACK >> 8));
    cpu.p = nz8(cpu.p, TASK_SCHEDULER_STACK >> 8);
    op_xba();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (TASK_SCHEDULER_STACK & 0xFF));
    cpu.p = nz8(cpu.p, TASK_SCHEDULER_STACK & 0xFF);
    cpu.s = cpu.a;
    cpu.pc = TASK_SCHEDULER_NEXT;
}

/*
 * @routine 81:BD4D TaskSlotFree
 * WHAT IT DOES: Frees a task slot: the scheduler passes over it until a
 * task is created there again.
 * HOW: Its state byte becomes 0.
 * Entry: M 8-bit; Y = the slot (its byte offset). Exit: A low byte = 0;
 * N clear, Z set; the rest unchanged.
 */
void TaskSlotFree(uint16_t slot)
{
    cpu.a &= 0xFF00;
    cpu.p = nz8(cpu.p, 0);
    wr8(TASK_STATE + slot, TASK_FREE);
}

void abi_TaskSlotFree(void)
{
    TaskSlotFree(cpu.y);
}

/*
 * @routine 81:BD20 TaskExit
 * WHAT IT DOES: Ends the running task for good and hands over to the
 * scheduler.
 * HOW: Frees the running task's slot, then moves onto the scheduler's own
 * stack and into its loop. Its stack is left as it was.
 * Entry: any widths; reached by a jump (JMP or JML) where a task ends.
 * Exit: never returns; at the scheduler
 * with M and X 8-bit, Y = the slot, A = $0326, S = $0326, N and Z from
 * $26 (clear); D, DB and the other flags unchanged.
 */
void abi_TaskExit(void)
{
    uint8_t slot;

    op_sep(0x30);
    slot = rd8(TASK_CURRENT);
    cpu.y = slot;
    cpu.p = nz8(cpu.p, slot);
    TaskSlotFree(slot);
    task_to_scheduler();
    task_yield();
}

/*
 * @routine 81:BD31 TaskWait
 * WHAT IT DOES: Makes the running task wait A frames (its next turn comes
 * once the NMI has counted them down), letting the other tasks run.
 * HOW: Pushes B, D, X, Y and P on the task's stack, puts the count and the
 * waiting state in its slot and saves S there, then moves onto the
 * scheduler's own stack and into its loop. The scheduler resumes the task
 * by pulling them back and returning (RTL) to the caller.
 * Entry: A low byte = frames to wait; X and Y at either width (pushed at
 * that width); called by JSL. Exit: when the task resumes, every register
 * as it was at entry (the scheduler pulls them), the frames later; P from
 * the stack. Between, at the scheduler: M and X 8-bit, Y = the slot,
 * A = $0326, S = $0326, N and Z clear; C, V, D, I unchanged.
 */
void abi_TaskWait(void)
{
    uint8_t frames = (uint8_t)cpu.a, slot;

    push8(cpu.db);
    push8((uint8_t)(cpu.d >> 8));
    push8((uint8_t)cpu.d);
    if (!(cpu.p & P_X)) push8((uint8_t)(cpu.x >> 8));
    push8((uint8_t)cpu.x);
    if (!(cpu.p & P_X)) push8((uint8_t)(cpu.y >> 8));
    push8((uint8_t)cpu.y);
    push8(cpu.p);
    op_sep(0x30);
    slot = rd8(TASK_CURRENT);
    cpu.y = slot;
    cpu.p = nz8(cpu.p, slot);
    wr8(TASK_WAIT + slot, frames);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | TASK_WAITING);
    cpu.p = nz8(cpu.p, TASK_WAITING);
    wr8(TASK_STATE + slot, TASK_WAITING);
    cpu.a = cpu.s;                            /* TSC */
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(TASK_SAVED_S + slot, cpu.s);
    task_to_scheduler();
    task_yield();
}
