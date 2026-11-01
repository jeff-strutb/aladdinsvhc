/*
 * escort.c: the actor some levels start beside the player.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"
#include "wram_video.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

/* Per level: 0, no escort; else its $07 value + 1 (bank $81). */
#define LEVEL_ESCORTS 0xB79Eu
#define ESCORT_HIT_BOX 0x08
#define ESCORT_LOOK 0x0220
#define ACTOR_NEW 0x0C

/*
 * @routine 81:85A1 EscortStart
 * WHAT IT DOES: Starts the level's escort actor beside the player, when
 * the level has one.
 * HOW: The level's entry in the escort table, less one, is the escort's
 * $07; its record is made new, with hit box 8, sprite attributes $0220
 * and the player's x and y.
 * Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: no escort: X =
 * the level; A low byte = 0; Z set. Else: M 8-bit; A = the player's y
 * (16 bits); N, Z from it; X = the level. Y, C, V unchanged.
 */
void abi_EscortStart(void)
{
    uint8_t v;

    cpu.x = rd8(abs_db(LEVEL));
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    v = rd8(abs_db((uint16_t)(LEVEL_ESCORTS + cpu.x)));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v == 0)
        return;
    v--;
    wr8(abs_db(ACTOR_ESCORT + ACTOR_07), v);
    wr8(abs_db(ACTOR_ESCORT + ACTOR_STATE), ACTOR_NEW);
    wr8(abs_db(ACTOR_ESCORT + ACTOR_06), ESCORT_HIT_BOX);
    op_rep(P_M);
    wr16(abs_db(ACTOR_ESCORT + ACTOR_LOOK), ESCORT_LOOK);
    cpu.a = rd16(abs_db(PLAYER + ACTOR_X + 1));
    wr16(abs_db(ACTOR_ESCORT + ACTOR_X + 1), cpu.a);
    cpu.a = rd16(abs_db(PLAYER + ACTOR_Y + 1));
    cpu.p = nz16(cpu.p, cpu.a);
    wr16(abs_db(ACTOR_ESCORT + ACTOR_Y + 1), cpu.a);
    op_sep(P_M);
}
