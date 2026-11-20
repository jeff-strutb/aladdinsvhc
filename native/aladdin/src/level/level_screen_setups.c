/*
 * level_screen_setups.c: the levels' own screen set-ups, by level from the
 * table at $81:8802: palette cycles and value sequences, the layers on the
 * main and sub screens, colour maths, and per-line effects.
 */
#include <stdio.h>

#include "level/level.h"
#include "system/state.h"
#include "system/unconverted.h"
#include "video/video.h"

#define NMI_SHADOW_SPLIT 1      /* $31 to the NMI's switches: the IRQ on (the NMI itself off until NmiEnable) */
#define CYCLE_RECORD_BYTES 14   /* a palette cycle's record, from $15F1 */
#define SEQUENCES_FROM 0x54     /* the value sequences' records follow the six cycles' at $1645 */
#define SEQUENCE_RECORD_BYTES 16
#define BG2_FOLLOWS_Y 0x40      /* g.bg2_follows: the backdrop layer takes its y only */
#define LINE_CHANNEL_BG2 4      /* the per-line effect on BG2's x (the original's HDMA channel 4) */

/* The original's PaletteCycleStart (81:EC8E) by its record offset from
   $15F1: below $54 a palette cycle (14 bytes a record); from $54 the same
   three bytes of a value sequence's record ($1645 + 16 n: running counted
   up, the number, started cleared), which the levels' set-ups start this
   way. */
static void cycle(uint8_t offset, uint8_t number)
{
    Sequence *s;

    if (offset < SEQUENCES_FROM) {
        PaletteCycleStart((uint8_t)(offset / CYCLE_RECORD_BYTES), number);
        return;
    }
    s = &g.sequences[(offset - SEQUENCES_FROM) / SEQUENCE_RECORD_BYTES];
    s->started = 0;
    s->number = number;
    s->running++;
}

/* The per-line effect on BG2's x from the table at the line data's start
   ($7E:F400, direct, two bytes a line value), that effect alone on. */
static void bg2_scroll_lines(void)
{
    g.line_channel[LINE_CHANNEL_BG2].target = LINE_BG2_X;
    g.line_channel[LINE_CHANNEL_BG2].direct = 1;
    g.line_channel[LINE_CHANNEL_BG2].table_at = 0;
    g.line_table_at[LINE_CHANNEL_BG2 - 4] = 0;
    g.line_effects_on = 1u << LINE_CHANNEL_BG2;
}

/*
 * @routine 81:888B Level00ScreenSetup
 * WHAT IT DOES: Level 0's screen set-up (Level14ScreenSetup's too): layer
 * set-up mode $0A, the backdrop layer following in y only, value sequence
 * 0 started (twice), $18 into two screen map places' high bytes, value
 * sequences 1-3 started with numbers 3, 6, 9, and the per-line effect on
 * BG2's x from $7E:F400.
 * HOW: The record's $7F:F481 and $7F:F495 are the high bytes of the screen
 * map's places $240 and $24A (rows 18, past the 16 TileAt reads), written
 * as the original writes them.
 */
static void Level00ScreenSetup(void)
{
    g.layer_setup_mode = 0x0A;
    g.bg2_follows = BG2_FOLLOWS_Y;
    cycle(0x54, 0x00);
    g.screen_map.places[0x240] = (uint16_t)((g.screen_map.places[0x240] & 0x00FF) | 0x1800);
    g.screen_map.places[0x24A] = (uint16_t)((g.screen_map.places[0x24A] & 0x00FF) | 0x1800);
    cycle(0x54, 0x00);
    cycle(0x64, 0x03);
    cycle(0x74, 0x06);
    cycle(0x84, 0x09);
    bg2_scroll_lines();
}

/*
 * @routine 81:87EF LevelScreenSetup
 * @routine 81:888A LevelNoScreenSetup
 * WHAT IT DOES: Starts the level's screen: the view-edge rule 0, the
 * IRQ's split on at line 8 (the NMI's switches $31), then the level's own
 * set-up by the table at $81:8802 (levels $13, $1E, $1F and $20 on: none).
 */
void LevelScreenSetup(void)
{
    g.view_edge_rule = 0;
    g.nmi_on = 0;
    g.split_on = NMI_SHADOW_SPLIT;
    g.split_line = 0;
    switch ((uint8_t)g.level) {
    case 0x00:
        Level00ScreenSetup();
        break;
    case 0x13:
    case 0x1E:
    case 0x1F:
        break;
    default:
        if (g.level >= 0x20)
            break;
        /* the other levels' set-ups: converted next (stage 5, every
           level's set-up) */
        {
            static char names[0x20][24];

            snprintf(names[g.level], sizeof names[g.level], "Level%02XScreenSetup", (unsigned)g.level);
            unconverted(names[g.level]);
        }
        break;
    }
}
