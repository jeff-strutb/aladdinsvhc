/*
 * view_near.c: whether an actor is near the middle of the view, across
 * or up and down. No code refers to either test.
 *
 * The actor is the record the direct page is. Coordinates are the
 * positions' whole pixels.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram.h"
void abi_ActorNearViewX(void);
void abi_ActorNearViewY(void);

#define VIEW_LEFT 0x1773u          /* the view's left edge (pixels, word) */
#define VIEW_TOP 0x1777u           /* the view's top edge (pixels, word) */
#define VIEW_HALF 0x0080u          /* from the edge to the view's middle */
#define VIEW_SPAN_X 0xBC7Bu        /* in the data bank ($80): half-widths, a word each */
#define VIEW_SPAN_Y 0xBC85u        /* and half-heights */

/* The test both make: the span's half-size and its double to scratch;
   the actor's coordinate at `mine` (direct page) less the view's edge at
   `edge` less $80, plus the half-size, compared with the size. */
static void near_view(uint16_t spans, uint8_t mine, uint16_t edge)
{
    op_rep(0x20);
    lda16(ea_absy(spans));
    sta16(ea_abs((uint16_t)SCRATCH));
    ACC16(asl);
    sta16(ea_abs((uint16_t)(SCRATCH + 2)));
    sec();
    lda16(ea_dp(mine));
    sbc16_v(rd16(ea_abs(edge)));
    sec();
    sbc16_v(VIEW_HALF);
    clc();
    adc16_v(rd16(ea_abs((uint16_t)SCRATCH)));
    cmp16_v(rd16(ea_abs((uint16_t)(SCRATCH + 2))));
    op_sep(0x20);
}

/*
 * @routine 81:AD48 ActorNearViewX
 * WHAT IT DOES: Says whether the actor is within a half-width of the
 * view's middle across ($80 past its left edge). No code refers to it.
 * HOW: Span Y's half-width kept in scratch with its double; x - the view's
 * left edge - $80 + the half-width is below the width (unsigned) when
 * within.
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
 * and that holds the spans ($80); Y = the span's byte offset; called by
 * JSL. Exit: C clear when within; A = the sum (16 bits); N, Z, C from its
 * compare; scratch $0C-$0F = half-width, width; M 8-bit; X, Y, V
 * unchanged.
 */
void abi_ActorNearViewX(void)
{
    near_view(VIEW_SPAN_X, ACTOR_X + 1, VIEW_LEFT);
}

/*
 * @routine 81:AD84 ActorNearViewY
 * WHAT IT DOES: Says whether the actor is within a half-height of the
 * view's middle up and down ($80 below its top). No code refers to it.
 * HOW: As ActorNearViewX with the half-height of span Y, the actor's y
 * and the view's top.
 * Entry: M 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
 * and that holds the spans ($80); Y = the span's byte offset; called by
 * JSL. Exit: C clear when within; A = the sum (16 bits); N, Z, C from its
 * compare; scratch $0C-$0F = half-height, height; M 8-bit; X, Y, V
 * unchanged.
 */
void abi_ActorNearViewY(void)
{
    near_view(VIEW_SPAN_Y, ACTOR_Y + 1, VIEW_TOP);
}
