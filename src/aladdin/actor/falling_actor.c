/*
 * falling_actor.c: an actor falling down the level: the map tile under it, and its end when
 * it is above the level's top. Drafted by cdraft.py; named and described
 * by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_player.h"
#include "actor.h"

void abi_TileAtProbeLong(void);

/*
 * @routine 83:C903 ActorFallTileTest
 * WHAT IT DOES: For a falling actor: the map tile 10 pixels below its
 * position, Z set when its kind is 2; an actor above the level's top (y
 * negative) is ended instead.
 * HOW: Its x pixels to $000C and its y pixels plus 10 to $000E,
 * TileAtProbeLong, the kind ($0012) compared with 2. Its y negative: its
 * return address dropped (PLA PLA, 8-bit), then the jump to
 * ActorEndPlaced ($83:96CA): ActorFreePlaced and the actor loop.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor's code at its own level (83:C85A). Exit: M 16-bit (as
 * TileAtProbeLong left it); A low = the kind; Z set when it is 2, C from
 * the comparison. Above the top: does not return.
 */
void abi_ActorFallTileTest(void)
{
    uint16_t y;
    uint8_t kind;

    op_rep(0x21);
    wr16(dba(SCRATCH), dp16(ACTOR_X + 1));
    y = dp16(ACTOR_Y + 1);
    a16_load(y);
    if (y & 0x8000) {
        /* above the top of the map: the actor ends */
        op_sep(P_M);
        pla8();                           /* its own return dropped */
        pla8();
        actor_leave(0x8396CAu);           /* ActorEndPlaced */
        return;
    }
    /* the tile 10 pixels below it */
    cpu.a = op_adc16(y, 0x000A, &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
    abi_TileAtProbeLong();
    kind = rd8(dba(V_0012));
    a8_load(kind);
    cmp8_of(kind, 0x02);
}

