/*
 * unconverted.c: video/'s routines not converted yet. Each prints its name
 * once to stderr and does nothing (plan: no silent stubs); each goes when
 * its routine is converted, and the report lists them.
 */
#include <stdio.h>

#include "actor/actor.h"
#include "video/video.h"
#include "video/video_internal.h"

static void unconverted(const char *name, int *told)
{
    if (!*told) {
        *told = 1;
        fprintf(stderr, "unconverted: %s\n", name);
    }
}

#define UNCONVERTED(decl, name) decl { static int told; unconverted(name, &told); }

UNCONVERTED(void LayersCopyToVram(void), "LayersCopyToVram (80:8366)")
UNCONVERTED(void LayersFill(void), "LayersFill (81:C060)")
UNCONVERTED(void video_lines_send(uint32_t top __attribute__((unused))), "the per-line effects")
UNCONVERTED(void PaletteCycleStart(uint8_t slot __attribute__((unused)), uint8_t cycle __attribute__((unused))), "PaletteCycleStart (81:EC8E)")

/* another area's, wanted by video/: a weak placeholder until it lands */
__attribute__((weak)) unsigned actor_pieces(const Actor *a __attribute__((unused)), const SpritePiece **pieces)
{
    static int told;

    unconverted("actor_pieces (actor/, wanted by OamDrawActor)", &told);
    *pieces = 0;
    return 0;
}
