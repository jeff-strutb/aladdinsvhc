/*
 * game_tables.h: the screens' tables from the cartridge (game/), read
 * through data/ (data.h). Owned by the stage 4 system session.
 */
#ifndef ALADDIN_GAME_TABLES_H
#define ALADDIN_GAME_TABLES_H

#include <stdint.h>

/* ---- the opening story (game/opening_story.c) ---- */

#define OPENING_SCENES 6
#define OPENING_ACTORS 22

typedef struct OpeningScene {
    uint8_t gfx_set;            /* its packed graphics set; 0 none of its own */
    uint8_t copy;               /* its entry of the second VRAM copy table (the record's X / 7) */
    uint8_t palette;            /* its palette set */
} OpeningScene;

typedef struct OpeningActor {
    uint8_t kind;               /* the actor's kind */
    uint8_t spawn_byte;         /* its $07 */
    uint16_t x, y;              /* its place, pixels */
} OpeningActor;

uint16_t opening_scene_frames(unsigned scene);  /* the frames scene 0-5 runs */
OpeningScene opening_scene(unsigned scene);     /* scene 0-5 */
OpeningActor opening_actor(unsigned entry);     /* entry 0-21 */

#endif
