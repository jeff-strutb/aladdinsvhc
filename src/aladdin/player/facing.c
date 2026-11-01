/*
 * facing.c: the way the joypad's Left and Right point.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram_pad.h"

/* The facing for each pair of the joypad's Left and Right bits (Right 1,
   Left 2): 0 right, 1 left, $FF neither or both */
#define PAD_FACING 0x80C72Cu

/*
 * @routine 81:CE7C PadFacing
 * WHAT IT DOES: Gives the way joypad 1's Left and Right point: 0 right,
 * 1 left, or $FF (negative) when neither or both are held.
 * HOW: Looks the held Left and Right bits up in a table of four.
 * Entry: M 8-bit, X 8-bit; DB = $80. Exit: A low byte = the facing, N and
 * Z from it; X = the Left and Right bits; C and the rest unchanged.
 */
void abi_PadFacing(void)
{
    op_lda8(PAD1_HELD + 1);
    cpu.a &= 0xFF03;
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
    cpu.x = (uint8_t)cpu.a;
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    op_lda8(PAD_FACING + cpu.x);
}
