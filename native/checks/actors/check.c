/*
 * check.c: the native actor code against the original, frame by frame.
 *
 * Reads work-RAM dumps of the original, one per frame (snesbox's `dump`
 * after each frame: actors.sh makes them from the attract play), from the
 * directory and frame range on its command line:
 *
 *   aladdin_actor_check --headless --updates 1 -- DIR FIRST LAST [KINDS]
 *
 * KINDS: the kinds to compare, hex, by commas (36,95,96); the core's kinds
 * when left out. Dumps are DIR/fNNNNN.wram, FIRST to LAST.
 *
 * The first frame's dump sets the actors' state (records, free lists,
 * kinds' graphics, animation sets and data, palette slots). Then for each
 * frame: an actor the game's other code made in that frame (a record free
 * in the native state and live in the dump) is made the same way here: the
 * story's by ActorSpawnAt at the dumped place when the dump shows it run
 * once (the story spawns before the actor loop), or by ActorCreate and the
 * dumped record when it shows it new (made after the loop). Then
 * PlayerLoop, ActorLoop and ActorsInView, as FrameStep runs them, as many
 * times as the original ran FrameStep in that frame (its play-time count;
 * the range point and the story's text task from the dump), and every record of a
 * converted kind compared with the dump, field by field. Routines of other
 * areas the actors call are answered here from the dump and logged.
 *
 * Prints each differing field (the first 40), each logged call, and a
 * summary line; exits 1 when a field differs.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "actor/actor_own.h"
#include "data/actor_tables.h"
#include "data/data_internal.h"

#define WRAM_SIZE 0x20000
#define RECORD(i) (0x08DB + 0x50 * (i))

static const char *dir;
static int first, last, frame;
static uint8_t wram[WRAM_SIZE], before[WRAM_SIZE];
static long compared, differing;

static uint8_t converted[256] = { 0x36, 0x4A, 0x82, 0x85, 0x95, 0x96 };
static unsigned converted_count = 6;
static int custom_kinds;            /* a KINDS argument: other kinds follow the dump (follow_others) */
static uint8_t untracked[ACTOR_RECORDS];  /* a listed kind made mid-frame by code not compared: copied, not compared */

static int is_converted(uint8_t kind)
{
    return memchr(converted, kind, converted_count) != NULL;
}

static void load(int f, uint8_t *w)
{
    char path[1024];
    FILE *fp;

    snprintf(path, sizeof path, "%s/f%05d.wram", dir, f);
    fp = fopen(path, "rb");
    if (!fp || fread(w, 1, WRAM_SIZE, fp) != WRAM_SIZE) {
        fprintf(stderr, "check: cannot read %s\n", path);
        exit(2);
    }
    fclose(fp);
}

static uint16_t w16(const uint8_t *w, uint32_t a) { return (uint16_t)(w[a] | w[a + 1] << 8); }
static uint32_t w24(const uint8_t *w, uint32_t a) { return (uint32_t)w[a] | (uint32_t)w[a + 1] << 8 | (uint32_t)w[a + 2] << 16; }

static Actor *record_at(uint16_t addr)
{
    if (addr < RECORD(0) || (addr - RECORD(0)) % 0x50 || (addr - RECORD(0)) / 0x50 >= ACTOR_RECORDS)
        return NULL;
    return &g.actors[(addr - RECORD(0)) / 0x50];
}

/* The game's state outside the actors: every top-level value or array of g
   that state_fields.csv maps to work RAM, generated (gen_world.py). With a
   KINDS argument the check loads these from the dumps each frame, so the
   kinds meet the level's map, the player's state, the random number and the
   flags as the original had them. */
typedef struct {
    const char *name;
    size_t offset, elem;
    unsigned count, at, bytes;
} WorldField;

static const WorldField world[] = {
#include "world_fields.inc"
};

/* What the check keeps itself, not from the dump: the actors' pools, kind
   counts and animation and palette-slot tables (the code under test writes
   them, and they are compared), the draw lists and the video's. */
static int world_managed(const char *n)
{
    static const char *const kept[] = { "kind_", "anim_", "actor_", "palette_slot", "draw_", "placed", "free_", "tasks",
                                         "vram_", "tile_blocks", "sprite", "text_map", "palette", "unpacked", "band_",
                                         "pause_block", "range_" };
    unsigned k;

    for (k = 0; k < sizeof kept / sizeof kept[0]; k++)
        if (!strncmp(n, kept[k], strlen(kept[k])))
            return 1;
    return 0;
}

/* Aladdin's own fields: set by his turn of the loop, which the check does
   not run unless his kinds are listed. */
static int world_player(const char *n)
{
    return !strncmp(n, "player_", 7) || !strcmp(n, "health") || !strncmp(n, "test_box", 8);
}

static void load_world(const uint8_t *w, int player)
{
    unsigned f, k, b;

    for (f = 0; f < sizeof world / sizeof world[0]; f++) {
        const WorldField *wf = &world[f];

        if (world_managed(wf->name) || world_player(wf->name) != player)
            continue;
        for (k = 0; k < wf->count; k++) {
            uint8_t *to = (uint8_t *)&g + wf->offset + k * wf->elem;

            memset(to, 0, wf->elem);
            for (b = 0; b < wf->bytes && b < wf->elem; b++)
                to[b] = w[wf->at + k * wf->bytes + b];
        }
    }
    if (!player) {
        /* the play field's scroll ($1738 + $3A, $3E: a path with [] the
           generated table leaves out), which the map tests read for the player */
        g.layers[0].scroll_x = w24(w, 0x1738 + 0x3A);
        g.layers[0].scroll_y = w24(w, 0x1738 + 0x3E);
    }
    if (!player)
        for (k = 0; k < sizeof g.screen_map.places / sizeof g.screen_map.places[0]; k++)
            g.screen_map.places[k] = (uint16_t)(w[0x1F000 + 2 * k] | w[0x1F001 + 2 * k] << 8);
}

/* the record offsets of own.bytes, in order */
static const uint8_t own_at[32] = {
    0x0A, 0x11, 0x13, 0x16, 0x17, 0x18, 0x29, 0x2A, 0x35, 0x36,
    0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4C, 0x4D, 0x4E, 0x4F, 0x4A, 0x4B,
};

static void record_from(Actor *a, const uint8_t *w, int i)
{
    const uint8_t *r = w + RECORD(i);
    int k;

    a->state = r[0x00];
    a->wait = r[0x01];
    a->resume = ACTOR_START;
    a->code = NULL;                     /* set by the loop when it starts the record */
    a->kind = r[0x06];
    a->spawn_byte = r[0x07];
    a->anim_flags = r[0x08];
    a->flags = r[0x09];
    a->x = w24(r, 0x0B);
    a->y = w24(r, 0x0E);
    a->view_box = r[0x12];
    a->facing = r[0x14];
    a->flip = r[0x15];
    a->speed_x = signed24(w24(r, 0x19));
    a->accel_x = r[0x1C];
    a->speed_y = signed24(w24(r, 0x1D));
    a->accel_y = r[0x20];
    a->spawn_at = r[0x21];
    a->picture_gfx = (uint16_t)(w16(r, 0x22) - 0x2000);
    a->anim_table = w16(r, 0x24);
    a->frame_time = r[0x26];
    a->picture = r[0x27];
    a->redraw = r[0x28];
    a->look = w16(r, 0x2B);
    a->anim = r[0x2D];
    a->anim_high = r[0x2E];
    a->frame = r[0x2F];
    a->loop_back = r[0x30];
    a->gfx_base = w16(r, 0x31);
    a->pool_number = r[0x33];
    a->placement = r[0x34];
    a->next = record_at(w16(r, 0x37));
    a->tile_slot = r[0x39];
    for (k = 0; k < 32; k++)
        a->own.bytes[k] = r[own_at[k]];
    /* the places a kind may keep as a field: the word's address in the data
       bank $80 (the tables the demo reads there; a table in another bank
       starts wrong until the kind sets it) */
    a->probes.at = w16(r, 0x16) ? cart_offset(0x80, w16(r, 0x16)) : 0;
    a->probe_at = r[0x18];
    a->own_pictures.at = w16(r, 0x29) ? cart_offset(0x80, w16(r, 0x29)) : 0;
    a->stream.at = w16(r, 0x4A) ? cart_offset(0x80, w16(r, 0x4A)) : 0;
}

static void state_from(const uint8_t *w)
{
    int i;

    for (i = 0; i < ACTOR_RECORDS; i++)
        record_from(&g.actors[i], w, i);
    g.free_pool_top = (int8_t)(w[0x15E4] & 0x80 ? -1 : w[0x15E4] / 2);
    for (i = 0; i < ACTOR_POOL_SIZE; i++)
        g.free_pool[i] = record_at(w16(w, 0x15A2 + 2 * i));
    g.free_pool_b_top = (int8_t)(w[0x15E6] & 0x80 ? -1 : w[0x15E6] / 2);
    for (i = 0; i < ACTOR_POOL_B_SIZE; i++)
        g.free_pool_b[i] = record_at(w16(w, 0x15DA + 2 * i));
    memcpy(g.kind_count, w + 0xFF00, 256);
    memcpy(g.placed, w + 0xC600, 256);
    memcpy(g.anim_set_of_kind, w + 0x1F000, 256);
    memcpy(g.anim_set_place, w + 0x1E000, 256);
    for (i = 0; i < 16; i++) {
        g.anim_set_pictures[i] = w16(w, 0x1E100 + 2 * i);
        g.anim_set_table[i] = w16(w, 0x1E300 + 2 * i);
    }
    memcpy(g.actor_pictures, w + 0x2000, sizeof g.actor_pictures);
    for (i = 0; i < 2048; i++)
        g.anim_data[i] = w16(w, 0xB000 + 2 * i);
    for (i = 0; i < 256; i++) {
        g.kind_gfx_at[i] = w16(w, 0x1E500 + 2 * i);
        g.kind_look[i] = w16(w, 0x1E700 + 2 * i);
        g.kind_extra_slots[i] = w[0x1E900 + i];
        g.kind_palette[i] = w[0x1EA00 + i];
        g.kind_slot[i] = w[0x1EB00 + i];
    }
    memcpy(g.palette_slot_users, w + 0x15E9, 4);
    memcpy(g.palette_slot_kind, w + 0x15ED, 4);
}

/* ---- other areas' routines, answered from the dump and logged: the
   check links the whole game and takes these calls over with the linker's
   --wrap (CMakeLists.txt) ---- */

void __wrap_SoundQueueAdd(uint8_t sound) { printf("frame %d: sound $%02X\n", frame, sound); }
void __wrap_PaletteLoad(int set) { printf("frame %d: palette set $%02X\n", frame, set); }
void __wrap_TileSlotFill(uint8_t kind) { printf("frame %d: palette slot filled for kind $%02X\n", frame, kind); }
void __wrap_TaskCreate(uint8_t slot, uint8_t param, TaskEntry entry)
{
    printf("frame %d: task in slot %u, parameter %u, entry %d\n", frame, slot, param, (int)entry);
}
int __wrap_task_busy(uint8_t slot) { return before[0x006F + 0x18 * slot] != 0; }

/* 82:ABCE TileSlotClaim, as the record has it, for the check */
int __wrap_TileSlotClaim(uint8_t kind, Actor *a)
{
    int x;

    for (x = 3; x >= 0 && g.palette_slot_users[x]; x--)
        ;
    if (x < 0)
        return -1;
    g.palette_slot_users[x]++;
    g.palette_slot_kind[x] = kind;
    a->tile_slot = (uint8_t)(x + 1);
    g.kind_slot[kind] = (uint8_t)(x + 1);
    a->look = palette_slot_look((uint8_t)(x + 1));
    g.kind_look[kind] = a->look;
    return 0;
}

/* ---- the comparison ---- */

static long differ_by_kind[256], first_differ_frame[256];
static long anim_set_differ;        /* AnimSetsLoad against the first dump: not a record's */

static void differ(int i, const char *field, long native, long original)
{
    differing++;
    if (!differ_by_kind[g.actors[i].kind]++)
        first_differ_frame[g.actors[i].kind] = frame;
    if (differing <= 40)
        printf("frame %d actor %d kind $%02X: %s native $%lX, original $%lX\n", frame, i, g.actors[i].kind, field,
               native, original);
}

#define SAME_SET(field, nat, orig) do { compared++; if ((long)(nat) != (long)(orig)) { anim_set_differ++; differing += !custom_kinds; \
        if (anim_set_differ <= 5) printf("anim sets: %s native $%lX, original $%lX\n", field, (long)(nat), (long)(orig)); } } while (0)
#define SAME(field, nat, orig) do { compared++; if ((long)(nat) != (long)(orig)) differ(i, field, (long)(nat), (long)(orig)); } while (0)

/* A data reference (a cartridge offset) against the original's word: the
   same place in the LoROM bank, whichever bank the word's table is in. */
static int ref_is(uint32_t at, uint16_t word)
{
    return word ? ((at & 0x7FFFu) | 0x8000u) == word && at != 0 : at == 0;
}

static void compare(int i)
{
    const uint8_t *r = wram + RECORD(i);
    Actor *a = &g.actors[i];
    Actor *next = record_at(w16(r, 0x37));
    int k;

    SAME("state", a->state, r[0x00]);
    if (!r[0x00] && !a->state)
        return;
    SAME("wait", a->wait, r[0x01]);
    SAME("kind", a->kind, r[0x06]);
    SAME("+$07", a->spawn_byte, r[0x07]);
    SAME("anim_flags", a->anim_flags, r[0x08]);
    SAME("flags", a->flags, r[0x09]);
    SAME("x", a->x, w24(r, 0x0B));
    SAME("y", a->y, w24(r, 0x0E));
    SAME("view_box", a->view_box, r[0x12]);
    SAME("facing", a->facing, r[0x14]);
    SAME("flip", a->flip, r[0x15]);
    SAME("speed_x", (uint32_t)a->speed_x & 0xFFFFFF, w24(r, 0x19));
    SAME("accel_x", a->accel_x, r[0x1C]);
    SAME("speed_y", (uint32_t)a->speed_y & 0xFFFFFF, w24(r, 0x1D));
    SAME("accel_y", a->accel_y, r[0x20]);
    SAME("spawn_at", a->spawn_at, r[0x21]);
    SAME("picture_gfx", (uint16_t)(a->picture_gfx + 0x2000), w16(r, 0x22));
    SAME("anim_table", a->anim_table, w16(r, 0x24));
    SAME("frame_time", a->frame_time, r[0x26]);
    SAME("picture", a->picture, r[0x27]);
    SAME("look", a->look, w16(r, 0x2B));
    SAME("anim", a->anim, r[0x2D]);
    SAME("frame", a->frame, r[0x2F]);
    SAME("loop_back", a->loop_back, r[0x30]);
    SAME("gfx_base", a->gfx_base, w16(r, 0x31));
    SAME("pool_number", a->pool_number, r[0x33]);
    SAME("placement", a->placement, r[0x34]);
    SAME("next", a->next ? (long)(a->next - g.actors) : -1, next ? (long)(next - g.actors) : -1);
    SAME("tile_slot", a->tile_slot, r[0x39]);
    for (k = 0; k < 32; k++) {
        char name[16];
        uint8_t o = own_at[k];

        snprintf(name, sizeof name, "+$%02X", o);
        if (o == 0x17 || o == 0x2A || o == 0x4B || o == 0x4E || o == 0x4F)
            continue;           /* a pair's second byte, with its first; $4E-$4F: kind $95's hops keep their return there, none here */
        if (o == 0x16 || o == 0x29 || o == 0x4A) {
            /* a place a kind keeps as its own bytes or as a field (probes,
               own_pictures, stream): the word matches in either form */
            uint16_t word = w16(r, o), own = (uint16_t)(a->own.bytes[k] | a->own.bytes[k + 1] << 8);
            uint32_t at = o == 0x16 ? a->probes.at : o == 0x29 ? a->own_pictures.at : a->stream.at;

            compared++;
            if (own != word && !ref_is(at, word))
                differ(i, name, own ? own : at, word);
        } else if (o == 0x18) {
            compared++;
            if (a->own.bytes[k] != r[0x18] && a->probe_at != r[0x18])
                differ(i, name, a->own.bytes[k] ? a->own.bytes[k] : a->probe_at, r[0x18]);
        } else {
            SAME(name, a->own.bytes[k], r[o]);
        }
    }
}

/* Actors the game's other code made in this frame, made here the same
   way: records free here and live in the dump. Before the loop (`after` 0):
   the story's (kinds $36, $95, $96, which the dump shows run once), by
   ActorSpawnAt at the dumped place, in the order the free list hands out
   records, as the story's calls take them. After it (`after` 1): those the
   dump shows new, by ActorCreate and the maker's settings from the dump. */
static int made_here(int i, int after)
{
    const uint8_t *r = wram + RECORD(i);

    if (g.actors[i].state || !r[0x00] || (r[0x00] == ACTOR_STATE_NEW) != after)
        return 0;
    if (after)
        return is_converted(r[0x06]);
    return r[0x06] == 0x36 || r[0x06] == 0x95 || r[0x06] == 0x96;
}

static void made_by_others(int after)
{
    while (g.free_pool_top >= 0 && g.free_pool[g.free_pool_top]) {
        int i = (int)(g.free_pool[g.free_pool_top] - g.actors);
        const uint8_t *r = wram + RECORD(i);
        Actor *a;

        if (!made_here(i, after))
            break;
        a = after ? ActorCreate(r[0x06]) : ActorSpawnAt(r[0x06], r[0x07], w16(r, 0x0C), w16(r, 0x0F));
        if (a != &g.actors[i]) {
            printf("frame %d: kind $%02X not made in actor %d (got %ld)\n", frame, r[0x06], i, a ? (long)(a - g.actors) : -1L);
            differing++;
            break;
        }
        if (after)
            record_from(a, wram, i);    /* the maker's own settings */
        printf("frame %d: actor %d made (kind $%02X, +$07 $%02X)\n", frame, i, r[0x06], r[0x07]);
    }
    for (int i = 1; i < ACTOR_RECORDS; i++)
        if (made_here(i, after)) {
            if (custom_kinds && !after)
                continue;               /* made_for_kinds handles it */
            printf("frame %d: kind $%02X in actor %d, which the free list does not hand out next\n", frame, wram[RECORD(i) + 6], i);
            differing++;
        }
}

/* The story's or the title's ActorsFreeAll (81:9D82) when every pool
   record is free in the dump, or new there (made after it): tried on a copy
   of the state, and kept when afterwards the kinds' counts, with the new
   records' kinds counted in, are the dump's. With no actor live it shows
   only in the counts: it counts each record's kind off, live or not. */
static void freed_by_others(void)
{
    static GameState saved;
    uint8_t count[256];
    int i;

    for (i = ACTOR_RECORDS - ACTOR_POOL_SIZE; i < ACTOR_RECORDS; i++)
        if (wram[RECORD(i)] && wram[RECORD(i)] != ACTOR_STATE_NEW)
            return;
    if (!memcmp(g.kind_count, wram + 0xFF00, 256))
        return;
    saved = g;
    ActorsFreeAll();
    memcpy(count, g.kind_count, 256);
    for (i = ACTOR_RECORDS - ACTOR_POOL_SIZE; i < ACTOR_RECORDS; i++)
        if (wram[RECORD(i)] == ACTOR_STATE_NEW)
            count[wram[RECORD(i) + 6]]++;
    if (memcmp(count, wram + 0xFF00, 256)) {
        g = saved;
        return;
    }
    printf("frame %d: ActorsFreeAll (the game's)\n", frame);
}

/* The free lists, the palette slots and the converted kinds' counts. */
static void compare_pools(void)
{
    int i = 0, k;
    int top = (int8_t)(wram[0x15E4] & 0x80 ? -1 : wram[0x15E4] / 2);

    SAME("free list's top", g.free_pool_top, top);
    for (k = 0; k <= top && k < ACTOR_POOL_SIZE; k++)
        SAME("free list entry", g.free_pool[k] - g.actors, record_at(w16(wram, 0x15A2 + 2 * k)) - g.actors);
    for (k = 0; k < 4; k++) {
        SAME("palette slot users", g.palette_slot_users[k], wram[0x15E9 + k]);
        SAME("palette slot kind", g.palette_slot_kind[k], wram[0x15ED + k]);
    }
    for (k = 0; k < (int)converted_count; k++) {
        SAME("kind count", g.kind_count[converted[k]], wram[0xFF00 + converted[k]]);
        SAME("kind look", g.kind_look[converted[k]], w16(wram, 0x1E700 + 2 * converted[k]));
        SAME("kind slot", g.kind_slot[converted[k]], wram[0x1EB00 + converted[k]]);
    }
}

/* With a KINDS argument, before the loop: a listed kind live in the dump
   and free here was made by other code. Made after the loop (new in the
   dump): copied as it is, not run yet, so the loop starts it here as there.
   Made before (run once in the dump) from a placement (+$34 not $FF): its
   record rebuilt as ActorPlace leaves it (the record as it was before,
   the kind, its set-up's look and palette slot from the dump, +$07, x and
   y from the placement, new), so the loop starts it here. Made before by
   other code: follow_others follows it. */
static void made_for_kinds(void)
{
    int i;

    /* Aladdin's record, made by the level's set-up in the frame it first runs:
       new, with what the set-up gave it (his place, kind and +$07, look and
       box, as the dump has them) and nothing his start sets */
    if (is_converted(0x00) && !g.actors[0].state && !before[RECORD(0)] && wram[RECORD(0)] && !untracked[0]) {
        Actor *p = &g.actors[0];
        const uint8_t *r = wram + RECORD(0);

        record_from(p, wram, 0);
        p->state = ACTOR_STATE_NEW;
        p->wait = 0;
        p->resume = ACTOR_START;
        p->kind = r[0x06];
        p->spawn_byte = r[0x07];
        p->anim_flags = 0;
        p->flags = 0;
        p->facing = p->flip = 0;
        p->speed_x = p->speed_y = 0;
        p->accel_x = p->accel_y = 0;
        p->spawn_at = 0;
        p->redraw = 0;
        p->probes.at = 0;
        p->probe_at = 0;
        p->own_pictures.at = 0;
        memset(p->own.bytes, 0, sizeof p->own.bytes);
        printf("frame %d: actor 0 (kind $%02X) made by the level's set-up: new\n", frame, p->kind);
    }

    for (i = 1; i < ACTOR_RECORDS; i++) {
        const uint8_t *r = wram + RECORD(i);
        Actor *a = &g.actors[i];

        if (a->state || !r[0x00] || !is_converted(r[0x06]) || untracked[i])
            continue;
        if (r[0x00] == ACTOR_STATE_NEW) {
            continue;                   /* made after the loop: copied after it (made_new_after) */
        } else if (i == 1 && r[0x06] == 0x08 && level_escort(g.level)) {
            /* the level's escort: EscortStart made it before the loop, and the dump shows it run once */
            if (!is_converted(0x00))
                record_from(&g.actors[0], wram, 0);    /* his place (listed, his record is the native one's) */
            EscortStart();
            printf("frame %d: actor 1 (kind $08) made by EscortStart\n", frame);
        } else if (r[0x34] != 0xFF && r[0x34] < placement_count(w16(wram, 0x035E))
                   && placement_at(w16(wram, 0x035E), r[0x34]).kind == r[0x06]
                   && !(before[0xC600 + r[0x34]] & 1) && (wram[0xC600 + r[0x34]] & 1)) {
            /* placed in this frame: the level's placement +$34 names has this kind and its placed flag went
               up (ActorPlace sets it) in this frame (+$34 is also a kind's own byte, 0 in a record other code
               made: that follows the dump; the +$07 can not be compared, a kind may change it in its first turn) */
            Placement p = placement_at(w16(wram, 0x035E), r[0x34]);

            record_from(a, before, i);
            a->kind = r[0x06];
            a->look = w16(r, 0x2B);
            a->tile_slot = r[0x39];
            a->spawn_byte = (uint8_t)(p.byte & 0x1F);
            a->x = with_pixels(a->x, (uint16_t)(p.x_word & 0x3FFF));
            a->y = with_pixels(a->y, (uint16_t)(p.y_word & 0x3FFF));
            a->placement = r[0x34];
            a->state = ACTOR_STATE_NEW;
            printf("frame %d: actor %d (kind $%02X) placed from placement %u\n", frame, i, r[0x06], r[0x34]);
        }
    }
}

/* With a KINDS argument, after the loop: a listed kind new in the dump and
   free here was made after the original's loop: copied as it is (not run
   yet), so the next frame's loop starts it here as there. */
static void made_new_after(void)
{
    int i;

    for (i = 1; i < ACTOR_RECORDS; i++) {
        const uint8_t *r = wram + RECORD(i);

        if (!g.actors[i].state && r[0x00] == ACTOR_STATE_NEW && is_converted(r[0x06])) {
            record_from(&g.actors[i], wram, i);
            printf("frame %d: actor %d (kind $%02X) made new by other code: copied\n", frame, i, r[0x06]);
        }
    }
}

/* With a KINDS argument, after each frame: every record of a kind not
   listed, the free lists and the kinds' counts taken from the dump, so the
   listed kinds meet the same surroundings as in the original. A listed kind
   that code not compared made mid-frame (live in the dump, free here, not
   new) is copied too, and not compared until it ends. */
static void follow_others(void)
{
    int i, k;

    for (i = 0; i < ACTOR_RECORDS; i++) {
        int live = wram[RECORD(i)] != 0;
        int tracked = g.actors[i].state && is_converted(g.actors[i].kind) && !untracked[i];

        if (!live)
            untracked[i] = 0;
        if (tracked)
            continue;               /* the native code's own: compared above */
        if (live && is_converted(wram[RECORD(i) + 6]) && !untracked[i]) {
            untracked[i] = 1;
            printf("frame %d: actor %d (kind $%02X) made by code not compared: followed, not compared\n", frame, i,
                   wram[RECORD(i) + 6]);
        }
        record_from(&g.actors[i], wram, i);
    }
    g.free_pool_top = (int8_t)(wram[0x15E4] & 0x80 ? -1 : wram[0x15E4] / 2);
    for (k = 0; k < ACTOR_POOL_SIZE; k++)
        g.free_pool[k] = record_at(w16(wram, 0x15A2 + 2 * k));
    g.free_pool_b_top = (int8_t)(wram[0x15E6] & 0x80 ? -1 : wram[0x15E6] / 2);
    for (k = 0; k < ACTOR_POOL_B_SIZE; k++)
        g.free_pool_b[k] = record_at(w16(wram, 0x15DA + 2 * k));
    memcpy(g.kind_count, wram + 0xFF00, 256);
    memcpy(g.placed, wram + 0xC600, 256);
    memcpy(g.palette_slot_users, wram + 0x15E9, 4);
    memcpy(g.palette_slot_kind, wram + 0x15ED, 4);
}

/* How many times the original ran FrameStep (81:8311) in this frame: its
   PlayTimeTick steps the play time's frames ($147C, 0-59) and seconds
   ($147D) once a call. */
static int frame_steps(void)
{
    int was = before[0x147D] * 60 + before[0x147C], now = wram[0x147D] * 60 + wram[0x147C];
    int steps = (now - was + 256 * 60) % (256 * 60), i;

    /* A FrameStep whose loops ran before this dump and whose tail (ActorsInView, the sprites, the play time's
       tick) did not yet: the level's first, or one that overran its frame (the original slows down there).
       Any live actor record (the player's is his turn's: not counted) that changed since the last dump says the
       loops ran; the tail comes with the next tick */
    if (steps == 0)
        for (i = 1; i < ACTOR_RECORDS; i++) {
            const uint8_t *b = before + RECORD(i), *n = wram + RECORD(i);

            if (b[0] && n[0] && !(b[0] == ACTOR_STATE_NEW && n[0] == ACTOR_STATE_NEW) && memcmp(b, n, 0x50) != 0)
                return -1;      /* once: the loops alone (a record just made, or just freed, is other code's) */
        }
    return steps;
}

/* AnimSetsLoad (81:8F47) for the dumped level against what the original
   loaded: each kind's place, each place's starts, and the picture lists and
   animation tables as far as the sets loaded reach. Not each kind's set:
   the level's load runs LevelActorGraphicsLoad (81:B9F4) next, which fills
   that table again (AnimSetsFill 0, then its marks). */
static void check_anim_sets(void)
{
    int i, n = 0, frame_was = frame;
    unsigned bytes = 0, words = 0;

    frame = first;
    g.level = w16(wram, 0x035E);
    AnimSetsLoad();
    for (i = 0; i < 256; i++) {
        SAME_SET("anim set place", g.anim_set_place[i], wram[0x1E000 + i]);
        if (g.anim_set_place[i] != 4 && g.anim_set_place[i] + 1 > n)
            n = g.anim_set_place[i] + 1;
    }
    for (i = 0; i < n; i++) {
        SAME_SET("set's pictures start", g.anim_set_pictures[i], w16(wram, 0x1E100 + 2 * i));
        SAME_SET("set's table start", g.anim_set_table[i], w16(wram, 0x1E300 + 2 * i));
    }
    /* the sets' data: up to the last set's end */
    for (i = 0; i < 256; i++)
        if (g.anim_set_place[i] == n - 1 && n) {
            uint16_t b, w;

            (void)anim_set_pictures(g.anim_set_of_kind[i], &b);
            (void)anim_set_table(g.anim_set_of_kind[i], &w);
            bytes = g.anim_set_pictures[n - 1] + b;
            words = g.anim_set_table[n - 1] / 2u + w;
            break;
        }
    for (i = 0; i < (int)bytes; i++)
        SAME_SET("picture list byte", g.actor_pictures[i], wram[0x2000 + i]);
    for (i = 0; i < (int)words; i++)
        SAME_SET("animation data word", g.anim_data[i], w16(wram, 0xB000 + 2 * i));
    printf("anim sets: level $%02X, %d sets, %u picture bytes, %u table words compared\n", g.level, n, bytes, words);
    frame = frame_was;
}

/* One FrameStep's two loops. FrameStep sets the range point after them, so
   the loops see the last frame's and ActorsInView this frame's (the dumps'
   $1A44/$1A46). With a KINDS argument Aladdin's turn is not run unless his
   kinds are listed: his record and fields are the dump's, as his turn left
   them (the end of the frame: what an actor changed in the loop after him
   shows early). */
/* The loop over the other records with a KINDS argument, one record at a
   time as the original took them: a record the native code runs (a listed
   kind, live here) takes its turn; any other (an unlisted kind, one other
   code made, a free one) is loaded as the dump left it at the end of the
   frame, right where its turn comes, so the records after it see it as
   the original's did (the apple that hit a guard is in its new place when
   the guard looks), and the records before it saw it as the frame began. */
static void other_records(void)
{
    int i;

    for (i = 1; i < ACTOR_RECORDS; i++) {
        Actor *a = &g.actors[i];
        int tracked = a->state && is_converted(a->kind) && !untracked[i];

        if (tracked) {
            if (!ActorTurn(a))
                return;
        } else if (!a->state && wram[RECORD(i)] == ACTOR_STATE_NEW) {
            /* new in the dump: made after the loop, so not there for this frame's: made_new_after copies it */
        } else {
            if (wram[RECORD(i)] && is_converted(wram[RECORD(i) + 6]) && !untracked[i] && !a->state) {
                untracked[i] = 1;
                printf("frame %d: actor %d (kind $%02X) made by code not compared: followed, not compared\n", frame, i,
                       wram[RECORD(i) + 6]);
            }
            record_from(a, wram, i);
            if (a->state > ACTOR_STATE_NEW && wram[RECORD(i)] > ACTOR_STATE_NEW)
            {
                /* a state a hit puts it in ($8C, $CC): the hit is a later record's (ActorTakesShot sets the shot's
                   state, and ActorStateSet8C also sets bit 7 of its +$08, which no later shot test lets pass);
                   the dump's is the frame's end, which follow_others loads after the loop */
                a->state = ACTOR_STATE_WAITING;
                a->anim_flags &= 0x7F;
            }
        }
        if (i + 1 >= ACTOR_RECORDS - ACTOR_POOL_SIZE && g.pause_block)
            return;
    }
}

static void loops(void)
{
    if (custom_kinds) {
        g.range_x = w16(before, 0x1A44);
        g.range_y = w16(before, 0x1A46);
        if (is_converted(0x00)) {
            PlayerLoop();
        } else {
            record_from(&g.actors[0], wram, 0);
            load_world(wram, 1);
            if (wram[RECORD(0)] >= 0x80 && before[RECORD(0)] < 0x80) {
                /* the dump has him hit in this frame (state $8C): by an actor after most of the loop; the
                   earlier ones saw him as the frame began, so the loop gets that: the hit lands at its end */
                g.actors[0].state = before[RECORD(0)];
                g.health = before[0x0367];
                g.player_hurt_count = before[0x141B];
            }
        }
        other_records();
        return;
    }
    PlayerLoop();
    ActorLoop();
}

static void in_view(void)
{
    g.range_x = w16(wram, 0x1A44);
    g.range_y = w16(wram, 0x1A46);
    ActorsInView();
}

static void init(void)
{
    int i, steps, tail_pending = 0;

    data_load();
    load(first, wram);
    check_anim_sets();
    state_from(wram);
    if (custom_kinds)
        for (i = 1; i < ACTOR_RECORDS; i++)
            if (g.actors[i].state && g.actors[i].state != ACTOR_STATE_NEW && is_converted(g.actors[i].kind)) {
                /* running already: where it goes on in its kind's code is the kind's own, unknown here */
                untracked[i] = 1;
                printf("frame %d: actor %d (kind $%02X) already running in the first dump: followed, not compared\n", first, i,
                       g.actors[i].kind);
            }
    for (frame = first + 1; frame <= last; frame++) {
        memcpy(before, wram, WRAM_SIZE);
        load(frame, wram);
        g.range_x = w16(wram, 0x1A44);
        g.range_y = w16(wram, 0x1A46);
        g.pause_block = wram[0x0404];
        if (custom_kinds) {
            load_world(before, 0);      /* the world the frame starts from */
            /* but the pads as this frame read them (before its loops: the
               task scheduler's first turn): the dump's, not the last frame's */
            memcpy(g.pad_held, wram + 0x0377, sizeof g.pad_held);
            memcpy(g.pad_pressed, wram + 0x037B, sizeof g.pad_pressed);
            g.pad1_held = w16(wram, 0x037F);
            g.pad1_pressed = w16(wram, 0x0381);
            memcpy(g.action_held, wram + 0x0383, sizeof g.action_held);
            memcpy(g.action_pressed, wram + 0x0387, sizeof g.action_pressed);
        }
        freed_by_others();
        if (custom_kinds)
            made_for_kinds();
        else
            made_by_others(0);
        steps = frame_steps();
        if (steps > 0 && tail_pending) {
            /* the rest of a FrameStep whose loops ran before the last dump:
               its ActorsInView and its play time's tick came in this frame */
            in_view();
            steps--;
        }
        tail_pending = 0;
        if (steps < 0) {
            /* a FrameStep's loops ran before this dump and the rest of it did
               not (the level's first: the in-view flag and the play time
               untouched); its tail comes with the next tick */
            loops();
            tail_pending = 1;
        }
        for (; steps > 0; steps--) {
            loops();
            in_view();
        }
        if (custom_kinds)
            made_new_after();
        else
            made_by_others(1);
        if (!custom_kinds)
            compare_pools();
        for (i = custom_kinds && is_converted(0x00) ? 0 : 1; i < ACTOR_RECORDS; i++) {
            if (custom_kinds && !g.actors[i].state && wram[RECORD(i)] && wram[RECORD(i) + 6] != g.actors[i].kind)
                ;               /* freed here and reused by another kind in the same frame: the original's new record is not the old kind's */
            else if (!untracked[i] && (is_converted(g.actors[i].kind) || is_converted(wram[RECORD(i) + 6])))
                compare(i);
            g.actors[i].redraw = wram[RECORD(i) + 0x28];    /* cleared by the sprite code, not compared */
        }
        if (custom_kinds)
            follow_others();
    }
    for (i = 0; i < 256; i++)
        if (differ_by_kind[i])
            printf("actor check: kind $%02X: %ld fields differ, the first at frame %ld\n", i, differ_by_kind[i],
                   first_differ_frame[i]);
    if (anim_set_differ)
        printf("actor check: AnimSetsLoad: %ld fields differ from the first dump%s\n", anim_set_differ,
                   custom_kinds ? " (told, not counted: a first dump that is not a level's, as a title's, has other sets; the dump's tables are used from there)" : "");
    printf("actor check: frames %d-%d, %ld fields compared, %ld differ\n", first + 1, last, compared, differing);
}

static void update(const nshvc_input *in)
{
    (void)in;
    nshvc_quit();
}

static void draw(float between) { (void)between; }

int main(int argc, char **argv)
{
    static nshvc_game game;
    int i;

    for (i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--"))
            break;
    if (argc - i - 1 < 3) {
        fprintf(stderr, "usage: aladdin_actor_check --headless --updates 1 -- DIR FIRST LAST\n");
        return 2;
    }
    dir = argv[i + 1];
    first = atoi(argv[i + 2]);
    last = atoi(argv[i + 3]);
    if (argc - i - 1 >= 4) {
        char *p = argv[i + 4];

        converted_count = 0;
        custom_kinds = 1;
        while (*p && converted_count < 256) {
            converted[converted_count++] = (uint8_t)strtoul(p, &p, 16);
            if (*p == ',')
                p++;
        }
    }
    game.title = "Aladdin actor check";
    game.id = "aladdin-actor-check";
    game.update_hz = 60;
    game.init = init;
    game.update = update;
    game.draw = draw;
    {
        int rc = nshvc_run(&game, i, argv);

        return rc ? rc : differing != 0;
    }
}
