/*
 * player_relation.c: where the player is from an actor: its direction, and
 * whether it is inside a box in front of the actor.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorDirectionToPlayer(void);
void abi_PlayerInFrontBox(void);
void abi_ActorAimAt(void);
#include "actor.h"
#include "wram.h"

/* ActorDirectionToPlayer from 81:ABFB: the direction from the actor to
   the point at scratch $10, $12 (the table at $BBE3), M and X 16-bit. */
static void direction_to_point(void)
{
    uint16_t dy, dx, small, large, quarter, a;

    /* the distances (Y: bit 2 when the point is above, bit 1 when left) */
    cpu.y = 0;
    flag_set(P_C, 1);
    dy = rd16(dba(SCRATCH + 6));
    dy = op_sbc16(dy, dp16(ACTOR_Y + 1), &cpu.p);
    if (!(cpu.p & P_C)) {
        dy = (uint16_t)((dy ^ 0xFFFF) + 1);
        cpu.y = 4;
    }
    wr16(dba(SCRATCH), dy);
    flag_set(P_C, 1);
    dx = rd16(dba(SCRATCH + 4));
    dx = op_sbc16(dx, dp16(ACTOR_X + 1), &cpu.p);
    if (!(cpu.p & P_C)) {
        dx = (uint16_t)((dx ^ 0xFFFF) + 1);
        cpu.y = (uint16_t)(cpu.y + 2);
    }
    /* bit 0 when the y distance is the greater: SCRATCH the smaller */
    cmp16_of(dx, dy);
    large = dx;
    small = dy;
    if (dx < dy) {
        cpu.y = (uint16_t)(cpu.y + 1);
        cpu.x = dy;
        wr16(dba(SCRATCH), dx);
        large = dy;
        small = dx;
    }
    /* how steep: 0 when the smaller is within a quarter of the larger,
       1 within three quarters, else 2 */
    cpu.x = 0;
    flag_set(P_C, (large >> 1) & 1);
    quarter = (uint16_t)(large >> 2);
    wr16(dba(SCRATCH + 2), quarter);
    cmp16_of(quarter, small);
    if (quarter < small) {
        cpu.x = 1;
        a = op_adc16(quarter, quarter, &cpu.p);
        a = op_adc16(a, quarter, &cpu.p);
        cmp16_of(a, small);
        if (a < small)
            cpu.x = 2;
    }
    wr16(dba(SCRATCH), (uint16_t)(cpu.y << 2));
    flag_set(P_C, 0);
    cpu.x = op_adc16(cpu.x, rd16(dba(SCRATCH)), &cpu.p);
    a16_load(rd16(dba(0xBBE3) + cpu.x));
    op_sep(0x30);
}

/*
 * @routine 81:ABEA ActorDirectionToPlayer
 * WHAT IT DOES: The direction from the actor to the player, as one of the
 * directions in the table at $BBE3.
 * HOW: The distances across and up or down (made positive, the quadrant
 * kept: 4 for the player above, 2 for left); the band is 0 when the smaller
 * distance is under a quarter of the larger, 1 under three quarters, else
 * 2; the quadrant * 4 plus the band picks the direction.
 * Entry: D = the actor; DB = $81; called by JSL. Exit: M and X 8-bit; A =
 * the direction (16 bits, low byte as M 8-bit); X = its index; Y = the
 * quadrant; scratch $0C-$13 the distances.
 */
void abi_ActorDirectionToPlayer(void)
{
    op_rep(0x30);
    cpu.x = PLAYER;
    wr16(dba(SCRATCH + 4), rd16(dba(PLAYER + ACTOR_X + 1)));
    wr16(dba(SCRATCH + 6), rd16(dba(PLAYER + ACTOR_Y + 1)));
    direction_to_point();
}

/*
 * @routine 81:AEFD PlayerInFrontBox
 * WHAT IT DOES: Says whether the player is inside box X in front of the
 * actor: C clear when inside.
 * HOW: Height: the actor's y plus the half-height less the player's y,
 * below the height (twice the half-height). Width: the actor's x plus the
 * box's offset for its facing less the player's x, below the box's width
 * (the two offsets' sum), tested only when the height passes.
 * Entry: M 8-bit, X 8-bit; D = the actor; X = the box's byte offset; DB =
 * $81; called by JSL. Exit: M 8-bit; C as above; A = the last sum (16
 * bits); Y = the facing less one when the height passed; scratch $0C-$11 the
 * half-height, height and width.
 */
void abi_PlayerInFrontBox(void)
{
    uint32_t box = dba(0xBC8F) + cpu.x;
    uint16_t half, d, w;

    y8_load(rd8(dpa(ACTOR_FACING)));
    op_rep(P_M);
    /* the box: half its height, its height, its width */
    half = rd16(box);
    wr16(dba(SCRATCH), half);
    flag_set(P_C, half & 0x8000);
    wr16(dba(SCRATCH + 2), (uint16_t)(half << 1));
    w = rd16(box + 2);
    wr16(dba(SCRATCH + 4), op_adc16(w, rd16(box + 4), &cpu.p));
    flag_set(P_C, 0);
    d = dp16(ACTOR_Y + 1);
    d = op_adc16(d, rd16(dba(SCRATCH)), &cpu.p);
    flag_set(P_C, 1);
    d = op_sbc16(d, rd16(dba(PLAYER + ACTOR_Y + 1)), &cpu.p);
    cpu.a = d;
    cmp16_of(d, rd16(dba(SCRATCH + 2)));
    if (d < rd16(dba(SCRATCH + 2))) {
        /* across, on the side it faces */
        cpu.y = (uint8_t)(cpu.y - 1);
        nz8_of((uint8_t)cpu.y);
        if (!(cpu.y & 0x80))
            cpu.x = (uint8_t)(cpu.x + 2);
        d = dp16(ACTOR_X + 1);
        d = op_adc16(d, rd16(dba(0xBC91) + cpu.x), &cpu.p);
        flag_set(P_C, 1);
        d = op_sbc16(d, rd16(dba(PLAYER + ACTOR_X + 1)), &cpu.p);
        cpu.a = d;
        cmp16_of(d, rd16(dba(SCRATCH + 4)));
    }
    op_sep(P_M);
}

/* Helpers no code refers to: each runs only in the unit calls of
   cov_helpers_unused.box. */
#define OFFSET_POINTS 0xBBDFu       /* (data bank) per entry: x offset along the facing, y offset (words) */

/* The point at scratch $10, $12: the player offset by the entry at
   OFFSET_POINTS + X, across the way the actor's flip says. */
static void player_offset_point(void)
{
    uint16_t x, d;

    op_rep(0x30);
    a16_load((uint16_t)(dp16(ACTOR_FLIP) & 0x0001));
    x = rd16(dba((uint16_t)(PLAYER + ACTOR_X + 1)));
    if (cpu.a == 0) {
        flag_set(P_C, 0);
        d = rd16(dba((uint16_t)(OFFSET_POINTS + cpu.x)));
        x = op_adc16(x, d, &cpu.p);
    } else {
        flag_set(P_C, 1);
        d = rd16(dba((uint16_t)(OFFSET_POINTS + cpu.x)));
        x = op_sbc16(x, d, &cpu.p);
    }
    wr16(dba(SCRATCH + 4), x);
    flag_set(P_C, 0);
    x = rd16(dba((uint16_t)(PLAYER + ACTOR_Y + 1)));
    d = rd16(dba((uint16_t)(OFFSET_POINTS + 2 + cpu.x)));
    a16_load(op_adc16(x, d, &cpu.p));
    wr16(dba(SCRATCH + 6), cpu.a);
}

/*
 * @routine 81:ABC0 PlayerOffsetPoint
 * WHAT IT DOES: The point at scratch $10, $12: the player's position
 * offset by entry X of OFFSET_POINTS: across, added or taken away by the
 * actor's flip, and down. No code refers to it.
 * HOW: M and X 16-bit; the actor's flip bit 0 clear: the player's x plus
 * the entry's x offset, set: less it; the player's y plus the entry's y
 * offset.
 * Entry: D = the actor; X = the entry's offset (high byte 0); DB = $81;
 * called by JSR. Exit: M and X 16-bit; A = the point's y; N, V, Z, C from
 * its ADC; X, Y unchanged.
 */
void abi_PlayerOffsetPoint(void)
{
    player_offset_point();
}

/*
 * @routine 81:ABB4 ActorDirectionToPlayerOffset
 * WHAT IT DOES: The direction from the actor to a point by the player
 * (PlayerOffsetPoint), as ActorDirectionToPlayer gives it to the player.
 * No code refers to it.
 * HOW: PlayerOffsetPoint, then into ActorDirectionToPlayer after it puts
 * the player's position at $10, $12 (81:ABFB).
 * Entry: as PlayerOffsetPoint; called by JSL. Exit: as
 * ActorDirectionToPlayer.
 */
void abi_ActorDirectionToPlayerOffset(void)
{
    player_offset_point();
    direction_to_point();
}

/*
 * @routine 81:ABBA ActorAimAtPlayerOffset
 * WHAT IT DOES: The direction from the actor to a point by the player
 * (PlayerOffsetPoint), as ActorAimAt gives it. No code refers to it.
 * HOW: PlayerOffsetPoint, then on into ActorAimAt.
 * Entry: as PlayerOffsetPoint; called by JSL. Exit: as ActorAimAt.
 */
void abi_ActorAimAtPlayerOffset(void)
{
    player_offset_point();
    abi_ActorAimAt();
}
