/*
 * task_scheduler.c: the scheduler of the game's cooperative tasks: it scans
 * the slots for a task that is ready or new and gives it control on its own
 * stack (docs/aladdin/TASKS.md).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_InputRead(void);

#define SCAN_AGAIN 0x032Au      /* set (by the NMI's code): the scan starts over before a task runs */
#define SLOTS_LEFT 0x0329u      /* slots the scan has still to look at */
#define SCANS_AT_FIRST 0x032Du  /* counted up each time slot 0's task is to run */
#define SCHEDULER_START 0x81BC6Du /* TaskScheduler */
#define TASK_READY 0x04
#define TASK_RUNNING 0x08
#define TASK_NEW 0x0C

/* The scan from TaskScheduler (at_next 0) or from TaskSchedulerNext's step
   past slot Y (at_next 1), to the RTL into the task found; or, where the
   original starts the scan over (no task ready, or SCAN_AGAIN set), to that
   jump back to TaskScheduler: the call ends there, and the original's code
   scans on (task_hand_off). */
static void scheduler(int at_next)
{
    uint8_t lo, hi, bank;

    if (at_next)
        goto next;
    stz8(ea_abs(SCAN_AGAIN));               /* 81BC6D */
    lda8(ea_abs(TASK_SLOT_COUNT));
    sta8(ea_abs(SLOTS_LEFT));
    ldy8_v(0x00);
test:
    ldx8(ea_absy(TASK_STATE));              /* 81BC78 */
    cpx8_v(TASK_READY);
    if (IF_C)
        goto found;
next:
    clc();                                  /* 81BC7F */
    tya8();
    adc8_v(0x18);
    tay8();
    MEM8(dec, ea_abs(SLOTS_LEFT));
    if (!IF_Z)
        goto test;
    task_hand_off(SCHEDULER_START);         /* 81BC89 jmp: the scan again */
found:
    lda8(ea_abs(SCAN_AGAIN));               /* 81BC8C */
    if (!IF_Z)
        task_hand_off(SCHEDULER_START);     /* 81BC8F bne */
    sty8(ea_abs(TASK_CURRENT));
    tya8();
    if (IF_Z) {
        MEM8(inc, ea_abs(SCANS_AT_FIRST));
        call_frame(0xBC9C, abi_InputRead);  /* 81BC9A jsr */
    }
    lda8_v(TASK_RUNNING);                   /* 81BC9D */
    sta8(ea_absy(TASK_STATE));
    op_rep(0x20);
    lda16(ea_absy(TASK_SAVED_S));
    tcs();
    lda16_v(0x0000);
    tcd();
    op_sep(0x20);
    cpx8_v(TASK_NEW);
    if (!IF_Z) {
        plp();                              /* 81BCB2 */
        if (cpu.p & P_X) ply8(); else ply16();
        if (cpu.p & P_X) plx8(); else plx16();
        pld();
        plb();
    }
    lo = op_pull8();                        /* 81BCB7 rtl */
    hi = op_pull8();
    bank = op_pull8();
    task_hand_off((uint32_t)bank << 16 | (uint16_t)((hi << 8 | lo) + 1));
}

/*
 * @routine 81:BC6D TaskScheduler
 * WHAT IT DOES: Runs the next task that is ready or new, scanning the slots
 * from the first: it switches to that task's stack and gives it control,
 * new at its entry, waiting where its wait was called. With none ready it
 * scans again until one is (the NMI counts the waits down).
 * HOW: SCAN_AGAIN 0, SLOTS_LEFT = the slot count, Y = 0; on into
 * TaskSchedulerNext's test of slot Y.
 * Entry: M and X 8-bit; S = the scheduler's stack ($0326); reached by a
 * jump (PowerOn's JML, and the scan's own restart). Exit: never returns:
 * at the task, by RTL.
 */
void abi_TaskScheduler(void)
{
    scheduler(0);
}

/*
 * @routine 81:BC7F TaskSchedulerNext
 * WHAT IT DOES: The scheduler's scan from the slot after Y: the next task
 * that is ready (state 4) or new ($0C) runs; past the last slot the scan
 * starts over at TaskScheduler.
 * HOW: Y on by a slot ($18), SLOTS_LEFT down: not 0, the slot's state 4
 * or more is found; else on at TaskScheduler. Found: SCAN_AGAIN set, the
 * scan starts over; else TASK_CURRENT = Y; slot 0 counts SCANS_AT_FIRST
 * up and reads the pad (InputRead); the state becomes running (8); S =
 * its saved S, D = 0; a waiting task's P, Y, X (at the pulled width), D,
 * B pulled; RTL into the task (its wait call's return, or its entry less
 * one for a new task).
 * Entry: M and X 8-bit; Y = the slot just looked at; S = the scheduler's
 * stack ($0326); reached by a jump from TaskToScheduler (a task waits,
 * exits or starts again elsewhere) and by the scan's fall-through. Exit:
 * never returns: at the task, by RTL, with P, X, Y, D, B as the task left
 * them (a new task: X = its state $0C, Y = its slot, D = 0, M 8-bit).
 */
void abi_TaskSchedulerNext(void)
{
    scheduler(1);
}
