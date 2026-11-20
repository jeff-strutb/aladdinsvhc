/*
 * kind59.c: actor kind $59, the figures of the Genie's screen (level $2F):
 * by its +$07 a pose, or a tally that waits for the screen's step and then
 * shows the lives, a row of lamps, the gems or the apples as they change,
 * each change with a spark (kind $A4) and a sound. Its wait routine is
 * Kind23Wait (kind_waits_rest.c: 83:DEB9 Kind59Wait is the same bytes).
 *
 * The names follow the record's: the tally it calls the gems shows
 * g.lives, the lamps' row g.health_marks and g.health, the lives g.health,
 * the apples g.continues.
 */
#include "actor/kinds_c.h"
#include "data/kinds_c_tables.h"

#define K59_SPARK 0xA4          /* the kind of the spark it spawns at a change */
#define K59_SPARK_SOUND 0x28
#define K59_COUNT_SOUND 0x0C
#define K59_TALLY 0x06          /* the +$07 it keeps once it goes on */

/* its own bytes */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t shown;              /* +$40: the gems' or apples' count shown ($FF: none yet) */
    uint8_t row_shown;          /* +$41: a row lamp's pose shown */
    uint8_t unused42;
    uint8_t row;                /* +$43: a row lamp: its number from 1 */
    uint8_t row_at;             /* +$44: a row lamp: 4 times its number less 1, into the lamps' places */
    uint8_t lives_shown;        /* +$45: the lives shown ($FF: none yet) */
} Kind59Own;

_Static_assert(sizeof(Kind59Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind59Own *own(Actor *a) { return (Kind59Own *)a->own.bytes; }

enum {
    POSE = ACTOR_RESUME_OWN,    /* for good */
    BRIEF,                      /* after its 24 frames */
    WAIT_GEMS,                  /* each frame until the screen's step */
    WAIT_TALLY,
    WAIT_APPLES,
    LIVES,                      /* each frame of the tallies */
    LAMP,
    GEMS,
    APPLES,
};

/* on the screen: its y's top bit clear */
static int on_screen(Actor *a) { return !(pixels(a->y) & 0x8000); }

static void place(Actor *a, uint16_t x, uint16_t y_below)
{
    a->x = with_pixels(a->x, x);
    a->y = with_pixels(a->y, (uint16_t)(g.genie_screen_y + y_below));
}

/* Once the step has come: +$08 bit 0 set, +$07 6, spawn point 0. */
static void going_on(Actor *a)
{
    a->anim_flags |= 0x01;
    a->spawn_byte = K59_TALLY;
    a->spawn_at = 0;
}

/* The lives (83:DD72): each frame the lives shown, from 4 up as pose $0F,
   below as lives + $0C with a spark and its sound. */
static int lives_frame(Actor *a)
{
    Kind59Own *o = own(a);

    place(a, 0x44, 0x10);
    if (g.health == o->lives_shown)
        return 1;
    o->lives_shown = g.health;
    if (o->lives_shown >= 4) {
        AnimStart(a, 0x0F);
        return 1;
    }
    AnimStart(a, (uint8_t)(o->lives_shown + 0x0C));
    a->spawn_at = (uint8_t)((o->lives_shown + 0x43) << 1);
    if (on_screen(a)) {
        (void)ActorSpawnSameLook(a, K59_SPARK);
        SoundQueueAdd(K59_SPARK_SOUND);
    }
    return 1;
}

/* A lamp of a row (83:DDBF): each frame at its place; hidden while the
   row's lamps fall short of it, else lit ($0B) while the health reaches
   it, dark ($0C) while not, a change with a spark and its sound. */
static int lamp_frame(Actor *a)
{
    Kind59Own *o = own(a);
    uint8_t pose;

    place(a, k59_lamp_x(o->row_at), k59_lamp_y(o->row_at));
    if (g.health_marks < o->row) {
        a->anim_flags &= (uint8_t)~0x08;
        return 1;
    }
    pose = g.health >= o->row ? 0x0B : 0x0C;
    if (pose == o->row_shown)
        return 1;
    o->row_shown = pose;
    AnimStart(a, pose);
    if (on_screen(a)) {
        (void)ActorSpawnSameLook(a, K59_SPARK);
        SoundQueueAdd(K59_SPARK_SOUND);
    }
    return 1;
}

/* The gems (83:DE07 on): each frame the count shown ($0A from $0B up). */
static int gems_frame(Actor *a)
{
    Kind59Own *o = own(a);

    place(a, 0x24, 0x18);
    if (g.lives == o->shown)
        return 1;
    o->shown = g.lives;
    AnimStart(a, o->shown >= 0x0B ? 0x0A : o->shown);
    if (on_screen(a)) {
        SoundQueueAdd(K59_COUNT_SOUND);
        (void)ActorSpawnSameLook(a, K59_SPARK);
    }
    return 1;
}

/* The apples (83:DE6C on): each frame the count shown, plus 1. */
static int apples_frame(Actor *a)
{
    Kind59Own *o = own(a);

    place(a, 0x4E, 0x14);
    if (g.continues == o->shown)
        return 1;
    o->shown = g.continues;
    AnimStart(a, (uint8_t)(o->shown + 1));
    if (on_screen(a)) {
        SoundQueueAdd(K59_COUNT_SOUND);
        (void)ActorSpawnSameLook(a, K59_SPARK);
    }
    return 1;
}

/* A frame of waiting for the screen's step; the wrong sign ends it. */
static int wait_step(Actor *a)
{
    Kind59Own *o = own(a);
    int negative = (int8_t)g.genie_screen_step < 0;

    if (!g.genie_screen_step)
        return 1;
    if (a->resume == WAIT_APPLES) {
        if (!negative)
            return ActorEndPlaced(a);
        going_on(a);
        o->shown = 0xFF;
        a->resume = APPLES;
        return 1;
    }
    if (negative)
        return ActorEndPlaced(a);
    if (a->resume == WAIT_GEMS) {
        going_on(a);
        a->resume = GEMS;
        return 1;
    }
    a->anim_flags |= 0x01;
    o->row_shown = 0;
    o->row = (uint8_t)(a->spawn_byte & 0x0F);
    o->row_at = (uint8_t)(o->row << 2);
    o->row++;
    a->spawn_byte = K59_TALLY;
    a->spawn_at = 0;
    if (o->row != 1) {
        a->resume = LAMP;
        return 1;
    }
    o->lives_shown = 0xFF;
    a->resume = LIVES;
    return 1;
}

/*
 * @routine 83:DD10 Kind59Start
 * WHAT IT DOES: Actor kind $59, a figure of the Genie's screen, by its
 * +$07: 0 or 1 poses (facing 0, flip its +$07, x pixel's low byte $80) for
 * good; 4 poses, waits 24 frames and ends; 2, 6 and $10 up wait for the
 * screen's step to be positive (negative: they end), 8 for it to be
 * negative (positive: it ends). Then, its +$07 6 from there on, each frame
 * at the screen's y: 2 shows the gems; 8 the apples; $10 the lives; $11-$1F
 * a lamp of a row, its number the +$07's low 4 bits plus 1. A change shows
 * (AnimStart) and, with the figure on the screen (its y's top bit clear),
 * spawns a spark (kind $A4) and makes a sound. Other odd +$07 below $10 the
 * original jumps through the bytes after its table (no placement has one):
 * said once, and still.
 */
static int start(Actor *a)
{
    uint8_t way = a->spawn_byte;

    a->flags |= 0x90;
    own(a)->shown = 0xFF;
    if (way >= 0x10)
        way = K59_TALLY;
    switch (way) {
    case 0:
    case 1:
        a->anim_flags |= 0x02;
        a->facing = 0;
        a->flip = a->spawn_byte;
        a->x = with_pixels(a->x, (uint16_t)((pixels(a->x) & 0xFF00) | 0x80));
        AnimStart(a, 0);
        a->resume = POSE;
        return 1;
    case 2:
        a->resume = WAIT_GEMS;
        return 1;
    case 4:
        AnimStart(a, 0x0F);
        a->resume = BRIEF;
        return 0x18;
    case 6:
        a->resume = WAIT_TALLY;
        return 1;
    case 8:
        a->resume = WAIT_APPLES;
        return 1;
    default:
        return kind_lost(a, "Kind59Start");
    }
}

int Kind59Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case POSE:
        return 1;
    case BRIEF:
        return ActorEndPlaced(a);
    case WAIT_GEMS:
    case WAIT_TALLY:
    case WAIT_APPLES:
        return wait_step(a);
    case LIVES:
        return lives_frame(a);
    case LAMP:
        return lamp_frame(a);
    case GEMS:
        return gems_frame(a);
    case APPLES:
        return apples_frame(a);
    default:
        return kind_lost(a, "Kind59Code");
    }
}
