/*
 * palette_steps.c: one palette at a time: from the cartridge's palettes, to
 * white, or back to its kept colours.
 */
#include <stdio.h>

#include "actor/actor.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

/* 16 colours from `colour(n, i)` into the palette from colour `to` on */
static void palette_put(unsigned to, uint16_t (*colour)(uint8_t, unsigned), uint8_t n)
{
    for (unsigned i = 0; i < 16; i++)
        if (to + i < 256)
            g.palette[to + i] = colour(n, i);
    if (to + 16 > 256)
        fprintf(stderr, "video: a palette at colour %u runs past the palette\n", to);
    g.palette_upload++;
}

/*
 * @routine 81:9903 ActorPaletteFromRom
 * WHAT IT DOES: Sets the actor's sprite palette (its look's bits 9-11) to
 * palette `rom` of the actors' palettes, and has the palette sent.
 */
void ActorPaletteFromRom(const Actor *a, uint8_t rom)
{
    palette_put(128u + 16u * (a->look >> 9 & 7u), actor_palette_colour, rom);
}

/*
 * @routine 81:9941 PaletteFromRom
 * WHAT IT DOES: Sets palette `palette` counted from palette 4 to palette
 * `rom` of the screens' palettes, and has the palette sent.
 */
void PaletteFromRom(uint8_t rom, uint8_t palette)
{
    palette_put(64u + 16u * palette, rom_palette_colour, rom);
}

/*
 * @routine 81:9CD7 PaletteWhite
 * WHAT IT DOES: Sets palette `palette` (its low 4 bits) to white ($FFFF, as
 * the original stores it) and has the palette sent.
 */
void PaletteWhite(uint8_t palette)
{
    for (unsigned i = 0; i < 16; i++)
        g.palette[(palette & 15u) * 16 + i] = 0xFFFF;
    g.palette_upload++;
}

/*
 * @routine 81:9CF1 PaletteRestoreReversed
 * WHAT IT DOES: Sets palette `palette` back to the colours
 * PaletteSaveReversed kept (the first 16 kept, last first), and has the
 * palette sent.
 */
void PaletteRestoreReversed(uint8_t palette)
{
    for (unsigned i = 0; i < 16; i++)
        g.palette[(palette & 15u) * 16 + i] = g.palette_saved[15 - i];
    g.palette_upload++;
}

/*
 * @routine 81:9CC8 PaletteBlinkStep
 * Dead: nothing calls it (the record); no long pointer to 81:9CC8 in the
 * cartridge; the one $C8 $9C pair in bank $81 (81:A343) is a BCS operand
 * and the STZ after it.
 */
