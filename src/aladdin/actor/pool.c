/*
 * pool.c: actor records taken from the free list.
 *
 * The records an actor can be created in are kept on a stack of their
 * addresses (ACTOR_FREE_LIST, ACTOR_FREE_TOP the offset of its top word).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/*
 * @routine 81:AB3F ActorAlloc
 * WHAT IT DOES: Takes a free actor record, if there is one.
 * HOW: The free list's top offset, negative when none is free; else the
 * offset steps down a word and the address at the old top is the record.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: none free: Y = the offset; N set; X 8-bit and unchanged.
 * Taken: X 16-bit; X = the record's address; Y = the old top offset; N, Z
 * from X (N clear). A, C, V unchanged.
 */
void abi_ActorAlloc(void)
{
    uint8_t top = rd8(abs_db(ACTOR_FREE_TOP));

    cpu.y = top;
    cpu.p = nz8(cpu.p, top);
    if (top & 0x80)
        return;
    op_rep(P_X);
    wr8(abs_db(ACTOR_FREE_TOP), (uint8_t)(rd8_rmw(abs_db(ACTOR_FREE_TOP)) - 1));
    wr8(abs_db(ACTOR_FREE_TOP), (uint8_t)(rd8_rmw(abs_db(ACTOR_FREE_TOP)) - 1));
    cpu.x = rd16(abs_db((uint16_t)(ACTOR_FREE_LIST + cpu.y)));
    cpu.p = nz16(cpu.p, cpu.x);
}
