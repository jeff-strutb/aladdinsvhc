/*
 * kind33.c: actor kind $33, a platform swinging under the placed one: the
 * placed one (+$07 0) queues its kind's graphics when it holds its palette
 * slot alone, makes the platform (+$07 3) and then shows the frame that
 * matches the platform's direction; the platform swings to and fro about
 * the placed one's place and carries the player standing on it. Its wait
 * routine is Kind33Wait (kind_waits_rest.c). Its second entry in the
 * original's table (83:BB96) is the RTS after that wait's JMP: no state
 * reaches it, and it has none.
 */
#include <stdio.h>

#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"
#include "player/player.h"

/* its own bytes (the platform's; the placed one keeps only partner and
   shown) */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t partner[2];         /* +$40-$41: the pair's other record, its index in g.actors */
    uint8_t phase;              /* +$42: 0 swinging one way, then 1 the other */
    uint8_t past;               /* +$43: nonzero once this swing is slowing */
    uint8_t accel[2];           /* +$44-$45: taken from or added to the speed each frame */
    uint8_t speed[2];           /* +$46-$47: its angle's change a frame */
    uint8_t angle[2];           /* +$48-$49: its angle; the high byte its direction */
    uint8_t hub_y[2];           /* +$4C-$4D: the placed one's y pixels, where it swings from */
    uint8_t ridden;             /* +$4E: nonzero while the player stands on it */
    uint8_t shown;              /* +$4F: the placed one: the frame it shows */
    uint8_t hub_x[2];           /* +$4A-$4B: the placed one's x pixels */
} Kind33Own;

_Static_assert(sizeof(Kind33Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind33Own *own(Actor *a) { return (Kind33Own *)a->own.bytes; }

static uint16_t word(const uint8_t *b) { return (uint16_t)(b[0] | b[1] << 8); }
static void set_word(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }

static Actor *partner(Actor *a) { return &g.actors[word(own(a)->partner)]; }

enum {
    GRAPHICS_SECOND = ACTOR_RESUME_OWN, /* the placed one: its second upload, a frame after the first */
    HUB,                        /* the placed one: after its graphics */
    HUB_LOOKED,                 /* after its look's frame */
    HUB_FRAME,                  /* each frame of its showing */
    SWING,                      /* the platform: each frame */
};

/*
 * @routine 83:BB4B Kind33GraphicsQueue
 * WHAT IT DOES: Queues one of kind $33's graphics uploads (`at` 0 or 6 in
 * its table) for the vertical blank: a VRAM queue record from where video
 * unpacked the kind's graphics to its sprite tiles.
 */
static void graphics_queue(uint8_t at)
{
    VramQueueEntry *c;

    if (g.vram_queue_count >= VRAM_QUEUE_SIZE) {
        static int told;

        if (!told++)
            fprintf(stderr, "Kind33GraphicsQueue: the VRAM queue is full: not queued\n");
        return;
    }
    c = &g.vram_queue[g.vram_queue_count++];
    c->to = look_vram(g.kind_look[0x33], k33_upload_tile(at));
    c->from = &g.unpacked[(uint16_t)(g.kind_gfx_at[0x33] + k33_upload_from(at)) % sizeof g.unpacked];
    c->bytes = k33_upload_bytes(at);
    c->step = 0x80;
}

/*
 * @routine 83:BB2F Kind33Graphics
 * WHAT IT DOES: The placed one of kind $33 first: when it holds its
 * palette slot alone it queues its kind's graphics (two uploads, a frame
 * apart); then Kind33Hub. (Its slot is 1-4; a record with none would read
 * the byte before the slots' counts in the original: it takes the "not
 * alone" way here.)
 */
static int graphics(Actor *a)
{
    if (a->tile_slot && g.palette_slot_users[a->tile_slot - 1] == 1) {
        graphics_queue(0);
        a->resume = GRAPHICS_SECOND;
        return 1;
    }
    a->resume = HUB;
    return 1;
}

/*
 * @routine 83:BAE0 Kind33Look
 * WHAT IT DOES: The placed one of kind $33's look: +$08 bit 1 and +$09
 * bits 7 and 4 set, animation 0 from its start.
 */
void Kind33Look(Actor *a)
{
    a->anim_flags |= 0x02;
    a->flags |= 0x90;
    AnimStart(a, 0);
}

/*
 * @routine 83:B970 Kind33Hub
 * WHAT IT DOES: The placed one of kind $33 after its graphics: makes its
 * platform (+$07 3, swinging from its place, its look ours) and then each
 * frame shows the frame of animation 0 that matches the platform's
 * direction.
 */
static int hub(Actor *a)
{
    Actor *p;

    a->spawn_at = 0;
    a->spawn_byte = 3;
    p = ActorSpawn(a, a->kind);
    if (p) {
        set_word(own(a)->partner, (uint16_t)(p - g.actors));
        set_word(own(p)->partner, (uint16_t)(a - g.actors));
        set_word(own(p)->hub_x, pixels(a->x));
        set_word(own(p)->hub_y, pixels(a->y));
        p->look = a->look;
    }
    Kind33Look(a);
    a->resume = HUB_LOOKED;
    return 1;
}

static int hub_frame(Actor *a)
{
    uint8_t frame = k33_frame((uint8_t)(partner(a)->facing >> 3));

    a->frame = frame;
    if (frame != own(a)->shown) {
        own(a)->shown = frame;
        AnimResume(a, 0);
    }
    return 1;
}

/*
 * @routine 83:BA83 Kind33SwingMove
 * WHAT IT DOES: Puts the platform where its angle sets it about the placed
 * one, and the player with it: lands him on it when he falls onto it,
 * carries him by its move while he stands on it, and marks when he is off.
 */
static void swing_move(Actor *a)
{
    Kind33Own *o = own(a);
    uint16_t old_x = pixels(a->x), old_y = pixels(a->y);
    Actor *player = &g.actors[0];

    a->x = (uint32_t)word(o->hub_x) << 8;
    a->y = (uint32_t)word(o->hub_y) << 8;
    a->facing = o->angle[1];
    ActorMoveByVector(a, 0x04);
    if (!o->ridden) {
        if (PlayerLandOnActor(a, 0))
            o->ridden++;
        return;
    }
    player->x = with_pixels(player->x, (uint16_t)(pixels(a->x) - old_x + pixels(player->x)));
    player->y = with_pixels(player->y, (uint16_t)(pixels(a->y) - old_y + pixels(player->y)));
    if (!PlayerLandOnActor(a, 0))
        o->ridden = 0;
}

/*
 * @routine 83:B9C1 Kind33Swing
 * WHAT IT DOES: Kind $33's platform: swings about the placed one, faster
 * towards the bottom of each swing and slower past it, one way and back,
 * then stops and starts over; it carries the player standing on it and
 * may take hold of him; it ends when the placed one's record is free.
 */
static int swing(Actor *a)
{
    Kind33Own *o = own(a);

    a->anim_flags |= 0x01;
    a->flags |= 0x10;
    AnimStart(a, 1);
    o->ridden = 0;
    o->shown = 0;
    a->spawn_at = 0;
    a->own.bytes[OWN_13] = 0;
    a->y = with_pixels(a->y, 0x8080);
    o->phase = 0;
    o->past = 1;
    set_word(o->accel, 0x0009);
    set_word(o->speed, 0x020A);
    set_word(o->angle, 0x4AFD);
    a->resume = SWING;
    return 1;
}

/*
 * @routine 83:BA76 Kind33SwingStop
 * WHAT IT DOES: Kind $33's platform at the stop of its swing: speed,
 * phase, past and angle 0, acceleration 9, then on swinging. (No branch,
 * jump, word or resume address in the ROM leads to its own address: the
 * swing reaches the same code from inside.)
 */
static void swing_stop(Kind33Own *o)
{
    o->phase = 0;
    o->past = 0;
    set_word(o->speed, 0);
    set_word(o->accel, 0x0009);
    set_word(o->angle, 0);
}

/*
 * @routine 83:BA79 Kind33PartnerState
 * WHAT IT DOES: The state of the pair's other record.
 */
static uint8_t partner_state(Actor *a) { return partner(a)->state; }

/* A frame of the swing: the angle on by the speed; the speed up towards
   the bottom ($44 one way, $48 back), down past it; when it would go below
   0 the swing turns, or after the way back stops. The faster step back
   adds the compare's carry (1), as the original's ADC does. */
static int swing_frame(Actor *a)
{
    Kind33Own *o = own(a);
    uint16_t speed = word(o->speed), accel = word(o->accel);
    int faster;

    if (!partner_state(a))
        return ActorEndPlaced(a);
    swing_move(a);
    ActorHoldPlayer(a);
    if (!o->phase) {
        set_word(o->angle, (uint16_t)(word(o->angle) + speed));
        faster = !o->past && o->angle[1] < 0x44;
        if (faster) {
            set_word(o->speed, (uint16_t)(speed + accel));
            return 1;
        }
    } else {
        set_word(o->angle, (uint16_t)(word(o->angle) - speed));
        faster = !o->past && o->angle[1] >= 0x48;
        if (faster) {
            set_word(o->speed, (uint16_t)(speed + accel + 1));
            return 1;
        }
    }
    if (!o->past)
        o->past++;
    speed = (uint16_t)(speed - accel);
    set_word(o->speed, speed);
    if (!(speed & 0x8000))
        return 1;
    if (o->phase) {
        swing_stop(o);
        return 1;
    }
    o->phase++;
    o->past = 0;
    return 1;
}

/*
 * @routine 83:B966 Kind33Start
 * WHAT IT DOES: Actor kind $33's start: the placed one (+$07 0) to its
 * graphics, its platform (+$07 nonzero) to its swing.
 */
int Kind33Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return a->spawn_byte ? swing(a) : graphics(a);
    case GRAPHICS_SECOND:
        graphics_queue(6);
        return hub(a);
    case HUB:
        return hub(a);
    case HUB_LOOKED:
        a->resume = HUB_FRAME;
        return 1;
    case HUB_FRAME:
        return hub_frame(a);
    case SWING:
        return swing_frame(a);
    default:
        return kind_lost(a, "Kind33Code");
    }
}
