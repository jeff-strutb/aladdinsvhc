/*
 * screen_setup.c: the display's settings and the scroll state put back to
 * their defaults before a screen is built.
 */
#include "system/state.h"
#include "video/video.h"
#include "video/video_internal.h"

/*
 * @routine 81:96CB ScrollStateClear
 * WHAT IT DOES: Clears the scroll state: both scroll layers' positions,
 * BG2's shifts, and BG1-3's positions.
 * HOW: The original clears words: the layers' $3A-$41, $1A1E-$1A25 and
 * $19EC-$1A03. $1A1E and $1A22 and the layers' $3D and $41 are read by
 * nothing, so they have no field.
 */
void ScrollStateClear(void)
{
    for (int i = 0; i < 2; i++) {
        g.layers[i].scroll_x = 0;
        g.layers[i].scroll_y = 0;
    }
    g.bg2_x_shift = 0;
    g.bg2_y_shift = 0;
    for (int i = 0; i < 3; i++) {
        g.bg_x[i] = 0;
        g.bg_y[i] = 0;
    }
}

/*
 * @routine 81:984A DisplayShadowsClear
 * Dead: nothing in the game calls it (the record's own comment). The
 * cartridge holds no JSL or JML to 81:984A and no JSR or JMP to $984A in
 * bank $81; the one $4A $98 pair in that bank (81:C18F) is the instructions
 * XBA, LSR, TYA.
 */

/*
 * @routine 81:9865 DisplayDefaults
 * WHAT IT DOES: Puts the display's settings back to their defaults: mode 1
 * with BG3 in front, BG1 on the play field's map and BG2 on the backdrop's
 * with the low tile set, BG3 on the text map and its tiles; colour maths
 * prevented everywhere and adding nothing, the fixed colour black; windows,
 * mosaic, the per-line effects, Mode 7 and the split screen off.
 * HOW: The shadows the NMI copies, set; the original also writes the
 * sprites' base (fixed here: the sprites' tiles are the high set's from
 * 256) and turns HDMA and Mode 7's outside setting off directly. It leaves
 * the sprite and colour windows' logic ($03AE) as it is.
 */
void DisplayDefaults(void)
{
    g.blend_prevent = 3;                /* the original's $30 */
    g.blend_clip = 0;
    g.blend_use_sub = 0;
    g.blend_direct_colour = 0;
    g.mode = 1;                         /* the original's 9 */
    g.bg3_priority = 1;
    g.tile_size_16 = 0;
    g.bg_map_at[0] = 0x4800;            /* BG1 the play field's map, 64 wide */
    g.bg_map_size[0] = 1;
    g.bg_map_at[1] = 0x4000;            /* BG2 the backdrop's, 64 wide */
    g.bg_map_size[1] = 1;
    g.bg_map_at[2] = 0x5C00;            /* BG3 the text map */
    g.bg_map_size[2] = 0;
    g.bg_tiles_at[0] = 0x0000;          /* BG1 and BG2 the low tile set */
    g.bg_tiles_at[1] = 0x0000;
    g.bg_tiles_at[2] = 0x5000;          /* BG3 its 2bpp tiles */
    g.bg_tiles_at[3] = 0x0000;
    g.fixed_red = 0;
    g.fixed_green = 0;
    g.fixed_blue = 0;
    g.window_main = 0;
    g.window_sub = 0;
    for (int i = 0; i < 6; i++)
        g.window_use[i] = 0;            /* $03AA-$03AC */
    for (int i = 0; i < 4; i++)
        g.window_logic[i] = 0;          /* $03AD: BG1-4 */
    g.mosaic_size = 0;
    g.mosaic_layers = 0;
    g.line_effects_on = 0;
    video_mode7_off();
    g.mode7_how = 0;
    g.split_on = 0;                     /* the interrupt shadow's $01 */
}
