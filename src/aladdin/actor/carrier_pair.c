/*
 * carrier_pair.c: an actor that sends in a pair: a carrier (kind $38) and
 * the thing it holds (kind $53, actor/kind53.c), each knowing the other.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorCreateIn(void);
void abi_ActorSpawnPlace(void);

#define NEW_RECORD 0x0020u  /* the record ActorCreateIn last took (word) */
#define RANGE_X 0x1A44u  /* the point actors are kept in range of (words) */
#define RANGE_Y 0x1A46u
#define PAIR_OTHER 0x4Eu  /* word: the record of the other of the pair */
#define PAIR_SENDER 0x4Cu  /* word: the record of the actor that sent them, 0 here */
#define PAIRS_LIVE 0x4Bu  /* actors it has sent that are live */
#define PAIR_WAIT 0x40u  /* frames until it sends the next */

/* The record ActorCreateIn last took, in X (16-bit). */
static uint32_t pair_place(void)
{
    op_rep(P_X);
    cpu.x = rd16(dba(NEW_RECORD));
    return dba(0) + cpu.x;
}

/* The new one started (state $0C), facing as the sender. */
static void pair_start(uint32_t rec)
{
    a8_load(0x0C);
    wr8(rec + ACTOR_STATE, 0x0C);
    a8_load(rd8(dpa(ACTOR_FACING)));
    wr8(rec + ACTOR_FACING, (uint8_t)cpu.a);
    wr8(rec + ACTOR_FLIP, (uint8_t)cpu.a);
}

/*
 * @routine 84:A056 CarrierPairSend
 * WHAT IT DOES: Sends in a carrier (kind $38) 40 pixels above the point
 * actors are kept in range of, and a thing it holds (kind $53) at the
 * carrier's spawn point $44, both new and facing as the sender, each with
 * the other's record in $4E; counts them live and waits $3C frames before
 * the next.
 * HOW: Its own position kept on the stack and set to the range point (y
 * less 40) for the placing; kind $38 by ActorCreateIn (Y = $4A), $4A up by
 * 2; the new record (from $0020) in its $4E; the carrier's $07 = its own
 * plus one, ActorSpawnPlace at $44, state $0C, facing and flip as its own,
 * $45, $46 and $4C (16-bit) cleared; $41 up, $40 = $3C. Then kind $53 the
 * same way, placed at the carrier's spawn point $44 less 4 across and 12
 * down, state $0C, its facing, $4C cleared; the two records crossed into
 * each other's $4E. Its position back, $4B up, and the free list's top up
 * by 4 (both records taken).
 * Entry: M 8-bit, X 8-bit; D = the sender's record; DB = $84; called by
 * JSR (84:9FE7). Exit: M and X 8-bit; A = the free list's new top (low
 * byte); C from its ADC.
 */
void abi_CarrierPairSend(void)
{
    uint32_t first, second;

    /* both start at the range point, 40 up; this one's place kept */
    op_rep(P_M);
    a16_load(dp16(ACTOR_X + 1));
    pha16();
    a16_load(dp16(ACTOR_Y + 1));
    pha16();
    dp16_set(ACTOR_X + 1, rd16(dba(RANGE_X)));
    flag_set(P_C, 1);
    cpu.a = op_sbc16(rd16(dba(RANGE_Y)), 0x0028, &cpu.p);
    dp16_set(ACTOR_Y + 1, cpu.a);
    op_sep(P_M);
    /* a kind $38 from its stream */
    a8_load(0x38);
    wr8(dpa(ACTOR_06), 0x38);
    abi_ActorCreateIn();
    inc8_at(dpa(ACTOR_STREAM));
    inc8_at(dpa(ACTOR_STREAM));
    first = pair_place();
    dp16_set(PAIR_OTHER, cpu.x);
    a8_load((uint8_t)(rd8(dpa(ACTOR_07)) + 1));
    wr8(first + ACTOR_07, (uint8_t)cpu.a);
    a8_load(0x44);
    wr8(dpa(ACTOR_SPAWN_AT), 0x44);
    abi_ActorSpawnPlace();
    first = dba(0) + cpu.x;
    pair_start(first);
    wr8(first + ACTOR_45, 0);
    wr8(first + ACTOR_46, 0);
    cpu.a = cpu.d;
    wr16(first + ACTOR_4C, cpu.d);
    inc8_at(dpa(ACTOR_41));
    a8_load(0x3C);
    wr8(dpa(PAIR_WAIT), 0x3C);
    op_sep(P_X);
    /* a kind $53, 4 left and 12 below it; each the other's pair */
    y8_load(rd8(dpa(ACTOR_STREAM)));
    a8_load(0x53);
    wr8(dpa(ACTOR_06), 0x53);
    abi_ActorCreateIn();
    second = pair_place();
    a8_load(0x44);
    wr8(dpa(ACTOR_SPAWN_AT), 0x44);
    abi_ActorSpawnPlace();
    second = dba(0) + cpu.x;
    op_rep(0x21);
    cpu.a = op_adc16(rd16(second + ACTOR_X + 1), 0xFFFC, &cpu.p);
    wr16(second + ACTOR_X + 1, cpu.a);
    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(second + ACTOR_Y + 1), 0x000C, &cpu.p);
    wr16(second + ACTOR_Y + 1, cpu.a);
    op_sep(P_M);
    pair_start(second);
    cpu.a = cpu.d;
    wr16(second + ACTOR_4C, cpu.d);
    cpu.a = dp16(PAIR_OTHER);
    wr16(second + PAIR_OTHER, cpu.a);
    cpu.y = cpu.x;
    cpu.x = cpu.a;
    cpu.a = cpu.y;
    wr16(dba(PAIR_OTHER) + cpu.x, cpu.a);
    pla16();
    dp16_set(ACTOR_Y + 1, cpu.a);
    pla16();
    dp16_set(ACTOR_X + 1, cpu.a);
    op_sep(0x30);
    inc8_at(dpa(PAIRS_LIVE));
    /* the two records back on the free list's count */
    flag_set(P_C, 0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | op_adc8(rd8(dba(ACTOR_FREE_TOP)), 0x04, &cpu.p));
    wr8(dba(ACTOR_FREE_TOP), (uint8_t)cpu.a);
}
