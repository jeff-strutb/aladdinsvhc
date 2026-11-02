/*
 * backdrop_offset.c: the backdrop layer's scroll taken from the play
 * field's, with an offset down. Drafted by cdraft.py; named and described
 * by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram_video.h"

#define BACKDROP_OFFSET_Y 0x001A2Bu   /* added to the play field's scroll Y for the backdrop's: 4 bytes, from the fraction byte */

/*
 * @routine 81:CA6A BackdropFollowOffsetY
 * WHAT IT DOES: Moves the backdrop with the play field: the same scroll X,
 * and its scroll Y the play field's plus an offset.
 * HOW: The scroll X word copied; the scroll Y, from its fraction byte,
 * plus BACKDROP_OFFSET_Y as two 16-bit adds with carry (4 bytes).
 * Entry: M 16-bit, X 16-bit; DB = $80; called by JSR from the scroll
 * handler the level picks ($0841). Exit: M 16-bit, X 16-bit; A = the
 * result's high word; C, N, V, Z from its add.
 */
void abi_BackdropFollowOffsetY(void)
{
    uint16_t low;

    /* x: the play field's; y (fraction byte up, 4 bytes): the play field's
       plus the offset */
    wr16(dba(LAYER1 + LAYER_SCROLL_X), rd16(dba(LAYER0 + LAYER_SCROLL_X)));
    flag_set(P_C, 0);
    low = op_adc16(rd16(dba(LAYER0 + LAYER_SCROLL_Y - 1)), rd16(dba(BACKDROP_OFFSET_Y)), &cpu.p);
    wr16(dba(LAYER1 + LAYER_SCROLL_Y - 1), low);
    cpu.a = op_adc16(rd16(dba(LAYER0 + LAYER_SCROLL_Y + 1)), rd16(dba(BACKDROP_OFFSET_Y + 2)), &cpu.p);
    wr16(dba(LAYER1 + LAYER_SCROLL_Y + 1), cpu.a);
}

