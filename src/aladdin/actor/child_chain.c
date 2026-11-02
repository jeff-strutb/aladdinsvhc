/*
 * child_chain.c: an actor's chain of children: each new child linked to
 * the one before, placed on its parent, numbered; and the nine-piece
 * chain built at (712, 192), its head and tail kept. Drafted by cdraft.py;
 * named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorCreateChild(void);
void abi_ActorFreeListSort(void);
void abi_ChildChainAdd(void);

#define CHAIN_LAST 0x40u  /* word: the record of the last child created; in a child, its number */
#define CHAIN_COUNT 0x42u  /* children created so far */
#define CHAIN_BEFORE 0x4Eu  /* in a child: the record of the child after it */

/* The nine-piece chain: its head's and tail's records (words; the head's
   is the ACTOR_CURRENT some bank-$84 code works on) */
#define CHAIN_HEAD 0x001B14u
#define CHAIN_TAIL 0x001B16u
#define KIND_73 0x73  /* the head */
#define KIND_75 0x75  /* the three pieces after it */
#define KIND_74 0x74  /* the five pieces after those */

/*
 * @routine 84:94EE ChildChainAdd
 * WHAT IT DOES: Creates a child of kind A and adds it to the actor's
 * chain: the new child's $37 is the last one's record, the last one's $4E
 * the new one's; the child is put at the actor's position and numbered
 * (its $40 = the count so far); it becomes the last.
 * HOW: ActorCreateChild (X = the new record; not tested); $40 nonzero: new
 * $37 = $40, $40's $4E = the new record (16-bit). The new x and y pixels =
 * the actor's; its $40 = $42; $42 up; $40 = the new record.
 * Entry: M 8-bit, X 8-bit; A = the kind; D = the parent's record; DB =
 * $84; called by JSR (84:94AE and the eight calls after it). Exit: M
 * 8-bit, X 8-bit; X = the new record's low byte.
 */
void abi_ChildChainAdd(void)
{
    uint16_t last, y;
    uint32_t child;
    uint8_t n;

    abi_ActorCreateChild();
    child = dba(0) + cpu.x;
    last = dp16(CHAIN_LAST);
    cpu.y = last;
    nz16_of(last);
    if (last) {                           /* linked after the last one */
        wr16(child + ACTOR_NEXT, last);
        wr16(dba(CHAIN_BEFORE) + last, cpu.x);
        cpu.a = cpu.x;
    }
    wr16(child + ACTOR_X + 1, dp16(ACTOR_X + 1));
    y = dp16(ACTOR_Y + 1);
    wr16(child + ACTOR_Y + 1, y);
    cpu.a = y;
    n = rd8(dpa(CHAIN_COUNT));
    a8_load(n);
    wr8(child + CHAIN_LAST, n);           /* its number */
    wr8(dpa(CHAIN_COUNT), (uint8_t)(n + 1));
    nz8_of((uint8_t)(n + 1));
    dp16_set(CHAIN_LAST, cpu.x);
    op_sep(P_X);
}

/*
 * @routine 84:948E ChainBuildNine
 * WHAT IT DOES: Builds the actor's chain of nine children at x 712, y
 * 192: a head of kind $73, three pieces of kind $75, five of kind $74;
 * keeps the head's record in CHAIN_HEAD and the tail's in CHAIN_TAIL.
 * HOW: The free list sorted (ActorFreeListSort) for as many passes as
 * half its top offset; the actor's x and y pixels set; $21, CHAIN_LAST
 * and CHAIN_COUNT cleared; ChildChainAdd nine times, CHAIN_LAST copied
 * to CHAIN_HEAD after the first and to CHAIN_TAIL after the last (two
 * 8-bit copies each).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; called by
 * JSR (84:93E3). Exit: as ChildChainAdd; A low byte = the tail's high
 * byte, N and Z from it.
 */
void abi_ChainBuildNine(void)
{
    static const uint8_t kinds[9] = { KIND_73, KIND_75, KIND_75, KIND_75,
                                      KIND_74, KIND_74, KIND_74, KIND_74, KIND_74 };
    uint8_t top = rd8(dba(ACTOR_FREE_TOP));

    a8_load((uint8_t)(top >> 1));
    flag_set(P_C, top & 1);
    abi_ActorFreeListSort();
    dp16_set(ACTOR_X + 1, 0x02C8);
    dp16_set(ACTOR_Y + 1, 0x00C0);
    cpu.a = 0x00C0;
    wr8(dpa(ACTOR_SPAWN_AT), 0);
    dp16_set(CHAIN_LAST, 0);
    wr8(dpa(CHAIN_COUNT), 0);
    for (int i = 0; i < 9; i++) {
        a8_load(kinds[i]);
        abi_ChildChainAdd();
        if (i == 0) {
            wr16(dba(CHAIN_HEAD), dp16(CHAIN_LAST));
            a8_load(rd8(dpa(CHAIN_LAST + 1)));
        }
    }
    wr16(dba(CHAIN_TAIL), dp16(CHAIN_LAST));
    a8_load(rd8(dpa(CHAIN_LAST + 1)));
}
