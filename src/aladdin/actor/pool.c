/*
 * pool.c: actor records taken from the free list.
 *
 * The records an actor can be created in are kept on a stack of their
 * addresses (ACTOR_FREE_LIST, ACTOR_FREE_TOP the offset of its top word).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"

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

/*
 * @routine 81:ADC0 ActorFreeListSort
 * WHAT IT DOES: Orders the top of the free list so the lowest record
 * addresses are taken first, when at least A records are free.
 * HOW: With fewer than A free, nothing (a path the scripts never take,
 * the code after this routine). Else A passes of a bubble sort from the
 * top down: each pass compares A pairs of neighbouring entries, and swaps
 * a pair whose lower entry is the smaller; the passes are counted in
 * scratch $0C.
 * Entry: M 8-bit, X 8-bit; A low byte = the passes; DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none free: Y = the top offset; N set.
 * Sorted: M 8-bit; scratch $0C = 0; Y = the top offset; X = 0; A = the
 * last entry compared or swapped (16 bits); Z set; C from the last
 * compare; V unchanged.
 */
void abi_ActorFreeListSort(void)
{
    uint8_t top = rd8(abs_db(ACTOR_FREE_TOP)), x;
    uint16_t passes;

    cpu.y = top;
    cpu.p = nz8(cpu.p, top);
    if (top & 0x80)
        return;
    wr8(abs_db(SCRATCH), (uint8_t)cpu.a);
    wr8(abs_db(SCRATCH + 1), 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)(top >> 1));
    cpu.p = cmp8_flags(cpu.p, (uint8_t)(top >> 1), rd8(abs_db(SCRATCH)));
    if (!(cpu.p & P_C)) {
        jump_routine(0x81ADF9);
        return;
    }
    op_rep(P_M);
    do {
        uint8_t y = top;

        op_push8(y);                           /* PHY */
        x = rd8(abs_db(SCRATCH));
        do {
            uint16_t lower = rd16(abs_db((uint16_t)(ACTOR_FREE_LIST - 2 + y)));

            cpu.a = lower;
            op_cmp16(rd16(abs_db((uint16_t)(ACTOR_FREE_LIST + y))));
            if (!(cpu.p & P_C)) {
                op_push8((uint8_t)(lower >> 8));   /* PHA */
                op_push8((uint8_t)lower);
                cpu.a = rd16(abs_db((uint16_t)(ACTOR_FREE_LIST + y)));
                wr16(abs_db((uint16_t)(ACTOR_FREE_LIST - 2 + y)), cpu.a);
                cpu.a = op_pull8();                /* PLA */
                cpu.a |= (uint16_t)(op_pull8() << 8);
                wr16(abs_db((uint16_t)(ACTOR_FREE_LIST + y)), cpu.a);
            }
            y = (uint8_t)(y - 2);
            x--;
        } while (x != 0);
        top = op_pull8();                      /* PLY */
        passes = (uint16_t)(rd16_rmw(abs_db(SCRATCH)) - 1);
        wr16_rmw(abs_db(SCRATCH), passes);
    } while (passes != 0);
    cpu.y = top;
    cpu.x = 0;
    cpu.p = nz16(cpu.p, 0);
    op_sep(P_M);
}

/*
 * @routine 81:AADD ActorRecordsInit
 * WHAT IT DOES: Empties every actor record and fills the free lists for a
 * new level.
 * HOW: Clears all the records; the second free list holds the 5 records
 * from $09CB and the first the 28 from $0B5B, the highest address at the
 * bottom; each of the 28 gets its number (28 down to 1) at $33; the four
 * animated-tile slots get no holders.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
 * Exit: M and X 8-bit; X = $FF; A = the pool walk's last index less $50
 * (16 bits; $FFB0); N set, Z clear (DEX); C clear (the last SBC); Y, V
 * unchanged; scratch $0C = 0.
 */
void abi_ActorRecordsInit(void)
{
    int x;
    uint16_t a;
    uint8_t n;

    op_rep(P_X);
    for (x = 0x0AF0; x >= 0; x--)
        wr8(abs_db((uint16_t)(ACTOR_RECORDS + x)), 0);
    op_rep(P_M);
    op_sep(P_X);
    wr8(abs_db(ACTOR_FREE_TOP_B), 0x08);
    for (x = 8, a = 0x09CB; x >= 0; x -= 2, a += 0x50)
        wr16(abs_db((uint16_t)(ACTOR_FREE_LIST_B + x)), a);
    wr8(abs_db(ACTOR_FREE_TOP), 0x36);
    for (x = 0x36, a = ACTOR_POOL; x >= 0; x -= 2, a += 0x50)
        wr16(abs_db((uint16_t)(ACTOR_FREE_LIST + x)), a);
    op_sep(P_M);
    op_rep(P_X);
    wr8(abs_db(SCRATCH), 0x1C);
    x = 0x0870;
    do {
        n = rd8(abs_db(SCRATCH));
        wr8(abs_db((uint16_t)(ACTOR_POOL + ACTOR_33 + x)), n);
        wr8(abs_db(SCRATCH), (uint8_t)(rd8_rmw(abs_db(SCRATCH)) - 1));
        cpu.p |= P_C;
        a = op_sbc16((uint16_t)x, 0x0050, &cpu.p);
        x = a;
    } while (!(a & 0x8000));
    cpu.a = a;
    for (x = 3; x >= 0; x--)
        wr8(abs_db((uint16_t)(ACTOR_TILE_SLOT_USERS + 1 + x)), 0);
    cpu.x = 0xFFFF;
    cpu.p = nz16(cpu.p, 0xFFFF);
    op_sep(P_M | P_X);
}
