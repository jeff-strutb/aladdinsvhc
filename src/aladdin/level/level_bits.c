/*
 * level_bits.c: the eight bytes at $7E:F260, set at a level's start from
 * the bits of a table byte picked by $035D.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"

void abi_LevelFlagFromMask(void);

/*
 * @routine 83:C2E7 Bits035DExpand
 * WHAT IT DOES: Sets the eight bytes at $7E:F260 from the eight bits of the
 * byte for $035D in the table at $80:D27E, last first.
 * HOW: LevelFlagFromMask for X = 7 down to 0.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: X = $FF; N set,
 * Z clear; A and Y as the last call leaves them; M and X 8-bit.
 */

void abi_Bits035DExpand(void)
{
    cpu.x = 7;
    do {
        call_frame(0xC2EB, abi_LevelFlagFromMask);
        x8_load((uint8_t)(cpu.x - 1));
    } while (!(cpu.x & 0x80));
}
