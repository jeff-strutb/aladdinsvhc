/*
 * level_tables.h: the tables of a game's and a level's start, play and
 * exit, read through data/ (game/progress.c, play_loop.c, level_load.c,
 * level/level_tables.c, level/view_edges.c). Owned by the levels-and-
 * screens session.
 */
#ifndef ALADDIN_LEVEL_TABLES_H
#define ALADDIN_LEVEL_TABLES_H

#include <stdbool.h>
#include <stdint.h>

#include "data/data.h"

/* ---- the demos (DemoStart): by demo 0-1 ---- */
uint16_t demo_input_start(uint8_t demo);    /* its pad script's start, as g.demo_input_at */
uint8_t demo_sequence(uint8_t demo);        /* its progress sequence */
uint8_t demo_stage(uint8_t demo);           /* its stage in it */
uint8_t demo_restart(uint8_t demo);         /* its restart point (g.level_restart) */

/* ---- the progress (LevelFromProgress): a sequence's first entry, and the
   level at an entry ---- */
uint8_t progress_first(uint8_t sequence);
uint8_t progress_level(uint8_t entry);

/* ---- a level's start (LevelLoad, LevelCompanionStart, LevelSongPlay) ---- */
uint8_t level_player_kind(uint8_t level);   /* the player's kind ($06) */
uint8_t level_player_byte(uint8_t level);   /* the player's $07 */
uint8_t level_companion(uint8_t level);     /* its companion's $07 plus 1; 0 none */
uint8_t level_song(uint8_t level);          /* its song; bit 7: none */
/* the buttons each action takes under a control option (the options'
   first kept byte): action 0-3, as g.control_masks */
uint16_t control_option_mask(uint8_t option, unsigned action);

/* ---- a level's play (RestartPointCheck, LevelExitCheck) ---- */
uint16_t level_restart_x(uint8_t level);    /* the view's x past which its restart point is set; bit 15: none */
bool level_exit_by_view(uint8_t level);     /* its exit is reached by the view passing the first spot's x */
uint8_t progress_next_sequence(uint8_t sequence);   /* the sequence after one whose stages are done */

/* its exit spots (LevelSpotsSet): how many, and their list */
uint8_t level_spot_count(uint16_t level);
DataRef level_spot_list(uint16_t level);
uint16_t spot_x(DataRef list, unsigned spot);
uint16_t spot_y(DataRef list, unsigned spot);
uint8_t spot_leads_to(DataRef list, unsigned spot);   /* bits 0-6 the next stage (0: the next sequence), bit 7 the player faces left */

/* ---- the view's edges (view_edges.c): the player's x from the view's
   left kept within: 16, 240; and the hurt edge, 0 ---- */
uint16_t view_edge_limit(unsigned which);   /* 0 the least, 1 the most, 4 the hurt edge (the record's word offsets / 2) */

/* ---- the level's switch flags (level/level_flags.c): a stage's mask of
   the eight, and each flag's bit ---- */
uint8_t level_flag_mask(uint8_t stage);
uint8_t flag_bit(unsigned flag);            /* 1 << flag, from the table */
/* the flagged spots of a level from $0E on: how many, and spot i's x, y */
uint8_t flag_spot_count(uint8_t level);
uint16_t flag_spot_x(uint8_t level, unsigned i);
uint16_t flag_spot_y(uint8_t level, unsigned i);

/* ---- a level's own progress entry (Level035CSet) ---- */
uint8_t level_progress_sequence(uint8_t level);
uint8_t level_progress_stage(uint8_t level);

#endif
