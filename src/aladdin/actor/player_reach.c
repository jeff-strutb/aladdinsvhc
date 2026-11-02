/*
 * player_reach.c: whether the player is within an actor's reach, by an entry of the reach
 * table. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"

/* In bank $80: 8 bytes an entry: the x reach, the y reach (bytes), ... */
#define REACH_TABLE 0xD9B0u
#define PLAYER_X (PLAYER + ACTOR_X + 1)
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)

#define REACH_HALF_X 0x002Cu  /* scratch words: the reach, and twice it */
#define REACH_SPAN_X 0x002Eu

/*
 * @routine 83:FCF5 PlayerInReach
 * WHAT IT DOES: Tells whether the player is within entry X's reach of the
 * actor: C clear when in reach (a reach of 0 is not tested).
 * HOW: The x reach r, nonzero: the actor's x pixels plus r less the
 * player's compared unsigned with 2r; C set (out) returns. Then the y
 * reach r, nonzero: its y pixels plus r less the player's compared with r.
 * 16-bit A; $002C, $002E hold r and 2r.
 * Entry: M 8-bit, X 8-bit; X = the entry's offset; D = the actor's record;
 * DB = $80; called by JSR (by 83:FCA5). The game passes only entry 0,
 * whose reaches are 0 (83:AED2 loads X = 8, then X = 0): the comparisons
 * are reached only by unit_player_in_reach.box. Exit: M 16-bit; C clear:
 * in reach; C set: out. A as the last comparison left it.
 */
void abi_PlayerInReach(void)
{
    uint32_t entry = dba(REACH_TABLE) + cpu.x;
    uint16_t reach, span, d;

    op_rep(P_M);
    flag_set(P_C, 0);
    /* across: his x within the reach either side of its */
    reach = rd16(entry) & 0x00FF;
    a16_load(reach);
    if (reach) {
        wr16(dba(REACH_HALF_X), reach);
        span = (uint16_t)(reach << 1);
        flag_set(P_C, reach & 0x8000);
        wr16(dba(REACH_SPAN_X), span);
        flag_set(P_C, 0);
        d = op_adc16(dp16(ACTOR_X + 1), rd16(dba(REACH_HALF_X)), &cpu.p);
        flag_set(P_C, 1);
        d = op_sbc16(d, rd16(dba(PLAYER_X)), &cpu.p);
        cpu.a = d;
        cmp16_of(d, rd16(dba(REACH_SPAN_X)));
        if (d >= span)
            return;
    }
    /* down: his y within the reach below its */
    reach = rd16(entry + 1) & 0x00FF;
    a16_load(reach);
    if (!reach)
        return;
    wr16(dba(REACH_HALF_X), reach);
    flag_set(P_C, 0);
    d = op_adc16(reach, dp16(ACTOR_Y + 1), &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, rd16(dba(PLAYER_Y)), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(dba(REACH_HALF_X)));
}

