/*
 * play_time.c: the time the game has been played, counted each frame in
 * decimal digits.
 */
#include "game/game.h"
#include "system/state.h"

#define FRAMES_A_SECOND 60

/*
 * @routine 81:85CE PlayTimeTick
 * WHAT IT DOES: Counts one more frame of play time.
 * HOW: Frames to 60 make a second; the seconds' units carry at 10, tens at
 * 6, minutes' units at 10, tens at 6, hours' units at 10 into the tens,
 * which count on without a limit. The seconds count (play_seconds) steps
 * too, without carrying.
 */
void PlayTimeTick(void)
{
    static const uint8_t limits[5] = { 10, 6, 10, 6, 10 };
    unsigned i;

    if (++g.play_frames < FRAMES_A_SECOND)
        return;
    g.play_frames = 0;
    g.play_seconds++;
    for (i = 0; i < 5; i++) {
        if (++g.play_digits[i] < limits[i])
            return;
        g.play_digits[i] = 0;
    }
    g.play_digits[5]++;
}
