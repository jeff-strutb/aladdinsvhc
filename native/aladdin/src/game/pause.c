/*
 * pause.c: the pause check. In the original, Start during play froze every task but the one that ran this, showed PAUSED
 * and paused the music, until Start again. Here Start opens the menu instead (pause/menu.c), from the same places: it
 * holds the game while it is open (the game is not stepped, so the tasks, the frame counts and the picture stand as
 * they were, as the original's frozen tasks did), pauses the music and gives the sound the original's pause gave, and
 * sets the same cooldown when it shuts.
 */
#include "game/game.h"
#include "pause/menu.h"
#include "system/state.h"
#include "system/task.h"

#define START_HIGH 0x10             /* Start, in a pad word's high byte */
#define FULL_BRIGHTNESS 0x0F

/*
 * @routine 81:8398 PauseCheck
 * WHAT IT DOES: Asks for the menu when Start is newly pressed during play (full brightness, no demo, nothing blocking,
 * the level's start text done), where the original paused; the menu opens from the next update. Otherwise: the
 * cooldown counted down.
 * HOW: The cooldown nonzero, the brightness not full, pause_block, the play mode or $03FA nonzero, or Start not newly
 * pressed: the cooldown counted down; the original's tests, all of them.
 */
TaskRun PauseCheck(Task *t)
{
    (void)task_enter(t);
    if (g.pause_cooldown || (g.brightness & 0x0F) != FULL_BRIGHTNESS
        || (g.pause_block | g.play_mode | g.play_starts) || !((g.pad1_pressed >> 8) & START_HIGH)) {
        if (g.pause_cooldown)
            g.pause_cooldown--;
    } else {
        menu_request();
    }
    return task_return(t);
}
