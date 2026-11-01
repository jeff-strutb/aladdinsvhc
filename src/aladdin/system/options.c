/*
 * options.c: the option settings work RAM keeps across a reset.
 *
 * Power-on keeps the three option bytes when work RAM still holds the
 * signature and the cartridge title; the sum beside them is kept up to
 * date whenever the options screen changes one.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram_state.h"

/*
 * @routine 80:80FE KeptOptionsSum
 * WHAT IT DOES: Adds up the three kept option bytes.
 * HOW: The second byte plus the first, then plus the third, each addition
 * with the carry cleared first (decimal when D is set, as the CPU adds).
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: A low =
 * the 8-bit sum; N, V, Z, C from the last addition. A high, X, Y and the
 * other flags unchanged.
 */
void abi_KeptOptionsSum(void)
{
    uint8_t a;

    cpu.p &= (uint8_t)~P_C;
    a = rd8(KEPT_OPTIONS + 1);
    a = op_adc8(a, rd8(KEPT_OPTIONS), &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    a = op_adc8(a, rd8(KEPT_OPTIONS + 2), &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
}

/*
 * @routine 80:810A KeptOptionsSeal
 * WHAT IT DOES: Stores the kept option bytes' sum beside them, after one
 * of them has changed.
 * HOW: KeptOptionsSum, then the sum to its byte.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSL.
 * Exit: as KeptOptionsSum.
 */
void abi_KeptOptionsSeal(void)
{
    abi_KeptOptionsSum();
    wr8(KEPT_OPTIONS_SUM, (uint8_t)cpu.a);
}
