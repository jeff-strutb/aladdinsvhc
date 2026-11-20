/*
 * check.c: the native actor code against the original, frame by frame.
 *
 * Reads work-RAM dumps of the original, one per frame (snesbox's `dump`
 * after each frame: actors.sh makes them from the attract play), from the
 * directory and frame range on its command line:
 *
 *   aladdin_actor_check --headless --updates 1 -- DIR FIRST LAST
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "actor/actor_own.h"
#include "data/actor_tables.h"

#define WRAM_SIZE 0x20000
#define RECORD(i) (0x08DB + 0x50 * (i))

static const char *dir;
static int first, last, frame;
static uint8_t wram[WRAM_SIZE], before[WRAM_SIZE];
static long compared, differing;

static const uint8_t converted[] = { 0x36, 0x4A, 0x82, 0x85, 0x95, 0x96 };

static int is_converted(uint8_t kind)
{
    return memchr(converted, kind, sizeof converted) != NULL;
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

/* the record offsets of own.bytes, in order */
static const uint8_t own_at[30] = {
    0x0A, 0x11, 0x13, 0x16, 0x17, 0x18, 0x29, 0x2A, 0x35, 0x36,
    0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4C, 0x4D, 0x4E, 0x4F,
};

static void record_from(Actor *a, const uint8_t *w, int i)
{
    const uint8_t *r = w + RECORD(i);
    int k;

    a->state = r[0x00];
    a->wait = r[0x01];
    a->resume = ACTOR_START;
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
    for (k = 0; k < 30; k++)
        a->own.bytes[k] = r[own_at[k]];
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

/* ---- other areas' routines, answered from the dump and logged ---- */

void unconverted(const char *name) { printf("frame %d: unconverted %s\n", frame, name); }

void SoundQueueAdd(uint8_t sound) { printf("frame %d: sound $%02X\n", frame, sound); }
void PaletteLoad(int set) { printf("frame %d: palette set $%02X\n", frame, set); }
void TileSlotFill(uint8_t kind) { printf("frame %d: palette slot filled for kind $%02X\n", frame, kind); }
void TaskCreate(uint8_t slot, uint8_t param, TaskEntry entry)
{
    printf("frame %d: task in slot %u, parameter %u, entry %d\n", frame, slot, param, (int)entry);
}
int task_busy(uint8_t slot) { return before[0x006F + 0x18 * slot] != 0; }

/* 82:ABCE TileSlotClaim, as the record has it, for the check */
int TileSlotClaim(uint8_t kind, Actor *a)
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

static void differ(int i, const char *field, long native, long original)
{
    differing++;
    if (differing <= 40)
        printf("frame %d actor %d kind $%02X: %s native $%lX, original $%lX\n", frame, i, g.actors[i].kind, field,
               native, original);
}

#define SAME(field, nat, orig) do { compared++; if ((long)(nat) != (long)(orig)) differ(i, field, (long)(nat), (long)(orig)); } while (0)

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
    for (k = 0; k < 30; k++) {
        char name[16];

        snprintf(name, sizeof name, "+$%02X", own_at[k]);
        if (own_at[k] != 0x4E && own_at[k] != 0x4F)   /* kind $95's hops keep their return address there: none here */
            SAME(name, a->own.bytes[k], r[own_at[k]]);
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
    while (g.free_pool_top >= 0) {
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
    for (k = 0; k < (int)sizeof converted; k++) {
        SAME("kind count", g.kind_count[converted[k]], wram[0xFF00 + converted[k]]);
        SAME("kind look", g.kind_look[converted[k]], w16(wram, 0x1E700 + 2 * converted[k]));
        SAME("kind slot", g.kind_slot[converted[k]], wram[0x1EB00 + converted[k]]);
    }
}

/* How many times the original ran FrameStep (81:8311) in this frame: its
   PlayTimeTick steps the play time's frames ($147C, 0-59) and seconds
   ($147D) once a call. */
static int frame_steps(void)
{
    int was = before[0x147D] * 60 + before[0x147C], now = wram[0x147D] * 60 + wram[0x147C];

    return (now - was + 256 * 60) % (256 * 60);
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
        SAME("anim set place", g.anim_set_place[i], wram[0x1E000 + i]);
        if (g.anim_set_place[i] != 4 && g.anim_set_place[i] + 1 > n)
            n = g.anim_set_place[i] + 1;
    }
    for (i = 0; i < n; i++) {
        SAME("set's pictures start", g.anim_set_pictures[i], w16(wram, 0x1E100 + 2 * i));
        SAME("set's table start", g.anim_set_table[i], w16(wram, 0x1E300 + 2 * i));
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
        SAME("picture list byte", g.actor_pictures[i], wram[0x2000 + i]);
    for (i = 0; i < (int)words; i++)
        SAME("animation data word", g.anim_data[i], w16(wram, 0xB000 + 2 * i));
    printf("anim sets: level $%02X, %d sets, %u picture bytes, %u table words compared\n", g.level, n, bytes, words);
    frame = frame_was;
}

static void init(void)
{
    int i, steps;

    data_load();
    load(first, wram);
    check_anim_sets();
    state_from(wram);
    for (frame = first + 1; frame <= last; frame++) {
        memcpy(before, wram, WRAM_SIZE);
        load(frame, wram);
        g.range_x = w16(wram, 0x1A44);
        g.range_y = w16(wram, 0x1A46);
        g.pause_block = wram[0x0404];
        freed_by_others();
        made_by_others(0);
        for (steps = frame_steps(); steps > 0; steps--) {
            PlayerLoop();
            ActorLoop();
            ActorsInView();
        }
        made_by_others(1);
        compare_pools();
        for (i = 1; i < ACTOR_RECORDS; i++) {
            if (is_converted(g.actors[i].kind) || is_converted(wram[RECORD(i) + 6]))
                compare(i);
            g.actors[i].redraw = wram[RECORD(i) + 0x28];    /* cleared by the sprite code, not compared */
        }
    }
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
