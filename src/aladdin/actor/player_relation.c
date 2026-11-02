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
#include "actor.h"
#include "wram.h"

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
    uint16_t dy, dx, small, large, quarter, a;

    op_rep(0x30);
    cpu.x = PLAYER;
    wr16(dba(SCRATCH + 4), rd16(dba(PLAYER + ACTOR_X + 1)));
    wr16(dba(SCRATCH + 6), rd16(dba(PLAYER + ACTOR_Y + 1)));
    /* the distances (Y: bit 2 when he is above, bit 1 when left) */
    cpu.y = 0;
    flag_set(P_C, 1);
    dy = op_sbc16(rd16(dba(SCRATCH + 6)), dp16(ACTOR_Y + 1), &cpu.p);
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
    uint16_t half, d;

    y8_load(rd8(dpa(ACTOR_FACING)));
    op_rep(P_M);
    /* the box: half its height, its height, its width */
    half = rd16(box);
    wr16(dba(SCRATCH), half);
    flag_set(P_C, half & 0x8000);
    wr16(dba(SCRATCH + 2), (uint16_t)(half << 1));
    wr16(dba(SCRATCH + 4), op_adc16(rd16(box + 2), rd16(box + 4), &cpu.p));
    flag_set(P_C, 0);
    d = op_adc16(dp16(ACTOR_Y + 1), rd16(dba(SCRATCH)), &cpu.p);
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
        d = op_adc16(dp16(ACTOR_X + 1), rd16(dba(0xBC91) + cpu.x), &cpu.p);
        flag_set(P_C, 1);
        d = op_sbc16(d, rd16(dba(PLAYER + ACTOR_X + 1)), &cpu.p);
        cpu.a = d;
        cmp16_of(d, rd16(dba(SCRATCH + 4)));
    }
    op_sep(P_M);
}
