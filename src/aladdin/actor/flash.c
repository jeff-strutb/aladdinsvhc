/*
 * flash.c: an actor flashing while the countdown at $1B29 runs: its
 * sprites in palette 3 on odd counts, or palette 2 turned white.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorLookFlash(void);
void abi_PaletteFlash(void);
void abi_PaletteRestoreReversed(void);
void abi_PaletteWhite(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

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
    lda8(ea_dp(0x06));  /* 848E3C lda */
    ACC8(asl);  /* 848E3E asl */
    tax8();  /* 848E3F tax */
    lda8(ea_abs(0x1B29));  /* 848E40 lda */
    ACC8(lsr);  /* 848E43 lsr */
    if (!IF_C) goto L_848E4F;  /* 848E44 bcc */
    lda8(ea_dp(0x2C));  /* 848E46 lda */
    and8_v(0xF1);  /* 848E48 and */
    ora8_v(0x06);  /* 848E4A ora */
    sta8(ea_dp(0x2C));  /* 848E4C sta */
    return;  /* 848E4E rts */
L_848E4F:
    lda8(ea_longx(0x7FE701u));  /* 848E4F lda */
    sta8(ea_dp(0x2C));  /* 848E53 sta */
    return;  /* 848E55 rts */
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
    lda8(ea_abs(0x1B29));  /* 848E56 lda */
    ACC8(lsr);  /* 848E59 lsr */
    if (!IF_C) goto L_848E63;  /* 848E5A bcc */
    lda8_v(0x02);  /* 848E5C lda */
    abi_PaletteWhite();  /* 848E5E jsl */
    return;  /* 848E62 rts */
L_848E63:
    lda8_v(0x02);  /* 848E63 lda */
    abi_PaletteRestoreReversed();  /* 848E65 jsl */
    return;  /* 848E69 rts */
}
