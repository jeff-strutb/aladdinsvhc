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

enum { TASK_FREE = 0x00, TASK_WAITING = 0x01, TASK_NEW = 0x0C };

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
    /* the JSR's return address, left on the task's stack */
    wr8(cpu.s, 0xBD); wr8((uint16_t)(cpu.s - 1), 0x27);
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

/* The data bank's view of `addr` (absolute addressing). */
static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/* The byte at [D+off] + y: a long pointer on the direct page, indexed. */
static uint32_t dp_long_y(uint8_t off, uint16_t y)
{
    return (rd24((uint16_t)(cpu.d + off)) + y) & 0xFFFFFF;
}

/* A new task's start: the stack emptied (its saved S = its top); leaves
   X = the stack top, M and X 16-bit. */
static void task_start_at(uint8_t slot)
{
    uint16_t top;

    op_rep(P_M | P_X);
    top = rd16(abs_db((uint16_t)(TASK_STACK_TOP + slot)));
    cpu.a = top;
    cpu.p = nz16(cpu.p, top);
    wr16(abs_db((uint16_t)(TASK_SAVED_S + slot)), top);
    cpu.x = top;
    cpu.p = nz16(cpu.p, top);
}

/*
 * @routine 81:BCB8 TaskCreate
 * WHAT IT DOES: Starts a new task in a slot: it runs from its entry at its
 * first turn, with the parameter byte its creator gives it.
 * HOW: Reads the five bytes after the JSL (slot, parameter, entry word,
 * bank) through the return address on the stack: marks the slot new, sets
 * its parameter, empties its stack and puts the entry (less one, for the
 * scheduler's RTL) and its bank at the stack's top. Then steps the return
 * address over the five bytes and returns after them.
 * Entry: any widths; called by JSL with the five bytes after it; DB a bank
 * whose $0000-$1FFF is work RAM. Exit: past the five bytes; M and X 8-bit;
 * X = the low byte of the new task's stack top; Y = 5; A low byte = the
 * return bank (its high byte from the stepped address); C, V from the
 * bank's ADC; D as at entry, N and Z from it (PLD).
 */
void abi_TaskCreate(void)
{
    uint16_t s0 = cpu.s, d0 = cpu.d, top, v;
    uint8_t slot, b;

    wr8(s0, (uint8_t)(d0 >> 8));                /* PHD */
    wr8((uint16_t)(s0 - 1), (uint8_t)d0);
    cpu.s = (uint16_t)(s0 - 2);
    op_sep(P_M | P_X);
    cpu.a = cpu.s;                              /* TSC, TCD */
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.d = cpu.a;
    cpu.y = 1;
    slot = rd8(dp_long_y(3, 1));
    cpu.y = 2;
    cpu.x = slot;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | TASK_NEW);
    wr8(abs_db((uint16_t)(TASK_STATE + slot)), TASK_NEW);
    b = rd8(dp_long_y(3, 2));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | b);
    wr8(abs_db((uint16_t)(TASK_PARAM + slot)), b);
    cpu.y = 3;
    task_start_at(slot);
    top = cpu.x;
    v = (uint16_t)(rd16(dp_long_y(3, 3)) - 1);
    wr16(abs_db((uint16_t)(top + 1)), v);
    cpu.y = 5;
    v = rd16(dp_long_y(3, 5));
    wr16(abs_db((uint16_t)(top + 3)), v);
    cpu.p &= (uint8_t)~P_C;                     /* the return address + 5 */
    v = op_adc16(rd16((uint16_t)(cpu.s + 3)), 0x0005, &cpu.p);
    wr16((uint16_t)(cpu.s + 3), v);
    op_sep(P_M | P_X);
    b = op_adc8(rd8((uint16_t)(cpu.s + 5)), 0x00, &cpu.p);
    cpu.a = (uint16_t)((v & 0xFF00) | b);
    wr8((uint16_t)(cpu.s + 5), b);
    cpu.d = rd16((uint16_t)(cpu.s + 1));        /* PLD */
    cpu.p = nz16(cpu.p, cpu.d);
    cpu.s = (uint16_t)(cpu.s + 2);
    v = rd16((uint16_t)(cpu.s + 1));            /* RTL */
    b = rd8((uint16_t)(cpu.s + 3));
    cpu.s = (uint16_t)(cpu.s + 3);
    cpu.pc = (uint32_t)b << 16 | (uint16_t)(v + 1);
    cpu.unwound = 1;
}

/*
 * @routine 81:BCF6 TaskExec
 * WHAT IT DOES: Starts the running task again at another entry, its stack
 * emptied, from its next turn.
 * HOW: Marks the slot new and empties its stack, reads the three bytes
 * after the JSL (entry word, bank) through the return address on the
 * stack and puts the entry (less one) and its bank at the stack's top;
 * then moves onto the scheduler's stack and into its loop.
 * Entry: any widths; called by JSL with the three bytes after it; DB a
 * bank whose $0000-$1FFF is work RAM. Exit: never returns; at the
 * scheduler with M and X 8-bit, D = the S at entry, Y = 3, X = the low byte
 * of the stack top, A = $0326, S = $0326, N and Z from $26 (clear); C, V,
 * DB unchanged.
 */
void abi_TaskExec(void)
{
    uint16_t s0 = cpu.s, top, v;
    uint8_t slot;

    op_sep(P_M | P_X);
    cpu.a = cpu.s;                              /* TSC, TCD */
    cpu.p = nz16(cpu.p, cpu.a);
    cpu.d = cpu.a;
    cpu.y = 1;
    slot = rd8(abs_db(TASK_CURRENT));
    cpu.x = slot;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | TASK_NEW);
    wr8(abs_db((uint16_t)(TASK_STATE + slot)), TASK_NEW);
    task_start_at(slot);
    top = cpu.x;
    v = (uint16_t)(rd16(dp_long_y(1, 1)) - 1);
    wr8(s0, (uint8_t)(v >> 8));                 /* PHA */
    wr8((uint16_t)(s0 - 1), (uint8_t)v);
    cpu.s = (uint16_t)(s0 - 2);
    cpu.y = 3;
    cpu.a = rd16(dp_long_y(1, 3));
    wr16(abs_db((uint16_t)(top + 3)), cpu.a);
    cpu.a = (uint16_t)(rd8((uint16_t)(s0 - 1)) | rd8(s0) << 8); /* PLA */
    cpu.s = s0;
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db((uint16_t)(top + 1)), cpu.a);
    op_sep(P_M | P_X);
    cpu.s = s0;
    task_to_scheduler();
    task_yield();
}
