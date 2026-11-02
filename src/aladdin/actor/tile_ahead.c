/*
 * tile_ahead.c: an actor's look at the map tile ahead of it and below,
 * and an actor's end from code in another bank.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_ActorFreePlacedAtY(void);

/* Not yet filed: the class of the map tile at the actor's position offset
   by the word at $D062 + facing x 2 + the byte at $000C (in the bank DB
   names) and by the word at $000E, through $81:B82B. */
#define TILE_AHEAD 0x839A15u
#define TILE_BELOW 12              /* pixels below the actor's position */
#define CLASS_70 0x70              /* the class's bits 4-6 */

/*
 * @routine 83:99EE ActorTileAheadIs70
 * WHAT IT DOES: Tells whether the map tile ahead of the actor, 12 pixels
 * below its position, is of the class whose bits 4-6 are all set.
 * HOW: Offsets 0 (in the facing table) and 12 for $83:9A15, which returns
 * the tile's class (a byte by its behaviour's low 3 bits, through
 * $81:B82B); then AND #$70 and CMP #$70.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM, the offsets' bank. Exit: Z set when all three are set; A =
 * the class's bits 4-6; N, Z and C from the compare (C set only when
 * equal); X = the class; M and X 8-bit; Y and V, and the scratch bytes
 * from $000C, as $83:9A15 leaves them.
 */
void abi_ActorTileAheadIs70(void)
{
    uint8_t class;

    wr8(dba(SCRATCH), 0);
    a8_load(TILE_BELOW);
    wr8(dba(SCRATCH + 2), TILE_BELOW);
    wr8(dba(SCRATCH + 3), 0);
    call_routine(TILE_AHEAD, 0);
    class = (uint8_t)cpu.a & CLASS_70;
    a8_load(class);
    cmp8_of(class, CLASS_70);
}

/*
 * @routine 83:96D0 ActorFreePlacedAtYLong
 * WHAT IT DOES: ActorFreePlacedAtY for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ActorFreePlacedAtY; called by JSL. Exit: as
 * ActorFreePlacedAtY.
 */
void abi_ActorFreePlacedAtYLong(void)
{
    abi_ActorFreePlacedAtY();
}
