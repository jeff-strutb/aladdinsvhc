/*
 * player_near.c: where an actor is against the player, or a point.
 *
 * The actor is the record the direct page is; the player's is PLAYER.
 * Coordinates are the positions' whole pixels.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
#include "wram_player.h"

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static uint32_t dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* The player's x and y pixels. */
#define PLAYER_X_PIXEL (PLAYER + ACTOR_X + 1)
#define PLAYER_Y_PIXEL (PLAYER + ACTOR_Y + 1)

/* Boxes round an actor: a half-width (and a half-height) per entry. */
#define NEAR_BOX_TABLE 0xBC43u     /* half-width, half-height: 4 bytes an entry */
#define NEAR_POINT_X (SCRATCH + 8)  /* ActorNearPoint's point (words) */
#define NEAR_POINT_Y (SCRATCH + 10)
#define NEAR_SPAN_TABLE 0xBC7Bu    /* half-width: 2 bytes an entry */

/*
 * @routine 81:AB85 ActorSideOfPlayer
 * WHAT IT DOES: Says which way the actor must face to look at the player.
 * HOW: 0 (right) when the player's x is at or right of the actor's, else
 * 1 (left).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A low byte and X = the facing; A high byte = the
 * player's x high byte; N clear, Z from the facing; C from the compare;
 * Y, V unchanged.
 */
void abi_ActorSideOfPlayer(void)
{
    uint8_t side = 0;

    op_rep(P_M);
    cpu.x = 0;
    cpu.p |= P_C;
    op_lda16(abs_db(PLAYER_X_PIXEL));
    op_cmp16(rd16(dp(ACTOR_X + 1)));
    if (!(cpu.p & P_C)) {
        side = 1;
        cpu.x = 1;
        cpu.p = nz8(cpu.p, 1);
    }
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | side);
    cpu.p = nz8(cpu.p, side);
}

/* x - player's x + half, compared with 2 * half: C clear when the actor's
   x is within half of the player's. */
static void within(uint8_t field, uint16_t player_field, uint16_t half_at, uint16_t span_at)
{
    cpu.p |= P_C;
    op_lda16(dp(field));
    cpu.a = op_sbc16(cpu.a, rd16(abs_db(player_field)), &cpu.p);
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, rd16(abs_db(half_at)), &cpu.p);
    op_cmp16(rd16(abs_db(span_at)));
}

/* LDA abs,Y (16-bit) / STA scratch / ASL / STA scratch+2. */
static void half_and_span(uint16_t table, uint16_t at)
{
    op_lda16(((uint32_t)cpu.db << 16) + table + cpu.y);
    wr16(abs_db(at), cpu.a);
    op_asl16();
    wr16(abs_db(at + 2), cpu.a);
}

/*
 * @routine 81:ACDE ActorNearPlayer
 * WHAT IT DOES: Says whether the player is inside a box round the actor:
 * within a half-width across and a half-height up or down.
 * HOW: Takes box Y's half-width and half-height and keeps them and their
 * doubles in scratch; x - player's x + half-width is below the width
 * (unsigned) when within it across; the same for y, tested only when x
 * passes.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the box's byte
 * offset. Exit: C clear when inside; A = the last sum (16 bits); N, Z, C
 * from its compare; scratch $0C-$13 = half-width, width, half-height,
 * height; M 8-bit; X, Y, V unchanged.
 */
void abi_ActorNearPlayer(void)
{
    op_rep(P_M);
    half_and_span(NEAR_BOX_TABLE, SCRATCH);
    half_and_span(NEAR_BOX_TABLE + 2, SCRATCH + 4);
    within(ACTOR_X + 1, PLAYER_X_PIXEL, SCRATCH, SCRATCH + 2);
    if (!(cpu.p & P_C))
        within(ACTOR_Y + 1, PLAYER_Y_PIXEL, SCRATCH + 4, SCRATCH + 6);
    op_sep(P_M);
}

/*
 * @routine 81:AD68 ActorNearPlayerX
 * WHAT IT DOES: Says whether the player is within a half-width of the
 * actor across, whatever the height.
 * HOW: Takes span Y's half-width and keeps it and its double in scratch;
 * x - player's x + half-width is below the width (unsigned) when within.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; Y = the span's byte
 * offset. Exit: C clear when within; A = the sum (16 bits); N, Z, C from
 * its compare; scratch $0C-$0F = half-width, width; M 8-bit; X, Y, V
 * unchanged.
 */
void abi_ActorNearPlayerX(void)
{
    op_rep(P_M);
    half_and_span(NEAR_SPAN_TABLE, SCRATCH);
    within(ACTOR_X + 1, PLAYER_X_PIXEL, SCRATCH, SCRATCH + 2);
    op_sep(P_M);
}

/* A half-size word to `at` and its double to `at` + 2 (the ASL's C). */
static uint16_t half_and_whole(uint16_t half, uint16_t at)
{
    wr16(dba(at), half);
    flag_set(P_C, half & 0x8000);
    wr16(dba((uint16_t)(at + 2)), (uint16_t)(half << 1));
    return (uint16_t)(half << 1);
}

/* Its distance from the point in one axis plus the half-size, against
   the size: C clear when within. */
static int near_within(uint16_t mine, uint16_t point, uint16_t half, uint16_t whole)
{
    uint16_t d;

    flag_set(P_C, 1);
    d = op_sbc16(mine, point, &cpu.p);
    flag_set(P_C, 0);
    d = op_adc16(d, half, &cpu.p);
    cpu.a = d;
    cmp16_of(d, whole);
    return d < whole;
}

/*
 * @routine 81:AD17 ActorNearPoint
 * WHAT IT DOES: Says whether the actor is inside a box round the point
 * in scratch $14, $16: within a half-width across and a half-height up or
 * down.
 * HOW: Takes box Y's half-width and half-height and keeps them and their
 * doubles in scratch; the actor's x - the point's x + half-width is below
 * the width (unsigned) when within it across; the same for y, tested only
 * when x passes.
 * Entry: M 16-bit; D = the actor; DB = $81; Y = the box's byte offset;
 * $14, $16 = the point (words); called by JSL. Exit: C clear when inside;
 * A = the last sum; N, Z, C from its compare; scratch $0C-$13 =
 * half-width, width, half-height, height; M 16-bit; X, Y, V unchanged.
 */
void abi_ActorNearPoint(void)
{
    uint32_t box = dba(NEAR_BOX_TABLE) + cpu.y;

    half_and_whole(rd16(box), SCRATCH);
    half_and_whole(rd16(box + 2), SCRATCH + 4);
    if (near_within(dp16(ACTOR_X + 1), rd16(dba(NEAR_POINT_X)), rd16(dba(SCRATCH)), rd16(dba(SCRATCH + 2))))
        near_within(dp16(ACTOR_Y + 1), rd16(dba(NEAR_POINT_Y)), rd16(dba(SCRATCH + 4)), rd16(dba(SCRATCH + 6)));
}

/*
 * @routine 81:AC68 ActorAimAt
 * WHAT IT DOES: The direction from the actor to the point in scratch $10,
 * $12: an entry of the direction table at $BC03.
 * HOW: The differences across (point x - actor's x) and up or down, made
 * positive; the octant from their signs (+4 the point above, +2 left) and
 * which is larger (+1 up or down); the step in it from the smaller against
 * the larger (under 1/8, 3/8, 5/8, 7/8 of it, else 4, the larger / 8 kept
 * at $0E); the table's byte at the octant * 8 + the step.
 * Entry: M 16-bit, X 16-bit; D = the actor; DB = $81; $10, $12 = the point
 * (words); called by JSL. Exit: M and X 8-bit; A low byte = the direction;
 * X = the table offset, Y = the octant (low bytes); scratch $0C = the
 * octant * 8, $0E = the larger difference / 8.
 */
void abi_ActorAimAt(void)
{
    uint16_t dy, dx, large, small, eighth, a;

    /* the distances to the point (Y: bit 2 above, bit 1 left, bit 0 when
       the y distance is the greater) */
    cpu.y = 0;
    flag_set(P_C, 1);
    dy = op_sbc16(rd16(dba(V_0012)), dp16(ACTOR_Y + 1), &cpu.p);
    if (!(cpu.p & P_C)) {
        dy = (uint16_t)((dy ^ 0xFFFF) + 1);
        cpu.y = 4;
    }
    wr16(dba(SCRATCH), dy);
    flag_set(P_C, 1);
    dx = op_sbc16(rd16(dba(SCRATCH + 4)), dp16(ACTOR_X + 1), &cpu.p);
    if (!(cpu.p & P_C)) {
        dx = (uint16_t)((dx ^ 0xFFFF) + 1);
        cpu.y = (uint16_t)(cpu.y + 2);
    }
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
    /* the step in the octant: the smaller against 1, 3, 5 and 7 eighths
       of the larger */
    cpu.x = 0;
    flag_set(P_C, (large >> 2) & 1);
    eighth = (uint16_t)(large >> 3);
    wr16(dba(SCRATCH + 2), eighth);
    a = eighth;
    cmp16_of(a, small);
    while (a < small) {
        cpu.x = (uint16_t)(cpu.x + 1);
        if (cpu.x == 4)
            break;
        a = op_adc16(a, eighth, &cpu.p);
        a = op_adc16(a, eighth, &cpu.p);
        cmp16_of(a, small);
    }
    wr16(dba(SCRATCH), (uint16_t)(cpu.y << 3));
    flag_set(P_C, 0);
    cpu.x = op_adc16(cpu.x, rd16(dba(SCRATCH)), &cpu.p);
    a16_load(rd16(dba(0xBC03) + cpu.x));
    op_sep(0x30);
}
