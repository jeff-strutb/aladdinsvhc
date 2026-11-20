/*
 * level_flags.c: the level's eight switch flags (g.level_switch_flags),
 * which kind $3B's switches toggle and its markers show.
 */
#include "data/level_tables.h"
#include "level/level.h"
#include "system/state.h"

/*
 * @routine 83:C2F0 LevelFlagFromMask
 * WHAT IT DOES: Sets switch flag `flag` (0-7) to 1 when its bit is set in
 * the mask for the stage (g.progress_stage), else to 0. (No level the game
 * plays has a mask bit set.)
 */
uint8_t LevelFlagFromMask(uint8_t flag)
{
    uint8_t set = (level_flag_mask(g.progress_stage) & flag_bit(flag)) ? 1 : 0;

    g.level_switch_flags[flag & 7] = set;
    return set;
}

/*
 * @routine 83:C2A9 LevelFlagSpotFind
 * WHAT IT DOES: Finds a point (each word rounded down to 8 pixels) in the
 * level's list of flagged spots (levels $0E up): its flag, the count of
 * spots from it to the list's end less 1; -1 when not found.
 */
int LevelFlagSpotFind(uint16_t x, uint16_t y)
{
    uint8_t level = (uint8_t)g.level;
    uint8_t n = flag_spot_count(level);
    unsigned i = 0;

    x &= 0xFFF8;
    y &= 0xFFF8;
    do {
        if ((flag_spot_x(level, i) & 0xFFF8) == x && (flag_spot_y(level, i) & 0xFFF8) == y)
            return (uint8_t)(n - 1);
        i++;
    } while (--n);
    return -1;
}
