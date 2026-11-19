/*
 * kind49.c: actor kind $49, which carries the player along as it moves:
 * its start and its per-frame work.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorCarryPlayerLong(void);
void abi_ActorTestCarriesLong(void);
void abi_ActorFreeOutOfRangeLong(void);

#define K49_CARRY_BOX ACTOR_4C  /* the box ActorTestCarries tests the player with */
#define K49_BOXES 0x8068u       /* (bank $84) the parts by $07: each loads its box */

/*
 * @routine 84:804D Kind49Carry
 * @actor
 * WHAT IT DOES: Actor kind $49: each frame it carries the player along by
 * however far it moved (its own moves are its kind's wait work).
 * HOW: ACTOR_09 bits $90 set (bit 7: its kind's routine runs each frame
 * it waits); then each frame a yield and ActorCarryPlayerLong.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
 * the actor loop's jump for kind $49 in state $0C (the table at $83:8CAB,
 * through the stub at $83:923C). Exit: never returns.
 */
void abi_Kind49Carry(void)
{
    uint8_t flags = (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90);

    wr8(dpa(ACTOR_09), flags);
    a8_load(flags);
    for (;;) {
        actor_yield(0x848053u);
        jsl_frame(0x848055u);
        abi_ActorCarryPlayerLong();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
}

/*
 * @routine 84:805B Kind49Wait
 * WHAT IT DOES: What actors of kind $49 do each frame they wait: test
 * whether they carry the player, with a box by their $07 ($34, $38 or
 * $3C), and end when out of range.
 * HOW: JMP through the table at $84:8062 by $07 * 2 to a part that loads
 * Y with the box ($84:8068 for $07 0, which no script meets: LDY #$34);
 * K49_CARRY_BOX = Y; X = 0, ActorTestCarriesLong; X = $18 and on into
 * ActorFreeOutOfRangeLong (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL from
 * the kind's stub ($83:9240) through the actor loop's wait table at
 * $83:8F4F. Exit: as ActorFreeOutOfRangeLong.
 */
void abi_Kind49Wait(void)
{
    static const uint8_t box[3] = {0x34, 0x38, 0x3C};
    uint8_t v = rd8(dpa(ACTOR_07));

    a8_load((uint8_t)(v << 1));
    flag_set(P_C, v & 0x80);
    x8_load((uint8_t)(v << 1));
    if (v > 2) {                        /* past the table's three words: no $07 names it */
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(0x8062 + cpu.x)));
        return;
    }
    y8_load(box[v]);
    wr8(dpa(K49_CARRY_BOX), box[v]);
    x8_load(0x00);
    jsl_frame(0x848076u);
    abi_ActorTestCarriesLong();
    cpu.s = (uint16_t)(cpu.s + 3);
    x8_load(0x18);
    abi_ActorFreeOutOfRangeLong();
}

