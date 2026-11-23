/*
 * mode7.c: putting up a Mode 7 screen, and the sine table its rotations use.
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"
#include "video/video_internal.h"
#include "system/runahead.h"

/* 64 steps a turn, $100 = 1: the sine, then the cosine (the original's
   $7F:4F00 and $7F:4F80, one after the other). The build writes one value
   past each: the sine's 65th is the cosine's first, written again later;
   the cosine's 65th lands past the table and is read by nothing. */
static int16_t table[129];
RUNAHEAD_KEEP(table)

/*
 * @routine 81:B120 SineTableBuild
 * WHAT IT DOES: Builds the sine and cosine tables (64 steps a turn, $100 = 1)
 * from a quarter turn in the cartridge.
 * HOW: Each of the quarter's 17 values to its four places in each table,
 * mirrored and negated as the quadrant needs, in the original's order.
 */
static void sine_table_build(void)
{
    int16_t *sine = &table[0], *cosine = &table[64];

    for (int i = 0, j = 16; j >= 0; i++, j--) {
        int16_t v = mode7_quarter_sine((unsigned)i), n = (int16_t)(0 - v);

        sine[i] = v;
        cosine[48 + i] = v;
        sine[16 + j] = v;
        cosine[j] = v;
        sine[32 + i] = n;
        cosine[16 + i] = n;
        sine[48 + j] = n;
        cosine[32 + j] = n;
    }
    /* where the original keeps them, its bank $7F's $4F00 on (g.unpacked) */
    for (int i = 0; i < 129; i++) {
        g.unpacked[0x4F00 + 2 * i] = (uint8_t)table[i];
        g.unpacked[0x4F00 + 2 * i + 1] = (uint8_t)((uint16_t)table[i] >> 8);
    }
}

int16_t video_sine(unsigned step)
{
    return table[step & 63];
}

int16_t video_cosine(unsigned step)
{
    return table[64 + (step & 63)];
}

/*
 * @routine 81:AF3B Mode7ScreenSetup
 * WHAT IT DOES: Puts up a Mode 7 screen from 256 4bpp tiles and a 32 x 32
 * map: the tiles turned to a byte a pixel, each tile's colours in the
 * palette row the table gives for its row and `row_flag`; the map all tile
 * 0 but its top left 32 x 32, the map words' low bytes. Then Mode 7, its
 * centre (128, 64), an empty matrix sent as it is, outside the plane
 * transparent, the NMI on.
 * HOW: The original reads the tiles and the map back from video memory
 * (Mode7GraphicsConvert 81:B059, Mode7TilesConvert 81:B07E,
 * Mode7TileRowsConvert 81:B097, VramWordsToBuffer 81:B035,
 * Mode7MapToBuffer 81:B012, Mode7MapClear 81:AFC0, Mode7MapFromBuffer
 * 81:AFDF, folded in); the caller passes them (Jeff, 2026-11-19).
 */
void video_mode7_screen_setup(uint8_t row_flag, const uint8_t *planar_tiles, const uint8_t *map_words)
{
    sine_table_build();
    video_mode7_build(row_flag, planar_tiles, map_words);
    g.mode7_how = 2;
    g.mode = 7;
    g.m7_centre_x = 0x0080;
    g.m7_centre_y = 0x0040;
    for (int i = 0; i < 4; i++)
        g.m7_matrix[i] = 0;
    g.nmi_on = 1;
}
