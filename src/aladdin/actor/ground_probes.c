/*
 * ground_probes.c: tests of the ground round an actor of bank $A5: a
 * wall in front, a ledge ahead with room above it, ground above a point
 * toward the player.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorLedgeAhead(void);
void abi_ActorOutOfWall8(void);
void abi_ActorOutOfWallLong(void);
void abi_ActorProbeUpTowardPlayer(void);
void abi_PointInGroundLong(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define PROBES_LEFT 0x002Cu  /* scratch: probes left */

/*
 * @routine A5:EB17 ActorOutOfWall8
 * WHAT IT DOES: Moves the actor out of a wall at its probe 8 in front:
 * C set when it was moved.
 * HOW: ActorOutOfWallLong with A = 8.
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
 * ActorOutOfWall.
 */
void abi_ActorOutOfWall8(void)
{
    a8_load(0x08);
    abi_ActorOutOfWallLong();
}

/* The point a probe up from the last (16 pixels) */
static void probe_up(void)
{
    op_rep(P_M);
    flag_set(P_C, 0);
    cpu.a = op_adc16(rd16(dba(SCRATCH + 2)), 0xFFF0, &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
}

/* PointInGroundLong at the point (P kept across it), then whether its
   tile is ground (the kind's low 3 bits). */
static int ground_at_point(void)
{
    uint8_t kind;

    php();
    abi_PointInGroundLong();
    plp();
    op_sep(0x30);
    kind = rd8(dba(V_0012)) & 0x07;
    a8_load(kind);
    return kind != 0;
}

/*
 * @routine A5:EA2C ActorLedgeAhead
 * WHAT IT DOES: Looks for ground 48 pixels ahead of the actor with room
 * above it, from 32 pixels below its y up: C clear when found within four
 * tiles.
 * HOW: The point 48 pixels in front by the facing ($30, or -$30 facing
 * left) and 32 below; up to 4 probes (PointInGroundLong, a kind in the
 * behaviour's low 3 bits being ground), a tile up each time: first while
 * no ground, then, once in ground, a tile up at a time until a probe finds
 * none (C clear). C set when the probes run out.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSR. Exit:
 * M 8-bit, X 8-bit; C as above; the point at the last probe.
 */
void abi_ActorLedgeAhead(void)
{
    uint8_t facing = rd8(dpa(ACTOR_FACING));
    uint16_t ahead = facing ? 0xFFD0 : 0x0030;

    /* 48 pixels ahead, 32 below: up to 4 probes up for ground, then up
       to the rest for the space above it */
    op_rep(P_M);
    a16_load(0x0030);
    x8_load(facing);
    if (facing)
        a16_load(0xFFD0);
    flag_set(P_C, 0);
    wr16(dba(SCRATCH), op_adc16(ahead, dp16(ACTOR_X + 1), &cpu.p));
    a16_load(0x0020);
    flag_set(P_C, 0);
    cpu.a = op_adc16(0x0020, dp16(ACTOR_Y + 1), &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
    a16_load(0x0004);
    wr16(dba(PROBES_LEFT), 0x0004);
    while (!ground_at_point()) {
        if (!dec8_at(dba(PROBES_LEFT)))
            goto none;
        probe_up();
    }
    do {
        probe_up();
        if (!ground_at_point()) {
            flag_set(P_C, 0);
            return;
        }
    } while (dec8_at(dba(PROBES_LEFT)));
none:
    flag_set(P_C, 1);
}

/*
 * @routine A5:F242 ActorProbeUpTowardPlayer
 * WHAT IT DOES: Probes up from a point offset from the actor toward the
 * player, a tile at a time for X probes, for a tile whose behaviour has
 * bit 3 or 6: C clear when one is found.
 * HOW: The point's y = the actor's + A; its x = the actor's + Y when the
 * player is not left of the actor, else - Y; X probes (PointInGroundLong,
 * the behaviour byte tested with $48), the point a tile up after each.
 * Entry: M 16-bit, X 8-bit; A = the y offset; X = the probes; Y = the x
 * offset; D = the actor; DB = $81; called by JSR. Exit: M 16-bit, X
 * 8-bit; C as above.
 */
void abi_ActorProbeUpTowardPlayer(void)
{
    uint16_t px, offset;
    uint8_t kind;

    op_rep(P_M);
    flag_set(P_C, 0);
    cpu.a = op_adc16(cpu.a, dp16(ACTOR_Y + 1), &cpu.p);
    wr16(dba(SCRATCH + 2), cpu.a);
    cpu.a = (uint8_t)cpu.x;               /* the probes */
    wr16(dba(PROBES_LEFT), cpu.a);
    /* across: Y pixels the player's way */
    px = rd16(dba(PLAYER + ACTOR_X + 1));
    cmp16_of(px, dp16(ACTOR_X + 1));
    offset = cpu.y;
    nz16_of(offset);
    if (px < dp16(ACTOR_X + 1))
        offset = (uint16_t)((offset ^ 0xFFFF) + 1);
    flag_set(P_C, 0);
    cpu.a = op_adc16(offset, dp16(ACTOR_X + 1), &cpu.p);
    wr16(dba(SCRATCH), cpu.a);
    for (;;) {                            /* up 16 a probe till one meets a wall */
        abi_PointInGroundLong();
        op_sep(0x30);
        kind = rd8(dba(V_0012));
        a8_load(kind);
        flag_set(P_Z, !(kind & 0x48));
        if (kind & 0x48) {
            op_rep(P_M);
            flag_set(P_C, 0);
            return;
        }
        op_rep(P_M);
        flag_set(P_C, 0);
        cpu.a = op_adc16(rd16(dba(SCRATCH + 2)), 0xFFF0, &cpu.p);
        wr16(dba(SCRATCH + 2), cpu.a);
        if (!dec16_at(dba(PROBES_LEFT))) {
            flag_set(P_C, 1);
            return;
        }
    }
}
