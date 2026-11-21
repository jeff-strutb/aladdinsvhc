/*
 * stage_countdown.c: the countdown a stage of the ending's play-back runs.
 */
#include "data/game_tables.h"
#include "level/level.h"
#include "system/state.h"

/*
 * @routine 82:B563 StageCountdownSet
 * WHAT IT DOES: Sets the countdown to the stage's value from the table of
 * the ending's stages: the stage is g.demo_number (the original's $033F).
 */
void StageCountdownSet(void)
{
    g.play_countdown = ending_stage_frames(g.demo_number);
}
