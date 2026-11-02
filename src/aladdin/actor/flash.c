/*
 * flash.c: an actor flashing while the countdown at $1B29 runs: its
 * sprites in palette 3 on odd counts, or palette 2 turned white.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorLookFlash(void);
void abi_PaletteFlash(void);
void abi_PaletteRestoreReversed(void);
void abi_PaletteWhite(void);
#include "actor.h"
#include "actor_anim.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

#define FLASH_PHASE 0x001B29u  /* bit 0 set: the flash shows */

/*
 * @routine 84:8E3C ActorLookFlash
 * WHAT IT DOES: On odd counts of the flash countdown shows the actor's
 * sprites in palette 3; on even counts gives them back their kind's.
 * HOW: Bit 0 of $1B29: set, the palette bits of the sprite attributes'
 * high byte ($2C) set to 3; clear, that byte from its kind's at
 * $7F:E701 (kind * 2).
 * Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
 * work RAM; called by JSR. Exit: X = the kind * 2; A low byte = the new
 * byte.
 */
void abi_ActorLookFlash(void)
{
    uint8_t phase, look;

    cpu.x = (uint8_t)(rd8(dpa(ACTOR_06)) << 1);
    phase = rd8(dba(FLASH_PHASE));
    a8_load((uint8_t)(phase >> 1));
    flag_set(P_C, phase & 1);
    if (phase & 1)                        /* flashing: palette 3 */
        look = (uint8_t)((rd8(dpa(ACTOR_2C)) & 0xF1) | 0x06);
    else                                  /* its own: its kind's */
        look = rd8(CELL_ATTRS + 1 + cpu.x);
    a8_load(look);
    wr8(dpa(ACTOR_2C), look);
}

/*
 * @routine 84:8E56 PaletteFlash
 * WHAT IT DOES: On odd counts of the flash countdown turns palette 2
 * white; on even counts gives it back its kept colours.
 * HOW: Bit 0 of $1B29: set, PaletteWhite 2; clear, PaletteRestoreReversed
 * 2.
 * Entry: M 8-bit, X 8-bit; DB = $00-$3F or $80-$BF; called by JSR. Exit:
 * as PaletteWhite or PaletteRestoreReversed.
 */
void abi_PaletteFlash(void)
{
    uint8_t phase = rd8(dba(FLASH_PHASE));

    flag_set(P_C, phase & 1);
    a8_load(0x02);
    if (phase & 1)
        abi_PaletteWhite();
    else
        abi_PaletteRestoreReversed();
}
