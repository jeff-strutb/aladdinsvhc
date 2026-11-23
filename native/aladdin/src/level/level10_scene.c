/*
 * level10_scene.c: level $10's task, which its screen set-up starts: it
 * waits for Aladdin to reach a spot, then takes him over and moves the
 * camera's limits on through a short scene, as level $0D's does.
 */
#include "actor/actor.h"
#include "audio/sound.h"
#include "level/level.h"
#include "player/player.h"
#include "system/state.h"

#define LIMIT_FIRST 0x10A0      /* the view's x limit (g.scroll_x_max) as the scene starts */
#define SPOT_X 0x1190           /* where he must be: x at least, y's low byte at least */
#define SPOT_Y_LOW 0xA8
#define FADE_IN_SPEED 0x10
#define SCENE_PLAYER_BYTE 8
#define LIMIT_SCENE 0x11A0
#define SCROLL_KIND_SCENE 4
#define PAUSE_BEFORE_SONG 0x40
#define PAUSE_AFTER_SOUND 0x20
#define SONG 0x21
#define LIMIT_OPEN_HIGH 0x7F

enum { S_START, S_WAIT_SPOT, S_SONG, S_CUE, S_END };

/*
 * @routine 80:A5AC Level10Scene
 * WHAT IT DOES: Level $10's scene. Each frame until Aladdin is at x $1190
 * or more with his y's low byte $A8 or more; then sound command $F6 with
 * speed $10 is queued, every actor freed, he is given his start code, the
 * camera's limits move on and song $21 plays; on the cue the task queues
 * $F6 $10 again, waits 32 frames, sets the limits for the scene's end and
 * ends.
 */
TaskRun Level10Scene(Task *t)
{
    uint16_t *at = task_enter(t);
    Actor *player = &g.actors[0];

    switch (*at) {
    case S_START:
        g.scroll_x_max = LIMIT_FIRST;
        g.scene_cue_first = 0;
        *at = S_WAIT_SPOT;
        return task_wait(t, 1);
    case S_WAIT_SPOT:
        if ((uint16_t)(player->x >> 8) < SPOT_X || (uint8_t)(player->y >> 8) < SPOT_Y_LOW)
            return task_wait(t, 1);
        SoundQueueF6(FADE_IN_SPEED);
        ActorsClearForScene();
        player->resume = PLAYER_AT_START;
        player->spawn_byte = SCENE_PLAYER_BYTE;
        g.scroll_x_max = LIMIT_SCENE;
        g.scroll_kind = SCROLL_KIND_SCENE;
        g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | 1);
        *at = S_SONG;
        return task_wait(t, PAUSE_BEFORE_SONG);
    case S_SONG:
        SongPlay(SONG);
        *at = S_CUE;
        return task_wait(t, 1);
    case S_CUE:
        if (!g.scene_cue_first)
            return task_wait(t, 1);
        SoundQueueF6(FADE_IN_SPEED);
        *at = S_END;
        return task_wait(t, PAUSE_AFTER_SOUND);
    case S_END:
        g.scroll_x_min = LIMIT_SCENE;
        g.scroll_x_max = (uint16_t)((g.scroll_x_max & 0x00FF) | (uint16_t)LIMIT_OPEN_HIGH << 8);
        g.scroll_x_held &= 0xFF00;
        g.scroll_kind = 0;
        break;
    }
    return task_exit(t);
}
