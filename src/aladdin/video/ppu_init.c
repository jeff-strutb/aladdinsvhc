/*
 * ppu_init.c: putting PPU registers into a known state.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hw.h"

/*
 * @routine 80:80F6 PpuRegistersZero
 * WHAT IT DOES: Writes 0 to a run of consecutive PPU registers.
 * HOW: Stores 0 at $2100 + X, steps X and counts Y down, until the count
 * goes negative: Y + 1 registers for a count of $00-$7F.
 * Entry: M 8-bit, X 8-bit; X = the first register's offset from $2100,
 * Y = the count less one; DB a bank whose $2100-$21FF are the PPU
 * registers. Exit: X past the last register written, Y = the count it
 * stopped at (bit 7 set); N set, Z clear. A and the other flags unchanged.
 */
void abi_PpuRegistersZero(void)
{
    uint8_t x = (uint8_t)cpu.x, y = (uint8_t)cpu.y;

    do {
        wr8(INIDISP + x, 0);
        x++;
        y--;
    } while (!(y & 0x80));
    cpu.x = x;
    cpu.y = y;
    cpu.p = nz8(cpu.p, y);
}
