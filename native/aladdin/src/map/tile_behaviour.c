/*
 * tile_behaviour.c: the behaviour of each map tile (solid, slope ...),
 * which the collision tests read (map.h).
 */
#include "data/data.h"
#include "data/game_tables.h"
#include "map/map.h"
#include "system/state.h"

/*
 * @routine 81:BEC5 TileBehaviourLoad
 * WHAT IT DOES: Loads the tile behaviour table of the map LevelMapBuild
 * built: 4 KB, one byte a metatile, from the table of its metatiles' bank
 * (the record's map set, $005C).
 */
void TileBehaviourLoad(void)
{
    DataRef metatiles = level_metatiles(g.map_level);

    for (uint16_t i = 0; i < sizeof g.tile_behaviour; i++)
        g.tile_behaviour[i] = tile_behaviour_byte(metatiles, i);
}
