/*
 * camera_trigger.c: a level's task (made by level 4's and $16's screen
 * set-up) that waits for the camera to reach two places: at the first it
 * frees the actors, at the second (the player alive) it sets the player
 * and the escort going again and starts an actor of kind $6B.
 */
#include "actor/actor.h"
#include "actor/guard.h"
#include "audio/sound.h"
#include "level/level.h"
#include "player/player.h"
#include "system/state.h"

#define ESCORT 1                /* its record: actors[1] */
#define TRIGGER_FIRST_X 0x0B    /* the view's screen across and down where the actors go */
#define TRIGGER_FIRST_Y 0x02
#define TRIGGER_SECOND_X 0x0F   /* the screen across where the scene starts */
#define SCENE_PLAYER_BYTE 9
#define SCENE_ESCORT_BYTE 5
#define NEW_KIND 0x6B
#define NEW_KIND_X 0x0F38
#define NEW_KIND_Y 0x03B0
#define FADE_IN_SPEED 2

enum { TRIGGER_START, TRIGGER_FIRST, TRIGGER_SECOND };

/* the view's screen across and down: the high bytes of its pixel x and y */
static uint8_t screen_across(void) { return (uint8_t)(g.layers[0].scroll_x >> 16); }
static uint8_t screen_down(void) { return (uint8_t)(g.layers[0].scroll_y >> 16); }

/*
 * @routine 80:A22D CameraTriggerTask
 * WHAT IT DOES: Waits for the camera to reach screen $0B across and 2 down
 * and frees every actor; then waits for screen $0F across with the player
 * alive, gives the escort and the player new code, starts an actor of kind
 * $6B at (3896, 944) and queues sound command $F6 with speed 2, and ends.
 * HOW: The scene's three cues cleared; a frame at a time until each place;
 * at the second the play-starts mark up (no pause), the escort sent to
 * GuardRestart with its $07 5, the camera held across (scroll_x_held up),
 * every actor freed (the player and the escort kept), the player started
 * again with $07 9.
 */
TaskRun CameraTriggerTask(Task *t)
{
    uint16_t *at = task_enter(t);

    switch (*at) {
    case TRIGGER_START:
        g.scene_cue_first = 0;
        g.scene_cue = 0;
        g.scene_cue_bf = 0;
        *at = TRIGGER_FIRST;
        return task_wait(t, 1);
    case TRIGGER_FIRST:
        if (screen_across() != TRIGGER_FIRST_X || screen_down() != TRIGGER_FIRST_Y)
            return task_wait(t, 1);
        ActorsFreeAll();
        *at = TRIGGER_SECOND;
        return task_wait(t, 1);
    case TRIGGER_SECOND:
        if (screen_across() != TRIGGER_SECOND_X || !g.health)
            return task_wait(t, 1);
        g.play_starts++;
        g.actors[ESCORT].resume = G_RESTART;
        g.actors[ESCORT].spawn_byte = SCENE_ESCORT_BYTE;
        g.scroll_x_held = (uint16_t)((g.scroll_x_held & 0xFF00) | (uint8_t)(g.scroll_x_held + 1));
        ActorsFreeAll();
        g.actors[0].spawn_byte = SCENE_PLAYER_BYTE;
        g.actors[0].resume = PLAYER_AT_START;
        (void)ActorSpawnAt(NEW_KIND, 1, NEW_KIND_X, NEW_KIND_Y);
        SoundQueueF6(FADE_IN_SPEED);
        break;
    }
    return task_exit(t);
}
