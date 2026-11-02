/*
 * view_edges.c: two of the view-edge rules the routine at $81:E97D picks
 * by $145B (the table at $81:E986): what happens to a record that leaves
 * the view sideways.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_PlayerEnter8CFrom1Long(void);

#define VIEW_EDGE_RULE 0x00145Bu    /* the rule the table at $81:E986 picks (0-4) */
#define VIEW_X 0x001773u            /* the view's left edge (pixels, word) */
#define EDGE_NONE 0x04              /* the rule that does nothing (ViewEdgeNone) */

/*
 * @routine 81:EA11 ViewEdgeHurt
 * WHAT IT DOES: View-edge rule 3: when the record is more than 48 pixels
 * past either side of the view, Aladdin is hurt and the rule becomes 4
 * (none).
 * HOW: With the view's x less 48 not negative: its x less 48 at or past
 * the record's, or its x plus 304 below the record's, sets $145B to 4 and
 * calls PlayerEnter8CFrom1Long.
 * Entry: M 8-bit; D = the record; DB a bank whose $0000-$1FFF is work
 * RAM; called by JSR from the table. Exit: M 8-bit. Hurt: as
 * PlayerEnter8CFrom1Long leaves it. Otherwise: A = the last difference (16
 * bits); N, V, Z, C from it.
 */
void abi_ViewEdgeHurt(void)
{
    op_rep(0x20);
    lda16(ea_abs(VIEW_X));
    sec();
    sbc16_v(0x0030);
    if (IF_N)
        goto out;
    sec();
    sbc16_v(rd16(ea_dp(ACTOR_X + 1)));
    if (!IF_C) {
        lda16(ea_abs(VIEW_X));
        clc();
        adc16_v(0x0130);
        sec();
        sbc16_v(rd16(ea_dp(ACTOR_X + 1)));
        if (IF_C)
            goto out;
    }
    op_sep(0x20);
    lda8_v(EDGE_NONE);
    sta8(ea_abs(VIEW_EDGE_RULE));
    abi_PlayerEnter8CFrom1Long();
    return;
out:
    op_sep(0x20);
}

/*
 * @routine 81:EA3C ViewEdgeNone
 * WHAT IT DOES: View-edge rule 4: nothing.
 * HOW: RTS.
 * Entry: called by JSR from the table. Exit: every register and flag
 * unchanged.
 */
void abi_ViewEdgeNone(void)
{
}
