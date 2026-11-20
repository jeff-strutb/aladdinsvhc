/*
 * level_bits.c: the level's eight switch flags set from its stage's mask.
 */
#include "level/level.h"

/*
 * @routine 83:C2E7 Bits035DExpand
 * WHAT IT DOES: Sets the eight switch flags from the eight bits of the
 * stage's mask, the last first.
 */
void Bits035DExpand(void)
{
    for (int flag = 7; flag >= 0; flag--)
        (void)LevelFlagFromMask((uint8_t)flag);
}
