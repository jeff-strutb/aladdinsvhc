/*
 * level_maps.h: the level's play field and backdrop as whole-level engine
 * maps (video/level_maps.c, docs/aladdin/native/VIEW.md section 4.3).
 */
#ifndef ALADDIN_LEVEL_MAPS_H
#define ALADDIN_LEVEL_MAPS_H

#include <stdint.h>

#include "nshvc/nshvc.h"
#include "video/level_fill.h"

/* LayersFill, for a level's play: both layers' maps built from the level's map and the layers shown from them */
void LevelMapsBuild(void);
/* DisplayDefaults: no level's layers are shown (the still screens' pictures are the video maps) */
void LevelMapsOff(void);
/* LevelBackdropStart (levels 7 and $17): the backdrop is a picture uploaded into BACK_TALL, not the level's map */
void LevelMapsBackdropOff(void);
/* Kind $81's freeze (its layers draw no more): the layers' tiles about the scroll go into the video maps the
   picture then shows, and the level's maps are no longer shown */
void LevelMapsFreeze(void);
/* TileChangesApply: the 16 x 16 tile at pixel x, y of the play field becomes `tile` (a level tile word) */
void LevelMapsTile(uint16_t x, uint16_t y, uint16_t tile);
/* a screen map place patched in play (level $0A's place $21): that screen of the play field is built again from the place */
void LevelMapsPlace(unsigned place);
/* the view changed in play (VIEW.md 4.8): whether a layer's margins would be other than they are at a view whose original screen sits `dx`
   pixels in from each side, and the margins made again (the layout's words as they are now are kept); LevelMapsResize returns the layers made */
int LevelMapsNeedResize(int32_t dx);
int LevelMapsResize(void);
void LevelMapsReset(void);
/* the developer's state hash (ALADDIN_DEV_STATE_HASH): the layers' maps and their words, folded into h */
uint64_t level_maps_hash(uint64_t h);
/* the snapshot (system/snapshot.c): the layers' records and words as bytes, and back; max, save, check (fits), load (1 on success) */
size_t level_maps_snapshot_max(void);
size_t level_maps_snapshot_save(uint8_t *out);
int level_maps_snapshot_check(const uint8_t *in, size_t len);
int level_maps_snapshot_load(const uint8_t *in, size_t len);
/* whether layer 0 (the play field) or 1 (the backdrop) is shown from its level map now, and that map */
int level_maps_active(int layer);
nshvc_map level_maps_object(int layer);

/* the play field's tile where the wide view's fill drew it (VIEW.md 4.4): for a point at pixels x, y past the layout's right end across, in a
   view wider than 256 whose field has margins, 1 with the metatile's level tile word in *word, or 2 where the fill drew nothing (an empty
   tile); 0 everywhere else (the original's lookup holds: inside the layout, left of it, at 256 x 224, or a field with no margins) */
int level_maps_field_cell(uint16_t x, uint16_t y, uint16_t *word);

/* the margin of a layer's level map left of and above the level, in pixels (0, 0 when the layer has none): the level's
   (0, 0) is at this position in the map, so a layer shown from it adds it to the scroll */
void level_maps_margin(int layer, int32_t *x, int32_t *y);
/* the layer's level map's height in pixels, with its margins (0 when it has none) */
int32_t level_maps_height(int layer);
/* the developer's note of how far the layer's window goes past the level (ALADDIN_DEV_OVER) */
void level_maps_note(int layer, int32_t scroll_x, int32_t scroll_y);

/* the developer's map dump: a layer's whole map words (NULL when not built), its size, the level's own and its top
   left in the map, in tiles, and how each band past the level's edges was filled */
int level_maps_word(int layer, int32_t c, int32_t r, uint16_t *word);    /* a layer's whole-level map word at tile column c, row r (margins included); 0 if none */
const uint16_t *level_maps_words(int layer, uint32_t *w, uint32_t *h, uint32_t *level_w, uint32_t *level_h, uint32_t *margin_x,
                                 uint32_t *margin_y);
const FillReport *level_maps_fill(int layer);
void level_maps_provenance(int layer, const int32_t **src, const uint8_t **kind);

#endif
