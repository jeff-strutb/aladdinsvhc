/*
 * snapshot.c: the whole game state in a flat buffer (snapshot.h; docs/aladdin/native/FRAMERATE.md section 10).
 *
 * Sections, each a tag and a length (32 bits) then its bytes, after a header (magic, format, layout mark, total length):
 *   G      the game state, its pointers written as codes
 *   KEPT   the video parts between_keep registered (the sprite lists, the scroll latches...), each with its size
 *   STAT   the statics RUNAHEAD_KEEP registered, in order
 *   LMAP   the level maps' records and words
 *   VIEW   the view, the slots open
 *   ART    the shadows of what the game wrote to the engine's tiles, maps and palette
 *   OAM    the frame's sprites the game built (they outlive their update)
 *   SOUND  the driver's state and the tick's phase
 * The lists are the ones runahead uses (system/runahead.c, video/between.c), not another.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor/code_table.h"
#include "audio/music.h"
#include "data/data.h"
#include "system/runahead.h"
#include "system/snapshot.h"
#include "system/state.h"
#include "system/task.h"
#include "video/between.h"
#include "video/level_maps.h"
#include "video/slot_gfx.h"
#include "video/video.h"
#include "video/view.h"

#define MAGIC 0x504E5341u               /* "ASNP" */
#define FORMAT 1u

enum { TAG_G = 1, TAG_KEPT, TAG_STAT, TAG_LMAP, TAG_VIEW, TAG_ART, TAG_OAM, TAG_SOUND, TAG_COUNT };

/* a pointer field written as a code: its kind in the top byte, then an offset in g, an offset in the cartridge, or a code id */
enum { PK_NULL = 0, PK_G = 1, PK_CODE = 2, PK_CART = 3 };
#define PK_SHIFT (8 * sizeof(uintptr_t) - 8)
#define PK(kind, v) (((uintptr_t)(kind) << PK_SHIFT) | (uintptr_t)(v))

int snapshot_available(void) { return 1; }

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void put32(uint8_t *o, uint32_t v)
{
    o[0] = (uint8_t)v; o[1] = (uint8_t)(v >> 8); o[2] = (uint8_t)(v >> 16); o[3] = (uint8_t)(v >> 24);
}

/* ---- the layout mark ---- */

#define FOLD(v) h = (h ^ (uint64_t)(v)) * 0x100000001b3ull

uint32_t snapshot_layout_mark(void)
{
    static uint32_t mark;
    uint64_t h = 0xcbf29ce484222325ull;
    const char *name;
    void *p;
    size_t bytes;
    int owned;

    if (mark)
        return mark;
    FOLD(FORMAT);
    FOLD(sizeof(void *));
    FOLD(sizeof(GameState)); FOLD(sizeof(Actor)); FOLD(sizeof(Task)); FOLD(sizeof(VramQueueEntry)); FOLD(sizeof(View));
    FOLD(sizeof(Driver)); FOLD(sizeof(Channel)); FOLD(sizeof(nshvc_sprite));
    FOLD(offsetof(GameState, chain_head)); FOLD(offsetof(GameState, chain_tail)); FOLD(offsetof(GameState, free_pool));
    FOLD(offsetof(GameState, free_pool_b)); FOLD(offsetof(GameState, draw_list)); FOLD(offsetof(GameState, actors));
    FOLD(offsetof(GameState, vram_queue)); FOLD(offsetof(Actor, next)); FOLD(offsetof(Actor, code));
    FOLD(offsetof(VramQueueEntry, from));
    h = between_layout(h);
    for (unsigned i = 0; i < runahead_keep_count(); i++) {
        runahead_keep_at(i, &name, &p, &bytes);
        for (const char *c = name; *c; c++)
            FOLD((uint8_t)*c);
        FOLD(bytes);
    }
    FOLD(TAG_COUNT);
    mark = (uint32_t)(h ^ (h >> 32));
    (void)owned;
    return mark ? mark : 1u;
}

/* ---- the pointers in g ---- */

static GameState flat;                  /* g with its pointers as codes, or a snapshot's g being checked */

static int encode_g(const void *p, uintptr_t *out)
{
    const uint8_t *b = (const uint8_t *)p;

    if (!b) {
        *out = PK_NULL;
        return 1;
    }
    if (b < (const uint8_t *)&g || b >= (const uint8_t *)&g + sizeof g)
        return 0;
    *out = PK(PK_G, b - (const uint8_t *)&g);
    return 1;
}

static int encode_actor_ptrs(Actor **at, size_t n, const char *what)
{
    for (size_t i = 0; i < n; i++) {
        uintptr_t c;

        if (!encode_g(at[i], &c)) {
            fprintf(stderr, "snapshot: %s[%zu] points outside the game state\n", what, i);
            return 0;
        }
        memcpy(&at[i], &c, sizeof c);
    }
    return 1;
}

static int encode_flat(void)
{
    uintptr_t c;

    memcpy(&flat, &g, sizeof g);
    actor_code_check();
    if (!encode_actor_ptrs(&flat.chain_head, 1, "chain_head") || !encode_actor_ptrs(&flat.chain_tail, 1, "chain_tail") ||
        !encode_actor_ptrs(flat.free_pool, ACTOR_POOL_SIZE, "free_pool") ||
        !encode_actor_ptrs(flat.free_pool_b, ACTOR_POOL_B_SIZE, "free_pool_b") ||
        !encode_actor_ptrs(&flat.draw_list[0][0], sizeof flat.draw_list / sizeof flat.draw_list[0][0], "draw_list"))
        return 0;
    for (unsigned i = 0; i < ACTOR_RECORDS; i++) {
        int64_t id = actor_code_id(g.actors[i].code);

        if (!encode_actor_ptrs(&flat.actors[i].next, 1, "actor next"))
            return 0;
        if (id < 0) {
            fprintf(stderr, "snapshot: actor %u (kind $%02X) runs in a function that is in no table (%p): add it to actor/code_table.c\n",
                    i, g.actors[i].kind, (void *)(uintptr_t)g.actors[i].code);
            return 0;
        }
        c = id ? PK(PK_CODE, id) : PK_NULL;
        memcpy(&flat.actors[i].code, &c, sizeof c);
    }
    for (unsigned i = 0; i < VRAM_QUEUE_SIZE; i++) {
        uint32_t off;
        const uint8_t *from = g.vram_queue[i].from;

        if (!from)
            c = PK_NULL;
        else if (encode_g(from, &c))
            ;
        else if (data_cart_offset(from, &off))
            c = PK(PK_CART, off);
        else {
            fprintf(stderr, "snapshot: VRAM queue entry %u reads from %p, in neither the game state nor the cartridge\n", i, (const void *)from);
            return 0;
        }
        memcpy(&flat.vram_queue[i].from, &c, sizeof c);
    }
    return 1;
}

/* a code back to a pointer; 0 when it names nothing valid */
static int decode_actor_ptrs(Actor **at, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uintptr_t c;

        memcpy(&c, &at[i], sizeof c);
        if (c == PK_NULL) {
            at[i] = NULL;
        } else if ((c >> PK_SHIFT) == PK_G && (c & (((uintptr_t)1 << PK_SHIFT) - 1)) < sizeof g) {
            at[i] = (Actor *)((uint8_t *)&g + (c & (((uintptr_t)1 << PK_SHIFT) - 1)));
        } else {
            return 0;
        }
    }
    return 1;
}

static int decode_flat(void)
{
    if (!decode_actor_ptrs(&flat.chain_head, 1) || !decode_actor_ptrs(&flat.chain_tail, 1) ||
        !decode_actor_ptrs(flat.free_pool, ACTOR_POOL_SIZE) || !decode_actor_ptrs(flat.free_pool_b, ACTOR_POOL_B_SIZE) ||
        !decode_actor_ptrs(&flat.draw_list[0][0], sizeof flat.draw_list / sizeof flat.draw_list[0][0]))
        return 0;
    for (unsigned i = 0; i < ACTOR_RECORDS; i++) {
        uintptr_t c;
        int known;

        if (!decode_actor_ptrs(&flat.actors[i].next, 1))
            return 0;
        memcpy(&c, &flat.actors[i].code, sizeof c);
        if (c == PK_NULL) {
            flat.actors[i].code = NULL;
        } else if ((c >> PK_SHIFT) == PK_CODE) {
            flat.actors[i].code = actor_code_of((uint32_t)(c & 0xFFFFFFFFu), &known);
            if (!known)
                return 0;
        } else {
            return 0;
        }
    }
    for (unsigned i = 0; i < VRAM_QUEUE_SIZE; i++) {
        uintptr_t c;

        memcpy(&c, &flat.vram_queue[i].from, sizeof c);
        if (c == PK_NULL) {
            flat.vram_queue[i].from = NULL;
        } else if ((c >> PK_SHIFT) == PK_G && (c & (((uintptr_t)1 << PK_SHIFT) - 1)) < sizeof g) {
            flat.vram_queue[i].from = (const uint8_t *)&g + (c & (((uintptr_t)1 << PK_SHIFT) - 1));
        } else if ((c >> PK_SHIFT) == PK_CART && data_cart_ptr((uint32_t)(c & (((uintptr_t)1 << PK_SHIFT) - 1)))) {
            flat.vram_queue[i].from = data_cart_ptr((uint32_t)(c & (((uintptr_t)1 << PK_SHIFT) - 1)));
        } else {
            return 0;
        }
    }
    return 1;
}

/* ---- sizes ---- */

static size_t kept_total(void)
{
    const char *name;
    void *p;
    size_t used, total, n = 0;
    int owned;

    for (unsigned i = 0; i < between_kept_count(); i++) {
        between_kept_at(i, &name, &p, &used, &total, &owned);
        if (!owned)
            n += 4 + used;
    }
    return n;
}

static size_t stat_total(void)
{
    const char *name;
    void *p;
    size_t bytes, n = 0;

    for (unsigned i = 0; i < runahead_keep_count(); i++) {
        runahead_keep_at(i, &name, &p, &bytes);
        n += bytes;
    }
    return n;
}

size_t snapshot_max(void)
{
    return 16 + TAG_COUNT * 8 + sizeof(GameState) + kept_total() + stat_total() + level_maps_snapshot_max() + 4 + 6 * 4 +
           between_art_size() + oam_snapshot_size() + MUSIC_SNAPSHOT_BYTES + 4096;
}

/* ---- save ---- */

typedef struct Out {
    uint8_t *at, *end;
    int bad;
} Out;

static uint8_t *section(Out *o, uint32_t tag, size_t len)
{
    uint8_t *p = o->at;

    if (o->bad || o->at + 8 + len > o->end) {
        o->bad = 1;
        return NULL;
    }
    put32(p, tag);
    put32(p + 4, (uint32_t)len);
    o->at += 8 + len;
    return p + 8;
}

size_t snapshot_save(uint8_t *buf, size_t cap)
{
    Out o = { buf + 16, buf + cap, 0 };
    uint8_t *p;
    const char *name;
    void *live;
    size_t bytes, total;
    int owned;

    if (cap < 16 || !encode_flat())
        return 0;
    if ((p = section(&o, TAG_G, sizeof flat)) != NULL)
        memcpy(p, &flat, sizeof flat);
    if ((p = section(&o, TAG_KEPT, kept_total())) != NULL)
        for (unsigned i = 0; i < between_kept_count(); i++) {
            between_kept_at(i, &name, &live, &bytes, &total, &owned);
            if (owned)
                continue;
            put32(p, (uint32_t)bytes);
            memcpy(p + 4, live, bytes);
            p += 4 + bytes;
        }
    if ((p = section(&o, TAG_STAT, stat_total())) != NULL)
        for (unsigned i = 0; i < runahead_keep_count(); i++) {
            runahead_keep_at(i, &name, &live, &bytes);
            memcpy(p, live, bytes);
            p += bytes;
        }
    if (!o.bad && o.end - o.at >= (ptrdiff_t)(8 + level_maps_snapshot_max())) {
        uint8_t *at = o.at;
        size_t n = level_maps_snapshot_save(at + 8);

        put32(at, TAG_LMAP);
        put32(at + 4, (uint32_t)n);
        o.at += 8 + n;
    } else {
        o.bad = 1;
    }
    if ((p = section(&o, TAG_VIEW, 6 * 4)) != NULL) {
        put32(p, (uint32_t)view.w); put32(p + 4, (uint32_t)view.h); put32(p + 8, (uint32_t)view.dx);
        put32(p + 12, (uint32_t)view.dy); put32(p + 16, (uint32_t)view.square); put32(p + 20, slots_open_now());
    }
    if ((p = section(&o, TAG_ART, between_art_size())) != NULL)
        between_art_save(p);
    if ((p = section(&o, TAG_OAM, oam_snapshot_size())) != NULL)
        oam_snapshot_save(p);
    if ((p = section(&o, TAG_SOUND, MUSIC_SNAPSHOT_BYTES)) != NULL && !music_snapshot_save(p)) {
        fprintf(stderr, "snapshot: the sound's state could not be taken\n");
        return 0;
    }
    if (o.bad) {
        fprintf(stderr, "snapshot: the buffer of %zu bytes is too small\n", cap);
        return 0;
    }
    put32(buf, MAGIC);
    put32(buf + 4, FORMAT);
    put32(buf + 8, snapshot_layout_mark());
    put32(buf + 12, (uint32_t)(o.at - buf));
    return (size_t)(o.at - buf);
}

/* ---- load ---- */

typedef struct In {
    const uint8_t *data;
    size_t len;
} In;

/* the next section's body, if its tag is the one expected */
static int next(const uint8_t **at, const uint8_t *end, uint32_t tag, In *body)
{
    if (*at + 8 > end || get32(*at) != tag)
        return 0;
    body->len = get32(*at + 4);
    body->data = *at + 8;
    if ((size_t)(end - body->data) < body->len)
        return 0;
    *at = body->data + body->len;
    return 1;
}

int snapshot_load(const uint8_t *buf, size_t len)
{
    const uint8_t *at = buf + 16, *end = buf + len;
    In s[TAG_COUNT];
    const char *name;
    void *live;
    size_t bytes, total;
    int owned;
    View v;

    if (len < 16 || get32(buf) != MAGIC || get32(buf + 4) != FORMAT || get32(buf + 8) != snapshot_layout_mark() ||
        get32(buf + 12) != len) {
        fprintf(stderr, "snapshot: not a snapshot of this layout\n");
        return 0;
    }
    for (uint32_t tag = TAG_G; tag < TAG_COUNT; tag++)
        if (!next(&at, end, tag, &s[tag])) {
            fprintf(stderr, "snapshot: section %u missing or cut short\n", tag);
            return 0;
        }
    if (at != end || s[TAG_G].len != sizeof flat || s[TAG_STAT].len != stat_total() ||
        s[TAG_VIEW].len != 6 * 4 || s[TAG_SOUND].len != MUSIC_SNAPSHOT_BYTES) {
        fprintf(stderr, "snapshot: a section has the wrong length\n");
        return 0;
    }
    /* the kept parts: each size from the file, within what the file holds */
    {
        size_t pos = 0;

        for (unsigned i = 0; i < between_kept_count(); i++) {
            between_kept_at(i, &name, &live, &bytes, &total, &owned);
            if (owned)
                continue;
            if (pos + 4 > s[TAG_KEPT].len || pos + 4 + get32(s[TAG_KEPT].data + pos) > s[TAG_KEPT].len) {
                fprintf(stderr, "snapshot: the kept part %s is cut short\n", name);
                return 0;
            }
            pos += 4 + get32(s[TAG_KEPT].data + pos);
        }
        if (pos != s[TAG_KEPT].len) {
            fprintf(stderr, "snapshot: the kept parts do not add up\n");
            return 0;
        }
    }
    if (!oam_snapshot_check(s[TAG_OAM].data, s[TAG_OAM].len)) {
        fprintf(stderr, "snapshot: the frame's sprites do not add up\n");
        return 0;
    }
    if (!level_maps_snapshot_check(s[TAG_LMAP].data, s[TAG_LMAP].len) || !between_art_check(s[TAG_ART].data, s[TAG_ART].len)) {
        fprintf(stderr, "snapshot: the level maps or the engine's art do not fit this process\n");
        return 0;
    }
    memcpy(&flat, s[TAG_G].data, sizeof flat);
    if (!decode_flat()) {
        fprintf(stderr, "snapshot: a pointer code in the game state names nothing\n");
        return 0;
    }
    v.w = (int32_t)get32(s[TAG_VIEW].data); v.h = (int32_t)get32(s[TAG_VIEW].data + 4); v.dx = (int32_t)get32(s[TAG_VIEW].data + 8);
    v.dy = (int32_t)get32(s[TAG_VIEW].data + 12); v.square = (int32_t)get32(s[TAG_VIEW].data + 16);

    /* everything checked: put it in */
    video_view_adopt(&v, get32(s[TAG_VIEW].data + 20));
    memcpy(&g, &flat, sizeof g);
    {
        size_t pos = 0;

        for (unsigned i = 0; i < between_kept_count(); i++) {
            uint32_t saved, n;

            between_kept_at(i, &name, &live, &bytes, &total, &owned);
            if (owned)
                continue;
            saved = n = get32(s[TAG_KEPT].data + pos);
            if (n > total)
                n = (uint32_t)total;                /* after the view is adopted this does not happen: the lists hold what the view uses */
            memcpy(live, s[TAG_KEPT].data + pos + 4, n);
            memset((uint8_t *)live + n, 0, total - n);
            pos += 4 + saved;
        }
        pos = 0;
        for (unsigned i = 0; i < runahead_keep_count(); i++) {
            runahead_keep_at(i, &name, &live, &bytes);
            memcpy(live, s[TAG_STAT].data + pos, bytes);
            pos += bytes;
        }
    }
    oam_snapshot_load(s[TAG_OAM].data);
    if (!level_maps_snapshot_load(s[TAG_LMAP].data, s[TAG_LMAP].len))
        fprintf(stderr, "snapshot: more level map sizes than the pool holds\n");
    between_art_load(s[TAG_ART].data, s[TAG_ART].len, slots_count() > SLOTS_STD ? 256u + 16u * (slots_count() - SLOTS_STD) : 256u);
    video_mode7_rewrite();
    if (!music_snapshot_load(s[TAG_SOUND].data))
        fprintf(stderr, "snapshot: the sound driver is not running; the sound is left as it is\n");
    between_rebase();
    return 1;
}
