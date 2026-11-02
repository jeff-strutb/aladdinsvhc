/*
 * tile_actor.c: an actor drawn as tiles of the level's map, changed
 * through the tile change queue as it moves. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_TileChangeAdd(void);

/* In the data bank: per kind ($47) the offset of its entries; 4 bytes an
   entry: the tile, and the tile 16 pixels below (0: none) */
#define TILE_ACTOR_TILES 0xD633u
#define EMPTY_TILE 0x7F80u          /* the tile put while $19 is 0 */

#define TILE_SHOWN 0x19u           /* word: 0, the empty tile is put */
#define TILE_PLACE_X 0x42u         /* where its tiles go (words) */
#define TILE_PLACE_Y 0x44u
#define TILE_SET 0x47u             /* its set in TILE_ACTOR_TILES */
#define TILE_SIDE 0x4Au            /* picks its tile, with TILE_SHOWN's low byte and its flip */

/* The tile (or the empty one while TILE_SHOWN is 0) to TileChangeAdd, at
   x X and y Y. */
static void tile_put(uint16_t tile)
{
    uint16_t shown = dp16(TILE_SHOWN);

    cpu.x = shown;
    nz16_of(shown);
    if (shown)
        cpu.a = tile;
    else
        a16_load(EMPTY_TILE);
}

/*
 * @routine 83:E7F0 TileActorDraw
 * WHAT IT DOES: Puts the actor into the level's map as tiles at its tile
 * spot ($42, $44): the tile for its kind, frame and flip, and the tile 16
 * pixels below when it has one; the empty tile in place of each while its
 * $19 is 0.
 * HOW: The entry: ($4A xor $19) times 2, or its flip ($15), times 4, plus
 * the kind's offset from the table at its $47. TileChangeAdd at ($42, $44)
 * with the first tile; the second nonzero: TileChangeAdd at ($42, $44 +
 * 16).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; as TileChangeAdd, or A low = 0 and Z set
 * when there is no second tile.
 */
void abi_TileActorDraw(void)
{
    uint8_t pick;
    uint16_t at, tile;

    x8_load(rd8(dpa(TILE_SET)));
    pick = (uint8_t)(rd8(dpa(TILE_SIDE)) ^ rd8(dpa(TILE_SHOWN)));
    pick = (uint8_t)((uint8_t)((uint8_t)(pick << 1) | rd8(dpa(ACTOR_FLIP))) << 2);
    flag_set(P_C, 0);
    at = op_adc8(pick, rd8(dba(TILE_ACTOR_TILES) + cpu.x), &cpu.p);
    op_rep(0x30);
    cpu.a = cpu.x = at;
    nz16_of(at);
    phx16();
    /* its top tile at the place, its bottom tile 16 below when it has one */
    tile_put(rd16(dba(TILE_ACTOR_TILES) + at));
    cpu.x = dp16(TILE_PLACE_X);
    cpu.y = dp16(TILE_PLACE_Y);
    nz16_of(cpu.y);
    abi_TileChangeAdd();
    plx16();
    tile = rd16(dba(TILE_ACTOR_TILES + 2) + cpu.x);
    a16_load(tile);
    if (tile) {
        tile_put(tile);
        pha16();
        cpu.x = dp16(TILE_PLACE_X);
        flag_set(P_C, 0);
        cpu.y = op_adc16(dp16(TILE_PLACE_Y), 0x0010, &cpu.p);
        cpu.a = cpu.y;
        pla16();
        abi_TileChangeAdd();
    }
    op_sep(0x30);
}

