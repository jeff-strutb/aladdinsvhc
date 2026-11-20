/*
 * level0d_scene.c: level $0D's task, which its screen set-up starts: it
 * keeps Aladdin from rising too high until he reaches a spot, then takes
 * him over and moves the camera's limits on through a short scene.
 */
#include "audio/sound.h"
#include "level/level.h"
#include "player/player.h"
#include "system/state.h"

#define LIMIT_FIRST 0x0900      /* the view's x limit (g.scroll_x_max) as the scene starts */
#define TOP_STOP 0x18           /* the least y (low byte tested, then set whole) */
#define SPOT_X 0x09F0
#define SPOT_Y 0x0078
#define SCENE_PLAYER_BYTE 5
#define LIMIT_NEXT 0x0A00       /* the limit once he is at the spot; the view's x least once it gets there */
#define SCROLL_KIND_SCENE 4
#define SONG 0x25
#define PAUSE_BEFORE_SOUND 0xF0
#define PAUSE_AFTER_SOUND 0xC0
#define LIMIT_OPEN 0x7FFF
#define FADE_IN_SPEED 2

enum { S_START, S_KEPT, S_MOVE, S_SONG, S_CUE, S_SOUND, S_OPEN };

/*
 * @routine 80:A501 Level0DScene
 * WHAT IT DOES: Level $0D's scene. Each frame Aladdin is kept from going
 * above y $18 (when his y's low byte is $18 or less his y is set to $0018)
 * until he, alive, is at x $09F0 or more and y $78 or more; then he is
 * given his start code, sound command $F6 with speed 2 is queued, the
 * camera moves on until the view reaches screen $0A across, song $25
 * plays, and on the first cue the task waits 240 frames, queues $F6 2
 * again, waits 192 frames, opens the limit ($7FFF) and ends.
 */
TaskRun Level0DScene(Task *t)
{
    uint16_t *at = task_enter(t);
    Actor *player = &g.actors[0];
    uint16_t x, y;

    switch (*at) {
    case S_START:
        g.scroll_x_max = LIMIT_FIRST;
        g.scene_cue_first = 0;
        g.scene_cue = 0;
        *at = S_KEPT;
        return task_wait(t, 1);
    case S_KEPT:
        if ((uint8_t)(player->y >> 8) <= TOP_STOP)
            player->y = (player->y & 0xFFu) | (uint32_t)TOP_STOP << 8;
        x = (uint16_t)(player->x >> 8);
        y = (uint16_t)(player->y >> 8);
        if (x < SPOT_X || y < SPOT_Y || !g.health)
            return task_wait(t, 1);
        player->resume = PLAYER_AT_START;
        player->spawn_byte = SCENE_PLAYER_BYTE;
        SoundQueueF6(FADE_IN_SPEED);
        g.scroll_x_max = (uint16_t)((g.scroll_x_max & 0x00FF) | LIMIT_NEXT);
        g.scroll_kind = SCROLL_KIND_SCENE;
        *at = S_MOVE;
        return task_wait(t, 1);
    case S_MOVE:
        if ((uint8_t)(g.layers[0].scroll_x >> 16) < (LIMIT_NEXT >> 8))
            return task_wait(t, 1);
        g.scene_cue++;
        g.scroll_kind = 0;
        g.scroll_x_min = LIMIT_NEXT;
        *at = S_SONG;
        return task_wait(t, 1);
    case S_SONG:
        SongPlay(SONG);
        *at = S_CUE;
        return task_wait(t, 1);
    case S_CUE:
        if (!g.scene_cue_first)
            return task_wait(t, 1);
        *at = S_SOUND;
        return task_wait(t, PAUSE_BEFORE_SOUND);
    case S_SOUND:
        SoundQueueF6(FADE_IN_SPEED);
        *at = S_OPEN;
        return task_wait(t, PAUSE_AFTER_SOUND);
    case S_OPEN:
        g.scroll_x_max = LIMIT_OPEN;
        break;
    }
    return task_exit(t);
}
