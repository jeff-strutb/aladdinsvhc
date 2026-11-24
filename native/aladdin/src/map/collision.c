/*
 * collision.c: what map tile is at a point, and how high its surface is
 * (map.h).
 *
 * An actor tests the map at probe points: offsets from its position, in
 * its probe list (Actor.probes, the probe Actor.probe_at). A point's tile
 * comes from the screen map (the screen under the point) and the screen's
 * tiles; the tile's behaviour byte gives its kind, and a surface's height
 * at the point's x. What each test finds is the caller's MapPoint.
 *
 * Not converted, with their proof (the record's own notes, each checked
 * against its callers): ActorSnapToGround5 (81:B440), ActorSnapToGroundHere
 * (81:B437), ActorOutOfGroundAtActor (81:B4E4), ActorOutOfWallsByFlip
 * (81:B566), ActorSettleOnGround (81:B6E7), ActorGroundCountLong (81:B729),
 * ActorOutOfWallsMovePlayer (81:B575), GroundBehindKind7 (81:B5D0),
 * GroundBehindLong (81:B799): no code refers to them (no JSR, JSL, JMP or
 * table word); ActorOutOfWallByFlip (81:B647) and ActorGroundCount
 * (81:B72D): only those dead routines call them.
 */
#include <stdio.h>

#include "data/game_tables.h"
#include "map/map.h"
#include "system/state.h"
#include "video/level_maps.h"

#define PROBE_Y 2               /* a probe's y offset, after its x offset */
#define VIEW_HEIGHT 0x00F0      /* a point more than this below the view's top is off it */
#define LEVEL_TALL_A 0x000A     /* levels whose map the player tests off the view's top and bottom too, */
#define LEVEL_TALL_B 0x000F     /* once the view has moved down */
#define TILE_ROWS_MASK 0x01E0   /* a tile place's row bits: 16 rows of $20 bytes */
#define SCREEN_ROW 0x0040       /* a screen place's row: 32 screens of 2 bytes */
#define SCREEN_EMPTY 0x8000     /* a screen map place with no screen */
#define OUT_OF_GROUND_RISE 0x000F   /* ActorOutOfGround tests the actor this far above the surface */

static uint16_t pixels_of(uint32_t v) { return (uint16_t)(v >> 8); }
static uint32_t pixels_set(uint32_t v, uint16_t px) { return (v & 0xFFu) | (uint32_t)px << 8; }

static uint16_t probe_of(const Actor *a, uint16_t at)
{
    return probe_word(a->probes, at);
}

/* the point's y: the probe's y offset down from the actor */
static void probe_y(const Actor *a, MapPoint *p)
{
    p->y = (uint16_t)(probe_of(a, (uint16_t)(a->probe_at + PROBE_Y)) + pixels_of(a->y));
}

/*
 * @routine 81:B4AC ProbeAtActor
 * WHAT IT DOES: Makes the probe point at the actor's x and its probe's y.
 */
void ProbeAtActor(const Actor *a, MapPoint *p)
{
    p->x = pixels_of(a->x);
    probe_y(a, p);
}

/*
 * @routine 81:B4B9 ProbeLeft
 * WHAT IT DOES: Makes the probe point to the actor's left: the probe's x
 * offset back from it, and its y offset down.
 */
void ProbeLeft(const Actor *a, MapPoint *p)
{
    p->x = (uint16_t)(pixels_of(a->x) - probe_of(a, a->probe_at));
    probe_y(a, p);
}

/*
 * @routine 81:B4C9 ProbeRight
 * WHAT IT DOES: Makes the probe point to the actor's right: the probe's x
 * offset on from it, and its y offset down.
 */
void ProbeRight(const Actor *a, MapPoint *p)
{
    p->x = (uint16_t)(pixels_of(a->x) + probe_of(a, a->probe_at));
    probe_y(a, p);
}

/*
 * @routine 81:B7F2 ProbeOnView
 * WHAT IT DOES: For the player, says whether a point is above the view's
 * top or more than 240 pixels below it (at the top itself counts as
 * above), except on levels $0A and $0F once the view has moved down
 * (their maps are tested whole). The original then returns from the tile
 * lookup with behaviour and kind 0: TileAt does so here.
 */
static bool ProbeOnViewOff(const Actor *a, const MapPoint *p)
{
    uint16_t top = pixels_of(g.layers[0].scroll_y);

    if (a != &g.actors[0])
        return false;
    if (top != 0 && (g.level == LEVEL_TALL_A || g.level == LEVEL_TALL_B))
        return false;
    return top >= p->y || (uint16_t)(top + VIEW_HEIGHT) < p->y;
}

/*
 * @routine 81:B880 TileAtPlace
 * WHAT IT DOES: The tile at a place in the point's screen: its word,
 * behaviour and kind.
 * HOW: The place plus the screen's tiles' offset is the tile's byte offset
 * in the level's tiles; the tile's word over 8 (its metatile) indexes the
 * behaviour table; the behaviour's bit 7 is kept as mirrored, its low 3
 * bits give the kind.
 */
static uint8_t TileAtPlace(MapPoint *p, uint16_t place)
{
    uint16_t at = (uint16_t)(place + p->screen_tiles);
    uint16_t metatile;

    if (p->fill) {
        /* past the layout's right end in a wide view (no original): the tile the fill drew there, the one whose top is
           p->surface_y at p->x (TileAt's tile, or SurfaceHeightOver's above it); where it drew none, an empty tile */
        if (level_maps_field_cell(p->x, p->surface_y, &p->word) != 1) {
            p->word = 0;
            p->behaviour = 0;
            p->mirrored = 0;
            p->kind = TILE_KIND_EMPTY;
            return p->kind;
        }
    } else
        p->word = g.level_tiles[at / 2];
    metatile = (uint16_t)(p->word >> 3);
    if (metatile < sizeof g.tile_behaviour) {
        p->behaviour = g.tile_behaviour[metatile];
    } else {
        /* the original reads on past its table, into the text layer's map
           ($7E:D800); no level's map has such a word */
        static bool said;

        if (!said)
            fprintf(stderr, "map: tile word $%04X past the behaviour table\n", p->word);
        said = true;
        p->behaviour = 0;
    }
    p->mirrored = (uint8_t)(p->behaviour & TILE_MIRRORED);
    p->kind = tile_kind(p->behaviour);
    return p->kind;
}

/*
 * @routine 81:B831 TileAt
 * @routine 81:B82F TileAtProbe
 * @routine 81:B82B TileAtProbeLong
 * WHAT IT DOES: The map tile at the probe point: its word, behaviour and
 * kind (for the player, an empty tile off the view).
 * HOW: The point's screen place in the 32 x 32 screen map, one screen on
 * each way (its x and y high bytes plus one, the low 5 and 4 bits); the
 * screen's tiles' offset from there, a place with no screen taken as 0;
 * the tile's top y kept as the surface; the point's 16-pixel tile place in
 * its screen; then TileAtPlace. (TileAtProbe and TileAtProbeLong, its
 * entries for 8-bit and other banks' callers, are this.)
 */
uint8_t TileAt(const Actor *a, MapPoint *p)
{
    uint16_t tiles;

    if (ProbeOnViewOff(a, p)) {
        p->fill = 0;
        p->behaviour = 0;
        p->kind = TILE_KIND_EMPTY;
        return TILE_KIND_EMPTY;
    }
    p->screen_place = (uint16_t)(((((p->x >> 8) + 1) & 0x1F) << 1) | ((((p->y >> 8) + 1) & 0x0F) << 6));
    p->surface_y = (uint16_t)(p->y & 0xFFF0);
    tiles = g.screen_map.places[p->screen_place / 2];
    p->screen_tiles = (tiles & SCREEN_EMPTY) ? 0 : tiles;
    p->tile_place = (uint16_t)(((p->x >> 3) & 0x001E) | ((p->y << 1) & TILE_ROWS_MASK));
    p->fill = level_maps_field_cell(p->x, p->surface_y, &p->word) != 0;     /* 0 at 256 x 224: the original's lookup */
    return TileAtPlace(p, p->tile_place);
}

/*
 * @routine 81:B8EB SurfaceHeight
 * WHAT IT DOES: The y of a tile's surface at the point's x, for a tile of
 * kind `kind`, into the low byte of the surface (the tile's top).
 * HOW: Kind bits 4-6 all set: flat, at its low nibble below the tile's
 * top. A slope's surface is its low nibble less the point's x in the tile
 * (mirrored: 15 less it), shifted right by bits 4-6; the subtraction is
 * 8-bit and ORed in as it comes. The mirrored byte is shifted left.
 */
void SurfaceHeight(MapPoint *p, uint8_t kind)
{
    uint8_t drop, n;

    if ((kind & 0x70) == 0x70) {
        p->surface_y = (uint16_t)(p->surface_y | (kind & 0x0F));
        return;
    }
    drop = (uint8_t)(p->x & 0x0F);
    if (p->mirrored & 0x80)
        drop ^= 0x0F;
    p->mirrored = (uint8_t)(p->mirrored << 1);
    for (n = (uint8_t)(kind >> 4); n != 0; n--)
        drop >>= 1;
    p->surface_y = (uint16_t)(p->surface_y | (uint8_t)((kind & 0x0F) - drop));
}

/*
 * @routine 81:B8AD SurfaceHeightSolid
 * WHAT IT DOES: The surface's y for a tile of kind `kind`, a solid tile
 * taken as flat at its top.
 */
static void SurfaceHeightSolid(MapPoint *p, uint8_t kind)
{
    SurfaceHeight(p, kind == TILE_KIND_SOLID ? 0x70 : kind);
}

/*
 * @routine 81:B8B6 SurfaceHeightOver
 * WHAT IT DOES: The surface's y for a tile of kind `kind`; for a solid
 * tile, the surface of the tile above it: false when that tile is empty.
 * HOW: Kind 1: the surface up a tile and the place up a row; a place past
 * the screen's top takes the tiles of the screen above (that place's word
 * as it is); then TileAtPlace, and SurfaceHeight with its kind.
 */
static bool SurfaceHeightOver(MapPoint *p, uint8_t kind)
{
    uint16_t place;

    if (kind != TILE_KIND_SOLID) {
        SurfaceHeight(p, kind);
        return true;
    }
    p->surface_y = (uint16_t)(p->surface_y - 0x10);
    place = (uint16_t)((p->tile_place - 0x20) & 0x01FE);
    if (place >= TILE_ROWS_MASK)
        p->screen_tiles = g.screen_map.places[(uint16_t)(p->screen_place - SCREEN_ROW) / 2 % SCREEN_MAP_PLACES];
    kind = TileAtPlace(p, place);
    if (kind == TILE_KIND_EMPTY)
        return false;
    SurfaceHeight(p, kind);
    return true;
}

/*
 * @routine 81:B7D5 PointInGround
 * @routine 81:B7CF PointInGroundLong
 * WHAT IT DOES: Says whether the probe point is in the ground: a solid
 * tile, or a surface the point is at or below. Empty and out are not.
 */
int PointInGround(const Actor *a, MapPoint *p)
{
    uint8_t kind = TileAt(a, p);

    if (kind == TILE_KIND_EMPTY || (kind & TILE_KIND_OUT))
        return 0;
    if (kind == TILE_KIND_SOLID)
        return 1;
    SurfaceHeight(p, kind);
    return p->y >= p->surface_y;
}

/*
 * @routine 81:B7C3 ActorInGround
 * WHAT IT DOES: Says whether the actor's own position is in the ground.
 */
int ActorInGround(const Actor *a, MapPoint *p)
{
    p->x = pixels_of(a->x);
    p->y = pixels_of(a->y);
    return PointInGround(a, p);
}

/*
 * @routine 81:B7A3 GroundRight
 * @routine 81:B7BB GroundRightLong
 * WHAT IT DOES: Says whether the ground is at the probe point to the
 * actor's right.
 */
int GroundRight(const Actor *a, MapPoint *p)
{
    ProbeRight(a, p);
    return PointInGround(a, p);
}

/*
 * @routine 81:B7A8 GroundLeft
 * @routine 81:B7BF GroundLeftLong
 * WHAT IT DOES: Says whether the ground is at the probe point to the
 * actor's left.
 */
int GroundLeft(const Actor *a, MapPoint *p)
{
    ProbeLeft(a, p);
    return PointInGround(a, p);
}

/*
 * @routine 81:B79F GroundInFront
 * @routine 81:B795 GroundInFrontLong
 * WHAT IT DOES: Says whether the ground is at the probe point in front of
 * the actor, the way it faces.
 */
int GroundInFront(const Actor *a, MapPoint *p)
{
    return a->facing == 0 ? GroundRight(a, p) : GroundLeft(a, p);
}

/*
 * @routine 81:B7AD GroundBehind
 * WHAT IT DOES: Says whether the ground is at the probe point behind the
 * actor: its flip clear, to the right; set, to the left.
 */
int GroundBehind(const Actor *a, MapPoint *p)
{
    return a->flip == 0 ? GroundRight(a, p) : GroundLeft(a, p);
}

/*
 * @routine 81:B5F6 GroundBehindBit4
 * WHAT IT DOES: Tests the ground behind the actor, and when the tile's
 * behaviour has bit 4, shifts the behaviour left a bit (its bit 7 out:
 * the original's C).
 */
int GroundBehindBit4(Actor *a, MapPoint *p)
{
    uint8_t out;

    (void)GroundBehind(a, p);
    if (!(p->behaviour & 0x10))
        return 0;
    out = (uint8_t)(p->behaviour >> 7);
    p->behaviour = (uint8_t)(p->behaviour << 1);
    return 1 + out;
}

/*
 * @routine 81:B5B3 GroundBehindBit3
 * WHAT IT DOES: Tests the ground behind the actor at probes $10 and then
 * $14, until a tile with behaviour bit 3.
 */
uint8_t GroundBehindBit3(Actor *a, MapPoint *p)
{
    static const uint8_t probes[2] = { 0x10, 0x14 };
    uint8_t bit = 0;

    for (int i = 0; i < 2 && !bit; i++) {
        a->probe_at = probes[i];
        (void)GroundBehind(a, p);
        bit = (uint8_t)(p->behaviour & 0x08);
    }
    return bit;
}

/*
 * @routine 81:B60C ActorOutOfWall
 * @routine 81:B608 ActorOutOfWallLong
 * WHAT IT DOES: When the probe in front of the actor is in a wall, moves
 * the actor out of it: true when it was moved.
 * HOW: GroundInFront at `probe`; a tile whose kind is a wall and that the
 * actor cannot pass through moves the actor's x by the point's distance
 * past the tile's near edge: back by (x in tile) + 1 in its left half,
 * on by 16 - (x in tile) in its right half.
 */
static int out_of_wall(Actor *a, uint8_t probe, MapPoint *p)
{
    uint16_t in_tile;

    a->probe_at = probe;
    if (!GroundInFront(a, p) || !tile_is_wall(p->behaviour) || (p->behaviour & TILE_THROUGH))
        return 0;
    in_tile = (uint16_t)(p->x & 0x000F);
    if (in_tile & 0x0008)
        a->x = pixels_set(a->x, (uint16_t)(pixels_of(a->x) + 0x10 - in_tile));
    else
        a->x = pixels_set(a->x, (uint16_t)(pixels_of(a->x) - (in_tile + 1)));
    return 1;
}

int ActorOutOfWall(Actor *a, uint8_t probe)
{
    MapPoint p;

    return out_of_wall(a, probe, &p);
}

/*
 * @routine 81:B5AD ActorOutOfWallProbe4
 * WHAT IT DOES: Moves the actor out of a wall at probe 4 in front of it.
 */
int ActorOutOfWallProbe4(Actor *a)
{
    return ActorOutOfWall(a, 0x04);
}

/* ActorOutOfWall and ActorOutOfWallProbe4 for the callers that read the
   point they tested (its behaviour) after them */
int ActorOutOfWallAt(Actor *a, uint8_t probe, MapPoint *p)
{
    return out_of_wall(a, probe, p);
}

int ActorOutOfWallProbe4At(Actor *a, MapPoint *p)
{
    return out_of_wall(a, 0x04, p);
}

/*
 * @routine 81:B59F ActorOutOfWalls
 * WHAT IT DOES: Moves the actor out of a wall in front of it, testing
 * probes $0C, 8 and 4 until one moves it.
 */
int ActorOutOfWalls(Actor *a, MapPoint *p)
{
    MapPoint own;

    if (!p)
        p = &own;
    return out_of_wall(a, 0x0C, p) || out_of_wall(a, 0x08, p) || out_of_wall(a, 0x04, p);
}

/* A probe's ground for ActorSnapToGround and ActorOutOfGround: the tile at
   the point `probe` makes, and its surface by `over` (SurfaceHeightOver)
   or not (SurfaceHeightSolid). -1 no tile there (empty or out); else
   whether the surface step found ground (0: the tile above a solid one
   is empty). */
static int ground_at(Actor *a, MapPoint *p, void (*probe)(const Actor *, MapPoint *), bool over)
{
    uint8_t kind;

    probe(a, p);
    kind = TileAt(a, p);
    if (kind == TILE_KIND_EMPTY || (kind & TILE_KIND_OUT))
        return -1;
    if (over)
        return SurfaceHeightOver(p, kind);
    SurfaceHeightSolid(p, kind);
    return 1;
}

/* the actor's y moved by the surface less the point's y */
static uint16_t onto_surface(const Actor *a, const MapPoint *p)
{
    return (uint16_t)(p->surface_y - p->y + pixels_of(a->y));
}

/*
 * @routine 81:B44F ActorSnapToGround
 * WHAT IT DOES: When there is ground just under the actor (within 3
 * pixels), stands it on that ground: true. Else leaves it where it was.
 * HOW: Its y kept and moved 3 down; probe 0 at the actor's x (the tile
 * above for a solid tile), then to its left and right (a solid tile flat
 * at its top); the first ground found moves the y by the surface less the
 * point's y. With none, the kept y comes back.
 */
int ActorSnapToGround(Actor *a, MapPoint *p)
{
    MapPoint own;
    uint16_t kept = pixels_of(a->y);

    if (!p)
        p = &own;
    a->y = pixels_set(a->y, (uint16_t)(kept + 3));
    a->probe_at = 0;
    if (ground_at(a, p, ProbeAtActor, true) == 1 || ground_at(a, p, ProbeLeft, false) == 1
        || ground_at(a, p, ProbeRight, false) == 1) {
        a->y = pixels_set(a->y, onto_surface(a, p));
        return 1;
    }
    a->y = pixels_set(a->y, kept);
    return 0;
}

/*
 * @routine 81:B4F3 ActorOutOfGround
 * WHAT IT DOES: When the actor is in the ground at its probes, raises it
 * onto the surface, unless that would leave it in the ground 15 pixels
 * higher too, where it goes as far as that allows: true when raised.
 * HOW: Probe 0 at the actor's x (the tile above for a solid tile), then to
 * its left and right (a solid tile flat at its top), stopping at the
 * first tile; none: false. The point at or below the surface: the raised
 * y kept, the actor tested 15 pixels above it at probe 0, and a tile there
 * whose top is above the point lowers the kept y by the difference; the
 * actor takes the kept y. (Its probe's high byte, +$19, is also cleared
 * when a caller enters with 16-bit memory; the native record keeps the
 * probe in a byte.)
 */
int ActorOutOfGround(Actor *a)
{
    MapPoint p;
    uint16_t kept;
    uint8_t kind;

    a->probe_at = 0;
    if (ground_at(a, &p, ProbeAtActor, true) < 0 && ground_at(a, &p, ProbeLeft, false) < 0
        && ground_at(a, &p, ProbeRight, false) < 0)
        return 0;
    if (p.y < p.surface_y)
        return 0;
    kept = onto_surface(a, &p);
    a->y = pixels_set(a->y, (uint16_t)(kept - OUT_OF_GROUND_RISE));
    ProbeAtActor(a, &p);
    kind = TileAt(a, &p);
    if (kind != TILE_KIND_EMPTY && !(kind & TILE_KIND_OUT) && p.surface_y < p.y)
        kept = onto_surface(a, &p);
    a->y = pixels_set(a->y, kept);
    return kept != 0;
}

/* ActorSettleOnFloor's side test: 1 a floor tile under the point, 0 not a
   floor, -1 give up (with `strict`: a kind with bit 7, or a tile actors
   pass through; else those are 0). */
static int floor_at(const Actor *a, MapPoint *p, bool strict)
{
    uint8_t kind = TileAt(a, p);

    if (kind == TILE_KIND_EMPTY)
        return 0;
    if ((kind & TILE_KIND_OUT) || (p->behaviour & TILE_THROUGH))
        return strict ? -1 : 0;
    return tile_is_floor(p->behaviour);
}

/*
 * @routine 81:B673 ActorSettleOnFloor
 * WHAT IT DOES: When a floor tile is under the actor's left or right probe
 * 8, and the ground is under the actor itself, stops its fall and stands
 * it on that ground's tile top: true.
 * HOW: ProbeLeft at probe 8, then its tile: a floor kind (not passed
 * through) goes on; else the same at ProbeRight, where a pass-through tile
 * or a kind with bit 7 gives up. A point above the map's top (y negative)
 * gives up. Then the tile at the actor's own x at that y: ground (not
 * empty, not out) clears the vertical speed and moves y down to the next
 * tile boundary.
 */
int ActorSettleOnFloor(Actor *a, MapPoint *p)
{
    MapPoint own;
    uint16_t y;
    uint8_t kind;

    if (!p)
        p = &own;
    a->probe_at = 0x08;
    ProbeLeft(a, p);
    if (p->y & 0x8000)
        return 0;
    if (floor_at(a, p, false) != 1) {
        ProbeRight(a, p);
        if ((p->y & 0x8000) || floor_at(a, p, true) != 1)
            return 0;
    }
    p->x = pixels_of(a->x);
    kind = TileAt(a, p);
    if (kind == TILE_KIND_EMPTY || (kind & TILE_KIND_OUT))
        return 0;
    a->speed_y = 0;
    y = (uint16_t)(pixels_of(a->y) + 0x10 - (p->y & 0x000F));
    a->y = pixels_set(a->y, y);
    return y != 0;
}

/* One side of PlayerOnFirmGround's test: the player's probe on that side
   in the ground, its tile not one of the `through` behaviours and its
   kind not soft. */
static bool firm_side(bool left, uint8_t through)
{
    const Actor *player = &g.actors[0];
    MapPoint p;

    if (!(left ? GroundLeft(player, &p) : GroundRight(player, &p)))
        return false;
    return !(p.behaviour & through) && !tile_is_soft(p.behaviour);
}

/* PlayerOnFirmGround's test at a probe: left side first (not on behaviour
   bits 5 or 6), then the right (not on bit 6) */
static int firm_at(uint8_t probe)
{
    g.actors[0].probe_at = probe;
    return firm_side(true, 0x60) || firm_side(false, TILE_THROUGH);
}

/*
 * @routine 81:B756 PlayerOnFirmGround
 * WHAT IT DOES: Says whether the player stands on firm ground at probe 0:
 * the ground to his left or right solid and not soft.
 */
int PlayerOnFirmGround(void)
{
    return firm_at(0x00);
}

/*
 * @routine 81:B74E PlayerOnFirmGroundProbe8
 * WHAT IT DOES: PlayerOnFirmGround's test at probe 8.
 */
int PlayerOnFirmGroundProbe8(void)
{
    return firm_at(0x08);
}

/*
 * @routine 81:B75A PlayerOnFirmGroundProbe4
 * WHAT IT DOES: PlayerOnFirmGround's test at probe 4.
 */
int PlayerOnFirmGroundProbe4(void)
{
    return firm_at(0x04);
}
