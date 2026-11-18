/*
 * view_edges.c: the view-edge rules: what happens to the player when he
 * reaches a side of the view. ViewEdgeRuleRun runs the rule $145B picks
 * from the table at $81:E986: 0 PlayerKeepOnScreenX, 1 PlayerKeepOnScreen
 * (player/player_bounds.c), 2 ViewEdgePush, 3 ViewEdgeHurt, 4
 * ViewEdgeNone.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_PlayerEnter8CFrom1Long(void);
void abi_PlayerKeepOnScreenX(void);
void abi_PlayerOnFirmGroundProbe4(void);
void abi_PlayerOnFirmGroundProbe8(void);

#define VIEW_EDGE_RULE 0x00145Bu    /* the rule the table at $81:E986 picks (0-4) */
#define VIEW_X 0x001773u            /* the view's left edge (pixels, word) */
#define EDGE_NONE 0x04              /* the rule that does nothing (ViewEdgeNone) */
#define VIEW_EDGE_RULES 0xE986u     /* in bank $81: the rules' routines (words), by rule */
/* In bank $80: x pixels from the view's left edge (words): 16, the least
   the player is kept at; 240, the most; and at +8, 0 (the edge itself) */
#define VIEW_X_LIMITS 0xC9B9u

/*
 * @routine 81:E97D ViewEdgeRuleRun
 * WHAT IT DOES: Runs the view-edge rule in force: what happens to the
 * player as he reaches a side of the view.
 * HOW: JSR through VIEW_EDGE_RULES by VIEW_EDGE_RULE times 2.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's frame work. Exit: as the rule leaves them; X
 * entering the rule = the rule times 2.
 */
void abi_ViewEdgeRuleRun(void)
{
    a8_load(rd8(dba(VIEW_EDGE_RULE)));
    ACC8(asl);
    tax8();
    call_routine(0x810000u | rd16(0x810000u | (uint16_t)(VIEW_EDGE_RULES + cpu.x)), 0);
}

/* VIEW_X plus the limit word at VIEW_X_LIMITS + at, 16-bit, compared
   with the record's x pixels: the flags as the original's CLC, LDA, ADC,
   CMP leave them (C set: the record at or left of that place). */
static void view_edge_compare(uint16_t at)
{
    flag_set(P_C, 0);
    a16_load(rd16(dba(VIEW_X)));
    cpu.a = op_adc16(cpu.a, rd16(dba((uint16_t)(VIEW_X_LIMITS + at))), &cpu.p);
    cmp16_of(cpu.a, dp16(ACTOR_X + 1));
}

/*
 * @routine 81:E9CD ViewEdgePush
 * WHAT IT DOES: View-edge rule 2: at either side of the view (16 pixels
 * in), the player is pushed back onto the screen while he is off firm
 * ground; on firm ground he is not, and once at or left of the view's
 * left edge he is hurt and the rule becomes 3 (ViewEdgeHurt).
 * HOW: VIEW_X plus 16, and plus 240, compared with the record's x pixels.
 * Past neither: nothing. Past one: PlayerOnFirmGroundProbe8, then
 * PlayerOnFirmGroundProbe4; neither firm: PlayerKeepOnScreenX. Firm:
 * VIEW_X (plus 0) not below his x: PlayerEnter8CFrom1Long with 8-bit
 * registers, then VIEW_EDGE_RULE up by one. No script runs this rule:
 * the whole routine is written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from ViewEdgeRuleRun. Exit: M 8-bit, X 8-bit. Pushed: as
 * PlayerKeepOnScreenX leaves A and the flags. Hurt: N and Z from the new
 * rule. Otherwise: A = the last sum (16 bits); N, Z, C from its
 * comparison, V from its addition.
 */
void abi_ViewEdgePush(void)
{
    op_rep(P_M);
    view_edge_compare(0);
    if (!(cpu.p & P_C)) {                 /* right of the left limit */
        view_edge_compare(2);
        if (cpu.p & P_C)                  /* and left of the right one */
            goto out;
    }
    op_sep(P_M);
    jsl_frame(0x81E9E7u);                 /* the JSLs' returns on the stack: the tests' PHD sits below them */
    abi_PlayerOnFirmGroundProbe8();
    cpu.s = (uint16_t)(cpu.s + 3);
    if (!(cpu.p & P_C)) {
        jsl_frame(0x81E9EDu);
        abi_PlayerOnFirmGroundProbe4();
        cpu.s = (uint16_t)(cpu.s + 3);
        if (!(cpu.p & P_C)) {             /* off firm ground: pushed back */
            abi_PlayerKeepOnScreenX();
            goto out;
        }
    }
    /* on firm ground: hurt at or left of the view's left edge */
    op_rep(P_M);
    view_edge_compare(8);
    if (!(cpu.p & P_C))
        goto out;
    op_sep(0x30);
    abi_PlayerEnter8CFrom1Long();
    inc8_at(dba(VIEW_EDGE_RULE));
out:
    op_sep(0x30);
}

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
