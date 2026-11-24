/*
 * map.h: the level's map: building it (level_map.c), what is at a point of
 * it (collision.c, tile_behaviour.c) and changing its tiles while it is
 * played (tile_changes.c).
 */
#ifndef ALADDIN_MAP_H
#define ALADDIN_MAP_H

#include <stdbool.h>
#include <stdint.h>

#include "system/state.h"

/* level_map.c */
void LevelMapBuild(void);               /* 81:BEEF (LevelMapBuildLong 81:BEEB): for g.level */
void ScreenMapLayout(uint16_t level, unsigned layout, unsigned first_place);    /* 81:BFEE */

/* tile_behaviour.c */
void TileBehaviourLoad(void);           /* 81:BEC5: g.tile_behaviour for the map LevelMapBuild built */

/* tile_changes.c */
void TileChangesApply(void);            /* 81:999A */
uint16_t MapTileAddress(uint16_t x, uint16_t y);    /* 81:9C50: the tile under the point, its index in g.level_tiles */
void TileChangeAdd(uint16_t tile, uint16_t x, uint16_t y);  /* 81:9978: the map tile at the point to become `tile`, queued */
uint16_t MapQuarterAt(uint16_t x, uint16_t y);      /* 81:9B7D: the map word of the 8 x 8 quarter of the tile at the point */
uint16_t MapTileAt(uint16_t x, uint16_t y);         /* 81:9A84: the level's map tile at the point */
void MapTileSet(uint16_t tile, uint16_t x, uint16_t y);     /* 81:9B6D: in the map only, not on screen */

/* ---- collision.c: what map tile is at a point, and how high its surface is ----

   An actor tests the map at probe points: offsets from its position, in the
   probe list its record names (Actor.probes; the probe Actor.probe_at, 4
   bytes a probe: x offset, y offset, words). A point's tile comes from the
   screen map and the screen's tiles; its behaviour byte (g.tile_behaviour)
   gives its kind by the low 3 bits. What a test found is in its MapPoint:
   the original kept it in scratch ($0C-$1E), where the callers read it. */

/* a tile's kind (the table at $80:BF22, by behaviour bits 0-2): 0 empty,
   1 solid, $80 out (none stands there; no map has one); else a surface,
   bits 0-3 its height below the tile's top at its high end and bits 4-6 how
   far right a step down is shifted (7: flat at bits 0-3) */
enum { TILE_KIND_EMPTY = 0x00, TILE_KIND_SOLID = 0x01, TILE_KIND_OUT = 0x80 };

/* behaviour bits */
#define TILE_MIRRORED 0x80      /* the slope falls the other way */
#define TILE_THROUGH 0x40       /* actors pass through it */

typedef struct MapPoint {
    uint16_t x, y;              /* $0C, $0E: the point, pixels (the probes set it; TileAt takes it) */
    uint16_t surface_y;         /* $10: the tile's top, then (SurfaceHeight) its surface at x */
    uint8_t behaviour;          /* $12: the tile's behaviour byte */
    uint8_t kind;               /* $13: its kind */
    uint16_t screen_place;      /* $14: the screen's place in the screen map, a byte offset */
    uint16_t tile_place;        /* $16: the tile's place in its screen, a byte offset */
    uint16_t screen_tiles;      /* $18: the screen's tiles' offset in g.level_tiles, bytes */
    uint8_t mirrored;           /* $1A: the behaviour's bit 7 (SurfaceHeight shifts it out) */
    uint16_t word;              /* $1C: the tile's map word */
    uint8_t fill;               /* (no original) past the layout's right end in a wide view: the tiles are the fill's (TileAt) */
} MapPoint;

/* the probe point: at the actor's x, to its left, to its right; y the
   probe's y offset down from it */
void ProbeAtActor(const Actor *a, MapPoint *p);  /* 81:B4AC */
void ProbeLeft(const Actor *a, MapPoint *p);     /* 81:B4B9 */
void ProbeRight(const Actor *a, MapPoint *p);    /* 81:B4C9 */

/* the tile at p->x, p->y: its kind (and p's findings); for the player, a
   point above the view or more than 240 pixels below its top reads empty
   (not on levels $0A and $0F once the view has moved down) */
uint8_t TileAt(const Actor *a, MapPoint *p);     /* 81:B831, 81:B82F TileAtProbe, 81:B82B TileAtProbeLong */

/* p->surface_y moved to the surface of a tile of kind `kind` at p->x */
void SurfaceHeight(MapPoint *p, uint8_t kind);   /* 81:B8EB */

/* 1 when the point p is in the ground: a solid tile, or at or below a
   surface; 0 empty or out */
int PointInGround(const Actor *a, MapPoint *p);  /* 81:B7D5 (81:B7CF Long) */
int ActorInGround(const Actor *a, MapPoint *p);  /* 81:B7C3: at its own x, y */
int GroundRight(const Actor *a, MapPoint *p);    /* 81:B7A3 (81:B7BB Long): its probe to the right */
int GroundLeft(const Actor *a, MapPoint *p);     /* 81:B7A8 (81:B7BF Long) */
int GroundInFront(const Actor *a, MapPoint *p);  /* 81:B79F (81:B795 Long): the way it faces */
int GroundBehind(const Actor *a, MapPoint *p);   /* 81:B7AD: by its flip (clear: the right) */
/* GroundBehind, then: 0 when the tile has no behaviour bit 4; else
   p->behaviour shifted left a bit, as the original, and 1, or 2 when the
   bit shifted out (bit 7) was set (the original's C) */
int GroundBehindBit4(Actor *a, MapPoint *p);     /* 81:B5F6 */
/* GroundBehind at probe $10, then $14, until a tile with behaviour bit 3:
   that bit (0 or 8) of the last tile tested */
uint8_t GroundBehindBit3(Actor *a, MapPoint *p); /* 81:B5B3 */

/* moves: 1 when the actor was moved. Those with a MapPoint leave in it the
   last point they tested, as the original leaves its scratch (the player
   reads the behaviour and kind after them); NULL when not wanted */
int ActorOutOfWall(Actor *a, uint8_t probe);     /* 81:B60C (81:B608 Long): out of a wall at `probe` in front */
int ActorOutOfWallProbe4(Actor *a);              /* 81:B5AD */
/* the same two, leaving in p the point tested (kinds $08 and $90 read the
   wall tile's behaviour after them, the original's $0012) */
int ActorOutOfWallAt(Actor *a, uint8_t probe, MapPoint *p);  /* 81:B60C */
int ActorOutOfWallProbe4At(Actor *a, MapPoint *p);           /* 81:B5AD */
int ActorOutOfWalls(Actor *a, MapPoint *p);      /* 81:B59F: probes $0C, 8, then 4 */
int ActorSnapToGround(Actor *a, MapPoint *p);    /* 81:B44F: onto ground within 3 pixels below */
int ActorOutOfGround(Actor *a);                  /* 81:B4F3: raised out of the ground (0: no tile, or not in it) */
int ActorSettleOnFloor(Actor *a, MapPoint *p);   /* 81:B673: its fall stopped on a floor under probe 8 */

/* 1 when the player stands on firm ground at probe 0, 8 or 4 */
int PlayerOnFirmGround(void);                    /* 81:B756 */
int PlayerOnFirmGroundProbe8(void);              /* 81:B74E */
int PlayerOnFirmGroundProbe4(void);              /* 81:B75A */

/* wanted */

#endif
