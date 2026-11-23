/*
 * video.h: the picture. The game's tile sets, maps, palette and sprites are
 * engine objects; the game writes them when its graphics change, and
 * video_draw sets the whole picture from the game's state once a picture.
 *
 * The loop calls three things:
 *   video_init     from init: the engine objects made.
 *   video_vblank   from update, last, on each frame whose NMI is on: what the
 *                  original's NMI uploaded at the start of the next frame (the
 *                  VRAM queue, the lists, tile blocks, words, the text buffer
 *                  and its parts, the upload by number, the palette, the
 *                  sprites), written to the objects. Nothing runs between the
 *                  end of one update and the next one's tasks, so this is the
 *                  original's order, and the draw after it shows what the
 *                  original showed in the frame that NMI began.
 *   video_draw     from draw: the whole picture, from the fields of g the
 *                  original's NMI and IRQ copied to the PPU.
 *
 * Writes by video address. The original names its destinations by VRAM word
 * address in its queue records and tables; vram_write is the one place that
 * turns an address into the objects that show it. Every write goes to every
 * object that shows that address on any screen, so a screen that changes
 * its layer bases after writing (the title, the Genie screen) shows what was
 * written before:
 *   $0000-$3FFF  BG tiles from base 0, 4bpp, 1,024 tiles; on a Mode 7
 *                picture, also its map (low bytes) and pixels (high bytes)
 *   $0000-$0FFF  also BG tiles from base $5000, tiles 768-1,023 (the wrap)
 *   $4000-$47FF  BG2's map, 64 x 32
 *   $4800-$4FFF  BG1's map, 64 x 32
 *   $5000-$7FFF  BG tiles from base $5000, 4bpp, tiles 0-767; the sprites'
 *                tiles are 256 on (the sprites' base $6000)
 *   $5000-$6FFF  BG3's tiles, 2bpp, 1,024
 *   $5C00-$5FFF  BG3's map, 32 x 32 (also BG1's on the title)
 * Writes of the low bytes alone are Mode 7's map, of the high bytes alone its
 * pixels. The address has 15 bits, as the original's: past $7FFF a write
 * goes on at 0. Nothing is kept behind these writes: what the original read back
 * from video memory, its callers now pass (Mode7ScreenSetup; Jeff,
 * 2026-11-19).
 *
 * Screens outside video/ name what they write (VramCopy entries, a VideoMap);
 * the numeric address is for records that come from the cartridge's tables
 * and the queues.
 */
#ifndef ALADDIN_VIDEO_H
#define ALADDIN_VIDEO_H

#include <stdint.h>

#include "system/state.h"

void video_init(void);
void video_vblank(void);
void video_update_end(void);           /* at each update's end: Mode 7's changed art, held with the update's writes (between.h) */
void video_draw(void);

/* ---- writes by video address (the one numeric part) ---- */

typedef enum {
    VRAM_WORDS,         /* whole words */
    VRAM_LOW_BYTES,     /* the low bytes alone: Mode 7's map */
    VRAM_HIGH_BYTES     /* the high bytes alone: Mode 7's pixels */
} VramWrite;

/* count: words for VRAM_WORDS, bytes for the others */
void vram_write(uint16_t address, const void *data, uint32_t count, VramWrite how);
/* count words, the address stepping by `step` after each (1, 32 or 128) */
void vram_write_stepped(uint32_t address, const void *data, uint32_t count, uint16_t step);
/* one word count times from address on */
void vram_fill(uint16_t address, uint16_t word, uint32_t count);

/* ---- screen set-up (video/screen_setup.c, vram_copy.c, unpack.c, mode7.c) ---- */

void ScrollStateClear(void);                /* 81:96CB */
void DisplayDefaults(void);                 /* 81:9865 */
void video_view_level(void);         /* a level starts loading: the view from the window (view.h), before its limits are set */
void video_view_still(void);               /* a still screen starts (DisplayDefaults): the view from the window, unless a level is loading */
void video_view_follow(void);              /* at an update's start: the view follows the window (VIEW.md 4.8) */
void video_view_level_done(void);          /* the level's screen is set up: a later DisplayDefaults is a still screen */
void video_level_view_set(int on);          /* a level is the screen (LevelScreenSetup: 1) or no longer (DisplayDefaults: 0): the effects and the text layer take the level's view */
void VramCopy(int entry);               /* 81:F5DC: entry of the first copy table (the record's X / 7) */
void VramCopy2(int entry);              /* 81:F60D: entry of the second */
void UnpackSet(int set);                /* 81:BAF1: packed set to g.unpacked */
void AnimSetGraphicsUnpack(uint8_t set);    /* 81:BA9E */
void LevelGraphicsLoad(void);               /* 81:B92D: g.level's graphics */
/* 81:AF3B Mode7ScreenSetup: Mode 7 from 256 4bpp tiles (8,192 bytes, as the
   original's video memory held them from 0) and a 32 x 32 map's words (2,048
   bytes, as it held them from $4000), which the caller names: the record
   read them back from video memory (Jeff, 2026-11-19). Declared here as
   Mode7ScreenSetup once the title's call passes them; until then it is
   video_mode7_screen_setup. */
void video_mode7_screen_setup(uint8_t row_flag, const uint8_t *planar_tiles, const uint8_t *map_words);
void Mode7ScreenSetup(uint8_t row_flag);    /* the title's call until then: system/'s placeholder */

/* ---- palettes (video/palette.c, palette_cycles.c) ---- */

void PaletteLoad(int set);              /* 81:98C5 */
void PaletteSave(uint16_t first, uint8_t count);         /* 81:9DAA: palettes first.. kept to fade from */
void PaletteFadeStep(int8_t step, uint8_t count);       /* 81:9DCD */
void LevelPalettesLoad(void);               /* 81:86BF */
void PaletteCycleStart(uint8_t slot, uint8_t cycle);    /* 81:EC8E */
void PaletteCyclesStep(void);               /* 81:EC98: once a frame */

void Mode7Rotate(uint8_t angle);            /* 81:B1CD: 256 to a turn */

/* ---- the tile-block queue (video/tile_blocks.c): 16 x 16 blocks of map words ---- */

void TileBlockQueueRun(uint16_t to, uint16_t t);    /* 81:A23C */
void TileBlockQueueOne(uint16_t to, uint16_t t);    /* 81:A260 */
void TileBlockQueue(uint16_t to, uint16_t t);       /* 81:A287 */

/* ---- sprites (video/oam.c) ---- */

void OamResize(void);                      /* the view changed in play: the list sized for the new view, its contents kept */
void OamInit(void);                        /* at init, after view_init: the sprite list, sized for the view */
const nshvc_sprite *OamSprites(void);
const uint8_t *OamSpriteOwners(void);
const int16_t (*OamSpriteAnchors(void))[2];
const int32_t (*OamSpriteAnchorsFx(void))[2];   /* the same in 256ths of a pixel */  /* each sprite's actor's screen place as drawn (the picture's easing) */     /* each sprite's actor record, as far as g.sprite_count (the picture's easing) */      /* the frame's sprites, as far as g.sprite_count */
uint32_t OamSpriteCapacity(void);          /* how many the list holds */
void OamBufferClear(void);                  /* 81:95D5 */
void OamBuildBegin(void);                   /* 81:9230 */
void OamDrawActors(void);                   /* 81:9258 */

/* ---- the text layer (video/tilemap.c, screen_tasks.c) ---- */

void TextLayerFill(uint16_t entry);         /* 81:95B8 */
void TextLayerClear(void);                  /* 81:95C8 */
void VramFillMap(VideoMap map, uint16_t entry);   /* 81:95F5: the map's first 32 x 32 page (the record's $4000, $4800) */
void TextLayoutShow(int layout);        /* 81:972D: bit 7 blanks it */
void TextLayoutShowFirst(uint8_t layout);   /* 81:9718: and the text map's first part sent */
void TextLayoutShowSecond(uint8_t layout);  /* 81:971F */
void TextLayoutShowThird(uint8_t layout);   /* 81:9726 */
void TextLayoutDrawBig(uint8_t layout);     /* 81:979A */
void TextLayerClearFront(void);             /* 81:9607 */
void TextNumberTwoDigits(uint8_t number, uint16_t at);   /* 83:8BDE: at the text map's byte offset `at` */
void PaletteSaveReversed(uint8_t palette);  /* 81:9CA6 */
void PaletteFromRom(uint8_t rom, uint8_t palette);      /* 81:9941: palette 4 + `palette` */
void PaletteWhite(uint8_t palette);         /* 81:9CD7 */
void PaletteRestoreReversed(uint8_t palette);   /* 81:9CF1 */
struct Actor;
void ActorPaletteFromRom(const struct Actor *a, uint8_t rom);   /* 81:9903 */

/* ---- the scroll layers (video/scroll_layer.c) ---- */

void BgFollowLayers(void);                  /* 81:8568 */

void LayersFill(void);                      /* 81:C060 */

/* The task entries of video/ (the colour fades, the palette sequence, the
   brightness tasks, the Mode 7 zoom, the window effects) are in
   video/video_tasks.h, which needs system/task.h. */

/* ---- the sprite palette slots (video/tile_slots.c), for actor/ ---- */
int TileSlotClaim(uint8_t kind, Actor *a);  /* 82:ABCE: a free palette slot, tried from 4 down to 1, for `kind` and `a` (its slot, look; the kind's slot and look; the slot's user and kind); 0 claimed, -1 none */
void TileSlotFill(uint8_t kind);            /* 82:AB77: the kind's palette (32 bytes at its kind_palette * 32 in the actors' palettes) into its slot's place in g.palette; palette_upload up */

/* ---- the per-line tables (video/window_effects.c) ---- */
void HdmaTableFill(uint16_t at, uint16_t word, unsigned words);   /* 82:870D: `words` words of `word` into the per-line tables (g.line_data) from byte `at` (the Genie screen's) */

/* ---- the circle wipe (video/circle_wipe.c) ---- */
int CircleWipeStep(uint16_t radius, uint16_t x, uint16_t y);   /* 81:B264: the circle of that radius round the level point x, y into the window table (the window effect's table 4 shows it); 1 when drawn */
void ActorCircleWipeStep(Actor *a);         /* 83:EAAF: a's radius (+$45) one more and the circle round it (8 pixels up) drawn */

/* ---- an actor's own pictures (video/actor_pictures.c), for actor/ ---- */
void ActorPictureSend(Actor *a);            /* 81:949E: with flags bit 6 and its picture changed, its picture queued for video
                                               memory from its own list (a->own_pictures) */
void ActorPictureSendFromKind(Actor *a);    /* 81:9527: as ActorPictureSend, the parts' sources offsets in the actor's kind's unpacked graphics (kind $73's) */
void ActorPictureQueue(Actor *a);           /* 81:94A4: its picture, when it has changed, queued for its tile slot */

/* ---- for map/ (video/scroll_layer.c, level_blocks.c) and the camera (video/camera.c) ---- */
uint16_t TilemapWord(uint16_t x, uint16_t y);   /* 81:C177 (TilemapWordLong 81:C173): the VRAM word address of
                                                   BG1's 64 x 32 map word at pixel x, y */
void BlockSourceAdd(uint16_t source);           /* 81:C025: g.block_sources[g.block_sources_end++] = source,
                                                   the screen's block list (its number * $80) */

int16_t BlockAt(uint16_t x, uint16_t y);    /* 81:CB24: the screen map's entry for the block at x, y; negative empty */
void CameraStopAcross(void);                /* 81:CA84 */
void CameraStopUpDown(void);                /* 81:CADA */
void ActorYHistoryPush(const Actor *a);     /* 84:8EBD */
uint16_t RandomFromList(uint8_t list);      /* 81:AE8A: an entry (low byte) and the byte after it */
void ScrollUpdate(void);                    /* 81:C5A4: the play field's scroll for this frame, by g.scroll_kind */

/* wanted */
void LevelActorGraphicsLoad(void);          /* 81:B9F4: the level's actors' sprite graphics (LevelRamSetup) */
/* (levels and screens: defined in video/hud.c, for LevelLoad and the play loop) */
void HudRedrawAll(void);                    /* 81:F433 */
void HudRefresh(void);                      /* 81:F456: once a frame of play */

#endif
