/*
 * hud_layer.c: the text layer (BG3) in a level, where the view is wider than
 * the original's 256 columns (docs/aladdin/native/VIEW.md 6.2).
 *
 * The text map is 32 x 32 tiles and wraps, so in a wider view it would show
 * again beside itself, and the status line's two groups would be in the
 * middle of the view, not at its corners. In a level the layer is drawn
 * from a map of its own that holds three pages of the text map's tiles:
 *   - the left page: the cells of the left group's rectangle (columns 0-9,
 *     rows 1-4: the original's x 0-79, y 8-39, which holds the group at x 8-79);
 *   - the middle page: every other cell, at the original's places, which the
 *     view shows in its middle 256 columns: the pause text and the messages;
 *   - the right page: the cells of the right group's rectangle (columns 21-31,
 *     rows 2-3: x 168-255, y 16-31, which holds the group at x 168-247).
 * The pages are far apart in the map (the layer does not wrap), so that a
 * page is shown only where a line says. Each line of the left rectangle's rows
 * is cut by the engine's split inside a line (nshvc_line_from): the view's
 * first 80 columns take the left page, the columns from there on the middle
 * page, and in the right rectangle's rows the last 88 columns the right page.
 * The left group is then at the view's left edge, at the original's distance
 * from it, and the right group at its right edge; a 256-wide view shows what
 * the original's single map showed, cell for cell.
 *
 * The map is made from every write to the text map's video addresses
 * (write_words in video.c), in a level or not, so it is always the text
 * map's; video_draw uses it only in a level (video_level_view).
 *
 * The two groups are at the corners of a band no wider than 16:9 of the
 * picture's shown height (Jeff, 2026-11-22: so that a very wide view does not
 * put them at the screen's edges), centred in the view, and never narrower
 * than the original's 256 columns: at 256 x 224 the band is the view.
 *
 * Later refinement (VIEW.md 6.2): the groups are put at the display's safe
 * area (nshvc_display_info) and not at the view's own corners.
 */
#include "nshvc/nshvc.h"
#include "video/between.h"
#include "video/video_internal.h"
#include "video/view.h"

#define TEXT_COLUMNS 32
#define TEXT_ROWS 32
#define BLANK 0x0020                /* the blank tile, the layouts' (text.c) */

#define MID_COLUMN 256              /* the middle page's first column in the map; the left page is at 0 */
#define RIGHT_COLUMN 544            /* the right page's */
#define MAP_COLUMNS (RIGHT_COLUMN + TEXT_COLUMNS)

#define LEFT_LAST_COLUMN 9          /* the left rectangle: columns 0 to this, rows LEFT_ROW_FIRST to LEFT_ROW_LAST */
#define LEFT_ROW_FIRST 1
#define LEFT_ROW_LAST 4
#define RIGHT_FIRST_COLUMN 21       /* the right rectangle: this column to 31, rows RIGHT_ROW_FIRST to RIGHT_ROW_LAST */
#define RIGHT_ROW_FIRST 2
#define RIGHT_ROW_LAST 3

#define LEFT_COLUMNS_SHOWN 80       /* the view's first columns that take the left page (the left rectangle's 10 columns) */
#define RIGHT_COLUMNS_SHOWN 88      /* the view's last columns that take the right page (the right rectangle's 11 columns) */
#define HUD_WIDEST_W 16              /* the band's widest shape, width over height: 16:9 */
#define HUD_WIDEST_H 9
#define LEFT_FIRST_LINE (8 * LEFT_ROW_FIRST)
#define LEFT_LAST_LINE (8 * LEFT_ROW_LAST + 7)
#define RIGHT_FIRST_LINE (8 * RIGHT_ROW_FIRST)
#define RIGHT_LAST_LINE (8 * RIGHT_ROW_LAST + 7)

static nshvc_map hud_map;
static int level_view;              /* a level is the screen */

void video_level_view_set(int on)
{
    level_view = on;
}

int video_level_view(void)
{
    return level_view;
}

static int in_left(unsigned x, unsigned y)
{
    return x <= LEFT_LAST_COLUMN && y >= LEFT_ROW_FIRST && y <= LEFT_ROW_LAST;
}

static int in_right(unsigned x, unsigned y)
{
    return x >= RIGHT_FIRST_COLUMN && y >= RIGHT_ROW_FIRST && y <= RIGHT_ROW_LAST;
}

void hud_layer_init(void)
{
    static uint16_t blank[MAP_COLUMNS * TEXT_ROWS];

    for (unsigned i = 0; i < MAP_COLUMNS * TEXT_ROWS; i++)
        blank[i] = BLANK;
    hud_map = nshvc_map_create(MAP_COLUMNS, TEXT_ROWS);
    between_map_made(hud_map, "map.hud", MAP_COLUMNS, TEXT_ROWS);
    between_map_write_snes(hud_map, 0, 0, MAP_COLUMNS, TEXT_ROWS, blank);
    between_keep(&level_view, sizeof level_view);       /* the picture's, kept as the update before's (between.h) */
}

/*
 * hud_layer_write: `n` cells of the text map's row `y` from column `x` on (a write to its video addresses): the
 * cells outside the two rectangles to the middle page, those inside to their own page, the middle page's
 * places for them left blank.
 */
void hud_layer_write(unsigned x, unsigned y, unsigned n, const uint16_t *words)
{
    uint16_t mid[TEXT_COLUMNS];

    for (unsigned i = 0; i < n; i++)
        mid[i] = in_left(x + i, y) || in_right(x + i, y) ? BLANK : words[i];
    between_map_write_snes(hud_map, MID_COLUMN + x, y, n, 1, mid);
    for (unsigned i = 0; i < n; i++) {
        unsigned from = i;

        if (in_left(x + i, y)) {
            while (i + 1 < n && in_left(x + i + 1, y))
                i++;
            between_map_write_snes(hud_map, x + from, y, i - from + 1, 1, words + from);
        } else if (in_right(x + i, y)) {
            while (i + 1 < n && in_right(x + i + 1, y))
                i++;
            between_map_write_snes(hud_map, RIGHT_COLUMN + x + from, y, i - from + 1, 1, words + from);
        }
    }
}

/*
 * hud_layer_set: BG3 in a level: its map the one above, not wrapping, its
 * scroll x showing the middle page in the view's middle 256 columns; and the
 * lines of the two rectangles cut so the corners show their pages (above).
 * `scroll_x` is the layer's own, which the corners' pages take as 0.
 */
void hud_layer_set(nshvc_layer *l, int32_t scroll_x)
{
    int32_t shown_h = view.h - video_cropped_rows();
    int32_t band = shown_h * HUD_WIDEST_W / HUD_WIDEST_H & ~1;
    int32_t inset, middle, left, right;
    uint32_t left_to, right_from;

    if (band < VIEW_ORIGINAL_W)
        band = VIEW_ORIGINAL_W;
    if (band > view.w)
        band = view.w;
    inset = (view.w - band) / 2;
    middle = scroll_x + 8 * MID_COLUMN - view.dx;
    left = scroll_x - inset;
    right = scroll_x + 8 * RIGHT_COLUMN + VIEW_ORIGINAL_W - (inset + band);
    left_to = (uint32_t)(inset + LEFT_COLUMNS_SHOWN);
    right_from = (uint32_t)(inset + band - RIGHT_COLUMNS_SHOWN);

    l->map = hud_map;
    l->wrap = 0;
    l->scroll_x = middle;
    for (int32_t line = LEFT_FIRST_LINE; line <= LEFT_LAST_LINE && line < view.h; line++) {
        if (inset)
            nshvc_line_from((uint32_t)line, 0, NSHVC_LINE_SCROLL_X + 2, middle);
        nshvc_line_from((uint32_t)line, inset, NSHVC_LINE_SCROLL_X + 2, left);
        nshvc_line_from((uint32_t)line, (int32_t)left_to, NSHVC_LINE_SCROLL_X + 2, middle);
        if (line >= RIGHT_FIRST_LINE && line <= RIGHT_LAST_LINE) {
            nshvc_line_from((uint32_t)line, (int32_t)right_from, NSHVC_LINE_SCROLL_X + 2, right);
            if (inset)
                nshvc_line_from((uint32_t)line, (int32_t)(inset + band), NSHVC_LINE_SCROLL_X + 2, middle);
        }
    }
}
