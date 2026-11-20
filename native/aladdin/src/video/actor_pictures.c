/*
 * actor_pictures.c: an actor's new picture sent to its tile slot in video
 * memory when its animation changes it (the record's video/actor_pictures.c).
 */
#include <stdio.h>

#include "actor/actor.h"
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

#define PICTURE_PARTS_MAX VRAM_QUEUE_SIZE

/* a part's bytes as the queue holds them: the cartridge's, or the unpacked
   graphics (the original's bank $7F); NULL (reported) when neither */
static const uint8_t *part_bytes(const PicturePart *p)
{
    if (p->source.from == FROM_CART)
        return p->source.cart;
    if (p->source.from == FROM_UNPACKED && (uint32_t)p->source.at + p->bytes <= sizeof g.unpacked)
        return &g.unpacked[p->source.at];
    fprintf(stderr, "video: a picture part of %u bytes at %04X has no source; left out\n", p->bytes, p->source.at);
    return NULL;
}

/*
 * @routine 81:94A4 ActorPictureQueue
 * WHAT IT DOES: Queues the actor's picture, when it has changed, for the
 * NMI to write to its tile slot in VRAM.
 * HOW: The picture against the one last sent (+$28); a new one is kept and,
 * from the actor's picture list, its parts queued: the first to the slot's
 * VRAM address from the sprite attributes ((look & $1FF) * 16 + $6000),
 * each after it to the last one's address plus the part's step; each
 * stepping a word at a time. ActorPictureQueueNew (81:94AA)
 * and ActorPictureQueueParts (81:94B3) are its second half, here inline.
 */
void ActorPictureQueue(Actor *a)
{
    PicturePart parts[PICTURE_PARTS_MAX];
    unsigned n;
    uint16_t to;

    if (a->picture == a->redraw)
        return;
    a->redraw = a->picture;
    n = picture_parts(a->own_pictures, a->picture, parts, PICTURE_PARTS_MAX);
    to = (uint16_t)(((a->look & 0x01FF) << 4) + 0x6000);
    for (unsigned i = 0; i < n; i++) {
        const uint8_t *from;
        VramQueueEntry *c;

        to = (uint16_t)(to + parts[i].step);
        if ((from = part_bytes(&parts[i])) == NULL)
            continue;
        if (g.vram_queue_count >= VRAM_QUEUE_SIZE) {
            static int told;

            if (!told++)
                fprintf(stderr, "video: the VRAM queue is full: kind $%02X's picture %u not wholly queued\n",
                        a->kind, a->picture);
            return;
        }
        c = &g.vram_queue[g.vram_queue_count++];
        c->to = to;
        c->from = from;
        c->bytes = parts[i].bytes;
        c->step = 0x80;
    }
}

/*
 * @routine 81:9527 ActorPictureSendFromKind
 * WHAT IT DOES: As ActorPictureSend, for an actor whose graphics were
 * unpacked for its kind: when it shows its own picture (bit 6 of its flags)
 * and the picture has changed, queues the picture's graphics, each part's
 * source its list word as an offset from where its kind's graphics went
 * (g.kind_gfx_at).
 */
void ActorPictureSendFromKind(Actor *a)
{
    PicturePart parts[PICTURE_PARTS_MAX];
    unsigned n;
    uint16_t to;

    if (!(a->flags & 0x40) || a->picture == a->redraw)
        return;
    a->redraw = a->picture;
    n = picture_parts(a->own_pictures, a->picture, parts, PICTURE_PARTS_MAX);
    to = (uint16_t)(((a->look & 0x01FF) << 4) + 0x6000);
    for (unsigned i = 0; i < n && g.vram_queue_count < VRAM_QUEUE_SIZE; i++) {
        VramQueueEntry *c = &g.vram_queue[g.vram_queue_count++];
        uint32_t from = (uint32_t)g.kind_gfx_at[a->kind] + parts[i].address;

        to = (uint16_t)(to + parts[i].step);
        c->to = to;
        c->from = &g.unpacked[from % sizeof g.unpacked];
        c->bytes = parts[i].bytes;
        c->step = 0x80;
    }
}

/*
 * @routine 81:949E ActorPictureSend
 * WHAT IT DOES: When the actor shows its own picture (bit 6 of $09) and
 * the picture has changed, queues its graphics for VRAM.
 * HOW: Bit 6 clear: nothing. Else ActorPictureQueue.
 */
void ActorPictureSend(Actor *a)
{
    if (a->flags & 0x40)
        ActorPictureQueue(a);
}
