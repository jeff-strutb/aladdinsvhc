/*
 * random.c: the game's random numbers.
 */
#include "system/state.h"
#include "system/system.h"

/*
 * @routine 81:981B RandomNext
 * WHAT IT DOES: Steps the random number: the next in its sequence.
 * HOW: random * 3, plus its old low byte into the high byte (random * $0103
 * mod $10000); the new high byte is the number drawn.
 */
uint8_t RandomNext(void)
{
    uint16_t r = g.random;

    g.random = (uint16_t)(r * 3u + ((r & 0xFFu) << 8));
    return (uint8_t)(g.random >> 8);
}
