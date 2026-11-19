/*
 * player_relation.c: where the player is from an actor: its direction, and
 * whether it is inside a box in front of the actor.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorDirectionToPlayer(void);
void abi_ActorDirectionToPoint(void);
void abi_PlayerInFrontBox(void);
void abi_ActorAimAt(void);
#include "actor.h"
#include "wram.h"

/*
 * @routine 81:ABFB ActorDirectionToPoint
 * WHAT IT DOES: The direction from the actor to the point at scratch $10,
 * $12, as one of the directions in the table at $BBE3.
 * HOW: The distances across and up or down (made positive, the quadrant
 * kept: 4 for the point above, 2 for left); the band is 0 when the smaller
 * distance is under a quarter of the larger, 1 under three quarters, else
 * 2; the quadrant * 4 plus the band picks the direction.
 * Entry: M and X 16-bit; D = the actor; scratch $10, $12 = the point's x
 * and y; DB = $81; reached from ActorDirectionToPlayer and by a jump from
 * ActorDirectionToPlayerOffset. Exit: M and X 8-bit; A = the direction (16
 * bits, low byte as M 8-bit); X = its index; Y = the quadrant; scratch
 * $0C-$13 the distances.
 */
void abi_ActorDirectionToPoint(void)
{
    uint16_t a, t;

    /* the distances (Y: bit 2 when the point is above, bit 1 when left);
       each compare and add reads its scratch word, as the asm does */
    cpu.y = 0;
    a = rd16(dba(SCRATCH + 6));
    flag_set(P_C, 1);
    a = op_sbc16(a, dp16(ACTOR_Y + 1), &cpu.p);
    if (!(cpu.p & P_C)) {
        a = (uint16_t)((a ^ 0xFFFF) + 1);
        cpu.y = 4;
    }
    wr16(dba(SCRATCH), a);
    a = rd16(dba(SCRATCH + 4));
    flag_set(P_C, 1);
    a = op_sbc16(a, dp16(ACTOR_X + 1), &cpu.p);
    if (!(cpu.p & P_C)) {
        a = (uint16_t)((a ^ 0xFFFF) + 1);
        cpu.y = (uint16_t)(cpu.y + 2);
    }
    /* bit 0 when the y distance is the greater: SCRATCH the smaller, A
       the larger */
    t = rd16(dba(SCRATCH));
    cmp16_of(a, t);
    if (a < t) {
        cpu.y = (uint16_t)(cpu.y + 1);
        cpu.x = rd16(dba(SCRATCH));
        wr16(dba(SCRATCH), a);
        a = cpu.x;
    }
    /* how steep: 0 when the smaller is within a quarter of the larger,
       1 within three quarters, else 2 */
    cpu.x = 0;
    flag_set(P_C, (a >> 1) & 1);
    a = (uint16_t)(a >> 2);
    wr16(dba(SCRATCH + 2), a);
    t = rd16(dba(SCRATCH));
    cmp16_of(a, t);
    if (a < t) {
        cpu.x = 1;
        t = rd16(dba(SCRATCH + 2));
        a = op_adc16(a, t, &cpu.p);
        t = rd16(dba(SCRATCH + 2));
        a = op_adc16(a, t, &cpu.p);
        t = rd16(dba(SCRATCH));
        cmp16_of(a, t);
        if (a < t)
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
 * HOW: 16-bit registers; the player's x and y pixels to scratch $10 and
 * $12; then on into ActorDirectionToPoint.
 * Entry: D = the actor; DB = $81; called by JSL. Exit: as
 * ActorDirectionToPoint.
 */
void abi_ActorDirectionToPlayer(void)
{
    op_rep(0x30);
    cpu.x = PLAYER;
    wr16(dba(SCRATCH + 4), rd16(dba(PLAYER + ACTOR_X + 1)));
    wr16(dba(SCRATCH + 6), rd16(dba(PLAYER + ACTOR_Y + 1)));
    abi_ActorDirectionToPoint();
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
 * $80 (its box table at $BC8F); called by JSL. Exit: M 8-bit; C as above; A = the last sum (16
 * bits); Y = the facing less one when the height passed; scratch $0C-$11 the
 * half-height, height and width.
 */
static void in_front_box(void);

void abi_PlayerInFrontBox(void)
{
    y8_load(rd8(dpa(ACTOR_FACING)));
    in_front_box();
}

/*
 * @routine 81:AF01 PlayerInFrontBoxByFlip
 * WHAT IT DOES: PlayerInFrontBox with the actor's flip ($15) for its
 * facing. Nothing in the game calls it (no JSR, JSL, JMP, pointer or word
 * in the ROM names $81:AF01; PlayerInFrontBox's BRA goes past it):
 * unit_bank81_entries.box calls it at its entry.
 * HOW: Y = the flip; then as PlayerInFrontBox past its facing.
 * Entry: as PlayerInFrontBox. Exit: as PlayerInFrontBox, Y from the flip.
 */
void abi_PlayerInFrontBoxByFlip(void)
{
    y8_load(rd8(dpa(ACTOR_FLIP)));
    in_front_box();
}

/* PlayerInFrontBox past its LDY (at $81:AF03), Y the facing or flip. */
static void in_front_box(void)
{
    uint32_t box = dba(0xBC8F) + cpu.x;
    uint16_t half, d, w;

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
    cmp16_of(d, rd16(dba(SCRATCH + 2)));   /* one read, as the CMP's */
    if (!(cpu.p & P_C)) {
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
 * HOW: PlayerOffsetPoint, then on at ActorDirectionToPoint (81:ABFB),
 * past ActorDirectionToPlayer's putting the player's position at $10, $12.
 * Entry: as PlayerOffsetPoint; called by JSL. Exit: as
 * ActorDirectionToPlayer.
 */
void abi_ActorDirectionToPlayerOffset(void)
{
    player_offset_point();
    abi_ActorDirectionToPoint();
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
