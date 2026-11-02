/*
 * player_near.c: where an actor is against the player.
 *
 * The actor is the record the direct page is; the player's is PLAYER.
 * Coordinates are the positions' whole pixels.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"

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
