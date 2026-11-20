/*
 * rise_fall.c: an actor posed by its rise or fall.
 */
#include "actor/actor_own.h"
#include "data/actor_a_tables.h"

/*
 * @routine A5:EA20 ActorAnimRiseFall
 * WHAT IT DOES: Shows an actor rising or falling: animation $0C while it
 * rises (its vertical speed's top byte negative), $0E while it falls.
 */
void ActorAnimRiseFall(Actor *a)
{
    ActorAnimChange(a, ((a->speed_y >> 16) & 0x80) ? 0x0C : 0x0E);
}

/*
 * @routine A5:EB1E ActorPlaceAtSpot
 * WHAT IT DOES: Puts an actor at spot `spot` (a byte offset: 4 bytes a
 * spot, x then y words) of the table at $80:D02C, on whole pixels.
 */
void ActorPlaceAtSpot(Actor *a, uint8_t spot)
{
    a->x = (uint32_t)a5_spot_x(spot) << 8;
    a->y = (uint32_t)a5_spot_y(spot) << 8;
}
