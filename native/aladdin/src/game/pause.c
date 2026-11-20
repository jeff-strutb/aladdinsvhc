/*
 * pause.c: the pause: Start during play freezes every task but the one
 * that runs it, shows PAUSED and pauses the music, until Start again.
 */
#include "audio/sound.h"
#include "game/game.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video.h"

#define START_HIGH 0x10             /* Start, in a pad word's high byte */
#define FULL_BRIGHTNESS 0x0F
#define TEXT_PAUSED 0x16            /* text layouts: PAUSED; with bit 7, blanked */
#define LEVEL_NO_PAUSE_SOUND 0x12   /* the level that pauses without sound $0E */
#define SOUND_PAUSE 0x0E
#define SOUND_MUSIC_PAUSE 0xF3
#define SOUND_MUSIC_GO_ON 0xF4
#define TASK_FROZEN 0x02            /* a state the scheduler passes over (below ready) */
#define PAUSE_TEXT_BYTES 0x0380     /* the text upload while PAUSED shows */
#define PLAY_TEXT_BYTES 0x0160
#define COOLDOWN 0x10

/* slots 1-7's states while paused (the original kept them on its stack) */
static uint8_t frozen[TASK_SLOTS];

enum { PAUSE_TEST, PAUSE_HELD, PAUSE_LAST };

/*
 * @routine 81:8398 PauseCheck
 * WHAT IT DOES: Pauses the game when Start is pressed during play (full
 * brightness, no demo, nothing blocking, the level's start text done):
 * every other task frozen, PAUSED shown and the music paused, until Start
 * again; then a short cooldown.
 * HOW: The cooldown nonzero, the brightness not full, pause_block, the play
 * mode or $03FA nonzero, or Start not newly pressed: the cooldown counted
 * down. Else: the text upload $0380 bytes, paused counted up, PAUSED shown,
 * sound $0E (not on level $12) and $F3; slots 7 down to 1 kept and set to
 * 2; each frame until Start: PAUSED blanked, paused cleared, the cooldown
 * $10, sound $F4, the states back, one frame more, the text upload $0160.
 * (The original also sets $03F9 to 1 while paused; nothing reads it.)
 */
TaskRun PauseCheck(Task *t)
{
    uint16_t *at = task_enter(t);
    unsigned slot;

    switch (*at) {
    case PAUSE_TEST:
        if (g.pause_cooldown || (g.brightness & 0x0F) != FULL_BRIGHTNESS
            || (g.pause_block | g.play_mode | g.play_starts) || !((g.pad1_pressed >> 8) & START_HIGH)) {
            if (g.pause_cooldown)
                g.pause_cooldown--;
            break;
        }
        g.text_upload_bytes = PAUSE_TEXT_BYTES;
        g.paused++;
        TextLayoutShow(TEXT_PAUSED);
        if (g.level != LEVEL_NO_PAUSE_SOUND)
            SoundQueueAdd(SOUND_PAUSE);
        SoundQueueAdd(SOUND_MUSIC_PAUSE);
        for (slot = TASK_SLOTS - 1; slot >= 1; slot--) {
            frozen[slot] = g.tasks[slot].state;
            g.tasks[slot].state = TASK_FROZEN;
        }
        *at = PAUSE_HELD;
        return task_wait(t, 1);
    case PAUSE_HELD:
        if (!((g.pad1_pressed >> 8) & START_HIGH))
            return task_wait(t, 1);
        TextLayoutShow(TEXT_PAUSED | 0x80);
        g.paused = 0;
        g.pause_cooldown = COOLDOWN;
        SoundQueueAdd(SOUND_MUSIC_GO_ON);
        for (slot = 1; slot < TASK_SLOTS; slot++)
            g.tasks[slot].state = frozen[slot];
        *at = PAUSE_LAST;
        return task_wait(t, 1);
    case PAUSE_LAST:
        g.text_upload_bytes = PLAY_TEXT_BYTES;
        break;
    }
    return task_return(t);
}
