/*
 * anim.c: actors' animations.
 *
 * Each kind has an animation set: a table of animations in g.anim_data,
 * each a list of frame words (a time low, a picture high); a time of 0 ends
 * an animation, and the frame steps back from there. An actor's record holds
 * the animation, the frame, the time it has left and the picture's place in
 * its set's picture list (g.actor_pictures).
 */
#include <stdio.h>

#include "actor/actor_own.h"

/* A word of the animation data at byte offset `at`, as the original reads
   it at $7E:B000 + at. Past what the level loaded: reported once, 0. */
static uint16_t anim_word(uint32_t at)
{
    static int told;

    if (at + 1 >= 2 * (sizeof g.anim_data / sizeof g.anim_data[0])) {
        if (!told++)
            fprintf(stderr, "actor: an animation read past the animation data (offset %u)\n", (unsigned)at);
        return 0;
    }
    if (at & 1)
        return (uint16_t)(g.anim_data[at / 2] >> 8 | (g.anim_data[at / 2 + 1] & 0xFF) << 8);
    return g.anim_data[at / 2];
}

/* A word of the picture lists at byte offset `at`. */
static uint16_t picture_word(uint32_t at)
{
    static int told;

    if (at + 1 >= sizeof g.actor_pictures) {
        if (!told++)
            fprintf(stderr, "actor: a picture read past the picture lists (offset %u)\n", (unsigned)at);
        return 0;
    }
    return (uint16_t)(g.actor_pictures[at] | g.actor_pictures[at + 1] << 8);
}

/*
 * @routine 81:9158 AnimFrameLookup
 * WHAT IT DOES: Looks up an actor's current frame: its picture, and the
 * picture's place in its set's picture list. Returns the frame's word (the
 * time low, the picture high).
 */
static uint16_t AnimFrameLookup(Actor *a)
{
    uint16_t list = anim_word((uint16_t)(a->anim + a->anim_table));
    uint16_t word = anim_word((uint16_t)(a->frame * 2u + list + a->anim_table));

    a->picture = (uint8_t)(word >> 8);
    a->picture_gfx = (uint16_t)(picture_word((uint16_t)(a->picture * 2u + a->gfx_base)) + a->gfx_base);
    return word;
}

/* AnimStart and AnimResume: animation `n`, from its first frame when
   `first`. */
static void anim_start(Actor *a, uint8_t n, int first)
{
    uint8_t place;
    uint16_t head;

    if (first)
        a->frame = 0;
    a->picture = n;
    a->anim = (uint8_t)(n << 1);
    a->anim_flags |= 0x08;
    place = g.anim_set_place[a->kind];
    a->gfx_base = g.anim_set_pictures[place];
    a->anim_table = g.anim_set_table[place];
    /* the animation's head word: its first time low, the frames to step
       back at its end high (the offset read as a word with +$2E above it) */
    head = anim_word((uint16_t)(anim_word((uint16_t)(a->anim_table + (a->anim | a->anim_high << 8))) + a->anim_table));
    a->loop_back = (uint8_t)(head >> 8);
    a->frame_time = (uint8_t)head;
    (void)AnimFrameLookup(a);
    a->redraw = 0xFF;
}

/*
 * @routine 81:90C2 AnimStart
 * WHAT IT DOES: Starts an actor's animation `n` from its first frame.
 */
void AnimStart(Actor *a, uint8_t n)
{
    anim_start(a, n, 1);
}

/*
 * @routine 81:90BE AnimResume
 * WHAT IT DOES: Switches an actor to animation `n` at the frame it is on:
 * its set from its kind, the time and step back from the animation's head,
 * the picture looked up and marked changed.
 */
void AnimResume(Actor *a, uint8_t n)
{
    anim_start(a, n, 0);
}

/*
 * @routine 81:912B AnimStep
 * WHAT IT DOES: Plays an actor's animation on by one frame of time: when
 * the frame's time runs out, on to the next frame; a time of 0 is the end,
 * where the frame steps back by the animation's count.
 * Also 81:9131 AnimStepNext, its part once the time has run out.
 */
void AnimStep(Actor *a)
{
    uint16_t word;

    if (--a->frame_time != 0)
        return;
    a->frame++;
    word = AnimFrameLookup(a);
    if ((word & 0xFF) == 0) {
        a->frame = (uint8_t)((word >> 8) - a->loop_back);
        word = AnimFrameLookup(a);
    }
    a->frame_time = (uint8_t)word;
}

/* An actor's frame as sprite pieces (video's OamDrawActor): the list its
   picture_gfx names in g.actor_pictures, a count byte and then 5 bytes a
   piece (x, x when mirrored, y, the tile word). The pieces are valid until
   the next call. */
unsigned actor_pieces(const Actor *a, const SpritePiece **pieces)
{
    static SpritePiece out[255];
    uint32_t at = a->picture_gfx;
    unsigned n, i;

    if (at >= sizeof g.actor_pictures) {
        *pieces = out;
        return 0;
    }
    n = g.actor_pictures[at];
    if (at + 1 + 5u * n > sizeof g.actor_pictures)
        n = (unsigned)((sizeof g.actor_pictures - at - 1) / 5);
    for (i = 0; i < n; i++) {
        const uint8_t *b = &g.actor_pictures[at + 1 + 5 * i];

        out[i].x = (int8_t)b[0];
        out[i].x_mirrored = (int8_t)b[1];
        out[i].y = (int8_t)b[2];
        out[i].tile = (uint16_t)(b[3] | b[4] << 8);
    }
    *pieces = out;
    return n;
}
