/*
 * view_edges.c: two of the view-edge rules the routine at $81:E97D picks
 * by $145B (the table at $81:E986): what happens to a record that leaves
 * the view sideways.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
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
    uint16_t view = rd16(dba(VIEW_X)), x = dp16(ACTOR_X + 1);

    op_rep(P_M);
    flag_set(P_C, 1);
    cpu.a = op_sbc16(view, 0x0030, &cpu.p);
    if (cpu.a & 0x8000)                   /* the view near the map's left end */
        goto out;
    flag_set(P_C, 1);
    cpu.a = op_sbc16(cpu.a, x, &cpu.p);
    if (!(cpu.p & P_C)) {                 /* not more than 48 left of the view */
        flag_set(P_C, 0);
        cpu.a = op_adc16(view, 0x0130, &cpu.p);
        flag_set(P_C, 1);
        cpu.a = op_sbc16(cpu.a, x, &cpu.p);
        if (cpu.p & P_C)                  /* nor 48 past its right edge */
            goto out;
    }
    op_sep(P_M);
    a8_load(EDGE_NONE);
    wr8(dba(VIEW_EDGE_RULE), EDGE_NONE);
    abi_PlayerEnter8CFrom1Long();
    return;
out:
    op_sep(P_M);
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
