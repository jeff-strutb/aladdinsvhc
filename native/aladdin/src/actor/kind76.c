/*
 * kind76.c: actor kind $76, a figure of a scene that poses, waits, then
 * fades its own palette away under colour math and cues the scene on.
 */
#include "actor/actor_own.h"
#include "audio/sound.h"
#include "data/kinds_b_tables.h"
#include "video/video.h"

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t fade;               /* +$40: the fade's step, 0 down to -$1F */
} K76Own;

static inline K76Own *k76(Actor *a) { return (K76Own *)a->own.bytes; }

enum { POSE_1 = ACTOR_RESUME_OWN, FADE_START, FADE };

#define FADE_LAST 0xE1          /* the fade's last step, -$1F */

/*
 * @routine 84:8FBA Kind76Start
 * WHAT IT DOES: Actor kind $76 (through 83:9281 Kind76Enter): anim_flags
 * 3, flags $80 (its wait routine), its own picture list, flip 1, its
 * picture marked changed; pose 2 for $BD frames, pose 1 for $16, then
 * sound $46, flags $20, and its own sprite palette (8 + its look's
 * palette bits) saved and faded to black over 31 frames, with colour math
 * adding the sub screen (sprites only there) to BG2 and the sprites; then
 * the colour math off (its prevent bits 3, no layers blended, nothing on
 * the sub screen), the scene cued on (g.scene_go_on, $1B1F, counted
 * up), and it ends.
 * Its second entry (83:928A Kind76Second, state $80-$BF) jumps into kind
 * $7C's BackdropMover (84:9044); no script reaches it, and it is not in
 * this function (asked of its owner).
 */
int Kind76Start(Actor *a)
{
    K76Own *k = k76(a);

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags = 0x03;
        a->flags = 0x80;
        a->own_pictures = k76_pictures();
        a->flip = 0x01;
        a->redraw = 0xFF;
        AnimStart(a, 0x02);
        a->resume = POSE_1;
        return 0xBD;
    case POSE_1:
        AnimStart(a, 0x01);
        a->resume = FADE_START;
        return 0x16;
    case FADE_START:
        SoundQueueAdd(0x46);
        a->flags = 0x20;
        PaletteSave((uint8_t)((((a->look >> 8) & 0x0E) >> 1) | 0x08), 1);
        k->fade = 0;
        PaletteFadeStep(0, 1);
        /* blending: the sub screen added to BG2 and the sprites (the original's $02 and $12) */
        g.blend_prevent = 0;
        g.blend_clip = 0;
        g.blend_use_sub = 1;
        g.blend_direct_colour = 0;
        g.blend_layers = 0x12;
        g.blend_half = 0;
        g.blend_subtract = 0;
        g.sub_layers = 0x10;
        a->resume = FADE;
        return 1;
    default:                            /* FADE */
        k->fade--;
        PaletteFadeStep((int8_t)k->fade, 1);
        if (k->fade != FADE_LAST)
            return 1;
        /* blending off (the original's $30 and 0), no sub screen layers */
        g.blend_prevent = 3;
        g.blend_clip = 0;
        g.blend_use_sub = 0;
        g.blend_direct_colour = 0;
        g.blend_layers = 0;
        g.blend_half = 0;
        g.blend_subtract = 0;
        g.sub_layers = 0;
        g.scene_go_on++;
        return ActorEndPlaced(a);
    }
}
