/*
 * helpers.c: small shared steps other routines call: a flag result, a
 * shift, an empty return, a buffer clear.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

#define WRAM_C100 0x7EC100u        /* a word table the bank-$82 code keeps */

/*
 * @routine 82:AA87 CarryClearReturn
 * WHAT IT DOES: Returns with the carry clear, the "no" answer of the
 * tests that share it.
 * HOW: CLC.
 * Entry: any. Exit: C clear; everything else unchanged.
 */
void abi_CarryClearReturn(void)
{
    cpu.p &= (uint8_t)~P_C;
}

/*
 * @routine 82:A78E ShiftRight5
 * WHAT IT DOES: Divides the 16-bit A by 32, unsigned, rounding down.
 * HOW: Five LSRs of the 16-bit A.
 * Entry: M 16-bit. Exit: A = A >> 5; C = bit 4 of A at entry; N clear,
 * Z from the result. X, Y and the other flags unchanged.
 */
void abi_ShiftRight5(void)
{
    int i;

    for (i = 0; i < 5; i++)
        op_lsr16();
}

/*
 * @routine A5:F533 LongReturn
 * WHAT IT DOES: Nothing: a handler that has no work, called by JSL.
 * HOW: RTL.
 * Entry: any. Exit: everything unchanged.
 */
void abi_LongReturn(void)
{
}

/*
 * @routine 82:A978 WramC100Clear
 * WHAT IT DOES: Clears the word table at $7E:C100 from its start up to the
 * word at X.
 * HOW: Stores 0 at $7E:C100 + X, stepping X down two bytes at a time
 * while it stays positive.
 * Entry: M 16-bit, X 16-bit; X = the last word's offset ($0000-$7FFF).
 * Exit: A = 0; X = $FFFE (X even at entry) or $FFFF (odd); N set, Z clear.
 * Y and the other flags unchanged.
 */
void abi_WramC100Clear(void)
{
    uint16_t x = cpu.x;

    cpu.a = 0;
    do {
        wr16(WRAM_C100 + x, 0);
        x = (uint16_t)(x - 2);
    } while (!(x & 0x8000));
    cpu.x = x;
    cpu.p = nz16(cpu.p, x);
}

/*
 * @routine 83:84C7 ScratchBelowBy10
 * WHAT IT DOES: Compares the scratch word at $0012, less 10, with A: C
 * clear when it is below A.
 * HOW: A is kept in the scratch word at $0014 for the compare.
 * Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A = the
 * word less 10; N, Z and C from the compare; V from the subtraction. X, Y
 * and the other flags unchanged.
 */
void abi_ScratchBelowBy10(void)
{
    wr16(SCRATCH + 8, cpu.a);
    cpu.a = rd16(SCRATCH + 6);
    op_sub16(10);
    op_cmp16(rd16(SCRATCH + 8));
}
