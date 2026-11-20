/*
 * kind68.c: actor kind $68, the block platform: with +$07 $0F a spawner
 * that makes eight platforms (+$07 7 down to 0) and ends; else one platform,
 * set up by its +$07, that after its wait goes back and forth along a leg
 * and is drawn as blocks of the level's map, written where it moves to a
 * block at a time. Its wait routine carries Aladdin while he rides it. Its
 * second entry is kind $69's start (n5-kinds-d's, run by ActorRunAs); kind $67's second entry
 * is this kind's start.
 *
 * Not converted: its check for a copier (cartridge RAM written and read back
 * at $70:0040; on the retail board's open bus the values differ, so the
 * platform always moves: plan section 1 item 8).
 */
#include "actor/kinds_c.h"
#include "actor/kinds_d.h"
#include "data/kinds_c_tables.h"
#include "map/map.h"

#define MAKER 0x0F              /* +$07 of the spawner */
#define EMPTY_TILE 0x7F80       /* the tile put while it is on its way out */

/* its own bytes: +$16-$17 hold the rider flags (the original's $16 and $17), +$19 (word) its way
   back (the speed's low word), +$1C its leg's blocks left (the acceleration across), +$20 its
   pause (gravity) */
typedef struct {
    uint8_t before[OWN_40];
    uint8_t wait[2];            /* +$40-$41: frames of its first wait; later its pause's */
    uint8_t block_x[2];         /* +$42-$43: where its next blocks are written */
    uint8_t block_y[2];         /* +$44-$45 */
    uint8_t unused46;
    uint8_t shape;              /* +$47: its shape (bit 0 of its ways) */
    uint8_t unused48;
    uint8_t last_x;             /* +$49: its x pixels' low byte when it last wrote */
    uint8_t alt;                /* +$4A: bit 0 flips at each block step */
} Kind68Own;

_Static_assert(sizeof(Kind68Own) <= OWN_BYTES, "a view fits the own bytes");

static Kind68Own *own(Actor *a) { return (Kind68Own *)a->own.bytes; }

static uint16_t word(const uint8_t *b) { return (uint16_t)(b[0] | b[1] << 8); }
static void set_word(uint8_t *b, uint16_t v) { b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); }

/* +$19: 0 on the way out, 1 on the way back (a word) */
static uint16_t back(const Actor *a) { return (uint16_t)a->speed_x; }
static void set_back(Actor *a, uint16_t v) { a->speed_x = (int32_t)(((uint32_t)a->speed_x & ~0xFFFFu) | v); }

enum {
    SPAWNER = ACTOR_RESUME_OWN,
    MOVE_START,                 /* after its first wait: one more frame */
    MOVE,                       /* each frame of its leg */
    PAUSE,                      /* each frame of its pause at a leg's end */
    STAY,                       /* a copier's: for good (never reached on the retail board) */
};

/*
 * @routine 83:E7DE BlockPointInBounds
 * WHAT IT DOES: Tells whether the platform's block point is in the part of
 * the map its blocks may be written to: its x from $20 to $DF.
 */
static bool block_in_bounds(Actor *a)
{
    return (uint16_t)(word(own(a)->block_x) - 0x20) < 0x00C0;
}

/*
 * @routine 83:E837 BlockPointStep
 * WHAT IT DOES: Moves the platform's block point on by one block step: the
 * step from the list by bit 0 of its alternation, picked by its shape and its
 * facing, its y step turned round on the way back.
 */
static void block_step(Actor *a)
{
    Kind68Own *o = own(a);
    uint8_t pick = (uint8_t)((o->shape << 1) | a->facing);
    int16_t dy = k68_block_step_y(o->alt & 1, pick);

    set_word(o->block_x, (uint16_t)(word(o->block_x) + k68_block_step_x(o->alt & 1, pick)));
    if (back(a))
        dy = (int16_t)-dy;
    set_word(o->block_y, (uint16_t)(word(o->block_y) + dy));
}

/*
 * @routine 83:E86A BlockPlatformMove
 * WHAT IT DOES: Moves the platform a frame along its leg: x and y by the
 * move its kind's list picks by its shape, the leg and the facing.
 */
static void platform_move(Actor *a)
{
    Kind68Own *o = own(a);
    uint8_t pick = (uint8_t)((((o->shape << 1) | (back(a) & 0xFF)) << 1) | a->facing);
    uint8_t at = k68_move_at((uint8_t)(pick + k68_move_list(a->spawn_byte)));

    a->x = wrap24(a->x + k68_move_x(at));
    a->y = wrap24(a->y + k68_move_y(at));
}

/*
 * @routine 83:E7F0 TileActorDraw
 * WHAT IT DOES: Puts the actor into the level's map as tiles at its block
 * point: the tile for its shape, alternation and flip, and the tile 16
 * pixels below when it has one; the empty tile in place of each while it is
 * on its way out (+$19 0).
 */
static void tile_actor_draw(Actor *a)
{
    Kind68Own *o = own(a);
    uint8_t pick = (uint8_t)((((o->alt ^ (uint8_t)back(a)) << 1 | a->flip) & 0xFF) << 2);
    uint16_t tile = k68_tile(o->shape, pick, 0), below = k68_tile(o->shape, pick, 1);
    uint16_t x = word(o->block_x), y = word(o->block_y);

    TileChangeAdd(back(a) ? tile : EMPTY_TILE, x, y);
    if (below)
        TileChangeAdd(back(a) ? below : EMPTY_TILE, x, (uint16_t)(y + 0x10));
}

/*
 * @routine 83:E90D Kind68PlayerOn
 * WHAT IT DOES: Tells whether the player is on kind $68 and then stands him
 * on it: his feet on its top, his ground direction from its own.
 */
static bool player_on(Actor *a)
{
    Actor *player = &g.actors[0];
    Kind68Own *o = own(a);
    uint16_t kept_x = (uint16_t)(a->own.bytes[OWN_4C] | a->own.bytes[OWN_4D] << 8);
    uint8_t pick, spot_at, box_at;
    uint16_t spot_x, spot_y, box_x, box_y, half_w, half_h, feet, top;
    bool rising = (int8_t)(player->speed_y >> 16) < 0;
    uint8_t base;

    if ((uint16_t)(pixels(a->y) - g.range_y + 0xC0) >= 0x0180)
        return false;
    if ((uint8_t)g.player_no_actor_landing)
        return false;
    if (g.player_in_air && rising)
        return false;
    pick = (uint8_t)(((o->shape << 1) | a->flip) << 2);   /* the original's index is the table's (4 bytes a spot) */
    spot_at = pick;
    box_at = (uint8_t)(pick << 1);
    spot_x = (uint16_t)(kept_x + k68_spot_x(spot_at));
    box_x = (uint16_t)(spot_x + k68_box(box_at, 0));
    spot_y = (uint16_t)(pixels(a->y) + k68_spot_y(spot_at));
    box_y = (uint16_t)(spot_y + k68_box(box_at, 1));
    half_w = (uint16_t)k68_box(box_at, 2);
    half_h = (uint16_t)k68_box(box_at, 3);
    if ((uint16_t)(pixels(player->x) - box_x + half_w) >= (uint16_t)(2 * half_w))
        return false;
    base = 0x25;
    if (g.health ? g.player_hurt_count != 0 : rising)
        base = 0x20;
    feet = (uint16_t)(base + pixels(player->y));
    if ((uint16_t)(feet - box_y + half_h) >= (uint16_t)(2 * half_h))
        return false;
    if (!o->shape) {
        top = (uint16_t)(spot_y + 8);
    } else {
        int16_t d = (int16_t)(spot_x - pixels(player->x));

        top = (uint16_t)(spot_y - (uint16_t)((d < 0 ? -d : d) >> 1));
    }
    if (top >= feet || (uint16_t)(top - feet) < 0xFFE8)
        return false;
    player->y = with_pixels(player->y, (uint16_t)(top - 0x20));
    g.player_actor_ground = (uint8_t)k68_dirs((uint8_t)((o->shape << 1) | a->flip));
    g.player_on_actor++;
    return true;
}

/*
 * @routine 83:E8A9 Kind68Wait
 * WHAT IT DOES: Kind $68's wait: carries the player along across while he
 * rides it (unless that puts him into firm ground), with a pause of 8 frames
 * after he leaves; it ends itself while the scroll's y limit is below 256.
 */
int Kind68Wait(Actor *a)
{
    Actor *player = &g.actors[0];
    uint8_t *on = &a->own.bytes[OWN_16], *was_on = &a->own.bytes[OWN_17];
    uint16_t kept = pixels(player->x);

    *on = 0;
    if (a->accel_y) {
        a->accel_y--;
    } else if (player_on(a)) {
        uint16_t kept_x = (uint16_t)(a->own.bytes[OWN_4C] | a->own.bytes[OWN_4D] << 8);

        *on = 1;
        player->x = with_pixels(player->x, (uint16_t)(pixels(a->x) - kept_x + pixels(player->x)));
        if (PlayerOnFirmGroundProbe8() || PlayerOnFirmGroundProbe4() || PlayerOnFirmGround())
            player->x = with_pixels(player->x, kept);
    }
    if (!(g.scroll_limit_y >> 8))
        return ActorEndPlaced(a);
    if (!*on && *was_on)
        a->accel_y = 8;
    *was_on = *on;
    return ACTOR_GOES_ON;
}

/* The spawner: eight platforms, +$07 7 down to 0, at spawn points $6A down by 2. */
static int spawner(Actor *a)
{
    a->x = with_pixels(a->x, 0);
    a->y = with_pixels(a->y, 0);
    a->spawn_byte = 7;
    a->spawn_at = 0x6A;
    do {
        (void)ActorCreateChild8(a, 0x68);
        a->spawn_at = (uint8_t)(a->spawn_at - 2);
    } while ((int8_t)--a->spawn_byte >= 0);
    return ActorEndPlaced(a);
}

/*
 * @routine 83:E6BB Kind68Start
 * WHAT IT DOES: Actor kind $68 (also kind $67's second state), the block
 * platform: with +$07 $0F a spawner that makes eight platforms and ends;
 * else one platform set up by its +$07 that, after its wait, moves along
 * its leg a block at a time (written into the level map while in bounds),
 * pauses, and goes back.
 */
static int start(Actor *a)
{
    Kind68Own *o = own(a);
    uint8_t v07 = a->spawn_byte, ways;

    if (v07 == MAKER)
        return spawner(a);
    a->probe_at = 0;
    a->anim_flags = 0x03;
    a->flags = 0x90;
    o->alt = 0;
    set_back(a, 0);
    a->accel_y = 0;
    a->own.bytes[OWN_16] = 0;
    a->own.bytes[OWN_17] = 0;
    set_word(o->block_x, k68_block_x(v07));
    set_word(o->block_y, k68_block_y(v07));
    a->accel_x = k68_leg_blocks(v07);
    ways = k68_ways(v07);
    a->facing = (uint8_t)(ways >> 2);
    a->flip = (uint8_t)(a->facing ^ 1);
    o->shape = (uint8_t)(ways & 1);
    o->unused48 = 0;
    o->last_x = (uint8_t)pixels(a->x);
    ActorKeepSpot(a);
    AnimStart(a, k68_pose(o->shape));
    a->resume = MOVE_START;
    return k68_start_wait(v07);
}

/* A frame of its leg: moves; once ten pixels along it writes its blocks
   (while in bounds) and steps its block point; at a leg's end it turns. */
static int move_frame(Actor *a)
{
    Kind68Own *o = own(a);
    uint8_t d;
    uint8_t dir = 0x10;

    ActorKeepSpot(a);
    platform_move(a);
    d = (uint8_t)((uint8_t)pixels(a->x) - o->last_x);
    if (d & 0x80) {
        d = (uint8_t)-d;
        dir = 0xF0;
    }
    if (d < 0x0A)
        return 1;
    o->last_x = (uint8_t)(o->last_x + dir);
    if (block_in_bounds(a))
        tile_actor_draw(a);
    block_step(a);
    if (--a->accel_x) {
        o->alt ^= 1;
        return 1;
    }
    a->accel_x = k68_leg_blocks(a->spawn_byte);
    set_back(a, (uint16_t)(back(a) ^ 1));
    a->facing ^= 1;
    block_step(a);
    o->alt ^= 1;
    o->wait[0] = k68_pause((uint8_t)back(a));
    a->resume = PAUSE;
    return 1;
}

int Kind68Code(Actor *a)
{
    switch (a->resume) {
    case ACTOR_START:
        return start(a);
    case ACTOR_START_SECOND:
        return ActorRunAs(a, Kind69Start, ACTOR_START);
    case MOVE_START:
        a->resume = MOVE;
        return 1;
    case MOVE:
        return move_frame(a);
    case PAUSE:
        ActorKeepSpot(a);
        if (--own(a)->wait[0])
            return 1;
        a->resume = MOVE_START;
        return 1;
    default:
        return kind_lost(a, "Kind68Code");
    }
}
