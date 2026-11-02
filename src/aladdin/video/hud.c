/*
 * hud.c: the play screen's status line in the text layer: the health
 * marks, the two counters, a digit and a 2x2 mark. Each is drawn when
 * its shown value falls behind the game's (HudRefresh, each frame).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"

#define HEALTH_SHOWN 0x000366u     /* the health the marks show */
#define HUD_COUNT_1_SHOWN 0x000368u /* the first counter as shown (its value at $0369) */
#define HUD_COUNT_2_SHOWN 0x00036Au /* the second counter as shown (its value at $036B) */
#define HEALTH_MARK_PLACES 0x81F58Du /* by health less one: the mark's tilemap offset */
#define HEALTH_MARK_TILE 0x6A      /* the mark's top tile; its bottom is 16 on */
#define HUD_TILE_BITS 0x2C00       /* palette and priority of the status line's tiles */
#define DIGIT_TILE_0 0x05          /* the counters' digit tiles start here; bottoms 16 on */
#define HEALTH_MARK_GAINED_TILE 0x60 /* the mark drawn as health is gained (HEALTH_MARK_TILE as it is lost) */
#define HUD_COUNT_1 0x000369u      /* the first counter's value */
#define HUD_COUNT_2 0x00036Bu      /* the second counter's value */
#define HUD_VALUE 0x000364u        /* the value the status line's digit shows (less 1) */
#define HUD_VALUE_SHOWN 0x000363u  /* that value as shown */
#define HUD_MARK_VALUE 0x00036Eu   /* zero: the 2x2 mark is blank */
#define HUD_MARK_SHOWN 0x00036Du   /* that value as shown */

void abi_HudMarkRefresh(void);
void abi_HudCount1Show(void);
void abi_HudCount2Show(void);
void abi_HudDigitRefresh(void);

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
    uint8_t mark = (uint8_t)((uint8_t)cpu.a - 1), place;
    uint16_t top, bottom;

    cpu.y = HEALTH_MARK_TILE;
    cpu.x = mark;
    wr8(dba(HEALTH_SHOWN), mark);
    place = rd8(HEALTH_MARK_PLACES + mark);
    cpu.x = place;
    /* its top tile, and its bottom (16 on) a row below */
    top = HUD_TILE_BITS | HEALTH_MARK_TILE;
    wr16((TEXT_TILEMAP + 74 + place) & 0xFFFFFF, top);
    flag_set(P_C, 0);
    bottom = op_adc16(top, 0x0010, &cpu.p);
    wr16((TEXT_TILEMAP + 138 + place) & 0xFFFFFF, bottom);
    cpu.a = (uint16_t)((bottom & 0xFF00) | 0x01);
    nz8_of(0x01);
    wr8(dba(TEXT_TILEMAP_UPLOAD), 0x01);
}

/* A's two hex digits as big digits: tops at `top` and `top` + 2 in the
   text tilemap, bottoms a row below; the value kept at `shown`. */
/* A's two hex digits as big digits: tops at `top` and `top` + 2 in the
   text tilemap, bottoms a row below; the value kept at `shown`. */
static void counter_show(uint32_t shown, uint32_t top)
{
    uint8_t value = (uint8_t)cpu.a, digit;

    wr8(dba((uint16_t)shown), value);
    pha8();
    flag_set(P_C, 0);
    digit = op_adc8((uint8_t)(value >> 4), DIGIT_TILE_0, &cpu.p);
    wr8(top, digit);
    wr8(top + 64, (uint8_t)(digit | 0x10));
    flag_set(P_C, 0);
    pla8();
    digit = op_adc8((uint8_t)(value & 0x0F), DIGIT_TILE_0, &cpu.p);
    wr8(top + 2, digit);
    wr8(top + 66, (uint8_t)(digit | 0x10));
    a8_load(0x01);
    wr8(dba(TEXT_TILEMAP_UPLOAD), 0x01);
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

/*
 * @routine 81:F47F HudDigitRefresh
 * WHAT IT DOES: Draws the status line's digit for the value at $0364 when
 * it differs from the one shown ($0363): the value less 1, 0 for 0 and 9
 * at most; a value above 100 is held at 99; a negative value gets a mark
 * beside the digit.
 * HOW: The value kept as shown; negative: tile $2E at $7E:D908 and its low
 * nibble drawn; else the value less 1: negative, 0; below 9, it; 100 or
 * more ($65 up), $63 kept in both; else 9. The digit's tile (low nibble
 * plus 5) at $7E:D8C6 and its lower half (OR $10) at $7E:D906; the text
 * layer's upload set.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
 * Exit: M 8-bit; unchanged: A low = the value, Z set; drawn: A low = 1.
 */
void abi_HudDigitRefresh(void)
{
    uint8_t value = rd8(dba(HUD_VALUE)), digit;

    a8_load(value);
    cmp8_of(value, rd8(dba(HUD_VALUE_SHOWN)));
    if (value == rd8(dba(HUD_VALUE_SHOWN)))
        return;
    wr8(dba(HUD_VALUE_SHOWN), value);
    a8_load(rd8(dba(HUD_VALUE_SHOWN)));
    if (value & 0x80) {                   /* negative: a mark beside the digit */
        pha8();
        a8_load(0x2E);
        wr8(TEXT_TILEMAP + 264, 0x2E);
        pla8();
        digit = value;
    } else {                              /* the value less 1: 0 to 9 */
        digit = (uint8_t)(value - 1);
        a8_load(digit);
        if (digit & 0x80) {
            a8_load(0x00);
            digit = 0;
        } else {
            cmp8_of(digit, 0x09);
            if (digit >= 0x09) {
                cmp8_of(digit, 0x64);
                if (digit >= 0x64) {      /* held at 99 */
                    a8_load(0x63);
                    wr8(dba(HUD_VALUE), 0x63);
                    wr8(dba(HUD_VALUE_SHOWN), 0x63);
                }
                a8_load(0x09);
                digit = 9;
            }
        }
    }
    flag_set(P_C, 0);
    digit = op_adc8((uint8_t)(digit & 0x0F), 0x05, &cpu.p);
    wr8(TEXT_TILEMAP + 198, digit);
    wr8(TEXT_TILEMAP + 262, (uint8_t)(digit | 0x10));
    a8_load(0x01);
    wr8(dba(TEXT_TILEMAP_UPLOAD), 0x01);
}


/*
 * @routine 81:F54C HudMarkRefresh
 * WHAT IT DOES: Draws the status line's 2x2 mark for the value at $036E,
 * keeping it as shown ($036D): for 0 its blank, else its picture.
 * HOW: The value to $036D; zero: the four blank words ($7E:D8AA, $D8AC,
 * $D8EA, $D8EC); else tiles $2462, $2463, $2472, $2473 there. The text
 * layer's upload counted up.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSR (81:F45E). Exit: M 8-bit; A = the last tile word.
 */
void abi_HudMarkRefresh(void)
{
    static const uint16_t places[4] = { 0xAA, 0xAC, 0xEA, 0xEC };
    static const uint16_t mark[4] = { 0x2462, 0x2463, 0x2472, 0x2473 };
    uint8_t value = rd8(dba(HUD_MARK_VALUE));

    x8_load(value);
    wr8(dba(HUD_MARK_SHOWN), value);
    for (int i = 0; i < 4; i++) {         /* the 2x2 mark, or blanks */
        cpu.a = value ? mark[i] : 0x2020;
        wr16(TEXT_TILEMAP + places[i], cpu.a);
    }
    inc8_at(dba(TEXT_TILEMAP_UPLOAD));
}

/* A shown value against the game's: A and the flags as the compare
   leaves them; nonzero when they differ. */
static int differs(uint16_t value, uint16_t shown)
{
    uint8_t v = rd8(dba(value));

    a8_load(v);
    cmp8_of(v, rd8(dba(shown)));
    return !(cpu.p & P_Z);
}

/*
 * @routine 81:F456 HudRefresh
 * WHAT IT DOES: Each frame, brings the status line up to the game: the
 * 2x2 mark, the two counters, then either one health mark (when the
 * health shown differs from the health) or the digit.
 * HOW: Each shown value against its value: the mark (HudMarkRefresh),
 * counter 1 (HudCount1Show), counter 2 (HudCount2Show) when they differ.
 * Health shown equal to the health: on into HudDigitRefresh. Above or
 * equal (C set): HudHealthMarkShow of the shown health (one mark lost).
 * Below: one mark gained: the shown health up one, and the mark at the
 * old shown health's place drawn with tile $60 (its code, $81:F4C8, lies
 * after HudDigitRefresh and goes on inside HudHealthMarkShow, at its
 * store of the shown health).
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR (81:82D8, and on from 81:F433). Exit: as
 * HudDigitRefresh, or as HudHealthMarkShow with Y = $60 or $6A.
 */
void abi_HudRefresh(void)
{
    uint8_t shown, place;

    if (differs(HUD_MARK_VALUE, HUD_MARK_SHOWN))
        abi_HudMarkRefresh();
    if (differs(HUD_COUNT_1, HUD_COUNT_1_SHOWN))
        abi_HudCount1Show();
    if (differs(HUD_COUNT_2, HUD_COUNT_2_SHOWN))
        abi_HudCount2Show();
    if (!differs(HEALTH_SHOWN, HEALTH)) {
        abi_HudDigitRefresh();
        return;
    }
    if (cpu.p & P_C) {                    /* health lost: one mark down */
        abi_HudHealthMarkShow();
        return;
    }
    /* health gained: the next mark up drawn with the gained tile */
    shown = (uint8_t)cpu.a;
    wr8(dba(HEALTH_SHOWN), (uint8_t)(shown + 1));
    place = rd8(HEALTH_MARK_PLACES + shown);
    wr16(TEXT_TILEMAP + 74 + place, HUD_TILE_BITS | HEALTH_MARK_GAINED_TILE);
    wr16(TEXT_TILEMAP + 138 + place, (HUD_TILE_BITS | HEALTH_MARK_GAINED_TILE) + 0x0010);
    wr8(dba(TEXT_TILEMAP_UPLOAD), 0x01);
    cpu.x = place;
    cpu.y = HEALTH_MARK_GAINED_TILE;
    cpu.a = (uint16_t)(HUD_TILE_BITS | 0x01);
    nz8_of(0x01);
    flag_set(P_C, 0);
    flag_set(P_V, 0);
}
