/*
 * step_count.c: a byte the bank-$84 sequences count a step a frame, up to
 * wrap at 0 or down to $F0, while their task waits a frame each time.
 */
#include "machine.h"
#include "cpu_ops.h"

#define STEP_COUNT 0x001B19u       /* the count (cleared at $83:8A64) */

/*
 * @routine 84:8982 StepCountUp
 * WHAT IT DOES: Counts the step byte up by one, for a loop that runs
 * until it wraps to 0.
 * HOW: INC, then LDA of the byte.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A low =
 * the new count; N and Z from it. A high, X, Y and the other flags
 * unchanged.
 */
void abi_StepCountUp(void)
{
    uint8_t a = (uint8_t)(rd8_rmw(STEP_COUNT) + 1);

    wr8(STEP_COUNT, a);
    a = rd8(STEP_COUNT);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 84:8989 StepCountDown
 * WHAT IT DOES: Counts the step byte down by one and tells whether it has
 * reached $F0, for a loop that runs 16 steps down from 0.
 * HOW: DEC, then the byte compared with $F0.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A low =
 * the new count; N, Z and C from comparing it with $F0 (Z set at $F0). A
 * high, X, Y and the other flags unchanged.
 */
void abi_StepCountDown(void)
{
    uint8_t a = (uint8_t)(rd8_rmw(STEP_COUNT) - 1);

    wr8(STEP_COUNT, a);
    a = rd8(STEP_COUNT);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.p = cmp8_flags(cpu.p, a, 0xF0);
}
