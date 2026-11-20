/*
 * hud.c: the play screen's status line in the text layer: the health marks,
 * the two counters (the apples and the second), the lives digit and the
 * hover mark. Each is drawn when its shown value falls behind the game's
 * (HudRefresh, each frame).
 */
#include "data/video_tables.h"
#include "system/state.h"
#include "video/video.h"

#define HUD_TILE_BITS 0x2C00        /* the status line's palette and priority */
#define HEALTH_MARK_LOST 0x6A       /* a mark's top tile as health is lost; its bottom 16 on */
#define HEALTH_MARK_GAINED 0x60     /* as health is gained */
#define DIGIT_TILE_0 0x05           /* the digits' top tiles; bottoms OR $10 */
#define MARKS_TOP 74                /* the health marks' row, a byte offset in the text map */
#define ROW 64                      /* a row of the text map, in bytes */

/* the text map by the original's byte offsets: a word, or a low byte alone */
static void word_at(unsigned at, uint16_t w)
{
    if (at / 2u < 32 * 32)
        g.text_map[at / 2u] = w;
}

static void low_byte_at(unsigned at, uint8_t v)
{
    if (at / 2u < 32 * 32)
        g.text_map[at / 2u] = (uint16_t)((g.text_map[at / 2u] & 0xFF00) | v);
}

static void health_mark(uint8_t n, uint8_t tile)
{
    unsigned place = health_mark_place(n);

    word_at(MARKS_TOP + place, (uint16_t)(HUD_TILE_BITS | tile));
    word_at(MARKS_TOP + ROW + place, (uint16_t)((HUD_TILE_BITS | tile) + 0x10));
    g.text_map_upload = 1;
}

/*
 * @routine 81:F4D0 HudHealthMarkShow
 * WHAT IT DOES: Draws health mark `mark` (1 up) as lost, and keeps `mark`
 * less one as the health shown.
 */
void HudHealthMarkShow(uint8_t mark)
{
    g.health_shown = (uint8_t)(mark - 1);
    health_mark(g.health_shown, HEALTH_MARK_LOST);
}

/* a value's two hex digits as big digits at byte offset `top` (the tiles'
   low bytes alone); the value kept as shown */
static void counter_show(uint8_t value, uint8_t *shown, unsigned top)
{
    uint8_t hi = (uint8_t)((value >> 4) + DIGIT_TILE_0), lo = (uint8_t)((value & 15) + DIGIT_TILE_0);

    *shown = value;
    low_byte_at(top, hi);
    low_byte_at(top + ROW, (uint8_t)(hi | 0x10));
    low_byte_at(top + 2, lo);
    low_byte_at(top + 2 + ROW, (uint8_t)(lo | 0x10));
    g.text_map_upload = 1;
}

/*
 * @routine 81:F4F6 HudCount1Show
 * WHAT IT DOES: Draws the apples, two decimal digits, and keeps them as shown.
 */
void HudCount1Show(void)
{
    counter_show(g.apples, &g.apples_shown, 178);
}

/*
 * @routine 81:F521 HudCount2Show
 * WHAT IT DOES: Draws the second counter four columns to the right, and
 * keeps it as shown.
 */
void HudCount2Show(void)
{
    counter_show(g.hud_count_2, &g.hud_count_2_shown, 186);
}

/*
 * @routine 81:F47F HudDigitRefresh
 * WHAT IT DOES: Draws the lives digit when the lives differ from those
 * shown: the lives less 1, 0 for 0, 9 at most, the lives held at 99 from
 * 101 up; negative lives get a mark beside the digit (and the digit is
 * their low nibble).
 */
void HudDigitRefresh(void)
{
    uint8_t lives = g.lives, digit;

    if (lives == g.lives_shown)
        return;
    g.lives_shown = lives;
    if (lives & 0x80) {
        low_byte_at(264, 0x2E);
        digit = lives;
    } else {
        digit = (uint8_t)(lives - 1);
        if (digit & 0x80) {
            digit = 0;
        } else if (digit >= 9) {
            if (digit >= 0x64)
                g.lives = g.lives_shown = 0x63;
            digit = 9;
        }
    }
    digit = (uint8_t)((digit & 15) + DIGIT_TILE_0);
    low_byte_at(198, digit);
    low_byte_at(262, (uint8_t)(digit | 0x10));
    g.text_map_upload = 1;
}

/*
 * @routine 81:F54C HudMarkRefresh
 * WHAT IT DOES: Draws the status line's 2 x 2 hover mark when g.hover_on,
 * else its blanks, and keeps the value as shown.
 */
void HudMarkRefresh(void)
{
    static const uint16_t places[4] = { 0xAA, 0xAC, 0xEA, 0xEC };
    static const uint16_t mark[4] = { 0x2462, 0x2463, 0x2472, 0x2473 };

    g.hover_shown = g.hover_on;
    for (int i = 0; i < 4; i++)
        word_at(places[i], g.hover_on ? mark[i] : 0x2020);
    g.text_map_upload++;
}

/*
 * @routine 81:F456 HudRefresh
 * WHAT IT DOES: Each frame, brings the status line up to the game: the
 * hover mark, the two counters, then one health mark (lost when the health
 * shown is above the health, gained when below), or else the lives digit.
 * HOW: The gained mark's code (81:F4C8) runs on inside HudHealthMarkShow;
 * here it is health_mark with the gained tile.
 */
void HudRefresh(void)
{
    uint8_t shown;

    if (g.hover_on != g.hover_shown)
        HudMarkRefresh();
    if (g.apples != g.apples_shown)
        HudCount1Show();
    if (g.hud_count_2 != g.hud_count_2_shown)
        HudCount2Show();
    shown = g.health_shown;
    if (shown == g.health) {
        HudDigitRefresh();
    } else if (shown > g.health) {
        HudHealthMarkShow(shown);
    } else {
        g.health_shown = (uint8_t)(shown + 1);
        health_mark(shown, HEALTH_MARK_GAINED);
    }
}

/*
 * @routine 81:F433 HudRedrawAll
 * WHAT IT DOES: Draws the status line afresh: its text (layout 0), every
 * health mark place from the last to the first, the shown values zeroed,
 * the lives digit, then HudRefresh.
 */
void HudRedrawAll(void)
{
    uint8_t mark = g.health_marks;

    TextLayoutShow(0);
    do
        HudHealthMarkShow(mark);
    while (--mark);
    g.health_shown = 0;
    g.lives_shown = 0;
    g.hover_shown = 0;
    g.apples_shown = 0;
    g.hud_count_2_shown = 0;
    HudDigitRefresh();
    HudRefresh();
}

/*
 * @routine 81:F5CC HudDigitPut
 * @routine 81:F597 HudSixDigitsShow
 * Dead: no long pointer to 81:F597 and no word $F597 in bank $81 (the
 * record: "No script runs it"); HudDigitPut is only its.
 */
