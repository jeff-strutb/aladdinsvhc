/*
 * kind20.c: actor kind $20, a guard that patrols, stops to look about,
 * and charges when it sees Aladdin; struck, it is knocked away; knocked
 * down, it lies dazed and gets up; with $07 bit 4 it carries a pot (kind
 * $42) and throws it. One function of its record (Kind20Start) for its
 * three entries, and its wait routine; with the routines only it calls.
 *
 * It walks its beat ahead of it (one pixel a frame) until the floor ends,
 * a wall is ahead or it has gone its range; then stands, looks about
 * (turning every 17 frames for 64) and walks on. Each frame it may see
 * Aladdin: then it braces and charges, speeding up, until the floor ahead
 * ends or it has slowed to a stop; then it halts and looks about again.
 * Its spotting first writes to cartridge RAM at $70:0023 and reads it
 * back: a copier that keeps the byte hangs the game there. A retail
 * cartridge has no RAM there (plan section 1 item 8): that test is left
 * out.
 */

#include "actor/actor_own.h"
#include "actor/kinds_b.h"
#include "actor/kinds_d.h"
#include "audio/sound.h"
#include "data/kinds_d_tables.h"
#include "map/map.h"
#include "video/video.h"

/* its own bytes */
#define PLACE_BIT_FACING 0x01   /* $07: its facing */
#define PLACE_FAST 0x08         /* $07: the faster charge, the leap of a wall */
#define PLACE_CARRIER 0x10      /* $07: it carries a pot; it sees through walls */
#define COUNT OWN_42            /* frames left of a step; the daze's frames */
#define TURNS OWN_43            /* frames to the next turn while it looks about; while charging, the last picture
                                   it sounded on */
#define POT OWN_42              /* the carrier: its pot's record, an index in g.actors (+$42-$43) */
#define SLOWED OWN_4E           /* nonzero once its charge has slowed below a pixel a frame */
#define CHARGE_FRAMES OWN_4C    /* counts its charge's slowing frames */
#define SIGNAL OWN_4F           /* the carrier: set by the pot: 1 it hit Aladdin, 2 it passed him */
#define POT_PARENT OWN_40       /* the pot's +$40-$41: the carrier's record, an index in g.actors */

static uint16_t own16(const Actor *a, unsigned at)
{
    return (uint16_t)(a->own.bytes[at] | a->own.bytes[at + 1] << 8);
}

static void own16_set(Actor *a, unsigned at, uint16_t v)
{
    a->own.bytes[at] = (uint8_t)v;
    a->own.bytes[at + 1] = (uint8_t)(v >> 8);
}

enum {
    ENDING_WAIT = ACTOR_RESUME_OWN, /* $07 $1F (the ending's scene): 32 frames before it walks */
    PATROL,                     /* each frame of its walk */
    LOOK_STILL,                 /* looking about: 16 frames still */
    LOOK_TURN,                  /* 64 frames turning every 17 */
    LOOK_STILL_AGAIN,           /* 16 frames still */
    SPOT_BRACE,                 /* spotted: after 12 frames of animation 0 */
    SPOT_GO,                    /* after 4 frames of animation 3 */
    CHARGE,                     /* each frame of its charge */
    AIRBORNE,                   /* off a ledge or leaping a wall, until it lands */
    HALT_WAIT,                  /* halted: after 24 frames */
    HIT_KNOCKED,                /* struck from in front: knocked away */
    HIT_KNOCKED_RESUMED,        /* from behind */
    DAZED_FALL,                 /* knocked down: falling to the ground */
    DAZED,                      /* lying dazed */
    CARRIER_WAITS,              /* the carrier waits for Aladdin */
    THROW_POSE,                 /* after 16 frames turned to him */
    THROW_DONE,                 /* after 31 frames of the throw */
    WATCH,                      /* waiting for the pot's word */
    LAUGH,                      /* the pot hit him: after 32 frames */
    LAUGH_DONE,                 /* after 96 frames of animation 8 */
    RED                         /* the pot passed him: after 32 frames red */
};

/* ---- the routines only kind $20 calls ---- */

/*
 * @routine 83:9BE5 Actor44SetBy07
 * WHAT IT DOES: Its beat's reach: its words at +$44 and +$46 to $20 and
 * $40, or $28 and $50 when bit 1 of its $07 is set.
 */
static void beat_by_07(Actor *a)
{
    uint8_t reach = (a->spawn_byte & 0x02) ? 0x28 : 0x20;

    own16_set(a, OWN_44, reach);
    own16_set(a, OWN_46, (uint16_t)(reach << 1));
}

/*
 * @routine 83:9BFA Actor44Set40And80
 * WHAT IT DOES: Its beat's reach: its words at +$44 and +$46 to $40 and
 * $80.
 */
static void beat_40_80(Actor *a)
{
    own16_set(a, OWN_44, 0x40);
    own16_set(a, OWN_46, 0x80);
}

/*
 * @routine 83:9A34 ActorXTo40
 * WHAT IT DOES: Its beat from here: its x pixels to its word at +$40.
 */
static void beat_from_here(Actor *a)
{
    own16_set(a, OWN_40, pixels(a->x));
}

/*
 * @routine 83:9A3D ActorSpeedXSet1
 * WHAT IT DOES: Its speed across to one pixel a frame.
 */
static void speed_one(Actor *a)
{
    a->speed_x = 0x100;                 /* 1.0, 16.8 */
}

/*
 * @routine 83:9A15 ActorMapAtOffset
 * WHAT IT DOES: The kind of the map's tile at a point ahead of it (along
 * its facing) and `down` pixels below it: `entry` picks how far ahead (0:
 * 11 pixels, 4: 7).
 */
static uint8_t map_ahead(Actor *a, uint8_t entry, uint16_t down)
{
    MapPoint p;

    p.x = (uint16_t)(pixels(a->x) + k20_ahead_x((uint8_t)((a->facing << 1) + entry)));
    p.y = (uint16_t)(pixels(a->y) + down);
    return TileAt(a, &p);
}

/*
 * @routine 83:9A01 ActorMapAhead7Down28
 * WHAT IT DOES: The kind of the map's tile 7 pixels ahead of it and 28
 * below. Also 83:9A11 ActorMapAhead7Down36, 36 below (on a ledge).
 */
static uint8_t map_ahead7(Actor *a, uint16_t down)
{
    return map_ahead(a, 4, down);
}

/*
 * @routine 83:99EE ActorTileAheadIs70
 * WHAT IT DOES: Whether the tile 11 pixels ahead of it and 12 below is of
 * a kind whose bits 4-6 are all set (a wall).
 */
static bool wall_ahead(Actor *a)
{
    return (map_ahead(a, 0, 12) & 0x70) == 0x70;
}

/* Seen: it turns to face him. */
static bool seen_turn(Actor *a)
{
    a->facing = a->flip = ActorSideOfPlayer(a);
    return true;
}

/*
 * @routine 83:9A54 ActorSeesPlayerLine
 * WHAT IT DOES: Whether the map between it and Aladdin is clear: every 16
 * pixels from it toward him, at its height (12 lower when bit 3 of its
 * $07 is clear), for as many steps as whole 16-pixel spans between them,
 * an empty tile; none at all, and it does not see. Seen, it turns to face
 * him. Its +$48 the step, +$4C its sum, +$4A the steps left (its low byte
 * counted, as the original's).
 */
static bool sees_line(Actor *a)
{
    uint16_t d = (uint16_t)(pixels(a->x) - pixels(g.actors[0].x));
    uint16_t step = 0xFFF0;
    MapPoint p;

    if (d & 0x8000) {
        d = (uint16_t)-d;
        step = 0x0010;
    }
    if (!(d >> 4))
        return false;
    own16_set(a, OWN_4A, (uint16_t)(d >> 4));
    own16_set(a, OWN_48, step);
    own16_set(a, OWN_4C, 0);
    do {
        own16_set(a, OWN_4C, (uint16_t)(own16(a, OWN_4C) + own16(a, OWN_48)));
        p.x = (uint16_t)(pixels(a->x) + own16(a, OWN_4C));
        p.y = pixels(a->y);
        if (!(a->spawn_byte & 0x08))
            p.y = (uint16_t)(p.y + 12);
        if (TileAt(a, &p))
            return false;
    } while (--a->own.bytes[OWN_4A]);
    return seen_turn(a);
}

/*
 * @routine 83:9A46 ActorSeesPlayer
 * WHAT IT DOES: Whether it sees Aladdin: he is in its box 0 in front of it
 * (PlayerInFrontBox), and (unless bit 4 of its $07 is set) the map between
 * them is clear (ActorSeesPlayerLine). Seen, it turns to face him.
 */
static bool sees(Actor *a)
{
    if (!PlayerInFrontBox(a, 0))
        return false;
    if (a->spawn_byte & PLACE_CARRIER)
        return seen_turn(a);
    return sees_line(a);
}

/* ---- its states ---- */

static int spotted(Actor *a);

/* 83:97E5 Kind20StartWalk: animation 2, facing its $07 bit 0, its beat
   from here, a pixel a frame; on into its patrol's first frame. */
static int start_walk(Actor *a)
{
    AnimStart(a, 2);
    a->facing = a->flip = (uint8_t)(a->spawn_byte & PLACE_BIT_FACING);
    beat_from_here(a);
    speed_one(a);
    a->resume = PATROL;
    return 1;
}

/* 83:9832 Kind20LookAbout: out of a wall; 16 frames still. */
static int look_about(Actor *a)
{
    (void)ActorOutOfWallProbe4(a);
    AnimStart(a, 0);
    a->own.bytes[COUNT] = 0x10;
    a->resume = LOOK_STILL;
    return 1;
}

/* 83:9956 Kind20Halt: stands 24 frames, then takes up its beat from here
   and looks about. */
static int halt(Actor *a)
{
    AnimStart(a, 0);
    a->resume = HALT_WAIT;
    return 0x18;
}

/* 83:998C: a charging frame on its feet: its step sounds; slowing once
   COUNT is out, until it stops (halts). */
static int charge_on(Actor *a)
{
    if ((a->picture == 0x09 || a->picture == 0x0C) && a->picture != a->own.bytes[TURNS]) {
        a->own.bytes[TURNS] = a->picture;
        SoundQueueAdd(0x13);
    }
    if (a->own.bytes[COUNT]) {
        a->own.bytes[COUNT]--;
        if (ActorSideOfPlayer(a) != a->facing && a->own.bytes[COUNT] >= 0x10)
            a->own.bytes[COUNT] = 0x10;
        a->resume = CHARGE;
        return 1;
    }
    ActorDecelerateX(a);
    if (!a->own.bytes[SLOWED] && (a->speed_x & 0xFFFF) < 0x0100) {
        a->own.bytes[SLOWED]++;
        AnimStart(a, 2);
    }
    a->own.bytes[CHARGE_FRAMES]++;
    if (a->speed_x & 0xFFFF) {
        a->resume = CHARGE;
        return 1;
    }
    return halt(a);
}

/* Into the air: up at `speed_y` with gravity `gravity`, a frame at a time
   until it lands. */
static int airborne(Actor *a, int32_t speed_y, uint8_t gravity)
{
    a->speed_y = speed_y;
    a->accel_y = gravity;
    a->resume = AIRBORNE;
    return 1;
}

/* 83:9906: a charging frame's ground and probes: it halts where the floor
   ends with no floor further on, or at a wall it cannot leap; it runs off
   a ledge with a floor further on (falling), and the faster guard not yet
   slowed leaps a wall with room above it. */
static int charge_frame(Actor *a)
{
    MapPoint found;
    uint8_t ahead;

    (void)ActorSnapToGround(a, &found);
    ahead = map_ahead7(a, found.kind == 0x0F ? 36 : 28);   /* $0F: on a ledge */
    if (!ahead) {
        /* no floor just ahead: is there one further on? */
        if (!map_ahead(a, 0, 0x2B))
            return halt(a);
        return airborne(a, 0, 0x60);
    }
    if (!wall_ahead(a))
        return charge_on(a);
    if (!(a->spawn_byte & PLACE_FAST) || a->own.bytes[SLOWED] || map_ahead(a, 0, 0))
        return halt(a);
    return airborne(a, -0x1A0, 0x10);    /* up 1.625 (the bytes $60 $FE $FF) */
}

/*
 * @routine 83:9894 Kind20Spotted
 * WHAT IT DOES: The guard has seen Aladdin: unless a wall is ahead, or the
 * floor ahead ends with no floor further on (then it halts), it braces
 * (12 frames, then 4) and charges at 1.125 pixels a frame (1.5 with $07
 * bit 3), accelerating, for up to 128 frames before it slows.
 */
static int spotted(Actor *a)
{
    if (wall_ahead(a))
        return halt(a);
    if (!map_ahead7(a, 28) && !map_ahead(a, 0, 0x2B))
        return halt(a);
    AnimStart(a, 0);
    a->resume = SPOT_BRACE;
    return 0x0C;
}

/* 83:9AB9 Kind20CarrierWalks: without its pot it becomes a walking guard:
   facing Aladdin, its $07 that facing with bits 1 and 4, its beat from
   here. */
static int carrier_walks(Actor *a)
{
    a->spawn_byte = (uint8_t)(ActorSideOfPlayer(a) | 0x12);
    beat_40_80(a);
    return start_walk(a);
}

static Actor *pot_of(Actor *a) { return &g.actors[a->own.bytes[POT]]; }

/*
 * @routine 83:9AC9 Kind20Carrier
 * WHAT IT DOES: Kind $20's guard with $07 bit 4: makes its pot (kind $42)
 * over its head (spawn point $20) and waits for Aladdin; none could be
 * made, it walks. The pot's +$40 and its POT name each other's records.
 * When the pot's record comes before its own, the two records are swapped
 * whole, the guard to wait in the pot's old record (at
 * Kind20CarrierWaits), and this record goes on as the pot, at Kind42Start
 * (ActorRunAs).
 */
static int carrier(Actor *a)
{
    Actor *pot;

    a->facing = a->flip = (uint8_t)(a->spawn_byte & 0x01);
    AnimStart(a, 6);
    a->spawn_at = 0x20;
    a->own.bytes[SIGNAL] = 0;
    pot = ActorCreate(0x42);
    if (!pot)
        return carrier_walks(a);
    pot->state = ACTOR_STATE_NEW;
    pot->placement = 0xFF;
    pot->spawn_byte = (uint8_t)(a->spawn_byte & 0x02);
    pot->facing = a->facing;
    pot->flip = (uint8_t)(a->facing ^ 1);
    ActorSpawnPlace(a, pot);
    if (pot < a) {
        Actor kept;

        /* each names ITSELF before the swap (the original stores its own address in the pot's +$40 and the guard's POT
           word): the records' contents change places, so after it each names the other's record */
        own16_set(pot, POT_PARENT, (uint16_t)(pot - g.actors));
        own16_set(a, POT, (uint16_t)(a - g.actors));
        a->resume = CARRIER_WAITS;
        kept = *pot;
        *pot = *a;
        *a = kept;
        /* this record goes on as the pot, from its start, in this same turn
           (the original's JMP Kind42Start) */
        return ActorRunAs(a, Kind42Start, ACTOR_START);
    }
    own16_set(pot, POT_PARENT, (uint16_t)(a - g.actors));
    own16_set(a, POT, (uint16_t)(pot - g.actors));
    a->resume = CARRIER_WAITS;
    return 1;
}

/* 83:9C79: knocked down, until it lands: then 96 frames dazed. */
static int dazed_fall(Actor *a)
{
    if (ActorOutOfGround(a)) {
        a->own.bytes[COUNT] = 0x60;
        a->resume = DAZED;
        return 1;
    }
    a->resume = DAZED_FALL;
    return 1;
}

/* 83:9C49 Kind20Dazed's start: it can no longer be hit, its beat from
   here, animation 5 and preset 2; then its fall. */
static int dazed(Actor *a)
{
    beat_from_here(a);
    beat_40_80(a);
    a->facing = a->flip;
    a->flags &= (uint8_t)~0x80;
    if (a->spawn_byte & PLACE_CARRIER)
        ActorPaletteFromRom(a, 5);
    AnimStart(a, 5);
    ActorPictureSend(a);
    ActorMotionPreset(a, 0x02);
    return dazed_fall(a);
}

/*
 * @routine 83:97AF Kind20Start
 * WHAT IT DOES: Actor kind $20, the guard, from its start: its probes and
 * picture list, its wait routine on; with $07 $1F (the ending's scene)
 * animation 0 for 32 frames first and then $07 3; with bit 4, a guard
 * with a pot (Kind20Carrier); else its beat's reach by its $07 bit 1
 * (Actor44SetBy07) and its walk.
 * Its second entry, 83:9C19 Kind20Hit: struck from in front (its facing
 * not negative) knocked away (animation 4, sound $0F, preset 3); from
 * behind turned round (its facing's bit 0, its flip the other), sound 7,
 * and knocked away from frame 3 of animation 4 (preset $4C); with $07 bit
 * 4, its palette from entry 5 first.
 * Its third entry, 83:9C49 Kind20Dazed: knocked down, it can no longer be
 * hit, falls to the ground (preset 2), then lies dazed for 96 frames;
 * Aladdin bouncing on it or a shot from its front knocks it away
 * (Kind20Hit), a shot from behind starts the daze again; then it gets up
 * (Kind20Halt).
 * Also 83:97E5 Kind20StartWalk, 83:97F9 Kind20Patrol, 83:9832
 * Kind20LookAbout, 83:9900 Kind20Charge, 83:9956 Kind20Halt and 83:9B53
 * Kind20CarrierWaits (the carrier: near Aladdin (box $1C) and seeing him,
 * it turns to him and throws, its pot placed at spawn point $3A; then the
 * pot's word: gone, it walks; it hit him, it laughs and walks; it passed
 * him, it goes red and charges).
 */
int Kind20Start(Actor *a)
{
    uint16_t from;

    switch (a->resume) {
    case ACTOR_START:
        a->anim_flags |= 0x02;
        a->flags |= 0x90;
        a->probes = k20_probes();
        a->own_pictures = k20_pictures();
        if (a->spawn_byte == 0x1F) {
            AnimStart(a, 0);
            a->resume = ENDING_WAIT;
            return 0x20;
        }
        if (a->spawn_byte & PLACE_CARRIER)
            return carrier(a);
        beat_by_07(a);
        return start_walk(a);
    case ENDING_WAIT:
        a->spawn_byte = 0x03;
        beat_by_07(a);
        return start_walk(a);
    case PATROL:
        if (sees(a))
            return spotted(a);
        ActorMoveX(a);
        {
            MapPoint found;

            (void)ActorSnapToGround(a, &found);
            if (!map_ahead7(a, found.kind == 0x0F ? 36 : 28) || wall_ahead(a))
                return look_about(a);
        }
        from = (uint16_t)(own16(a, OWN_40) + own16(a, OWN_44) - pixels(a->x));
        if (from >= own16(a, OWN_46))
            return look_about(a);
        return 1;
    case LOOK_STILL:
        if (sees(a))
            return spotted(a);
        if (--a->own.bytes[COUNT])
            return 1;
        AnimStart(a, 1);
        a->own.bytes[COUNT] = 0x40;
        a->own.bytes[TURNS] = 0x11;
        a->resume = LOOK_TURN;
        return 1;
    case LOOK_TURN:
        if (sees(a))
            return spotted(a);
        if (!--a->own.bytes[TURNS]) {
            a->own.bytes[TURNS] = 0x11;
            a->facing ^= 1;
        }
        if (--a->own.bytes[COUNT])
            return 1;
        AnimStart(a, 0);
        a->own.bytes[COUNT] = 0x10;
        a->resume = LOOK_STILL_AGAIN;
        return 1;
    case LOOK_STILL_AGAIN:
        if (sees(a))
            return spotted(a);
        if (--a->own.bytes[COUNT])
            return 1;
        AnimStart(a, 2);
        a->flip = a->facing;
        a->resume = PATROL;
        return 1;
    case SPOT_BRACE:
        AnimStart(a, 3);
        a->resume = SPOT_GO;
        return 4;
    case SPOT_GO:
        a->speed_x = (a->spawn_byte & PLACE_FAST) ? 0x180 : 0x120;
        a->accel_x = 0x08;
        a->own.bytes[COUNT] = 0x80;
        a->own.bytes[SLOWED] = 0;
        return charge_frame(a);
    case CHARGE:
        ActorMoveX(a);
        return charge_frame(a);
    case AIRBORNE:
        ActorFly(a);
        (void)ActorOutOfWallProbe4(a);
        if (!ActorOutOfGround(a))
            return 1;
        return charge_on(a);
    case HALT_WAIT:
        speed_one(a);
        beat_from_here(a);
        beat_40_80(a);
        return look_about(a);

    case ACTOR_START_SECOND:
        if (a->spawn_byte & PLACE_CARRIER)
            ActorPaletteFromRom(a, 5);
        if (!(a->facing & 0x80)) {
            a->resume = HIT_KNOCKED;
            return ActorKnockedAway(a, 4, 0x0F, 3);
        }
        a->facing &= 0x01;
        a->flip = (uint8_t)(a->facing ^ 1);
        SoundQueueAdd(0x07);
        a->resume = HIT_KNOCKED_RESUMED;
        return ActorKnockedAwayResumed(a, 4, 3, 0x4C);
    case HIT_KNOCKED:
        return ActorKnockedAway(a, 4, 0x0F, 3);
    case HIT_KNOCKED_RESUMED:
        return ActorKnockedAwayResumed(a, 4, 3, 0x4C);

    case ACTOR_START_THIRD:
        return dazed(a);
    case DAZED_FALL:
        if (ActorBouncesPlayer(a)) {
            a->flags &= 0x7F;
            a->state = 0x8C;            /* overwritten by its wait, as the original's BRK stores state 1 */
        }
        (void)ActorHitByShotCC(a);
        AnimStep(a);
        ActorPictureSend(a);
        ActorStepY(a);
        ActorAccelerateY(a);
        return dazed_fall(a);
    case DAZED: {
        Actor *shot;

        if (ActorBouncesPlayer(a)) {
            a->flags &= 0x7F;
            a->state = 0x8C;
            a->resume = ACTOR_START_SECOND;
            return Kind20Start(a);
        }
        shot = ActorHitByShotCC(a);
        if (shot) {
            if (!(a->facing & 0x80))
                return dazed(a);
            a->resume = ACTOR_START_SECOND;
            return Kind20Start(a);
        }
        AnimStep(a);
        ActorPictureSend(a);
        if (--a->own.bytes[COUNT])
            return 1;
        a->flags |= 0x80;
        return halt(a);
    }

    case CARRIER_WAITS:
        if (!ActorNearPlayer(a, 0x1C) || !sees_line(a))
            return 1;
        a->facing = a->flip = ActorSideOfPlayer(a);
        pot_of(a)->facing = a->facing;
        pot_of(a)->flip = (uint8_t)(a->facing ^ 1);
        ActorSpawnPlace(a, pot_of(a));
        a->resume = THROW_POSE;
        return 0x10;
    case THROW_POSE:
        AnimStart(a, 7);
        a->spawn_at = 0x3A;
        ActorSpawnPlace(a, pot_of(a));
        a->resume = THROW_DONE;
        return 0x1F;
    case THROW_DONE:
        AnimStart(a, 0);
        a->resume = WATCH;
        return 1;
    case WATCH:
        if (pot_of(a)->state == ACTOR_STATE_FREE)
            return carrier_walks(a);
        if (!a->own.bytes[SIGNAL])
            return 1;
        if (a->own.bytes[SIGNAL] == 1) {
            a->resume = LAUGH;
            return 0x20;
        }
        ActorPaletteFromRom(a, 0x20);
        AnimStart(a, 9);
        a->resume = RED;
        return 0x20;
    case LAUGH:
        AnimStart(a, 8);
        a->resume = LAUGH_DONE;
        return 0x60;
    case LAUGH_DONE:
        return carrier_walks(a);
    case RED:
        ActorPaletteFromRom(a, 0x05);
        a->spawn_byte = PLACE_CARRIER;
        beat_40_80(a);
        return spotted(a);
    default:
        return 1;
    }
}

/*
 * @routine 83:9C06 ActorWaitTurnAnim08
 * WHAT IT DOES: Kind $20's wait routine: each frame it waits, Aladdin
 * stomped or hurt by it (ActorStompOrHurt), a shot taken (ActorHitByShotCC),
 * its animation on and its picture sent; it ends once out of range (box
 * 8).
 */
int ActorWaitTurnAnim08(Actor *a)
{
    (void)ActorStompOrHurt(a);
    (void)ActorHitByShotCC(a);
    AnimStep(a);
    ActorPictureSend(a);
    return ActorEndOutOfRange(a, 0x08);
}
