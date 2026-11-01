/*
 * multiply.c: the CPU's 8x8 multiplier.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/*
 * @routine 81:A7E6 MultiplyBy
 * WHAT IT DOES: Multiplies the factor already in WRMPYA by A.
 * HOW: Writes A to WRMPYB, waits the multiplier's time and reads the
 * 16-bit product.
 * Entry: M 8-bit, X 16-bit; A low byte = the second factor; DB a bank
 * whose $4200-$421F are the CPU's; called by JSR. Exit: Y = the product;
 * N, Z from it; A, X, C, V unchanged.
 */
void abi_MultiplyBy(void)
{
    wr8(abs_db(WRMPYB & 0xFFFF), (uint8_t)cpu.a);
    cpu.y = rd16(abs_db(RDMPYL & 0xFFFF));
    cpu.p = nz16(cpu.p, cpu.y);
}
