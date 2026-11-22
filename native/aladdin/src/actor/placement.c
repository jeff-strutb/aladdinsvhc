/*
 * placement.c: creating a level's placed actors as the camera comes near
 * them, and creating an actor of a kind.
 *
 * When a level starts its placements are indexed in 32-pixel bands: by
 * column (their x) and by row (their y). Each time the camera has moved 32
 * pixels from where it last placed, the actors of the band coming into reach
 * are created: those within 384 pixels round the camera's band that are not
 * placed already or gone for good (g.placed). A created actor gets its
 * record from the free list and its kind's set-up (its sprite graphics: a
 * palette slot, or the slot its kind already has).
 */
#include <stdio.h>
#include <string.h>

#include <stdlib.h>

#include "actor/actor_own.h"
#include "data/actor_tables.h"
#include "video/view.h"

/* The index ($7E:C100-$7E:C5FF; the original's clear runs on through
   $7E:C7FF, which also clears g.placed and a page no code names). */
static uint8_t column_count[256];   /* $7E:C100: per column, its placements; while indexing by row, each placement's row */
static uint8_t column_first[256];   /* $7E:C200: per column, its first placement's number */
static uint8_t row_count[256];      /* $7E:C300: per row, its placements */
static uint8_t row_start[256];      /* $7E:C400: per row, where its run starts in row_order */
static uint8_t row_order[256];      /* $7E:C500: placement numbers, row by row */

/* Where the camera last placed from ($1BE7-$1BF4), in bands of 32 pixels. */
static struct {
    uint16_t row_above;     /* $1BE7: the row band 64 pixels above the camera */
    uint16_t row_below;     /* $1BE9: the row band 288 below */
    uint16_t column_right;  /* $1BEB: the column band 288 right of the camera */
    uint16_t column_left;   /* $1BED: the column band 64 left */
    uint16_t x;             /* $1BEF: the camera's x when it last placed across */
    uint16_t y;             /* $1BF1: its y when it last placed down */
} placing;

/* The view's left and top edges ($1773, $1777): the camera is the original's, and the view is a frame round its screen
   (view.dx, view.dy), so the spawn window is measured from the view's own edges. At 256 x 224 they are the camera. */
static uint16_t camera_x(void) { return (uint16_t)(pixels(g.layers[0].scroll_x) - view.dx); }
static uint16_t camera_y(void) { return (uint16_t)(pixels(g.layers[0].scroll_y) - view.dy); }

/*
 * @routine 82:A9BB PlacementsByRow
 * WHAT IT DOES: Indexes the level's `n` placements by row: each one's row
 * (its y word over 32, 11 bits; 255 and up count as row 0), then row by row
 * from 0 the placements in it listed, the last placement first, each row's
 * count stepped and its run's start kept, until all are listed.
 */
static void PlacementsByRow(unsigned n)
{
    unsigned listed = 0, i;
    uint8_t row = 0, start = 0;

    for (i = 0; i < n; i++) {
        uint16_t r = (uint16_t)((placement_at(g.level, i).y_word >> 5) & 0x07FF);

        if (r < 0xFF)
            column_count[i] = (uint8_t)r;
    }
    do {
        for (i = n; i-- > 0;)
            if (column_count[i] == row) {
                row_count[row]++;
                row_order[listed++] = (uint8_t)i;
            }
        if (listed != start) {
            row_start[row] = start;
            start = (uint8_t)listed;
        }
        row++;
    } while (listed != n);
}

/*
 * @routine 82:A984 PlacementsByColumn
 * WHAT IT DOES: Counts the level's `n` placements into their columns
 * (their x word over 32, 10 bits): each column's count, and for a column
 * met first since the one before, the placement's number. Columns 255 and
 * up are left out, and the numbers count only those counted.
 */
static void PlacementsByColumn(unsigned n)
{
    unsigned i;
    uint8_t last = 0, number = 0;

    for (i = 0; i < n; i++) {
        uint16_t c = (uint16_t)((placement_at(g.level, i).x_word >> 5) & 0x03FF);

        if (c >= 0xFF)
            continue;
        column_count[c]++;
        if (c != last) {
            last = (uint8_t)c;
            column_first[c] = number;
        }
        number++;
    }
}

/*
 * @routine 82:A93A PlacementsIndex
 * WHAT IT DOES: Indexes the level's actor placements when it starts: by
 * row, then by column; every placement not placed.
 */
static void PlacementsIndex(void)
{
    unsigned n = placement_count(g.level);

    memset(&placing, 0, sizeof placing);
    memset(column_count, 0, sizeof column_count);
    memset(column_first, 0, sizeof column_first);
    memset(row_count, 0, sizeof row_count);
    memset(row_start, 0, sizeof row_start);
    memset(row_order, 0, sizeof row_order);
    memset(g.placed, 0, sizeof g.placed);
    if (n) {
        PlacementsByRow(n);
        memset(column_count, 0, sizeof column_count);
        PlacementsByColumn(n);
    }
}

/*
 * @routine 82:A906 ActorPlace
 * WHAT IT DOES: Places a new actor from placement `number`: the
 * placement's byte's low 5 bits to its +$07, its x and y pixels (the low 14
 * bits of its words), state new; the placement marked placed, and kept in
 * the actor.
 */
static void ActorPlace(Actor *a, uint8_t number)
{
    Placement p = placement_at(g.level, number);

    a->spawn_byte = (uint8_t)(p.byte & 0x1F);
    a->x = with_pixels(a->x, (uint16_t)(p.x_word & 0x3FFF));
    a->y = with_pixels(a->y, (uint16_t)(p.y_word & 0x3FFF));
    a->state = ACTOR_STATE_NEW;
    g.placed[number] = (uint8_t)(p.byte | 0x01);
    a->placement = number;
}

/* The window a placement is created in, from the camera at the last
   placing: 64 pixels before it to 64 past the view's far edge across (w
   + 128 long), and 64 before to 96 past it down (h + 160 long). At 256 x
   224 both are the original's 384 ($0180). The bands below cover the
   window to within one band of its far end: the far band starts 32 before
   the far end (w + 32 across, h + 64 down; the original's $120). */
#define WINDOW_BEFORE 0x40
#define WINDOW_AFTER_ACROSS 64
#define WINDOW_AFTER_DOWN 96
#define BAND 32

static uint16_t window_length(int across)
{
    return (uint16_t)(across ? view.w + WINDOW_BEFORE + WINDOW_AFTER_ACROSS
                             : view.h + WINDOW_BEFORE + WINDOW_AFTER_DOWN);
}

/* The far band's offset from the camera: the window's far end less a band. */
static uint16_t far_band_offset(int across)
{
    return (uint16_t)(across ? view.w + WINDOW_AFTER_ACROSS - BAND
                             : view.h + WINDOW_AFTER_DOWN - BAND);
}

/* Developer count (ALADDIN_PLACE_FAIL=1): placements that failed for want
   of a record or a palette slot; printed to stderr as they happen, and
   not game state. */
static int place_fail_trace(void)
{
    static int on = -1;

    if (on < 0) {
        const char *e = getenv("ALADDIN_PLACE_FAIL");

        on = e && *e && *e != '0';
    }
    return on;
}

/* Placement `number` created and placed, unless it is placed or gone, or
   its place across (`across`) or down is outside the window from `from`. */
static void bring_one(uint8_t number, int across, uint16_t from)
{
    Placement p;
    uint16_t at;
    Actor *a;

    if (g.placed[number] & 0x81)
        return;
    p = placement_at(g.level, number);
    at = across ? p.x_word : p.y_word;
    if ((uint16_t)(at + WINDOW_BEFORE - from) >= window_length(across))
        return;
    a = ActorCreate(p.kind);
    if (a)
        ActorPlace(a, number);
    else if (place_fail_trace())
        fprintf(stderr, "PLACE_FAIL %s level=%d number=%u kind=%02X\n", g.free_pool_top < 0 ? "record" : "slot",
                (int)g.level, number, p.kind);
}

/*
 * @routine 82:A83E PlacementsAcross
 * WHAT IT DOES: Creates the actors of row band `row` that are near the
 * camera across and not yet placed, in the row's order.
 */
static void PlacementsAcross(uint8_t row)
{
    uint8_t n = row_count[row], at = row_start[row];

    for (; n; n--, at++)
        bring_one(row_order[at], 1, placing.x);
}

/*
 * @routine 82:A89D PlacementsDown
 * WHAT IT DOES: Creates the actors of column band `column` that are near
 * the camera down and not yet placed, from the column's first on.
 */
static void PlacementsDown(uint8_t column)
{
    uint8_t n = column_count[column], number = column_first[column];

    for (; n; n--, number++)
        bring_one(number, 0, placing.y);
}

/*
 * @routine 82:A72C PlacementsStart
 * WHAT IT DOES: Indexes the level's placements as it starts, sets the
 * placing points from the camera, and creates the actors of the column
 * bands from 64 pixels left of the camera to the far band (12 at 256 wide).
 */
void PlacementsStart(void)
{
    uint16_t y = camera_y(), x = camera_x();
    uint8_t band;
    int n;

    PlacementsIndex();
    g.placed[0xFF] = 0xFF;
    placing.y = y;
    placing.row_below = (uint16_t)((y + far_band_offset(0)) >> 5);
    placing.row_above = (uint16_t)((uint16_t)(y - 0x0040) >> 5);
    placing.x = x;
    placing.column_right = (uint16_t)((x + far_band_offset(1)) >> 5);
    placing.column_left = (uint16_t)((uint16_t)(x - 0x0040) >> 5);
    band = (uint8_t)placing.column_left;
    /* the bands from the left one to the far one, each: 12 at 256 wide */
    for (n = (uint8_t)(placing.column_right - placing.column_left) + 1; n; n--)
        PlacementsDown(band++);
}

/* One axis of PlacementsScroll: the camera at `camera` 32 or more from
   `*last` moves `*last` 32 that way and both bands one; the band coming
   into view is brought in. The bands are bytes in the original: they step
   in their low byte. */
static void scroll_axis(uint16_t camera, uint16_t *last, uint16_t *ahead, uint16_t *behind, void (*bring)(uint8_t))
{
    uint16_t d = (uint16_t)(camera - *last);
    int back = (d & 0x8000) != 0;
    uint8_t step = back ? 0xFF : 0x01;

    if (back)
        d = (uint16_t)-d;
    if (d < 0x0020)
        return;
    *last = (uint16_t)(back ? *last - 0x20 : *last + 0x20);
    *ahead = (uint16_t)((*ahead & 0xFF00) | (uint8_t)(*ahead + step));
    *behind = (uint16_t)((*behind & 0xFF00) | (uint8_t)(*behind + step));
    bring((uint8_t)(back ? *behind : *ahead));
}

/*
 * @routine 82:A79C PlacementsScroll
 * WHAT IT DOES: Creates the placed actors the camera's move brings into
 * reach: across, then down, each time the camera is 32 pixels or more from
 * where it last placed, the column (or row) band on the side it moved to.
 */
void PlacementsScroll(void)
{
    scroll_axis(camera_x(), &placing.x, &placing.column_right, &placing.column_left, PlacementsDown);
    scroll_axis(camera_y(), &placing.y, &placing.row_below, &placing.row_above, PlacementsAcross);
}

/* ---- an actor of a kind ---- */

/* The claim of `count` neighbouring palette slots (2 or 3) for `kind`, the
   first free run from slot 1 on: TileSlotsClaim2 (82:AC10) and
   TileSlotsClaim3 (82:AC34). 0 claimed, -1 none free. */
static int slots_claim(uint8_t kind, Actor *a, int count)
{
    int first, k;

    for (first = 0; first + count <= 4; first++) {
        for (k = 0; k < count && !g.palette_slot_users[first + k]; k++)
            ;
        if (k < count)
            continue;
        for (k = 0; k < count; k++)
            g.palette_slot_users[first + k]++;
        g.palette_slot_kind[first] = kind;
        a->tile_slot = (uint8_t)(first + 1);
        g.kind_slot[kind] = (uint8_t)(first + 1);
        a->look = palette_slot_look((uint8_t)(first + 1));
        g.kind_look[kind] = a->look;
        return 0;
    }
    return -1;
}

/* A palette slot, or a run of 2 or 3, by the kind's extra slots. */
static int claim_by_size(uint8_t kind, Actor *a)
{
    uint8_t extra = g.kind_extra_slots[kind];

    if (extra == 0)
        return TileSlotClaim(kind, a);
    return slots_claim(kind, a, extra == 1 ? 2 : 3);
}

/*
 * @routine 82:AAE7 ActorKindTilesQueue
 * WHAT IT DOES: Queues a kind's graphics for upload to its palette slot's
 * sprite tiles (a VRAM queue record: the slot's tiles from its look, the
 * graphics where video unpacked them), unless they have no length; then
 * the slot's palette (TileSlotFill). Kind $33 only fills.
 */
static void ActorKindTilesQueue(uint8_t kind)
{
    if (kind != 0x33) {
        uint16_t bytes = kind_graphics_bytes(kind);
        VramQueueEntry *c;

        if (!bytes)
            return;
        if (g.vram_queue_count >= VRAM_QUEUE_SIZE) {
            static int told;

            if (!told++)
                fprintf(stderr, "actor: the VRAM queue is full: kind $%02X's graphics not queued\n", kind);
            return;
        }
        c = &g.vram_queue[g.vram_queue_count++];
        c->to = (uint16_t)(((g.kind_look[kind] & 0x01FF) << 4) + 0x6000);
        c->from = &g.unpacked[g.kind_gfx_at[kind] % sizeof g.unpacked];
        c->bytes = bytes;
        c->step = 0x80;
        g.uploads_held++;
    }
    TileSlotFill(kind);
}

/*
 * @routine 82:AA89 ActorKindTilesShared
 * WHAT IT DOES: Gives a new actor its kind's graphics: the palette slot
 * already loaded for the kind (its look and slot, one more user), or a new
 * slot (one, two or three) with its graphics queued. 0 given, -1 no slot free.
 */
static int ActorKindTilesShared(uint8_t kind, Actor *a)
{
    if (!(g.kind_look[kind] & 0x8000)) {
        if (claim_by_size(kind, a))
            return -1;
        ActorKindTilesQueue(kind);
        return 0;
    }
    a->look = g.kind_look[kind];
    a->tile_slot = g.kind_slot[kind];
    g.palette_slot_users[g.kind_slot[kind] - 1]++;
    return 0;
}

/*
 * @routine 82:AB3F ActorKindTiles
 * WHAT IT DOES: Gives a new actor a new palette slot (one, two or three)
 * for its kind, and the slot's palette (TileSlotFill). 0 given, -1 none free.
 */
static int ActorKindTiles(uint8_t kind, Actor *a)
{
    if (claim_by_size(kind, a))
        return -1;
    TileSlotFill(kind);
    return 0;
}

/*
 * @routine 82:AA61 ActorKindSetup
 * WHAT IT DOES: Runs the set-up of the new actor's kind, by the kind's
 * set-up class: 0 nothing (82:AA87 CarryClearReturn), 2 its graphics shared
 * or loaded, 4 a new palette slot. 0 done, -1 failed.
 */
static int ActorKindSetup(uint8_t kind, Actor *a)
{
    uint8_t cls = kind_setup_class(kind);

    switch (cls) {
    case 0:
        return 0;
    case 2:
        return ActorKindTilesShared(kind, a);
    case 4:
        return ActorKindTiles(kind, a);
    default: {
        /* the bytes past the table's kinds: the original jumps to what its
           jump table's next words name, which are not code */
        static uint8_t told[256];

        if (!told[kind]++)
            fprintf(stderr, "ActorKindSetup: kind $%02X has set-up class $%02X, past the table (not converted)\n", kind, cls);
        return -1;
    }
    }
}

/*
 * @routine 82:AA30 ActorCreateIn
 * WHAT IT DOES: Creates an actor of `kind` in the record `a` (one free:
 * the caller took it off the list, or it is the list's top): its kind's
 * set-up, and one more of the kind live; NULL when the set-up fails (the
 * kind stays in the record). The list is the caller's.
 */
Actor *ActorCreateIn(Actor *a, uint8_t kind)
{
    a->kind = kind;
    a->on_screen = 0;
    if (ActorKindSetup(kind, a))
        return NULL;
    g.kind_count[kind]++;
    return a;
}

/*
 * @routine 82:AA2B ActorCreate
 * WHAT IT DOES: Creates an actor of `kind` in the free list's top record,
 * if one is free and its kind's set-up succeeds: the record leaves the list
 * and one more of the kind is live. NULL otherwise (the kind stays in the
 * top record, which stays free).
 */
Actor *ActorCreate(uint8_t kind)
{
    Actor *a;

    if (g.free_pool_top < 0)
        return NULL;
    a = ActorCreateIn(g.free_pool[g.free_pool_top], kind);
    if (a)
        g.free_pool_top--;
    return a;
}
