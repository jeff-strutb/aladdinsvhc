/*
 * level_tables.c: where a level's play starts, its companion and song, its
 * restart point and its exit.
 */
#include "audio/sound.h"
#include "data/level_tables.h"
#include "data/system_tables.h"
#include "player/player.h"
#include "video/video.h"
#include "level/level.h"
#include "system/state.h"
#include "system/system.h"

#define VIEW_HALF 0x0080        /* the range point: the view's corner + this, in x and y */

/* 16.8 positions: the whole pixels set, the fraction kept */
static uint32_t whole_set(uint32_t at, uint16_t pixels)
{
    return (at & 0xFFu) | (uint32_t)pixels << 8;
}

/*
 * @routine 81:84B2 LevelStartPlace
 * WHAT IT DOES: Puts the player and the camera at the level's start (its
 * restart point): the player's place and facing, both layers' scroll, the
 * point actors are kept in range of, the scroll's limits across.
 * HOW: The play field's scroll, its position before the move and its drawn
 * edges take the camera's place; the backdrop takes it halved, in x unless
 * on level $10 or with a layer set-up mode, in y unless on levels 0, 3,
 * $0D, $14 or $1A. The range point is the scroll + $80 each way; the
 * scroll x between 0 and $7F00. (The original also clears the first byte
 * of the play field's layer record; that byte has no field yet: video/'s
 * ScrollLayer does not name bytes $00-$27.)
 */
void LevelStartPlace(void)
{
    LevelStart s = level_start((uint8_t)g.level, g.level_restart);
    Actor *player = &g.actors[0];
    ScrollLayer *field = &g.layers[0], *backdrop = &g.layers[1];
    uint16_t bx, by;

    DevStartAt(&s);
    bx = s.camera_x;
    by = s.camera_y;

    player->x = whole_set(player->x, s.x);
    player->facing = s.facing_left;
    player->flip = s.facing_left;
    player->y = whole_set(player->y, s.y);
    field->scroll_x = whole_set(field->scroll_x, s.camera_x);
    g.scroll_x_before = s.camera_x;
    field->drawn_x = s.camera_x;
    if (g.level != 0x10 && !g.layer_setup_mode)
        bx >>= 1;
    backdrop->scroll_x = whole_set(backdrop->scroll_x, bx);
    backdrop->drawn_x = bx;
    field->scroll_y = whole_set(field->scroll_y, s.camera_y);
    g.scroll_y_before = s.camera_y;
    field->drawn_y = s.camera_y;
    if (g.level != 0x00 && g.level != 0x03 && g.level != 0x0D && g.level != 0x14 && g.level != 0x1A)
        by >>= 1;
    backdrop->scroll_y = whole_set(backdrop->scroll_y, by);
    backdrop->drawn_y = by;
    g.range_x = (uint16_t)(s.camera_x + VIEW_HALF);
    g.range_y = (uint16_t)(s.camera_y + VIEW_HALF);
    g.scroll_x_min = 0;
    g.scroll_x_max = 0x7F00;
}

#define COMPANION 2             /* its record: actors[2] ($097B) */
#define COMPANION_KIND 0x72
#define COMPANION_LOOK 0x0240
#define COMPANION_PALETTE 0x3A
#define LEVEL_TWO_COMPANION_PALETTES 0x12
#define COMPANION_PALETTE_2 0x48
#define ACTOR_NEW 0x0C          /* a record's state: to start */
#define SONG_NONE 0x80
#define SPOT_REACH 8            /* the player within this of a spot */
#define LEADS_FACING_LEFT 0x80
#define EXITING 0x80            /* g.player_flags: his exit walk */

static uint16_t view_x(void)
{
    return (uint16_t)(g.layers[0].scroll_x >> 8);
}

/*
 * @routine 81:8623 LevelCompanionStart
 * WHAT IT DOES: On a level with a companion actor, starts it at the
 * player's place, and loads its palettes.
 * HOW: The level's byte, less one, is the companion's $07; record 2 made
 * new, kind $72, look $0240, the player's place (pixels); palette $3A, and
 * on level $12 $48 too.
 */
void LevelCompanionStart(void)
{
    uint8_t c = level_companion((uint8_t)g.level);
    Actor *a = &g.actors[COMPANION], *player = &g.actors[0];

    if (!c)
        return;
    a->spawn_byte = (uint8_t)(c - 1);
    a->state = ACTOR_NEW;
    a->kind = COMPANION_KIND;
    a->look = COMPANION_LOOK;
    a->x = whole_set(a->x, (uint16_t)(player->x >> 8));
    a->y = whole_set(a->y, (uint16_t)(player->y >> 8));
    PaletteLoad(COMPANION_PALETTE);
    if (g.level == LEVEL_TWO_COMPANION_PALETTES)
        PaletteLoad(COMPANION_PALETTE_2);
}

/*
 * @routine 81:86DE LevelSongPlay
 * WHAT IT DOES: Plays the level's song, or, when its entry has none, sends
 * sound command $F0 instead.
 */
void LevelSongPlay(void)
{
    uint8_t song = level_song((uint8_t)g.level);

    if (song & SONG_NONE)
        SoundQueueF0();
    else
        SongPlay(song);
}

/*
 * @routine 81:86F0 RestartPointCheck
 * WHAT IT DOES: Sets the level's restart point once the view has passed
 * it: from there a lost life starts the level at its second start.
 * HOW: With no restart point yet, and neither the exit reached nor a fall
 * out of the map ($1423-$1424 as one word), the level's restart x (bit 15:
 * none) below the view's x sets the restart point to 1.
 */
void RestartPointCheck(void)
{
    uint16_t x;

    if (g.level_restart || g.level_exiting || g.player_fallen_out)
        return;
    x = level_restart_x((uint8_t)g.level);
    if ((x & 0x8000) || x >= view_x())
        return;
    g.level_restart = 1;
}

/*
 * @routine 81:8716 LevelSpotsSet
 * WHAT IT DOES: Finds the level's exit spots: how many, and their list.
 */
void LevelSpotsSet(void)
{
    g.level_spot_count = level_spot_count(g.level);
    g.level_spots = level_spot_list(g.level);
}

/* within SPOT_REACH of `at`, either way (the original's unsigned
   at - v + 8 < 16) */
static bool near(uint16_t at, uint16_t v)
{
    return (uint16_t)(at - v + SPOT_REACH) < 2 * SPOT_REACH;
}

/*
 * @routine 81:872E LevelExitCheck
 * WHAT IT DOES: Ends the level when the player reaches its exit spot (on
 * some levels, when the view passes it): sets the next stage and starts
 * the player's exit walk.
 * HOW: Not while he is held or hurt, the exit is reached, or there are no
 * spots. Levels that exit by the view: the first spot's x below the view's
 * x. Others: each spot's x in turn within 8 of the player's, and then the
 * FIRST spot's y within 8 of his (as the original reads it), one test a
 * spot. Reached: the
 * first spot's byte gives the stage (0: the progress sequence moves on);
 * his code to the escort's exit (levels by the view) or the spot's exit,
 * his facing and flip from the byte's bit 7 (both the same byte, as the
 * original writes them); his exit flag, the exit and the stages ended
 * counted up; the restart point cleared.
 */
void LevelExitCheck(void)
{
    Actor *player = &g.actors[0];
    DataRef spots = g.level_spots;
    bool by_view = level_exit_by_view((uint8_t)g.level);
    uint8_t to, left;
    unsigned n;

    if (g.player_in_air | g.level_exiting | g.player_hurt_count)
        return;
    if (!g.level_spot_count)
        return;
    if (by_view) {
        if (spot_x(spots, 0) >= view_x())
            return;
    } else {
        /* as many tests as spots; an x match whose y fails sends the walk
           back to the first spot (the original's list pointer reset for the
           y test), so the next tested is spot 1 */
        for (n = 0, left = g.level_spot_count;; n++) {
            if (near(spot_x(spots, n), (uint16_t)(player->x >> 8))) {
                if (near(spot_y(spots, 0), (uint16_t)(player->y >> 8)))
                    break;
                n = 0;
            }
            if (--left == 0)
                return;
        }
    }
    to = spot_leads_to(spots, 0);
    g.progress_stage = (uint8_t)(to & 0x7F);
    if (!(to & 0x7F))
        g.progress_sequence = progress_next_sequence(g.progress_sequence);
    player->resume = by_view ? PLAYER_AT_ESCORT_EXIT : PLAYER_AT_SPOT_EXIT;
    g.player_flags |= EXITING;
    player->flip = player->facing = (uint8_t)(to >> 7);
    g.level_exiting++;
    g.stages_ended++;
    g.level_restart = 0;
}

/*
 * @routine 81:878B Level035CSet
 * WHAT IT DOES: Sets the progress sequence and stage from the level's own
 * entry.
 */
void Level035CSet(void)
{
    g.progress_sequence = level_progress_sequence((uint8_t)g.level);
    g.progress_stage = level_progress_stage((uint8_t)g.level);
}
