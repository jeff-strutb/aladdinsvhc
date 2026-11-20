/*
 * view_edges.c: the view-edge rules: what happens to the player when he
 * reaches a side of the view. ViewEdgeRuleRun runs the rule
 * g.view_edge_rule picks (the table at $81:E986): 0 PlayerKeepOnScreenX,
 * 1 PlayerKeepOnScreen (player/), 2 ViewEdgePush, 3 ViewEdgeHurt, 4
 * ViewEdgeNone.
 */
#include <stdio.h>

#include "data/level_tables.h"
#include "level/level.h"
#include "map/map.h"
#include "player/player.h"
#include "system/state.h"

enum { EDGE_KEEP_X, EDGE_KEEP, EDGE_PUSH, EDGE_HURT, EDGE_NONE };
enum { LIMIT_LEAST = 0, LIMIT_MOST = 1, LIMIT_EDGE = 4 };    /* view_edge_limit's entries */

#define HURT_MARGIN 0x0030      /* ViewEdgeHurt: this far past either side */
#define VIEW_WIDTH 0x0100

static uint16_t view_x(void) { return (uint16_t)(g.layers[0].scroll_x >> 8); }
static uint16_t x_of(const Actor *a) { return (uint16_t)(a->x >> 8); }

/* the view's x plus a limit at or right of the player (the original's C) */
static bool limit_at_or_past(unsigned limit, const Actor *player)
{
    return (uint16_t)(view_x() + view_edge_limit(limit)) >= x_of(player);
}

/*
 * @routine 81:E9CD ViewEdgePush
 * WHAT IT DOES: View-edge rule 2: at either side of the view (16 pixels
 * in), the player is pushed back onto the screen while he is off firm
 * ground; on firm ground he is not, and once at or left of the view's left
 * edge he is hurt and the rule becomes 3 (ViewEdgeHurt).
 */
static void ViewEdgePush(Actor *player)
{
    if (!limit_at_or_past(LIMIT_LEAST, player) && limit_at_or_past(LIMIT_MOST, player))
        return;
    if (!PlayerOnFirmGroundProbe8() && !PlayerOnFirmGroundProbe4()) {
        PlayerKeepOnScreenX(player);
        return;
    }
    if (!limit_at_or_past(LIMIT_EDGE, player))
        return;
    PlayerEnter8CFrom1();
    g.view_edge_rule++;
}

/*
 * @routine 81:EA11 ViewEdgeHurt
 * WHAT IT DOES: View-edge rule 3: when the player is more than 48 pixels
 * past either side of the view, he is hurt and the rule becomes 4 (none).
 * Not while the view is within 48 pixels of the map's left end.
 */
static void ViewEdgeHurt(Actor *player)
{
    uint16_t left = (uint16_t)(view_x() - HURT_MARGIN);

    if (left & 0x8000)
        return;
    if (left < x_of(player) && (uint16_t)(view_x() + VIEW_WIDTH + HURT_MARGIN) >= x_of(player))
        return;
    g.view_edge_rule = EDGE_NONE;
    PlayerEnter8CFrom1();
}

/*
 * @routine 81:E97D ViewEdgeRuleRun
 * @routine 81:EA3C ViewEdgeNone
 * WHAT IT DOES: Runs the view-edge rule in force: what happens to the
 * player as he reaches a side of the view (rule 4, ViewEdgeNone: nothing).
 */
void ViewEdgeRuleRun(Actor *player)
{
    switch (g.view_edge_rule) {
    case EDGE_KEEP_X: PlayerKeepOnScreenX(player); break;
    case EDGE_KEEP:   PlayerKeepOnScreen(player); break;
    case EDGE_PUSH:   ViewEdgePush(player); break;
    case EDGE_HURT:   ViewEdgeHurt(player); break;
    case EDGE_NONE:   break;
    default:
        fprintf(stderr, "level: view-edge rule %u past the rules' table\n", g.view_edge_rule);
        break;
    }
}
