/*
 * options_screen.c: the options screen's text: a setting's words and the
 * cursor beside the item.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

/* In the bank DB names: for each of 4 settings five tilemap words, one
   table per line (8 bytes apart); the cursor's place per item. */
#define SETTING_WORDS 0xCE35u
#define CURSOR_PLACES 0xCE5Du

static uint32_t in_db(uint32_t addr)
{
    return (uint32_t)cpu.db << 16 | addr;
}

/* Where the setting's five words go (tilemap byte offsets): column 21 of
   rows 6, 9, 11, 13 and 15. */
static const uint16_t setting_at[5] = { 426, 618, 746, 874, 1002 };

/*
 * @routine 83:8C16 OptionSettingShow
 * WHAT IT DOES: Shows a setting on the options screen: five tilemap words
 * down one column, chosen by A, and asks for the text layer's upload.
 * HOW: A doubled indexes five tables 8 bytes apart; each word goes to its
 * line of the text tilemap.
 * Entry: M 8-bit, X 8-bit; A = the setting (0-3); DB the tables' bank,
 * whose $0000-$1FFF is work RAM. Exit: A = the last word (16 bits); Y =
 * twice the setting; N and Z from the upload flag's new value; C from
 * doubling A. M 8-bit; X and the other flags unchanged.
 */
void abi_OptionSettingShow(void)
{
    uint8_t a = (uint8_t)cpu.a, y;
    int i;

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
    y = (uint8_t)(a << 1);
    cpu.y = y;
    op_rep(P_M);
    for (i = 0; i < 5; i++) {
        cpu.a = rd16(in_db(SETTING_WORDS + 8 * i + y));
        wr16(TEXT_TILEMAP + setting_at[i], cpu.a);
    }
    op_sep(P_M);
    a = (uint8_t)(rd8_rmw(TEXT_TILEMAP_UPLOAD) + 1);
    wr8(TEXT_TILEMAP_UPLOAD, a);
    cpu.p = nz8(cpu.p, a);
}

/*
 * @routine 83:8C43 OptionCursorDraw
 * WHAT IT DOES: Draws the options screen's cursor at item Y and asks for
 * the text layer's upload.
 * HOW: The item's place (a tilemap byte offset) gets three tiles on each
 * of two lines, and one tile on the line above, one column in, with
 * 16-bit registers.
 * Entry: M 8-bit, X 8-bit; Y = the item's offset; DB the table's bank,
 * whose $0000-$1FFF is work RAM. Exit: A = $2014; X = the place's low
 * byte; N and Z from the upload flag's new value; M and X 8-bit. Y and the
 * other flags unchanged.
 */
void abi_OptionCursorDraw(void)
{
    uint16_t x;
    uint8_t a;

    op_rep(P_M | P_X);
    x = rd16(in_db(CURSOR_PLACES + cpu.y));
    cpu.x = x;
    wr16(TEXT_TILEMAP - 62 + x, 0x207E);
    wr16(TEXT_TILEMAP + x, 0x2802);
    wr16(TEXT_TILEMAP + 2 + x, 0x2003);
    wr16(TEXT_TILEMAP + 4 + x, 0x2004);
    wr16(TEXT_TILEMAP + 64 + x, 0x2812);
    wr16(TEXT_TILEMAP + 66 + x, 0x2013);
    wr16(TEXT_TILEMAP + 68 + x, 0x2014);
    cpu.a = 0x2014;
    op_sep(P_M | P_X);
    a = (uint8_t)(rd8_rmw(TEXT_TILEMAP_UPLOAD) + 1);
    wr8(TEXT_TILEMAP_UPLOAD, a);
    cpu.p = nz8(cpu.p, a);
}
