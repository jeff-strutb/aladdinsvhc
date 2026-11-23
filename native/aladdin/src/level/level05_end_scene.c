/*
 * level05_end_scene.c: level 5's closing scene, a task its screen set-up
 * starts: once Aladdin gets far enough, the camera is taken over, actors
 * play the scene out on cues, part of the map is rebuilt, the screen
 * reddens and the level ends.
 */
#include "actor/actor.h"
#include "audio/sound.h"
#include "level/level.h"
#include "map/map.h"
#include "player/player.h"
#include "system/state.h"
#include "system/task.h"

#define ENTRY_X 0x0E20          /* where he must be, with the view's y under 256 */
#define SCENE_PLAYER_BYTE 0x0A
#define FADE_IN_SPEED 2
#define CAMERA_Y 0x00B8         /* the view's y the camera is brought to */
#define CAMERA_X 0x0E00         /* the view's x it goes on to */
#define SCROLL_KIND_SCENE 4
#define SONG_FIRST 0x30
#define SONG_SECOND 0x2F
#define MAP_COPY_COLUMNS 8      /* an 8 by 7 block of 16-pixel tiles, from (FROM_X, FROM_Y) to (TO_X, TO_Y) */
#define MAP_COPY_ROWS 7
#define FROM_X 0x0A10
#define FROM_Y 0x0240
#define TO_X 0x0D60
#define TO_Y 0x01E0
#define TILE 0x10
#define RED_FIRST 3             /* the fixed colour's red, counted up every 3 frames to 31 */
#define RED_FRAMES 3
#define EFFECT_SLOT_A 1         /* the two window effects the second song starts */
#define EFFECT_SLOT_B 6
#define EFFECT_A 0x10
#define EFFECT_B 0x16

enum {
    E_START, E_WAIT_ENTRY, E_CUE_C3, E_CAMERA_DOWN, E_CAMERA_ACROSS, E_CUE_BF, E_SECOND_SONG, E_CUE_BD,
    E_AFTER_MAP, E_WINDOWS_ON, E_WINDOWS_OFF, E_QUIET, E_CLOSE, E_WAIT_RED, E_RED
};

static uint16_t view_x(void) { return (uint16_t)(g.layers[0].scroll_x >> 8); }
static uint16_t view_y(void) { return (uint16_t)(g.layers[0].scroll_y >> 8); }

/* the colour maths and the windows for the second song: the layers the
   windows and the sum use, layer windows on for BG1-3 and the sprites,
   their logic, and the edges 0 and $FF */
static void second_song_screen(void)
{
    static const uint8_t logic[6] = { 1, 1, 1, 0, 1, 0 };
    static const uint8_t use[6] = { 0xF, 0xF, 0xF, 0, 0xF, 0 };

    g.window_main = 0x17;
    g.window_sub = 0x17;
    g.blend_layers = 0x17;
    g.blend_half = 0;
    g.blend_subtract = 0;
    g.blend_direct_colour = 0;
    g.blend_use_sub = 0;
    g.blend_prevent = 2;
    g.blend_clip = 0;
    for (int i = 0; i < 6; i++) {
        g.window_use[i] = use[i];
        g.window_logic[i] = logic[i];
    }
    g.window_edges[0] = 0x00;
    g.window_edges[1] = 0xFF;
    g.window_edges[2] = 0x00;
    g.window_edges[3] = 0xFF;
}

/* the 8 by 7 block of the map copied by its tiles, row by row */
static void map_rebuild(void)
{
    for (unsigned row = 0; row < MAP_COPY_ROWS; row++)
        for (unsigned column = 0; column < MAP_COPY_COLUMNS; column++)
            MapTileSet(MapTileAt((uint16_t)(FROM_X + TILE * column), (uint16_t)(FROM_Y + TILE * row)),
                       (uint16_t)(TO_X + TILE * column), (uint16_t)(TO_Y + TILE * row));
}

/*
 * @routine 80:A2A8 Level05EndScene
 * WHAT IT DOES: Level 5's closing scene. Once Aladdin, alive, is at x
 * $0E20 or more with the view's y under 256, he is given his start code
 * (his $07 $0A), and on the cue $C3 every actor goes and an actor of kind
 * $2E comes; the camera is brought down to y $B8 (or on the cue $C4) and
 * on to x $0E00, song $30; on the cue $BF song $2F and the windows and
 * colour maths set, two window effects started; on the cue $BD three
 * actors placed (kinds $2E, $70, $93) and an 8 by 7 block of the map's
 * tiles copied from ($0A10, $0240) to ($0D60, $01E0); then, after pauses,
 * the screen goes red over 29 steps and the level ends: the player's
 * stage-end code, the exit and the stages ended counted up.
 */
TaskRun Level05EndScene(Task *t)
{
    uint16_t *at = task_enter(t);
    Actor *player = &g.actors[0];

    switch (*at) {
    case E_START:
        g.scene_cue_first = g.scene_cue = g.scene_cue_bf = g.scene_cue_2 = g.scene_cue_close = 0;
        g.scene_cue_waited = g.scene_cue_c3 = g.scene_cue_c4 = 0;
        *at = E_WAIT_ENTRY;
        return task_wait(t, 1);
    case E_WAIT_ENTRY:
        if ((g.layers[0].scroll_y >> 16) & 0xFF || (uint16_t)(player->x >> 8) < ENTRY_X || !g.health)
            return task_wait(t, 1);
        g.play_starts++;
        player->resume = PLAYER_AT_START;
        player->spawn_byte = SCENE_PLAYER_BYTE;
        *at = E_CUE_C3;
        return task_wait(t, 1);
    case E_CUE_C3:
        if (!g.scene_cue_c3)
            return task_wait(t, 1);
        g.camera_window = 0;
        SoundQueueF6(FADE_IN_SPEED);
        ActorsClearForScene();
        (void)ActorSpawnAt(0x2E, 2, 0x0E80, 0x0148);
        *at = E_CAMERA_DOWN;
        return task_wait(t, 1);
    case E_CAMERA_DOWN:
        if (view_y() != CAMERA_Y && !g.scene_cue_c4)
            return task_wait(t, 1);
        g.layers[0].scroll_y = (g.layers[0].scroll_y & 0xFFu) | (uint32_t)CAMERA_Y << 8;
        g.scroll_kind = SCROLL_KIND_SCENE;
        g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | (uint8_t)(g.scroll_x_held + 1));
        g.scroll_y_held = (uint16_t)((g.scroll_y_held & 0xFF00) | (uint8_t)(g.scroll_y_held + 1));
        *at = E_CAMERA_ACROSS;
        return task_wait(t, 1);
    case E_CAMERA_ACROSS:
        if (view_x() < CAMERA_X)
            return task_wait(t, 1);
        SongPlay(SONG_FIRST);
        *at = E_CUE_BF;
        return task_wait(t, 1);
    case E_CUE_BF:
        if (!g.scene_cue_bf)
            return task_wait(t, 1);
        g.scene_cue_close++;
        SongPlay(SONG_SECOND);
        *at = E_SECOND_SONG;
        return task_wait(t, 0x10);
    case E_SECOND_SONG:
        second_song_screen();
        TaskCreate(EFFECT_SLOT_A, EFFECT_A, TASK_ENTRY_WINDOW_EFFECT);
        TaskCreate(EFFECT_SLOT_B, EFFECT_B, TASK_ENTRY_WINDOW_EFFECT);
        *at = E_CUE_BD;
        return task_wait(t, 1);
    case E_CUE_BD:
        if (!g.scene_cue_first)
            return task_wait(t, 1);
        (void)ActorSpawnAt(0x2E, 3, 0x0E80, 0x0118);
        (void)ActorSpawnAt(0x70, 1, 0x0DD0, 0x0228);
        (void)ActorSpawnAt(0x93, 0, 0x0DB0, 0x0220);
        map_rebuild();
        *at = E_AFTER_MAP;
        return task_wait(t, 0x80);
    case E_AFTER_MAP:
        g.main_layers = 0x13;
        g.sub_layers = 0x13;
        g.scroll_kind = 8;
        *at = E_WINDOWS_OFF;
        return task_wait(t, 0x1C);
    case E_WINDOWS_OFF:
        g.scroll_kind = 0;
        *at = E_QUIET;
        return task_wait(t, 0xB4);
    case E_QUIET:
        g.scene_cue_2++;
        TaskSlotFree(1);
        *at = E_CLOSE;
        return task_wait(t, 0x64);
    case E_CLOSE:
        g.blend_layers = 0x23;
        g.blend_half = 0;
        g.blend_subtract = 0;
        g.blend_direct_colour = 0;
        g.blend_use_sub = 0;
        g.blend_prevent = 0;
        g.blend_clip = 0;
        g.fixed_green = 0;
        g.fixed_blue = 0;
        g.fixed_red = RED_FIRST;
        *at = E_RED;
        return task_wait(t, RED_FRAMES);
    case E_RED:
        g.fixed_red = (uint8_t)((g.fixed_red + 1) & 0x1F);
        if (g.fixed_red)
            return task_wait(t, RED_FRAMES);
        g.level_restart = 0;
        g.progress_stage = 0;
        g.progress_sequence++;
        player->facing = 0;
        player->flip = 0;
        player->resume = PLAYER_AT_STAGE_END;
        g.player_flags |= 0x80;
        g.level_exiting++;
        g.stages_ended++;
        break;
    }
    return task_exit(t);
}
