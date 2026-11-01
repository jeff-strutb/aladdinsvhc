/*
 * hud.c: the play screen's status line in the text layer: the health
 * marks and the two counters. Each is drawn when its shown value falls
 * behind the game's (the refresh at $81:F43C).
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"

#define HEALTH_SHOWN 0x000366u     /* the health the marks show */
#define HUD_COUNT_1_SHOWN 0x000368u /* the first counter as shown (its value at $0369) */
#define HUD_COUNT_2_SHOWN 0x00036Au /* the second counter as shown (its value at $036B) */
#define HEALTH_MARK_PLACES 0x81F58Du /* by health less one: the mark's tilemap offset */
#define HEALTH_MARK_TILE 0x6A      /* the mark's top tile; its bottom is 16 on */
#define HUD_TILE_BITS 0x2C00       /* palette and priority of the status line's tiles */
#define DIGIT_TILE_0 0x05          /* the counters' digit tiles start here; bottoms 16 on */

/*
 * @routine 81:F4D0 HudHealthMarkShow
 * WHAT IT DOES: Draws health mark A (1 up) in the status line, and keeps
 * A less one as the health shown.
 * HOW: The mark's place comes from a table by A less one; its top and
 * bottom tiles go in the text tilemap a row apart, and the layer's upload
 * is asked for.
 * Entry: M 8-bit, X 8-bit; A = the mark; DB a bank whose $0000-$1FFF is
 * work RAM. Exit: A low = 1, A high = the bottom tile's high byte; X = the
 * place; Y = $6A; N and Z clear; V and C from adding the row; M 8-bit.
 * The other flags unchanged.
 */
void abi_HudHealthMarkShow(void)
{
    ldy8_v(HEALTH_MARK_TILE);
    ACC8(dec);
    tax8();
    sta8(ea_abs(HEALTH_SHOWN));
    lda8(ea_longx(HEALTH_MARK_PLACES));
    tax8();
    op_rep(0x20);
    tya16();
    ora16_v(HUD_TILE_BITS);
    sta16(ea_longx(TEXT_TILEMAP + 74));
    clc();
    adc16_v(0x0010);
    sta16(ea_longx(TEXT_TILEMAP + 138));
    op_sep(0x20);
    lda8_v(0x01);
    sta8(ea_abs(TEXT_TILEMAP_UPLOAD));
}

/* A's two hex digits as big digits: tops at `top` and `top` + 2 in the
   text tilemap, bottoms a row below; the value kept at `shown`. */
static void counter_show(uint32_t shown, uint32_t top)
{
    sta8(ea_abs((uint16_t)shown));
    pha8();
    ACC8(lsr);
    ACC8(lsr);
    ACC8(lsr);
    ACC8(lsr);
    clc();
    adc8_v(DIGIT_TILE_0);
    sta8(top);
    ora8_v(0x10);
    sta8(top + 64);
    clc();
    pla8();
    and8_v(0x0F);
    adc8_v(DIGIT_TILE_0);
    sta8(top + 2);
    ora8_v(0x10);
    sta8(top + 66);
    lda8_v(0x01);
    sta8(ea_abs(TEXT_TILEMAP_UPLOAD));
}

/*
 * @routine 81:F4F6 HudCount1Show
 * WHAT IT DOES: Draws the first counter, A as two digits, in the status
 * line, and keeps A as the value shown.
 * HOW: Each hex digit's top tile is 5 plus the digit, its bottom tile 16
 * on, a row below; the layer's upload is asked for.
 * Entry: M 8-bit; A = the value (two decimal digits as hex); DB a bank
 * whose $0000-$1FFF is work RAM. Exit: A low = 1; N and Z clear; V and C
 * from the last addition. A high, X, Y and the other flags unchanged.
 */
void abi_HudCount1Show(void)
{
    counter_show(HUD_COUNT_1_SHOWN, TEXT_TILEMAP + 178);
}

/*
 * @routine 81:F521 HudCount2Show
 * WHAT IT DOES: Draws the second counter, A as two digits, in the status
 * line, and keeps A as the value shown.
 * HOW: As HudCount1Show, four columns to the right.
 * Entry: M 8-bit; A = the value (two decimal digits as hex); DB a bank
 * whose $0000-$1FFF is work RAM. Exit: A low = 1; N and Z clear; V and C
 * from the last addition. A high, X, Y and the other flags unchanged.
 */
void abi_HudCount2Show(void)
{
    counter_show(HUD_COUNT_2_SHOWN, TEXT_TILEMAP + 186);
}
