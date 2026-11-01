/*
 * random.c: the game's random numbers.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/*
 * @routine 81:981B RandomNext
 * WHAT IT DOES: Steps the random number: the next in its sequence.
 * HOW: random * 3, plus its old low byte into the high byte (random *
 * $0103 mod $10000), byte by byte through scratch $1A-$1B.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: A low byte = the new high byte; N, V, Z, C from its ADC; scratch
 * $1A-$1B = the old random * 2; X, Y unchanged.
 */
void abi_RandomNext(void)
{
    uint8_t lo = rd8(abs_db(RANDOM)), hi, t, c;

    wr8(abs_db(SCRATCH + 0x0E), lo);
    hi = rd8(abs_db(RANDOM + 1));
    wr8(abs_db(SCRATCH + 0x0F), hi);
    t = rd8_rmw(abs_db(SCRATCH + 0x0E));        /* ASL */
    c = t >> 7;
    wr8(abs_db(SCRATCH + 0x0E), (uint8_t)(t << 1));
    t = rd8_rmw(abs_db(SCRATCH + 0x0F));        /* ROL */
    wr8(abs_db(SCRATCH + 0x0F), (uint8_t)(t << 1 | c));
    cpu.p &= (uint8_t)~P_C;
    t = op_adc8(rd8(abs_db(SCRATCH + 0x0E)), rd8(abs_db(RANDOM)), &cpu.p);
    wr8(abs_db(RANDOM), t);
    t = op_adc8(rd8(abs_db(RANDOM + 1)), rd8(abs_db(SCRATCH + 0x0F)), &cpu.p);
    wr8(abs_db(RANDOM + 1), t);
    cpu.p &= (uint8_t)~P_C;
    t = op_adc8(lo, rd8(abs_db(RANDOM + 1)), &cpu.p);
    wr8(abs_db(RANDOM + 1), t);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | t);
}
