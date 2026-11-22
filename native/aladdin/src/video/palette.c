/*
 * palette.c: loading and keeping palettes.
 *
 * g.palette is the picture's 256 colours as the game builds them; setting
 * g.palette_upload has them sent at the frame's end (video_vblank).
 * g.palette_saved keeps colours a fade works from, at the same places.
 */
#include <stdio.h>

#include "data/data.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

/*
 * @routine 81:98C5 PaletteLoad
 * WHAT IT DOES: Loads palette set `set` into the palette and has it sent.
 * HOW: The set's entry gives its colours, how many and the first they set.
 */
void PaletteLoad(int set)
{
    uint16_t n = palette_load_count((uint8_t)set);
    unsigned first = palette_load_first((uint8_t)set);

    for (unsigned i = 0; i < n; i++) {
        if (first + i >= 256) {
            fprintf(stderr, "video: palette set %u runs past the palette; the rest left out\n", set);
            break;
        }
        g.palette[first + i] = palette_load_colour((uint8_t)set, (uint16_t)i);
    }
    g.palette_upload++;
}

/*
 * How many colours from palette `first` a save or fade of `count` palettes
 * reaches: those of the palettes up to the SNES's 16 (256 colours), and,
 * when the palettes include the last of them (15, a sprite palette), all the
 * sprite palettes past it (the slots past the original's four; first 16 or
 * more names one of those alone).
 */
static unsigned fade_entries(unsigned first, unsigned count)
{
    unsigned from = (unsigned)first * 16, n = (unsigned)count * 16;

    if (first >= 16)
        return from + n <= PALETTE_ENTRIES ? n : PALETTE_ENTRIES - from;
    if (from + n > 256)
        n = 256 - from;
    if (first + count > 15)
        n += 16u * (slots_count() - SLOTS_STD);
    return n;
}

/*
 * @routine 81:9DAA PaletteSave
 * WHAT IT DOES: Keeps `count` palettes (1-16) from palette `first` on, for a
 * fade to work from.
 * HOW: Where they start noted (as a byte offset, as the original keeps it),
 * then their colours copied to the same places of the kept colours.
 */
void PaletteSave(uint16_t first, uint8_t count)
{
    unsigned from = (unsigned)first * 16;
    unsigned n = fade_entries(first, count);

    g.palette_saved_at = (uint16_t)(from * 2);
    for (unsigned i = 0; i < n; i++)
        g.palette_saved[from + i] = g.palette[from + i];
}

/*
 * @routine 81:9CBE PaletteOffset
 * Folded into its callers: palette n's first colour is n * 16.
 */

/*
 * @routine 81:9CA6 PaletteSaveReversed
 * WHAT IT DOES: Keeps palette `palette`'s 16 colours in the first kept
 * palette, last colour first.
 */
void PaletteSaveReversed(uint8_t palette)
{
    unsigned from = (palette & 15u) * 16;

    for (unsigned i = 0; i < 16; i++)
        g.palette_saved[15 - i] = g.palette[from + i];
}

/*
 * @routine 81:86BF LevelPalettesLoad
 * WHAT IT DOES: Loads the level's palettes: each set of its list, in order.
 */
void LevelPalettesLoad(void)
{
    unsigned n = level_palette_count(g.level);

    for (unsigned i = 0; i < n; i++)
        PaletteLoad(level_palette_set(g.level, i));
}

/*
 * @routine 81:9E06 PaletteChannelStep
 * WHAT IT DOES: Steps the colour's low channel by `step`, held to 0-31, and
 * turns the colour so the next channel is at the bottom: the stepped
 * channel to bits 10-14, the rest down 5 bits. Three turns step red, green
 * and blue and leave them in place.
 */
static uint16_t channel_step(uint16_t colour, int16_t step)
{
    uint16_t v = (uint16_t)((colour & 0x1F) + (uint16_t)step);

    if (v & 0x8000)
        v = 0;
    else if (v >= 0x20)
        v = 0x1F;
    return (uint16_t)(colour >> 5 | v << 10);
}

/*
 * @routine 81:9DF9 PaletteColourStep
 * WHAT IT DOES: Steps all three channels of a colour by `step`, each held
 * to 0-31.
 */
static uint16_t colour_step(uint16_t colour, int16_t step)
{
    colour = channel_step(colour, step);
    colour = channel_step(colour, step);
    return channel_step(colour, step);
}

/*
 * @routine 81:9DCD PaletteFadeStep
 * WHAT IT DOES: One step of a fade: every kept colour of `count` palettes
 * from where PaletteSave started, each channel moved by `step`, into the
 * palette, and has it sent.
 */
void PaletteFadeStep(int8_t step, uint8_t count)
{
    unsigned from = g.palette_saved_at / 2u;
    unsigned n = fade_entries(from / 16, count);

    for (unsigned i = 0; i < n; i++)
        g.palette[from + i] = colour_step(g.palette_saved[from + i], step);
    g.palette_upload++;
}
