/*
 * parallax.c: the backdrop's scroll, from the play field's.
 *
 * Each frame the scroll update ends in the handler the level picks
 * (g.layer_setup_mode, the original's $083F) that sets the backdrop layer's
 * scroll from the play field's: at the same speed, at half speed, not at
 * all, or in horizontal bands at speeds of their own. The bands' tables and
 * their working values are in the per-line effects' data (g.line_data, the
 * original's $7E:F400-$F9FF), where the original keeps them: the table at
 * $F400 (ParallaxDrift's) or $F400 / $F480 (BandTable's, in turn), the
 * bands' scrolls at $F600 and BandTable's words at $F620-$F62D, the
 * drifting bands' positions at $F800 and speeds at $F900.
 */
#include <stdio.h>

#include "data/parallax_tables.h"
#include "system/state.h"
#include "video/parallax.h"
#include "video/video_internal.h"

/* the places in g.line_data (the original's $7E:F400 on) */
enum {
    BAND_HDMA = 0x000,          /* $F400: ParallaxDrift's table, 3 bytes an entry */
    BAND_SCROLLS = 0x200,       /* $F600: BandTable's bands' scrolls, a word each */
    BAND_LINES_LEFT = 0x220,    /* $F620: the screen's lines not yet given a band */
    BAND_LINES = 0x222,         /* $F622: this band's lines */
    BAND_LINE = 0x224,          /* $F624: the level's line at the screen's next line */
    BAND_SET = 0x226,           /* $F626: the band set, times 2 */
    BAND_COUNT = 0x228,         /* $F628: bands left to look at */
    BAND_EDGE = 0x22A,          /* $F62A: this band's index in the edges' table */
    BAND_AT = 0x22C,            /* $F62C: this band's index in the scrolls */
    BAND_POS = 0x400,           /* $F800: the drifting bands' positions, 4 bytes each (fraction, pixels, past) */
    BAND_SPEED = 0x500,         /* $F900: their speeds, 4 bytes apart (the low word added) */
    LINE_DATA_AT = 0xF400       /* where g.line_data starts in the original's bank $7E */
};

#define DRIFT_STEP 0x00C0       /* added to g.band_drift each frame (24 bits, from its fraction byte) */
#define STAGE1_X 0x0F10         /* PlayField3Stages: the player's x where stage 0 can end */
#define STAGE1_SCROLL 0x0F80    /* and the scroll x past which it ends at once */
#define WIDE_X 0x1000           /* past this scroll x the backdrop moves at half speed */

/* a word of the line data, low byte first; past its end, reported */
static uint16_t ld16(unsigned at)
{
    if (at + 1 >= LINE_DATA_SIZE) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: the parallax reads the line data at %04X, past it\n", at);
        }
        return 0;
    }
    return (uint16_t)(g.line_data[at] | g.line_data[at + 1] << 8);
}

static void st16(unsigned at, uint16_t v)
{
    if (at + 1 >= LINE_DATA_SIZE) {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: the parallax writes the line data at %04X, past it\n", at);
        }
        return;
    }
    g.line_data[at] = (uint8_t)v;
    g.line_data[at + 1] = (uint8_t)(v >> 8);
}

/* a 16.8 position's whole pixels (the original's word from its second
   byte), and those pixels set, its fraction byte kept */
static uint16_t pixels(uint32_t position)
{
    return (uint16_t)(position >> 8);
}

static void pixels_set(uint32_t *position, uint16_t v)
{
    *position = (*position & 0xFF0000FFu) | (uint32_t)v << 8;
}

#define PLAY (&g.layers[0])
#define BACK (&g.layers[1])

/*
 * @routine 81:C7A5 ParallaxNone
 * WHAT IT DOES: Leaves the backdrop where it is: for levels whose backdrop
 * does not scroll with the play field.
 */
static void parallax_none(void)
{
}

/*
 * @routine 81:C797 ParallaxHalf
 * WHAT IT DOES: Scrolls the backdrop at half the play field's speed, both
 * ways: its pixels the play field's halved.
 */
static void parallax_half(void)
{
    pixels_set(&BACK->scroll_x, (uint16_t)(pixels(PLAY->scroll_x) >> 1));
    pixels_set(&BACK->scroll_y, (uint16_t)(pixels(PLAY->scroll_y) >> 1));
}

/*
 * @routine 81:C7A6 ParallaxFull
 * WHAT IT DOES: Scrolls the backdrop with the play field, at its speed.
 */
static void parallax_full(void)
{
    pixels_set(&BACK->scroll_x, pixels(PLAY->scroll_x));
    pixels_set(&BACK->scroll_y, pixels(PLAY->scroll_y));
}

/*
 * @routine 81:C7B3 ParallaxHalfY
 * WHAT IT DOES: Scrolls the backdrop with the play field across and at half
 * its speed up and down.
 */
static void parallax_half_y(void)
{
    pixels_set(&BACK->scroll_x, pixels(PLAY->scroll_x));
    pixels_set(&BACK->scroll_y, (uint16_t)(pixels(PLAY->scroll_y) >> 1));
}

/*
 * @routine 81:C7C1 ParallaxHalfXY80
 * WHAT IT DOES: Scrolls the backdrop at half the play field's speed both
 * ways, $80 lower.
 */
static void parallax_half_xy80(void)
{
    pixels_set(&BACK->scroll_x, (uint16_t)(pixels(PLAY->scroll_x) >> 1));
    pixels_set(&BACK->scroll_y, (uint16_t)((pixels(PLAY->scroll_y) >> 1) + 0x80));
}

/*
 * @routine 81:C7D4 ParallaxHalfX
 * WHAT IT DOES: Scrolls the backdrop at half the play field's speed across
 * and at its full speed up and down.
 */
static void parallax_half_x(void)
{
    pixels_set(&BACK->scroll_x, (uint16_t)(pixels(PLAY->scroll_x) >> 1));
    pixels_set(&BACK->scroll_y, pixels(PLAY->scroll_y));
}

/*
 * @routine 81:C7E7 ParallaxDrift
 * WHAT IT DOES: Scrolls a backdrop of 11 horizontal bands, each drifting at
 * its own speed, the tenth fixed to the play field: each band's position
 * (fraction and pixels, 4 bytes) steps by its speed (the low word added,
 * the carry into the high); the tenth's pixels become the play field's
 * scroll x; each band's pixels go into the bands' table's values (3 bytes
 * an entry, the count left as the level set it), and the first band's into
 * BG2's x.
 */
static void parallax_drift(void)
{
    for (unsigned i = 0; i < 11; i++) {
        unsigned at = BAND_POS + 4 * i;
        uint32_t sum = (uint32_t)ld16(at) + ld16(BAND_SPEED + 4 * i);

        st16(at, (uint16_t)sum);
        st16(at + 2, (uint16_t)(ld16(at + 2) + (sum >> 16)));
    }
    st16(BAND_POS + 9 * 4 + 1, pixels(PLAY->scroll_x));
    for (unsigned i = 0; i < 11; i++)
        st16(BAND_HDMA + 3 * i + 1, ld16(BAND_POS + 4 * i + 1));
    pixels_set(&g.bg_x[1], ld16(BAND_POS + 1));
}

/* BAND_DRIFT stepped on: 24 bits from its fraction byte (the original adds
   two words, so its fourth byte takes the carry) */
static void drift_on(void)
{
    g.band_drift += DRIFT_STEP;
}

static uint16_t drift_pixels(void)
{
    return (uint16_t)(g.band_drift >> 8);
}

/*
 * @routine 81:C948 BandScrolls
 * WHAT IT DOES: Works out each backdrop band's scroll x for this frame, by
 * the band set: set 0 halves the scroll once more per band upward, with two
 * drifting bands of cloud on top; set 2 has four drifting bands that the
 * play field's moves right hold back, over two at half and three quarters
 * of its speed. Both step the drift on first (set 0 after its halving).
 * HOW: Set 0: the scrolls from the bottom band up, the scroll x shifted
 * right once more each; the top two the drift's pixels plus the scroll x,
 * shifted right once and twice. Set 2: the drift's pixels less the scroll
 * x's move right this frame (none if it moved left), plus the scroll x,
 * shifted right 0-3 times for the top four; then the scroll x halved, and
 * plus its quarter. Other sets: the original's jump table has none (its
 * third word is not code), reported.
 */
static void band_scrolls(uint16_t set, uint16_t count)
{
    uint16_t sx = pixels(PLAY->scroll_x), v, moved;

    if (set == 0) {
        v = sx;
        for (unsigned y = (unsigned)count * 2; y != 0; y -= 2) {
            v = (uint16_t)(v >> 1);
            st16(BAND_SCROLLS + y - 2, v);
        }
        drift_on();
        v = (uint16_t)((uint16_t)(drift_pixels() + sx) >> 1);
        st16(BAND_SCROLLS, v);
        st16(BAND_SCROLLS + 2, (uint16_t)(v >> 1));
    } else if (set == 2) {
        drift_on();
        moved = (uint16_t)(sx - g.scroll_x_before);
        if (moved & 0x8000)
            moved = 0;
        g.band_drift = (g.band_drift & 0xFF0000FFu) | (uint32_t)(uint16_t)(drift_pixels() - moved) << 8;
        v = (uint16_t)(drift_pixels() + sx);
        st16(BAND_SCROLLS, v);
        st16(BAND_SCROLLS + 2, (uint16_t)(v >> 1));
        st16(BAND_SCROLLS + 4, (uint16_t)(v >> 2));
        st16(BAND_SCROLLS + 6, (uint16_t)(v >> 3));
        st16(BAND_SCROLLS + 8, (uint16_t)(sx >> 1));
        st16(BAND_SCROLLS + 10, (uint16_t)(sx + (sx >> 2)));
    } else {
        fprintf(stderr, "video: band set %u has no scrolls in the original's table\n", set);
    }
}

/*
 * @routine 81:C848 BandTable
 * WHAT IT DOES: Scrolls a backdrop of horizontal bands, each at its own
 * speed: writes the bands' table for this frame.
 * HOW: The set noted; the backdrop's y the play field's; the line the
 * scroll y + 8, the count the set's bands; BandScrolls. Then in the other
 * of the two tables than last frame's, after a 3-byte head: for each band
 * the screen's line reaches (its edges in the level), an entry of its
 * lines (to its lower edge, at most the 224 left; two entries past 127:
 * the lines less 127, then 127) and its scroll; a 0 to end (two bytes, as
 * the original's word store). The head: one line at the top band's
 * scroll, which BG2's x takes too. The backdrop's x is the play field's
 * halved; channel 4's table (g.line_table_at) this one.
 * @unreachable 81:C8B6-C8B8 The "none: one" step: the band's lines are at
 * least 1 there (the record's proof, master 2026-11-19).
 */
static void band_table(uint16_t set)
{
    uint16_t table, y, n, edge;

    st16(BAND_SET, set);
    pixels_set(&BACK->scroll_y, pixels(PLAY->scroll_y));
    st16(BAND_LINE, (uint16_t)(pixels(PLAY->scroll_y) + 8));
    st16(BAND_COUNT, (uint16_t)(band_set_count2(set) >> 1));
    band_scrolls(set, ld16(BAND_COUNT));
    g.band_table_buffer ^= 2;
    table = (uint16_t)(band_table_place(g.band_table_buffer) - LINE_DATA_AT);
    y = table;
    st16(BAND_LINES_LEFT, 224);
    st16(BAND_EDGE, band_edge_word(set));
    st16(BAND_AT, 0);
    for (;;) {
        edge = ld16(BAND_EDGE);
        if (ld16(BAND_LINE) >= band_edge_word(edge) && ld16(BAND_LINE) < band_edge_word((uint16_t)(edge + 2))) {
            n = (uint16_t)(band_edge_word((uint16_t)(edge + 2)) - ld16(BAND_LINE));
            if (n >= ld16(BAND_LINES_LEFT))
                n = ld16(BAND_LINES_LEFT);
            st16(BAND_LINES, n);
            if (n < 128) {
                st16((unsigned)y + 3, n);
                st16((unsigned)y + 4, ld16(BAND_SCROLLS + ld16(BAND_AT)));
                y = (uint16_t)(y + 3);
            } else {
                st16((unsigned)y + 3, (uint16_t)(n - 127));
                st16((unsigned)y + 6, 127);
                st16((unsigned)y + 4, ld16(BAND_SCROLLS + ld16(BAND_AT)));
                st16((unsigned)y + 7, ld16(BAND_SCROLLS + ld16(BAND_AT)));
                y = (uint16_t)(y + 6);
            }
            st16(BAND_LINE, band_edge_word((uint16_t)(edge + 2)));
            if (ld16(BAND_LINES_LEFT) <= ld16(BAND_LINES))
                break;                  /* the screen is full */
            st16(BAND_LINES_LEFT, (uint16_t)(ld16(BAND_LINES_LEFT) - ld16(BAND_LINES)));
        }
        st16(BAND_EDGE, (uint16_t)(ld16(BAND_EDGE) + 2));
        st16(BAND_AT, (uint16_t)(ld16(BAND_AT) + 2));
        st16(BAND_COUNT, (uint16_t)(ld16(BAND_COUNT) - 1));
        if (ld16(BAND_COUNT) == 0)
            break;
    }
    st16((unsigned)y + 3, 0);
    st16(table, 1);
    st16((unsigned)table + 1, ld16((unsigned)table + 4));
    pixels_set(&g.bg_x[1], ld16((unsigned)table + 4));
    pixels_set(&BACK->scroll_x, (uint16_t)(pixels(PLAY->scroll_x) >> 1));
    g.line_table_at[0] = table;
}

/*
 * @routine 81:CA55 BackdropWideX
 * WHAT IT DOES: PlayField3Stages's x for the backdrop in its later stages:
 * the play field's scroll x up to $1000, past it half as fast.
 */
static void backdrop_wide_x(void)
{
    uint16_t sx = pixels(PLAY->scroll_x);

    pixels_set(&BACK->scroll_x, sx >= WIDE_X ? (uint16_t)(((uint16_t)(sx - WIDE_X) >> 1) + WIDE_X) : sx);
}

/*
 * @routine 81:C9F2 PlayField3Stages
 * WHAT IT DOES: The backdrop of a level that changes as the player goes:
 * stage 0 follows the play field with an offset down until the player
 * reaches x $0F10 on firm ground, or the scroll passes $0F80; stage 1 eases
 * the backdrop's y back to 0, at most 8 a frame, and scrolls it across as
 * stage 2; stage 2 scrolls it across with the play field, at half speed
 * past $1000. Other stages: the original's table has three (reported).
 * HOW: Stage 1's step: the y less itself held to -8..8 (the original
 * adds the negated, held value to the whole word).
 */
static void playfield_3_stages(void)
{
    uint16_t v, d;

    switch (g.backdrop_stage) {
    case 0:
        BackdropFollowOffsetY();
        if (pixels(g.actors[0].x) < STAGE1_X)
            return;
        if (pixels(PLAY->scroll_x) < STAGE1_SCROLL && !PlayerOnFirmGround())
            return;
        g.backdrop_stage = (uint16_t)(g.backdrop_stage + 2);
        return;
    case 2:
        v = pixels(BACK->scroll_y);
        if (v == 0) {
            g.backdrop_stage = (uint16_t)(g.backdrop_stage + 2);
            return;
        }
        if (!(v & 0x8000))
            d = v < 8 ? v : 8;
        else
            d = v >= 0xFFF8 ? v : 0xFFF8;
        pixels_set(&BACK->scroll_y, (uint16_t)(v - d));
        backdrop_wide_x();
        return;
    case 4:
        backdrop_wide_x();
        return;
    default:
        fprintf(stderr, "video: backdrop stage %u is not one of the three\n", g.backdrop_stage);
        return;
    }
}

/*
 * @routine 81:C772 ParallaxUpdate
 * WHAT IT DOES: Sets the backdrop's scroll from the play field's in the way
 * the level asks: its handler, by g.layer_setup_mode (the original's
 * $083F, times 2): 0 ParallaxHalf, 2 ParallaxFull, 4 ParallaxHalfY, 6
 * ParallaxHalfXY80, 8 ParallaxHalfX, $0A BandTableSet2, $0C ParallaxDrift,
 * $0E BandTable, $10 PlayField3Stages, $12 ParallaxNone, $14
 * BackdropFollowOffsetY (the table at $81:C781, 11 words).
 * HOW: ParallaxUpdateLong (81:C77B), its call for other banks, is this.
 */
void ParallaxUpdate(void)
{
    switch (g.layer_setup_mode) {
    case 0x00: parallax_half(); break;
    case 0x02: parallax_full(); break;
    case 0x04: parallax_half_y(); break;
    case 0x06: parallax_half_xy80(); break;
    case 0x08: parallax_half_x(); break;
    case 0x0A: band_table(2); break;    /* BandTableSet2 (81:C7E2) */
    case 0x0C: parallax_drift(); break;
    case 0x0E: band_table(0); break;    /* BandTable (81:C848) */
    case 0x10: playfield_3_stages(); break;
    case 0x12: parallax_none(); break;
    case 0x14: BackdropFollowOffsetY(); break;
    default: {
        static int told;

        if (!told) {
            told = 1;
            fprintf(stderr, "video: parallax handler %u is not in the original's table\n", g.layer_setup_mode);
        }
        break;
    }
    }
}

/* map/'s, wanted (81:B756 PlayerOnFirmGround): a weak placeholder until it lands */
__attribute__((weak)) int PlayerOnFirmGround(void)
{
    static int told;

    if (!told) {
        told = 1;
        fprintf(stderr, "unconverted: PlayerOnFirmGround (map/, wanted by PlayField3Stages)\n");
    }
    return 0;
}
