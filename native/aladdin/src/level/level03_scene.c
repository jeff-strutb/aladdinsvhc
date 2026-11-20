/*
 * level03_scene.c: level 3's task, which its screen set-up starts: it waits
 * for Aladdin at a spot, takes him over and moves the camera's limits
 * through the scene, as level $0D's and $10's do.
 *
 * Its copier check and checksum, with their proof: the check (a count in
 * cartridge RAM, written and read back) never finds RAM on the retail
 * board, so the scene's start always goes on; the checksum (the task's own
 * code bytes, summed, less $EC) is kept at $00C0 and read by nothing, and
 * the code it sums is not in the cartridge image: not converted.
 */
#include "actor/actor.h"
#include "actor/guard.h"
#include "audio/sound.h"
#include "level/level.h"
#include "player/player.h"
#include "system/state.h"

#define START_SCREEN 4          /* the view's screen across from which the scene waits for him */
#define START_Y_LOW 0xB0        /* where he stands (low bytes): y exactly, x at least */
#define START_X_LOW 0xF0
#define LIMIT_FIRST 0x0400      /* the view's x limit (g.scroll_x_max) as the scene starts */
#define SCENE_PLAYER_BYTE 8
#define SCENE_ESCORT_BYTE 4
#define SCROLL_KIND_SCENE 4
#define PAST_SCREEN 5
#define SONG_AFTER 0x21
#define SONG_END 0x1E
#define FINAL_LIMIT_HIGH 0x7F
#define FINAL_PLAYER_BYTE 0x0D
#define FADE_IN_SPEED 2
#define ESCORT 1                /* its record: actors[1] */

enum { S_START, S_WAIT_SPOT, S_WAIT_SCREEN, S_SONG, S_CUE_1, S_PAUSE, S_FINAL, S_CUE_2 };

static void limit_x_max_high(uint8_t high)
{
    g.scroll_x_max = (uint16_t)((g.scroll_x_max & 0x00FF) | (uint16_t)high << 8);
}

static uint8_t screen_across(void) { return (uint8_t)(g.layers[0].scroll_x >> 16); }

/*
 * @routine 80:A12D Level03Scene
 * WHAT IT DOES: Level 3's scene. The view's x limit $0400, the cues
 * cleared; each frame until the view is at screen 4 or past and Aladdin,
 * alive, stands at y $B0 with x $F0 or more (low bytes): sound command $F6
 * with speed 2; every actor freed, his and the escort's code set, his $07
 * 8, the escort's 4, the limit $0500 and the scroll kind 4. Each frame
 * until the view is at screen 5; the view's x least $0500; 48 frames; song
 * $21. Each frame until the first cue; sound $F6 2; 128 frames; each frame
 * until x $F0 or more, he is not held and y $B0 or more: the play-starts
 * mark up, the limit's high byte $7F, his code again, his $07 $0D; each
 * frame until the second cue; song $1E; the task ends.
 */
TaskRun Level03Scene(Task *t)
{
    uint16_t *at = task_enter(t);
    Actor *player = &g.actors[0];

    switch (*at) {
    case S_START:
        g.scroll_x_max = LIMIT_FIRST;
        g.scene_cue_first = 0;
        g.scene_cue = 0;
        g.scene_cue_bf = 0;
        *at = S_WAIT_SPOT;
        return task_wait(t, 1);
    case S_WAIT_SPOT:
        if (screen_across() < START_SCREEN || (uint8_t)(player->y >> 8) != START_Y_LOW
            || (uint8_t)(player->x >> 8) < START_X_LOW || !g.health)
            return task_wait(t, 1);
        SoundQueueF6(FADE_IN_SPEED);
        ActorsFreeAll();
        player->resume = PLAYER_AT_START;
        g.actors[ESCORT].resume = G_RESTART;
        player->spawn_byte = SCENE_PLAYER_BYTE;
        g.actors[ESCORT].spawn_byte = SCENE_ESCORT_BYTE;
        limit_x_max_high(PAST_SCREEN);
        g.scroll_kind = SCROLL_KIND_SCENE;
        *at = S_WAIT_SCREEN;
        return task_wait(t, 1);
    case S_WAIT_SCREEN:
        if (screen_across() < PAST_SCREEN)
            return task_wait(t, 1);
        g.scroll_x_min = (uint16_t)((g.scroll_x_min & 0x00FF) | (uint16_t)PAST_SCREEN << 8);
        *at = S_SONG;
        return task_wait(t, 0x30);
    case S_SONG:
        SongPlay(SONG_AFTER);
        *at = S_CUE_1;
        return task_wait(t, 1);
    case S_CUE_1:
        if (!g.scene_cue_first)
            return task_wait(t, 1);
        SoundQueueF6(FADE_IN_SPEED);
        *at = S_PAUSE;
        return task_wait(t, 0x80);
    case S_PAUSE:
        *at = S_FINAL;
        return task_wait(t, 1);
    case S_FINAL:
        if ((uint8_t)(player->x >> 8) < START_X_LOW || g.player_in_air || (uint8_t)(player->y >> 8) < START_Y_LOW)
            return task_wait(t, 1);
        g.play_starts++;
        limit_x_max_high(FINAL_LIMIT_HIGH);
        player->resume = PLAYER_AT_START;
        player->spawn_byte = FINAL_PLAYER_BYTE;
        *at = S_CUE_2;
        return task_wait(t, 1);
    case S_CUE_2:
        if (!g.scene_cue)
            return task_wait(t, 1);
        SongPlay(SONG_END);
        break;
    }
    return task_exit(t);
}
