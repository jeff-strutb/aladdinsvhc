/*
 * menu.c: the pause menu (menu.h).
 *
 * Items: the twelve pad buttons (each shows its key, or its controller button when a controller was the last used; chosen,
 * it asks for the next key or controller button, which it takes, the button that had it taking the old one), RESUME,
 * DEFAULTS (the game's own keys and buttons), FULL SCREEN (full screen or a window, kept in the settings) and QUIT (asks
 * once more, saves the settings and ends the run through the engine). The pad's directions move, B or A chooses, Start or
 * Escape resumes (Escape, or a click, cancels a key being asked for); a click of the pointer chooses what it is on and
 * presses it. Buttons held as the menu shuts reach the game only once let go.
 *
 * The pictures are drawn once into images the engine lays over the window (menu_art.c): the panel at a whole number of
 * image pixels to a game pixel, kept for each; the changing parts (tags, items, hand, ring) redrawn when they change.
 */
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "audio/sound.h"
#include "data/level_tables.h"
#include "input/controls.h"
#include "nshvc/nshvc.h"
#include "pause/menu.h"
#include "pause/menu_art.h"
#include "pause/save_screen.h"
#include "pause/slot_grid.h"
#include "system/screen_mode.h"
#include "system/settings.h"
#include "system/state.h"
#include "system/system.h"

#define KEY_ESCAPE 0x1B
#define MAX_D 6                         /* the most image pixels to a game pixel (menu_art_layout) */
#define TINT 0xFF000000u
#define TINT_ALPHA 0.62f
enum { O_TINT, O_PANEL, O_TAGS };       /* the engine's overlay numbers the menu uses */

#define SOUND_MOVE 0x0C                 /* the options screen's cursor */
#define SOUND_OPEN 0x0E                 /* the original's pause sound */
#define LEVEL_NO_PAUSE_SOUND 0x12
#define COOLDOWN 0x10                   /* the original's, before Start can pause again */

static struct {
    int drawn;                          /* the overlays are set (menu_draw) */
    int open, requested, chosen, asking, quit_asked;
    int capture_wait;                   /* updates until the save screen's picture is taken (see open_menu) */
    uint16_t last;                      /* pad 1's buttons at the last update */
    int pointer_down;
    float ox, oy, scale;                /* where the panel lies in the window, and its window pixels a game pixel */
    nshvc_image tint, panel[MAX_D + 1], dyn[MAX_D + 1];
    uint32_t *buf;                      /* the dynamic picture being drawn */
    int shown_d;
    MenuView shown;                     /* what the dynamic image shows now; shown_d 0: none */
} m = { .asking = -1 };

static void sound(unsigned char c) { SoundQueueAdd(c); }

/* ---- the picture ---- */

/* what the button does in the game: the four actions through the buttons the options screen gives them, and the rest */
static const char *action_name(uint16_t button)
{
    static const char *const actions[4] = { "JUMP", "DASH", "THROW", "HOVER" };
    unsigned k;

    for (k = 0; k < 4; k++)
        if (control_option_mask(g.kept_options[0], k) & button)
            return actions[k];
    switch (button) {
    case PAD_UP: return "LOOK UP";
    case PAD_DOWN: return "CROUCH";
    case PAD_LEFT: return "GO LEFT";
    case PAD_RIGHT: return "GO RIGHT";
    case PAD_START: return "MENU";
    default: return "UNUSED";
    }
}

static void labels(MenuView *v)
{
    int b, i, controller = controls_source() == CONTROLS_CONTROLLER;

    for (i = 0; i < MENU_TAGS; i++) {
        const char *s;
        size_t n;

        b = menu_tags[i].button;
        strncpy(v->action[i], action_name(controls_buttons[b]), sizeof v->action[i] - 1);
        s = controller ? nshvc_controller_label(0, controls_ctl(b)) : nshvc_key_name((nshvc_key)controls_key(b));
        if (!s || !*s)
            s = "-";
        for (n = 0; n < sizeof v->label[i] - 1 && n < 8 && s[n]; n++)
            v->label[i][n] = s[n] >= 'a' && s[n] <= 'z' ? (char)(s[n] - 32) : s[n];
        v->label[i][n] = 0;
    }
}

static void overlays_off(void)
{
    nshvc_image none = { 0 };
    uint32_t i;

    for (i = O_TINT; i <= O_PANEL + 1; i++)
        nshvc_overlay_set(i, none, NULL, 0);
}

/* In draw (not in update: the picture a draw shows may be an update behind the game's state, and the menu's images and
   overlays are its own, set here, outside the game's registered state): the menu laid over the window as it now is. */
void menu_draw(void)
{
    nshvc_display disp = nshvc_display_info();
    float l = disp.safe_left, t = disp.safe_top, w = (float)disp.width - disp.safe_left - disp.safe_right;
    float h = (float)disp.height - disp.safe_top - disp.safe_bottom, s, c[6];
    int d;
    MenuView v;

    if (slot_grid_is_open()) {           /* the save screen is over it */
        if (m.drawn) {
            overlays_off();
            m.drawn = 0;
        }
        return;
    }
    if (!m.open || m.capture_wait) {
        if (m.drawn) {
            overlays_off();
            m.drawn = 0;
        }
        return;
    }
    if (w <= 0 || h <= 0) {
        overlays_off();
        m.drawn = 0;
        return;
    }
    m.drawn = 1;
    menu_art_layout(l, t, w, h, &s, &d, &m.ox, &m.oy);
    m.scale = s;

    memset(&v, 0, sizeof v);
    v.chosen = m.chosen;
    v.asking = m.asking;
    v.quit_asked = m.quit_asked;
    v.full_offered = screen_mode_can_switch();
    v.full_on = screen_mode_full();
    labels(&v);

    if (!m.tint.id) {
        static const uint32_t black[4] = { TINT, TINT, TINT, TINT };

        m.tint = nshvc_image_create(2, 2, NSHVC_IMAGE_RGBA);
        nshvc_image_write(m.tint, black);
    }
    if (!m.panel[d].id) {
        uint32_t *px = calloc((size_t)MENU_W * d * (size_t)MENU_H * d, 4);

        if (!px)
            return;
        menu_art_static(px, d);
        m.panel[d] = nshvc_image_create((uint32_t)(MENU_W * d), (uint32_t)(MENU_H * d), NSHVC_IMAGE_RGBA);
        nshvc_image_write(m.panel[d], px);
        free(px);
        m.dyn[d] = nshvc_image_create((uint32_t)(MENU_W * d), (uint32_t)(MENU_H * d), NSHVC_IMAGE_RGBA);
    }
    if (m.shown_d != d || memcmp(&m.shown, &v, sizeof v) != 0) {
        uint32_t *px = calloc((size_t)MENU_W * d * (size_t)MENU_H * d, 4);

        if (!px)
            return;
        menu_art_dynamic(px, d, &v);
        nshvc_image_write(m.dyn[d], px);
        free(px);
        m.shown = v;
        m.shown_d = d;
    }
    c[0] = 0, c[1] = 0, c[2] = (float)disp.width, c[3] = 0, c[4] = 0, c[5] = (float)disp.height;
    nshvc_overlay_set(O_TINT, m.tint, c, TINT_ALPHA);
    c[0] = m.ox, c[1] = m.oy, c[2] = m.ox + MENU_W * s, c[3] = m.oy, c[4] = m.ox, c[5] = m.oy + MENU_H * s;
    nshvc_overlay_set(O_PANEL, m.panel[d], c, 1);
    nshvc_overlay_set(O_TAGS, m.dyn[d], c, 1);
}

/* ---- moving among the items ---- */

static int move(int i, uint16_t dir)
{
    int full = screen_mode_can_switch();

    if (i < MENU_TAGS) {
        int right = menu_tags[i].right, row = menu_tags[i].row, j;

        if (dir & PAD_UP)
            row -= row > 0;
        else if (dir & PAD_DOWN) {
            if (row == 5)
                return right ? MENU_DEFAULTS : MENU_RESUME;
            row++;
        } else
            right = !right;
        for (j = 0; j < MENU_TAGS; j++)
            if (menu_tags[j].right == right && menu_tags[j].row == row)
                return j;
        return i;
    }
    {
        /* RESUME DEFAULTS / SAVE MENU FULL SCREEN / QUIT: the neighbour in the direction, FULL SCREEN skipped where not offered */
        static const int col[MENU_ITEMS - MENU_TAGS] = { 0, 1, 0, 1, 0 }, row[MENU_ITEMS - MENU_TAGS] = { 0, 0, 1, 1, 2 };
        int c = col[i - MENU_TAGS], r = row[i - MENU_TAGS], j, best = i;

        if (dir & PAD_UP && r == 0) {
            for (j = 0; j < MENU_TAGS; j++)
                if (menu_tags[j].row == 5 && menu_tags[j].right == c)
                    return j;
            return i;
        }
        if (dir & PAD_UP) r--;
        else if (dir & PAD_DOWN) r++;
        else c = !c;
        for (j = MENU_TAGS; j < MENU_ITEMS; j++) {
            if (j == MENU_FULL && !full)
                continue;
            if (row[j - MENU_TAGS] == r && col[j - MENU_TAGS] == c)
                return j;
            if (row[j - MENU_TAGS] == r && (dir & (PAD_UP | PAD_DOWN)) && best == i)
                best = j;
        }
        return best;
    }
}

/* ---- opening, shutting, choosing ---- */

static void open_menu(void)
{
    /* The saves' picture is the game as the real world stands. The picture on show when the menu opens is the runahead's ahead
       world (one update on) eased with the real one; only after two held updates does `between` hold the real world in both
       of its worlds, so the picture is taken in the second update after this one, and the menu's overlays are held back until
       then (menu_draw), so the picture read has no menu over it. */
    m.capture_wait = 2;
    m.open = 1;
    m.requested = 0;
    m.chosen = MENU_RESUME;
    m.asking = -1;
    m.quit_asked = 0;
    m.shown_d = 0;
    if (g.level != LEVEL_NO_PAUSE_SOUND)
        sound(SOUND_OPEN);
    sound(SOUND_MUSIC_PAUSE);
}

static void shut(uint16_t held)
{
    m.open = 0;
    m.asking = -1;
    m.quit_asked = 0;
    controls_swallow(held);
    g.pause_cooldown = COOLDOWN;
    sound(SOUND_MUSIC_GO_ON);
}

/* the first key gone down this update that may take a button; 0 none */
static int key_taken(const nshvc_input *in)
{
    int vk;

    for (vk = 1; vk < 256; vk++)
        if (NSHVC_KEY_PRESSED(in, vk) && !controls_key_reserved(vk))
            return vk;
    return 0;
}

static uint16_t ctl_taken(const nshvc_input *in, uint16_t *last)
{
    uint16_t now = in->controller[0].on ? in->controller[0].buttons : 0, press = (uint16_t)(now & ~*last);

    *last = now;
    return (uint16_t)(press & (uint16_t)-press);
}

/* returns 1 when the menu is shut by it */
static int activate(int item, uint16_t held)
{
    if (item != MENU_QUIT)
        m.quit_asked = 0;
    switch (item) {
    case MENU_RESUME:
        shut(held);
        return 1;
    case MENU_DEFAULTS:
        controls_defaults();
        settings_save();
        break;
    case MENU_SAVE:
        if (m.capture_wait) {           /* chosen before the picture was taken (a click): taken now */
            m.capture_wait = 0;
            save_screen_capture();
        }
        save_screen_open(SAVE_FROM_GAME);
        break;
    case MENU_FULL:
        screen_mode_set(!screen_mode_full());
        break;
    case MENU_QUIT:
        if (!m.quit_asked) {
            m.quit_asked = 1;
            break;
        }
        settings_save();
        nshvc_quit();
        break;
    default:
        m.asking = item;
        break;
    }
    return 0;
}

void menu_request(void) { m.requested = 1; }

int menu_update(const nshvc_input *in)
{
    static uint16_t ctl_last;
    uint16_t raw = controls_pad_raw(in), press = (uint16_t)((raw & ~m.last) | controls_pressed_keys(in)), held = raw;
    int esc = NSHVC_KEY_PRESSED(in, KEY_ESCAPE) != 0, click = 0, item = -1;

    m.last = raw;
    if (m.open && m.capture_wait && --m.capture_wait == 0)
        save_screen_capture();
    if (save_screen_requested() && !slot_grid_is_open())
        save_screen_open(SAVE_FROM_TITLE);
    if (slot_grid_is_open()) {          /* the save screen: the game is held; the menu stays as it was under it */
        int r = save_screen_update(in);

        if (r == SAVE_SCREEN_LOADED) {  /* the game is the one saved: the menu is shut without the pause's cooldown */
            m.open = 0;
            m.asking = -1;
            m.quit_asked = 0;
            controls_swallow(held);
            sound(SOUND_MUSIC_GO_ON);
            return 1;                   /* a load is the last thing its update does (snapshot.h) */
        } else if (r == SAVE_SCREEN_BACK) {
            controls_swallow(held);     /* what shut it must not reach the game or open it again */
            if (m.open)
                m.chosen = MENU_SAVE;
        }
        SoundCommandSend();
        return 1;
    }
    if (!m.open) {
        ctl_last = in->controller[0].on ? in->controller[0].buttons : 0;
        m.pointer_down = (in->pointer.buttons & 1) != 0;
        if (!esc && !m.requested)
            return 0;
        open_menu();
        SoundCommandSend();
        return 1;
    }
    if (in->pointer.inside && (in->pointer.buttons & 1) && !m.pointer_down && m.scale > 0) {
        click = 1;
        item = menu_art_item_at((in->pointer.x - m.ox) / m.scale, (in->pointer.y - m.oy) / m.scale, screen_mode_can_switch());
    }
    m.pointer_down = (in->pointer.buttons & 1) != 0;

    if (m.asking >= 0) {
        int vk;
        uint16_t ctl = ctl_taken(in, &ctl_last);

        if (esc)
            m.asking = -1;
        else if (click)
            m.asking = -1;
        else if ((vk = key_taken(in)) != 0) {
            controls_rebind_key(menu_tags[m.asking].button, vk);
            settings_save();
            m.asking = -1;
        } else if (ctl) {
            controls_rebind_ctl(menu_tags[m.asking].button, ctl);
            settings_save();
            m.asking = -1;
        }
        if (!click || item < 0) {
            SoundCommandSend();
            return 1;
        }
    } else {
        ctl_last = in->controller[0].on ? in->controller[0].buttons : 0;
        if (esc || (press & PAD_START)) {
            shut(held);
            SoundCommandSend();
            return 1;
        }
    }
    if (item >= 0) {
        if (item != m.chosen)
            m.quit_asked = 0;
        m.chosen = item;
        sound(SOUND_MOVE);
        if (activate(item, held)) {
            SoundCommandSend();
            return 1;
        }
    } else if (press & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT)) {
        int to = move(m.chosen, (uint16_t)(press & (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT)));

        if (to != m.chosen)
            sound(SOUND_MOVE);
        m.chosen = to;
        m.quit_asked = 0;
    } else if (press & (PAD_B | PAD_A)) {
        sound(SOUND_MOVE);
        if (activate(m.chosen, held)) {
            SoundCommandSend();
            return 1;
        }
    }
    SoundCommandSend();
    return 1;
}
