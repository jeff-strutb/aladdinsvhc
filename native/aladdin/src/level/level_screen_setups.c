/*
 * level_screen_setups.c: the levels' own screen set-ups, by level from the
 * table at $81:8802: palette cycles and value sequences, the layers on the
 * main and sub screens, colour maths, and per-line effects.
 *
 * Not converted, with their proof (no JSR, JSL, JMP or table word names
 * them; the record's own notes): BackdropSplitSetup (81:8D70),
 * ModeBandsSetup (81:8EC1) and ModeBandsSetupLong (81:8EBD).
 */
#include <stdio.h>

#include "level/level.h"
#include "system/state.h"
#include "system/task.h"
#include "system/unconverted.h"
#include "video/video.h"
#include "video/video_tasks.h"

#define NMI_SHADOW_SPLIT 1      /* $31 to the NMI's switches: the IRQ on (the NMI itself off until NmiEnable) */
#define CYCLE_RECORD_BYTES 14   /* a palette cycle's record, from $15F1 */
#define SEQUENCES_FROM 0x54     /* the value sequences' records follow the six cycles' at $1645 */
#define SEQUENCE_RECORD_BYTES 16
#define BG2_FOLLOWS_Y 0x40      /* g.bg2_follows: the backdrop layer takes its y only */
#define LINE_CHANNEL_BG2 4      /* the per-line effect on BG2's x (the original's HDMA channel 4) */
#define SCENE_SLOT 3            /* the level's own task */
#define EFFECT_SLOT 6           /* its screen effect's task */

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
 * WHAT IT DOES: Level 0's (and $14's and $1D's) screen set-up: layer
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


/* the screen map place's high byte (the original's writes to $7F:F000 +
   2 * place + 1) */
static void place_high(unsigned place, uint8_t v)
{
    g.screen_map.places[place] = (uint16_t)((g.screen_map.places[place] & 0x00FF) | (uint16_t)v << 8);
}

/* the layers on the main and sub screens, and the colour maths (the
   original's colour-maths shadows, by their bits) */
static void layers(uint8_t main, uint8_t sub, uint8_t cgwsel, uint8_t cgadsub)
{
    g.main_layers = main;
    g.sub_layers = sub;
    g.blend_direct_colour = cgwsel & 0x01;
    g.blend_use_sub = (uint8_t)((cgwsel >> 1) & 0x01);
    g.blend_prevent = (uint8_t)((cgwsel >> 4) & 0x03);
    g.blend_clip = (uint8_t)((cgwsel >> 6) & 0x03);
    g.blend_layers = (uint8_t)(cgadsub & 0x3F);
    g.blend_half = (uint8_t)((cgadsub >> 6) & 0x01);
    g.blend_subtract = (uint8_t)(cgadsub >> 7);
}

/*
 * @routine 81:88C4 Level01ScreenSetup
 * WHAT IT DOES: Level 1's screen set-up: nothing.
 */

/*
 * @routine 81:88C5 Level02ScreenSetup
 * WHAT IT DOES: Level 2's (and $15's) screen set-up: layer set-up mode
 * $0E, the backdrop layer following in y only, $08 and $06 into the screen
 * map places $220 and $22B's high bytes ($7F:F441, $F457), the IRQ's line
 * $0C, and the per-line effect on BG2's x.
 */
static void Level02ScreenSetup(void)
{
    g.layer_setup_mode = 0x0E;
    g.bg2_follows = BG2_FOLLOWS_Y;
    place_high(0x220, 0x08);
    place_high(0x22B, 0x06);
    g.split_line = 0x0C;
    bg2_scroll_lines();
}

/*
 * @routine 81:8908 Level03ScreenSetup
 * WHAT IT DOES: Level 3's screen set-up: layer set-up mode $0A, the
 * backdrop following in y only, $02 into places $240 and $244's high bytes
 * ($7F:F481, $F489), its scene (Level03Scene, slot 3), value sequences 0-3
 * (0, 3, 6, 9) and the per-line effect on BG2's x.
 */
static void Level03ScreenSetup(void)
{
    g.layer_setup_mode = 0x0A;
    g.bg2_follows = BG2_FOLLOWS_Y;
    place_high(0x240, 0x02);
    place_high(0x244, 0x02);
    TaskCreate(SCENE_SLOT, 0, TASK_ENTRY_LEVEL03_SCENE);
    cycle(0x54, 0x00);
    cycle(0x64, 0x03);
    cycle(0x74, 0x06);
    cycle(0x84, 0x09);
    bg2_scroll_lines();
}

/*
 * @routine 81:8943 Level04ScreenSetup
 * WHAT IT DOES: Level 4's (and $16's) screen set-up: CameraTriggerTask in
 * slot 3, palette cycle 2 in record 0.
 */
static void Level04ScreenSetup(void)
{
    TaskCreate(SCENE_SLOT, 0, TASK_ENTRY_CAMERA_TRIGGER);
    cycle(0x00, 0x02);
}

/*
 * @routine 81:8954 Level05ScreenSetup
 * WHAT IT DOES: Level 5's screen set-up: the camera window's step $28; its
 * end scene (slot 3) and window effect $14 (slot 6); palette cycles $0A,
 * $0C, $25; layers 1-3 and sprites on the main screen, sprites on the sub
 * screen, colour maths adding it to layers 1 and 2 and sprites, at half.
 */
static void Level05ScreenSetup(void)
{
    g.camera_window_step = 0x28;
    TaskCreate(SCENE_SLOT, 0, TASK_ENTRY_LEVEL05_END_SCENE);
    TaskCreate(EFFECT_SLOT, 20, TASK_ENTRY_WINDOW_EFFECT);
    cycle(0x00, 0x0A);
    cycle(0x0E, 0x0C);
    cycle(0x1C, 0x25);
    layers(0x17, 0x10, 0x03, 0x53);
}

/*
 * @routine 81:8995 Level06ScreenSetup
 * WHAT IT DOES: Level 6's screen set-up: BandsSpeedUpTask (slot 3) and
 * ColourBandTask (slot 6); value sequences $0C, $0F, $12, $15 and palette
 * cycles 4-7; the view-edge rule 3, scroll kind 2, layer set-up mode $0C,
 * the backdrop following in y only; per-line effects on channels 1 (the main
 * screen's layers from the table at $80:B99E) and 2 (BG2's x from $7E:F400), the band
 * lines from $80:B991 copied to the band table. The per-line part waits
 * for a per-line effect that names a table in the cartridge (video/'s
 * LineChannel names g.line_data only): reported, not made.
 */
static void Level06ScreenSetup(void)
{
    TaskCreate(SCENE_SLOT, 0, TASK_ENTRY_BANDS_SPEED_UP);
    TaskCreate(EFFECT_SLOT, 0, TASK_ENTRY_COLOUR_BAND);
    cycle(0x54, 0x0C);
    cycle(0x64, 0x0F);
    cycle(0x74, 0x12);
    cycle(0x84, 0x15);
    cycle(0x00, 0x04);
    cycle(0x0E, 0x05);
    cycle(0x1C, 0x06);
    cycle(0x2A, 0x07);
    g.view_edge_rule = 3;
    g.scroll_kind = 0x02;
    g.layer_setup_mode = 0x0C;
    g.bg2_follows = BG2_FOLLOWS_Y;
    unconverted("Level06ScreenSetup: its per-line bands (a table in the cartridge)");
}

/*
 * @routine 81:8A4A Level07ScreenSetup
 * WHAT IT DOES: Level 7's (and $17's) screen set-up: RedOutTask (slot 6);
 * palette cycles $11, $12, $13; the view-edge rule 1, scroll kind 6,
 * layers 1-3 and sprites on the main screen; packed set $1A unpacked;
 * layer set-up mode $12; BG2's map at $3800, 64 x 64.
 */
static void Level07ScreenSetup(void)
{
    TaskCreate(EFFECT_SLOT, 0, TASK_ENTRY_RED_OUT);
    cycle(0x00, 0x11);
    cycle(0x0E, 0x12);
    cycle(0x1C, 0x13);
    g.view_edge_rule = 1;
    g.scroll_kind = 0x06;
    g.main_layers = 0x17;
    UnpackSet(0x1A);
    g.layer_setup_mode = 0x12;
    g.bg_map_at[1] = 0x3800;
    g.bg_map_size[1] = 3;
}

/*
 * @routine 81:8A88 Level08ScreenSetup
 * WHAT IT DOES: Level 8's (and $18's) screen set-up: value sequence $2D,
 * palette cycles 8, 9, $0E, $0F, $10; layers 1-3 and sprites on the main
 * screen, sprites on the sub screen, colour maths at half.
 */
static void Level08ScreenSetup(void)
{
    cycle(0x54, 0x2D);
    cycle(0x00, 0x08);
    cycle(0x0E, 0x09);
    cycle(0x1C, 0x0E);
    cycle(0x2A, 0x0F);
    cycle(0x38, 0x10);
    layers(0x17, 0x10, 0x03, 0x53);
}

/*
 * @routine 81:8AC7 Level09ScreenSetup
 * WHAT IT DOES: Level 9's screen set-up: palette cycles $19, $1A, $26,
 * $27; the layers and colour maths as level 8's.
 */
static void Level09ScreenSetup(void)
{
    cycle(0x00, 0x19);
    cycle(0x0E, 0x1A);
    cycle(0x1C, 0x26);
    cycle(0x2A, 0x27);
    layers(0x17, 0x10, 0x03, 0x53);
}

/*
 * @routine 81:8AE5 Level0AScreenSetup
 * WHAT IT DOES: Level $0A's (and $19's) screen set-up: the camera window's
 * step $50; ClimbTopTask (slot 3); palette cycles $1D, $1E, $23; from the
 * restart point, $FE into screen map place $21's high byte ($7F:F043).
 * (The original also writes $00BD to $1C75, which nothing reads.)
 */
static void Level0AScreenSetup(void)
{
    g.camera_window_step = 0x50;
    TaskCreate(SCENE_SLOT, 0, TASK_ENTRY_CLIMB_TOP);
    cycle(0x00, 0x1D);
    cycle(0x0E, 0x1E);
    cycle(0x1C, 0x23);
    if (g.level_restart)
        place_high(0x21, 0xFE);
}

/* the eight cycles and sequences levels $0B and $0C share */
static void cycles_0b(void)
{
    cycle(0x00, 0x00);
    cycle(0x0E, 0x01);
    cycle(0x1C, 0x03);
    cycle(0x2A, 0x0B);
    cycle(0x54, 0x18);
    cycle(0x64, 0x1B);
    cycle(0x74, 0x1E);
    cycle(0x84, 0x21);
}

/*
 * @routine 81:8B1C Level0BScreenSetup
 * WHAT IT DOES: Level $0B's screen set-up: eight palette cycles and value
 * sequences; layers 1-3 and sprites on the main screen, layers 1 and 3 on
 * the sub screen, colour maths on all but layer 3, at half; layer set-up
 * mode 2.
 */
static void Level0BScreenSetup(void)
{
    cycles_0b();
    layers(0x17, 0x15, 0x03, 0x73);
    g.layer_setup_mode = 0x02;
}

/*
 * @routine 81:8B6E Level0CScreenSetup
 * WHAT IT DOES: Level $0C's screen set-up: level $0B's cycles and sequences.
 */
static void Level0CScreenSetup(void)
{
    cycles_0b();
}

/*
 * @routine 81:8BA7 Level0DScreenSetup
 * WHAT IT DOES: Level $0D's screen set-up: its scene (slot 3); palette
 * cycles 1, 3 in records 1, 2; layer set-up mode $14.
 */
static void Level0DScreenSetup(void)
{
    TaskCreate(SCENE_SLOT, 0, TASK_ENTRY_LEVEL0D_SCENE);
    cycle(0x0E, 0x01);
    cycle(0x1C, 0x03);
    g.layer_setup_mode = 0x14;
}

/*
 * @routine 81:8BC4 Level0EScreenSetup
 * WHAT IT DOES: Level $0E's screen set-up: GreyPastTask (slot 6); palette
 * cycles $0D, $1B and value sequence $15; layers 1-3 and sprites on both
 * screens; layer set-up mode 8; the switch flags from the stage's mask.
 */
static void Level0EScreenSetup(void)
{
    TaskCreate(EFFECT_SLOT, 0, TASK_ENTRY_GREY_PAST);
    cycle(0x00, 0x0D);
    cycle(0x0E, 0x1B);
    cycle(0x54, 0x15);
    g.main_layers = 0x17;
    g.sub_layers = 0x17;
    g.layer_setup_mode = 0x08;
    Bits035DExpand();
}

/*
 * @routine 81:8BF4 Level0FScreenSetup
 * WHAT IT DOES: Level $0F's (and $1B's) screen set-up: palette cycle $1C,
 * value sequence $15; layers 1-3 and sprites on both screens; layer set-up
 * mode 8; the switch flags from the stage's mask.
 */
static void Level0FScreenSetup(void)
{
    cycle(0x00, 0x1C);
    cycle(0x54, 0x15);
    g.main_layers = 0x17;
    g.sub_layers = 0x17;
    g.layer_setup_mode = 0x08;
    Bits035DExpand();
}

/*
 * @routine 81:8C14 Level10ScreenSetup
 * WHAT IT DOES: Level $10's screen set-up: layer set-up mode $10; from the
 * restart point, the backdrop's stage 4; its scene (slot 3); palette
 * cycles $0D, $2A, $2B and value sequences $15, $24, $27, $2A; the switch
 * flags from the stage's mask.
 */
static void Level10ScreenSetup(void)
{
    g.layer_setup_mode = 0x10;
    if (g.level_restart)
        g.backdrop_stage = (uint16_t)((g.backdrop_stage & 0xFF00) | 0x04);
    TaskCreate(SCENE_SLOT, 0, TASK_ENTRY_LEVEL10_SCENE);
    cycle(0x00, 0x0D);
    cycle(0x0E, 0x2A);
    cycle(0x1C, 0x2B);
    cycle(0x54, 0x15);
    cycle(0x64, 0x24);
    cycle(0x74, 0x27);
    cycle(0x84, 0x2A);
    Bits035DExpand();
}

/*
 * @routine 81:8C62 Level11ScreenSetup
 * WHAT IT DOES: Level $11's screen set-up: layer set-up mode 2; packed set
 * $21 unpacked; the scroll limit's high byte 1; the text map's last 64
 * words $4000 ($7E:DF80); per-line effects from tables in the cartridge
 * (the mode $80:B9C3, BG3's y $80:B9CA, the main layers $80:B9D4); layers 2, 3 and
 * sprites on the main screen; value sequences $24, $27, $2A. The per-line
 * part waits, as level 6's: reported, not made.
 */
static void Level11ScreenSetup(void)
{
    g.layer_setup_mode = 0x02;
    UnpackSet(0x21);
    g.scroll_limit_x = (uint16_t)((g.scroll_limit_x & 0x00FF) | 0x0100);
    for (unsigned i = 0x3C0; i < 0x400; i++)
        g.text_map[i] = 0x4000;
    unconverted("Level11ScreenSetup: its per-line modes, BG3 y and layers (tables in the cartridge)");
    g.main_layers = 0x16;
    cycle(0x64, 0x24);
    cycle(0x74, 0x27);
    cycle(0x84, 0x2A);
}

/*
 * @routine 81:8CF5 AutoScrollScreenSetup
 * WHAT IT DOES: The set-up of the levels whose view scrolls by itself
 * (Level12ScreenSetup's and Level1CScreenSetup's): the scroll kind given,
 * the view-edge rule 1, the camera held up and down, layer set-up mode 0;
 * the layers and colour maths as level 8's; window effect $12 (slot 6);
 * palette cycles $15-$18.
 */
static void AutoScrollScreenSetup(uint8_t scroll_kind)
{
    g.scroll_kind = scroll_kind;
    g.view_edge_rule = 1;
    g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | 0x01);
    g.layer_setup_mode = 0x00;
    layers(0x17, 0x10, 0x03, 0x53);
    TaskCreate(EFFECT_SLOT, 18, TASK_ENTRY_WINDOW_EFFECT);
    cycle(0x00, 0x15);
    cycle(0x0E, 0x16);
    cycle(0x1C, 0x17);
    cycle(0x2A, 0x18);
}

/*
 * @routine 81:8CF3 Level12ScreenSetup
 * @routine 81:8D61 Level1CScreenSetup
 * @routine 81:8D6C Level14ScreenSetup
 * @routine 81:8D42 Level15ScreenSetup
 * @routine 81:8D46 Level16ScreenSetup
 * @routine 81:8D4A Level17ScreenSetup
 * @routine 81:8D4E Level18ScreenSetup
 * @routine 81:8D52 Level19ScreenSetup
 * @routine 81:8D56 Level1AScreenSetup
 * @routine 81:8D5D Level1BScreenSetup
 * WHAT IT DOES: The levels that share another's set-up: $12 the
 * self-scrolling one with scroll kind $0A, $1C with kind $0E and the
 * windows masking layer 3 on the main screen; $14 and $1D level 0's; $15
 * level 2's; $16 level 4's; $17 level 7's; $18 level 8's; $19 level $0A's;
 * $1A level $0B's and then the start place again (LevelStartPlace); $1B
 * level $0F's.
 */

/*
 * @routine 81:87EF LevelScreenSetup
 * @routine 81:888A LevelNoScreenSetup
 * WHAT IT DOES: Starts the level's screen: the view-edge rule 0, the
 * IRQ's split on at line 8 (the NMI's switches $31), then the level's own
 * set-up by the table at $81:8802 (levels 1, $13, $1E, $1F and $20 on:
 * none).
 */
void LevelScreenSetup(void)
{
    g.view_edge_rule = 0;
    g.nmi_on = 0;
    g.split_on = NMI_SHADOW_SPLIT;
    g.split_line = 0;
    switch ((uint8_t)g.level) {
    case 0x00: case 0x14: case 0x1D: Level00ScreenSetup(); break;
    case 0x02: case 0x15: Level02ScreenSetup(); break;
    case 0x03: Level03ScreenSetup(); break;
    case 0x04: case 0x16: Level04ScreenSetup(); break;
    case 0x05: Level05ScreenSetup(); break;
    case 0x06: Level06ScreenSetup(); break;
    case 0x07: case 0x17: Level07ScreenSetup(); break;
    case 0x08: case 0x18: Level08ScreenSetup(); break;
    case 0x09: Level09ScreenSetup(); break;
    case 0x0A: case 0x19: Level0AScreenSetup(); break;
    case 0x0B: Level0BScreenSetup(); break;
    case 0x1A: Level0BScreenSetup(); LevelStartPlace(); break;
    case 0x0C: Level0CScreenSetup(); break;
    case 0x0D: Level0DScreenSetup(); break;
    case 0x0E: Level0EScreenSetup(); break;
    case 0x0F: case 0x1B: Level0FScreenSetup(); break;
    case 0x10: Level10ScreenSetup(); break;
    case 0x11: Level11ScreenSetup(); break;
    case 0x12: AutoScrollScreenSetup(0x0A); break;
    case 0x1C: AutoScrollScreenSetup(0x0E); g.window_main = 0x04; break;
    default: break;
    }
}
