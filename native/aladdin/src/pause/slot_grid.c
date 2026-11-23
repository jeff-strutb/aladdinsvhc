/*
 * slot_grid.c: the grid of cells (slot_grid.h).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio/sound.h"
#include "input/controls.h"
#include "pause/menu_art.h"
#include "pause/slot_grid.h"
#include "system/screen_mode.h"
#include "system/system.h"

#define KEY_ESCAPE 0x1B
#define MAX_D 6
#define TINT 0xFF000000u
#define TINT_ALPHA 0.62f
#define SOUND_MOVE 0x0C
enum { O_TINT = 3, O_PANEL, O_DYN };    /* overlay numbers (the pause menu uses 0 to 2) */

#define TOP 34                          /* the cells' area: y 34 to 244 */
#define AREA_H 210
#define SIDE 8
#define HINT_Y 246
#define CHOICE_Y 258
#define PAGE_Y 262
#define CAPTION_H 22                    /* the gap under a picture and its two caption lines */
#define BORDER 4

typedef struct {
    int sel, asking, answer, rev;
    char line[40];
    char choices[3][24];
} View;

static struct {
    int open, fresh, drawn;
    SlotGridSpec spec;
    View v;
    int rev;
    int n_choices;
    uint16_t last;
    int pointer_down;
    float ox, oy, scale;
    nshvc_image tint, panel[MAX_D + 1], dyn[MAX_D + 1];
    int panel_for[MAX_D + 1];           /* the panel image's title (a hash), to redraw it when the title changes */
    View shown;
    int shown_d, shown_rev;
    char shown_title[32];
} m;

static int per_page(void) { return m.spec.cols * m.spec.rows; }
static int page(void) { return m.v.sel / per_page(); }

/* cell place n of the page: the frame's box in game pixels */
static void box_of(int n, int *x, int *y, int *w, int *h)
{
    int cw = m.spec.pic_w + 2 * BORDER, ch = m.spec.pic_h + 2 * BORDER, rh = ch + CAPTION_H;
    int col = n % m.spec.cols, row = n / m.spec.cols;
    int gx = (MENU_W - 2 * SIDE - m.spec.cols * cw) / (m.spec.cols + 1);
    int gy = (AREA_H - m.spec.rows * rh) / (m.spec.rows + 1);

    if (gx < 0) gx = 0;
    if (gy < 0) gy = 0;
    *x = SIDE + gx + col * (cw + gx);
    *y = TOP + gy + row * (rh + gy);
    *w = cw;
    *h = ch;
}

static void sound(void) { SoundQueueAdd(SOUND_MOVE); }

void slot_grid_open(const SlotGridSpec *spec)
{
    memset(&m.v, 0, sizeof m.v);
    m.spec = *spec;
    m.open = 1;
    m.fresh = 1;
    m.v.sel = 0;
    m.shown_d = 0;
    memset(m.panel_for, 0, sizeof m.panel_for);
}

void slot_grid_close(void) { m.open = 0; }
int slot_grid_is_open(void) { return m.open; }
void slot_grid_changed(void) { m.rev++; }
int slot_grid_asking(void) { return m.v.asking; }
int slot_grid_selected(void) { return m.v.sel; }

void slot_grid_ask(const char *line, const char *const *choices, int n)
{
    int i;

    if (n < 1) n = 1;
    if (n > 3) n = 3;
    m.v.asking = 1;
    m.v.answer = 0;
    m.n_choices = n;
    strncpy(m.v.line, line, sizeof m.v.line - 1);
    m.v.line[sizeof m.v.line - 1] = 0;
    for (i = 0; i < n; i++) {
        strncpy(m.v.choices[i], choices[i], sizeof m.v.choices[i] - 1);
        m.v.choices[i][sizeof m.v.choices[i] - 1] = 0;
    }
}

/* ---- the picture ---- */

static void caption(uint32_t *px, int d, int x, int y, int w, const char *s, int pal)
{
    int tw = menu_art_text_w(s);

    menu_art_text(px, d, s, pal, x + (w - tw) / 2, y, 1);
}

static void choices_x(int *xs, int *total)
{
    int i, x = 0;

    for (i = 0; i < m.n_choices; i++) {
        xs[i] = x;
        x += menu_art_text_w(m.v.choices[i]) + 20 + 16;
    }
    *total = x - 16;
}

static void draw_dynamic(uint32_t *px, int d)
{
    int per = per_page(), first = page() * per, n, i;
    char buf[40];

    for (n = 0; n < per; n++) {
        const SlotCell *c = m.spec.cell(m.spec.ctx, first + n);
        int x, y, w, h, sel = first + n == m.v.sel;

        box_of(n, &x, &y, &w, &h);
        menu_art_box(px, d, x, y, w, h, sel);
        if (c->used && c->picture)
            menu_art_blit(px, d, x + BORDER, y + BORDER, c->picture, m.spec.pic_w, m.spec.pic_h);
        else
            caption(px, d, x, y + h / 2 - 4, w, c->used ? "NO PICTURE" : "EMPTY", sel ? MENU_PAL_CREAM : MENU_PAL_GOLD);
        caption(px, d, x - 8, y + h + 3, w + 16, c->line1, sel ? MENU_PAL_CREAM : MENU_PAL_GOLD);
        caption(px, d, x - 8, y + h + 12, w + 16, c->line2, sel ? MENU_PAL_CREAM : MENU_PAL_GOLD);
    }
    if (m.v.asking) {
        int xs[3], total, left;

        caption(px, d, 0, HINT_Y, MENU_W, m.v.line, MENU_PAL_PINK);
        choices_x(xs, &total);
        left = (MENU_W - total) / 2;
        for (i = 0; i < m.n_choices; i++) {
            int sel = i == m.v.answer;

            if (sel) {                  /* the chosen answer is in brackets (and cream) */
                menu_art_text(px, d, "(", MENU_PAL_CREAM, left + xs[i] - 10, CHOICE_Y, 1);
                menu_art_text(px, d, ")", MENU_PAL_CREAM, left + xs[i] + menu_art_text_w(m.v.choices[i]) + 2, CHOICE_Y, 1);
            }
            menu_art_text(px, d, m.v.choices[i], sel ? MENU_PAL_CREAM : MENU_PAL_GOLD, left + xs[i], CHOICE_Y, 1);
        }
    } else {
        caption(px, d, 0, HINT_Y, MENU_W, m.spec.start_chooses ? "B OR START CHOOSE   SELECT OR ESC BACK" : "B OR A CHOOSE   START OR ESC BACK", MENU_PAL_GOLD);
        if (m.spec.pages > 1) {
            sprintf(buf, "L  PAGE %d OF %d  R", page() + 1, m.spec.pages);
            caption(px, d, 0, PAGE_Y, MENU_W, buf, MENU_PAL_CREAM);
        }
    }
}

void slot_grid_render(uint32_t *px, int d)
{
    menu_art_panel(px, d, m.spec.title);
    draw_dynamic(px, d);
}

void slot_grid_draw(void)
{
    nshvc_display disp = nshvc_display_info();
    float l = disp.safe_left, t = disp.safe_top, w = (float)disp.width - disp.safe_left - disp.safe_right;
    float h = (float)disp.height - disp.safe_top - disp.safe_bottom, s, c[6];
    int d;

    if (!m.open || w <= 0 || h <= 0) {
        if (m.drawn) {
            nshvc_image none = { 0 };
            uint32_t i;

            for (i = O_TINT; i <= O_DYN; i++)
                nshvc_overlay_set(i, none, NULL, 0);
            m.drawn = 0;
        }
        return;
    }
    m.drawn = 1;
    menu_art_layout(l, t, w, h, &s, &d, &m.ox, &m.oy);
    m.scale = s;
    if (!m.tint.id) {
        static const uint32_t black[4] = { TINT, TINT, TINT, TINT };

        m.tint = nshvc_image_create(2, 2, NSHVC_IMAGE_RGBA);
        nshvc_image_write(m.tint, black);
    }
    if (!m.panel[d].id) {
        m.panel[d] = nshvc_image_create((uint32_t)(MENU_W * d), (uint32_t)(MENU_H * d), NSHVC_IMAGE_RGBA);
        m.dyn[d] = nshvc_image_create((uint32_t)(MENU_W * d), (uint32_t)(MENU_H * d), NSHVC_IMAGE_RGBA);
    }
    if (!m.panel_for[d] || strcmp(m.shown_title, m.spec.title) != 0) {
        uint32_t *px = calloc((size_t)MENU_W * d * (size_t)MENU_H * d, 4);

        if (!px)
            return;
        menu_art_panel(px, d, m.spec.title);
        nshvc_image_write(m.panel[d], px);
        free(px);
        m.panel_for[d] = 1;
        strncpy(m.shown_title, m.spec.title, sizeof m.shown_title - 1);
        m.shown_d = 0;
    }
    if (m.shown_d != d || m.shown_rev != m.rev || memcmp(&m.shown, &m.v, sizeof m.v) != 0) {
        uint32_t *px = calloc((size_t)MENU_W * d * (size_t)MENU_H * d, 4);

        if (!px)
            return;
        draw_dynamic(px, d);
        nshvc_image_write(m.dyn[d], px);
        free(px);
        m.shown = m.v;
        m.shown_d = d;
        m.shown_rev = m.rev;
    }
    c[0] = 0, c[1] = 0, c[2] = (float)disp.width, c[3] = 0, c[4] = 0, c[5] = (float)disp.height;
    nshvc_overlay_set(O_TINT, m.tint, c, TINT_ALPHA);
    c[0] = m.ox, c[1] = m.oy, c[2] = m.ox + MENU_W * s, c[3] = m.oy, c[4] = m.ox, c[5] = m.oy + MENU_H * s;
    nshvc_overlay_set(O_PANEL, m.panel[d], c, 1);
    nshvc_overlay_set(O_DYN, m.dyn[d], c, 1);
}

/* ---- input ---- */

static void turn(int by)
{
    int per = per_page(), pg = (page() + by + m.spec.pages) % m.spec.pages, n = m.v.sel % per;

    m.v.sel = pg * per + n;
}

static int cell_at(float x, float y)
{
    int n, bx, by, bw, bh;

    for (n = 0; n < per_page(); n++) {
        box_of(n, &bx, &by, &bw, &bh);
        if (x >= bx && x < bx + bw && y >= by && y < by + bh + CAPTION_H - 2)
            return page() * per_page() + n;
    }
    return -1;
}

static SlotResult ev(SlotEvent what, int index, int answer)
{
    SlotResult r;

    r.what = what;
    r.index = index;
    r.answer = answer;
    return r;
}

SlotResult slot_grid_update(const nshvc_input *in)
{
    uint16_t raw = controls_pad_raw(in), press = (uint16_t)((raw & ~m.last) | controls_pressed_keys(in));
    int esc = NSHVC_KEY_PRESSED(in, KEY_ESCAPE) != 0, click = 0, cx = -1, cy = -1;
    uint16_t dir = (uint16_t)(press & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT));
    uint16_t choose = (uint16_t)(m.spec.start_chooses ? PAD_B | PAD_A | PAD_START : PAD_B | PAD_A);
    uint16_t back = m.spec.start_chooses ? PAD_SELECT : PAD_START;

    m.last = raw;
    if (!m.open)
        return ev(SLOT_NONE, -1, 0);
    if (in->pointer.inside && (in->pointer.buttons & 1) && !m.pointer_down && m.scale > 0) {
        click = 1;
        cx = (int)((in->pointer.x - m.ox) / m.scale);
        cy = (int)((in->pointer.y - m.oy) / m.scale);
    }
    m.pointer_down = (in->pointer.buttons & 1) != 0;
    if (m.fresh) {                      /* what opened it is still down: it is not a press here */
        m.fresh = 0;
        return ev(SLOT_NONE, -1, 0);
    }

    if (m.v.asking) {
        if (esc || (press & back)) {
            m.v.asking = 0;
            return ev(SLOT_NONE, -1, 0);
        }
        if (click) {
            int xs[3], total, left, i;

            choices_x(xs, &total);
            left = (MENU_W - total) / 2;
            for (i = 0; i < m.n_choices; i++)
                if (cx >= left + xs[i] - 10 && cx < left + xs[i] + menu_art_text_w(m.v.choices[i]) && cy >= CHOICE_Y - 2 && cy < CHOICE_Y + 10) {
                    m.v.asking = 0;
                    return ev(SLOT_ANSWER, m.v.sel, i);
                }
            m.v.asking = 0;
            return ev(SLOT_NONE, -1, 0);
        }
        if (dir & PAD_LEFT && m.v.answer > 0) { m.v.answer--; sound(); }
        else if (dir & PAD_RIGHT && m.v.answer < m.n_choices - 1) { m.v.answer++; sound(); }
        else if (press & choose) {
            int a = m.v.answer;

            m.v.asking = 0;
            sound();
            return ev(SLOT_ANSWER, m.v.sel, a);
        }
        return ev(SLOT_NONE, -1, 0);
    }
    if (esc || (press & back))
        return ev(SLOT_BACK, -1, 0);
    if (click) {
        int c = cell_at((float)cx, (float)cy);

        if (c >= 0) {
            m.v.sel = c;
            sound();
            return ev(SLOT_CHOSEN, c, 0);
        }
        if (m.spec.pages > 1 && cy >= PAGE_Y - 4 && cy < PAGE_Y + 12) {
            if (cx < MENU_W / 2 - 40) { turn(-1); sound(); }
            else if (cx >= MENU_W / 2 + 40) { turn(1); sound(); }
        }
        return ev(SLOT_NONE, -1, 0);
    }
    if (press & PAD_L && m.spec.pages > 1) { turn(-1); sound(); }
    else if (press & PAD_R && m.spec.pages > 1) { turn(1); sound(); }
    else if (dir) {
        int n = m.v.sel % per_page(), col = n % m.spec.cols, row = n / m.spec.cols;

        if (dir & PAD_LEFT) {
            if (col > 0) m.v.sel--;
            else if (m.spec.pages > 1) { turn(-1); m.v.sel += m.spec.cols - 1; }
            else return ev(SLOT_NONE, -1, 0);
        } else if (dir & PAD_RIGHT) {
            if (col < m.spec.cols - 1) m.v.sel++;
            else if (m.spec.pages > 1) { turn(1); m.v.sel -= m.spec.cols - 1; }
            else return ev(SLOT_NONE, -1, 0);
        } else if (dir & PAD_UP) {
            if (row == 0) return ev(SLOT_NONE, -1, 0);
            m.v.sel -= m.spec.cols;
        } else {
            if (row == m.spec.rows - 1) return ev(SLOT_NONE, -1, 0);
            m.v.sel += m.spec.cols;
        }
        sound();
    } else if (press & choose) {
        sound();
        return ev(SLOT_CHOSEN, m.v.sel, 0);
    }
    return ev(SLOT_NONE, -1, 0);
}

/* for the checks: the cell chosen without a key */
void slot_grid_select_for_check(int index) { m.v.sel = index; }
