/*
 * pool_second.c: the second pool of records ($09CB-$0B0B): all of it freed at once.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorFree(void);
void abi_ActorTileSlotRelease(void);

#define SECOND_POOL_FIRST 0x09CBu  /* the second pool's first record; ACTOR_POOL follows its last */
#define ACTOR_RECORD_SIZE 0x0050u

/*
 * @routine 83:95C1 SecondPoolFreeAll
 * WHAT IT DOES: Frees every record of the second pool that is in use (as a
 * level ends).
 * HOW: D steps through the five records from $09CB: one whose state is
 * nonzero gets ActorFree and ActorTileSlotRelease. D and P kept.
 * Entry: any widths; DB a bank whose $0000-$1FFF is work RAM; called by
 * JSL (from 81:E7B5). Exit: D and P as at entry; A, X, Y as the last
 * record left them.
 */
void abi_SecondPoolFreeAll(void)
{
    phd();
    php();
    op_rep(P_M);
    cpu.a = cpu.d = SECOND_POOL_FIRST;
    do {
        uint8_t state = rd8(dpa(ACTOR_STATE));

        x8_load(state);
        if (state) {
            op_sep(P_M);
            abi_ActorFree();
            abi_ActorTileSlotRelease();
            op_rep(P_M);
        }
        flag_set(P_C, 0);
        cpu.a = op_adc16(cpu.d, ACTOR_RECORD_SIZE, &cpu.p);
        cpu.d = cpu.a;
        cmp16_of(cpu.a, ACTOR_POOL);
    } while (cpu.d != ACTOR_POOL);
    plp();
    pld();
}

