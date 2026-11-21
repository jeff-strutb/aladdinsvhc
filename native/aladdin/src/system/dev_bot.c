/*
 * dev_bot.c: a developer's player (not the game's): it plays a level by the pad's keys so a play can be made and
 * kept as an input file (native/checks/plays/). From update ALADDIN_DEV_BOT on, each SEG updates it tries every key
 * combination for those updates in a forked copy of the whole process, scores where Aladdin ends (further right,
 * alive, no health lost, or the level changed), applies the best for real and appends the update's keys to
 * ALADDIN_DEV_BOT_OUT in the input file's form. ALADDIN_DEV_BOT_SEG (updates a choice holds, default 16),
 * ALADDIN_DEV_BOT_ROLLOUTS and _LEN (the random runs tried when stuck, default 200 of 16 choices), ALADDIN_DEV_BOT_DEPTH (choices looked ahead, default 1), ALADDIN_DEV_BOT_GOAL ("x" right, "-x" left, "y" down,
 * "-y" up, "exit" the level's exit spots: what the score follows; ALADDIN_DEV_BOT_SWITCH=X: from player x X on, "exit").
 * ALADDIN_DEV_BOT_WAY="x,y;x,y;...": waypoints, planned from the map (DevMapDump) and pictures: the player goes to each in turn
 * (y -1: only x counts, x -1: only y), goes back to the nearest earlier one when it falls well below the last reached on a climb, and after the
 * last follows the exit spots. ALADDIN_DEV_BOT_GE=N: the search by an archive of places (below), N copies at a time: it finds the
 * whole way to the level's exit before the first update is played. Linux only (fork).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor/actor_own.h"
#include "data/actor_tables.h"
#include "data/level_tables.h"
#include "map/map.h"
#include "nshvc/nshvc.h"
#include "system/state.h"
#include "system/system.h"

/*
 * DevShot: ALADDIN_DEV_SHOTS=DIR and ALADDIN_DEV_SHOT_EVERY=N (default 60): a picture (DIR/uNNNNNN.ppm) every N
 * updates, from update ALADDIN_DEV_SHOT_FROM on (default 0); the engine's --shots writes every one.
 */
void DevShot(uint64_t u)
{
    static const char *dir;
    static unsigned every = 60, from;
    static int known;
    const uint32_t *px;
    char path[1024];
    FILE *f;
    int w, h, i;

    if (!known) {
        const char *v;

        known = 1;
        dir = getenv("ALADDIN_DEV_SHOTS");
        if ((v = getenv("ALADDIN_DEV_SHOT_EVERY")) && *v)
            every = (unsigned)strtoul(v, NULL, 0);
        if ((v = getenv("ALADDIN_DEV_SHOT_FROM")) && *v)
            from = (unsigned)strtoul(v, NULL, 0);
    }
    if (!dir || !every || u < from || u % every || !(px = nshvc_picture_read(&w, &h)))
        return;
    snprintf(path, sizeof path, "%s/u%06llu.ppm", dir, (unsigned long long)u);
    if (!(f = fopen(path, "wb")))
        return;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < w * h; i++) {
        putc(px[i] >> 16 & 255, f);
        putc(px[i] >> 8 & 255, f);
        putc(px[i] & 255, f);
    }
    fclose(f);
}

/*
 * DevMapDump: ALADDIN_DEV_MAP=FILE and ALADDIN_DEV_MAP_AT=U (default 4150): at update U, the level's map as text,
 * a character a 16-pixel tile (' ' empty, '#' solid, '/' '\\' a slope, '-' one he passes through, 'E' an exit spot,
 * 'S' where Aladdin is), the line starting with its y in pixels; for planning a play's waypoints. ALADDIN_DEV_PLACED=FILE:
 * at the same update, the level's actor placements (kind, byte, x, y).
 */
void DevMapDump(uint64_t u)
{
    static const char *path;
    static unsigned at = 4150;
    static int known, done;
    FILE *f;
    Actor dummy;
    unsigned x, y, i, width = 320, height = 96;

    if (!known) {
        const char *v;

        known = 1;
        path = getenv("ALADDIN_DEV_MAP");
        if ((v = getenv("ALADDIN_DEV_MAP_AT")) && *v)
            at = (unsigned)strtoul(v, NULL, 0);
    }
    if (done || u != at)
        return;
    if (getenv("ALADDIN_DEV_PLACED") && (f = fopen(getenv("ALADDIN_DEV_PLACED"), "w"))) {
        for (i = 0; i < placement_count(g.level); i++) {
            Placement pl = placement_at(g.level, i);

            fprintf(f, "%u kind=%02X byte=%02X x=%u y=%u\n", i, pl.kind, pl.byte, pl.x_word & 0x3FFF, pl.y_word & 0x3FFF);
        }
        fclose(f);
    }
    if (!path || !(f = fopen(path, "w")))
        return;
    done = 1;
    memset(&dummy, 0, sizeof dummy);
    dummy.kind = 1;
    for (y = 0; y < height; y++) {
        fprintf(f, "%4u ", y * 16);
        for (x = 0; x < width; x++) {
            MapPoint p;
            char c;

            memset(&p, 0, sizeof p);
            p.x = (uint16_t)(x * 16 + 8);
            p.y = (uint16_t)(y * 16 + 8);
            TileAt(&dummy, &p);
            c = p.kind == 0 ? ' ' : p.kind == 1 ? '#' : (p.behaviour & TILE_MIRRORED) ? '\\' : '/';
            if (p.kind && (p.behaviour & TILE_THROUGH))
                c = '-';
            for (i = 0; i < g.level_spot_count; i++)
                if (spot_x(g.level_spots, i) / 16 == x && spot_y(g.level_spots, 0) / 16 == y)
                    c = 'E';
            if ((g.actors[0].x >> 8) / 16 == x && (g.actors[0].y >> 8) / 16 == y)
                c = 'S';
            fputc(c, f);
        }
        fputc('\n', f);
    }
    fclose(f);
}

/*
 * DevDump: ALADDIN_DEV_DUMP=FILE and ALADDIN_DEV_DUMP_AT=U: the game's whole state (g) as bytes at update U, to find
 * what differs between two runs (with the type's offsets from a debug build's `ptype /o GameState`).
 */
void DevDump(uint64_t u)
{
    const char *patch = getenv("ALADDIN_DEV_PATCH");   /* U:OFFSET:SIZE:FILE: at update U that part of the state from a dump */
    unsigned pu, po, ps;
    char pf[512];

    if (patch && sscanf(patch, "%u:%u:%u:%511s", &pu, &po, &ps, pf) == 4 && u == pu && po + ps <= sizeof g) {
        FILE *q = fopen(pf, "rb");

        if (q && !fseek(q, (long)po, SEEK_SET) && fread((char *)&g + po, 1, ps, q) == ps)
            fprintf(stderr, "patched %u bytes at %u\n", ps, po);
        if (q)
            fclose(q);
    }
    static const char *path;
    static unsigned at;
    static int known;
    FILE *f;

    if (!known) {
        const char *v;

        known = 1;
        path = getenv("ALADDIN_DEV_DUMP");
        at = (v = getenv("ALADDIN_DEV_DUMP_AT")) && *v ? (unsigned)strtoul(v, NULL, 0) : 0;
    }
    if (!path || u != at || !(f = fopen(path, "wb")))
        return;
    fwrite(&g, sizeof g, 1, f);
    fclose(f);
}

#ifdef __unix__
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define UP 0x26
#define DOWN 0x28
#define LEFT 0x25
#define RIGHT 0x27
#define JUMP 'Z'
#define SWORD 'A'
#define APPLE 'X'

typedef struct { int keys[4]; } Combo;

static const Combo combos[] = {
    { { 0 } },
    { { RIGHT } }, { { LEFT } }, { { RIGHT, JUMP } }, { { LEFT, JUMP } }, { { JUMP } },
    { { RIGHT, SWORD } }, { { LEFT, SWORD } }, { { SWORD } }, { { UP } }, { { DOWN } },
    { { RIGHT, UP } }, { { LEFT, UP } }, { { RIGHT, DOWN } }, { { UP, JUMP } }, { { RIGHT, JUMP, SWORD } },
    { { APPLE } }, { { RIGHT, APPLE } },
    { { DOWN, JUMP } }, { { UP, RIGHT, JUMP } }, { { UP, LEFT, JUMP } }, { { DOWN, RIGHT, JUMP } }, { { DOWN, LEFT } },
};
#define COMBOS (sizeof combos / sizeof combos[0])

static void (*step)(const nshvc_input *);
static unsigned start, seg = 16, depth = 1;
static int goal_x = 1, goal_y = 0, goal_exit, goal_boss;
static unsigned switch_x = ~0u;      /* from this x on, the exit is the goal */
static FILE *out;
static int chosen = -1;
static unsigned held_left;
#define MAXQ 6000
static int queue[MAXQ];
static unsigned queued;
static unsigned stuck;
static int exploring;
static double stuck_ref = -1e18;
static unsigned rollouts = 200, rollout_len = 16;
static uint8_t level0;
static unsigned lives0, health0;
static int in_child;
static unsigned char visited[1024 * 512];     /* 16-pixel cells Aladdin has stood in this level */
static unsigned new_cells;

static void mark(void)
{
    unsigned cx = (g.actors[0].x >> 8) >> 4, cy = (g.actors[0].y >> 8) >> 4;
    unsigned char *c;

    if (cx >= 1024 || cy >= 512)
        return;
    c = &visited[cy * 1024 + cx];
    if (!*c) {
        *c = 1;
        new_cells++;
    }
}

static void keys_of(int c, nshvc_input *in)
{
    unsigned i;

    memset(in, 0, sizeof *in);
    for (i = 0; i < 4 && combos[c].keys[i]; i++)
        in->keys[combos[c].keys[i] >> 5] |= 1u << (combos[c].keys[i] & 31);
}

/* the distance to the level's exit spots (x: the nearest spot's, y: the first's) */
static double exit_distance(void)
{
    double px = (double)(g.actors[0].x >> 8), py = (double)(g.actors[0].y >> 8), best = 1e9, d;
    unsigned i;

    for (i = 0; i < g.level_spot_count; i++) {
        d = px - (double)spot_x(g.level_spots, i);
        if (d < 0)
            d = -d;
        if (d < best)
            best = d;
    }
    d = py - (double)spot_y(g.level_spots, 0);
    return best + (d < 0 ? -d : d);
}

/* a boss (kind $98, Jafar's first form): its hits left (+$11) */
static double boss_hits(void)
{
    unsigned i;

    for (i = 1; i < ACTOR_RECORDS; i++)
        if (g.actors[i].state && g.actors[i].kind == 0x98)
            return (double)g.actors[i].own.bytes[OWN_11];
    return 0;
}

/*
 * Waypoints (ALADDIN_DEV_BOT_WAY="x,y;x,y;..."): the player goes to each place in turn (near: within 20 pixels across,
 * 28 up and down); after the last, the level's exit spots are the goal.
 */
#define MAXWAY 64
static int way_x[MAXWAY], way_y[MAXWAY];
static unsigned way_n, way_at;

static int near_way(unsigned i)
{
    int dx = (int)(g.actors[0].x >> 8) - way_x[i], dy = (int)(g.actors[0].y >> 8) - way_y[i];

    return (way_x[i] < 0 || (dx > -20 && dx < 20)) && (way_y[i] < 0 || (dy > -28 && dy < 28));
}

static double way_score(void)
{
    unsigned i = way_at;
    double dx, dy;

    while (i < way_n && near_way(i))
        i++;
    if (i >= way_n)
        return 1000.0 * i - (g.level_spot_count ? exit_distance() : 0);
    dx = way_x[i] < 0 ? 0 : (double)((int)(g.actors[0].x >> 8) - way_x[i]);
    dy = way_y[i] < 0 ? 0 : (double)((int)(g.actors[0].y >> 8) - way_y[i]);
    return 1000.0 * i - (dx < 0 ? -dx : dx) - (dy < 0 ? -dy : dy);
}

static double score(void)
{
    double s = way_n ? way_score() : goal_boss ? -1000.0 * boss_hits() + 500.0 * g.scene_cue_first : (goal_exit || (g.actors[0].x >> 8) >= switch_x) && g.level_spot_count ? -exit_distance()
                                               : goal_x * (double)(g.actors[0].x >> 8) + goal_y * (double)(g.actors[0].y >> 8);

    s -= 40.0 * (health0 - g.health) * (g.health < health0);
    if (g.lives < lives0 || g.level != level0 || g.level_exiting)
        s += g.level_exiting || g.level != level0 ? 1e6 : -1e6;
    return s;
}

/* the best score over the choices for `d` more segments, from this state */
static double look(unsigned d, int *best)
{
    double top = -1e18;
    unsigned c;

    for (c = 0; c < COMBOS; c++) {
        int fd[2];
        pid_t pid;
        double r = -1e18;

        if (pipe(fd))
            return top;
        pid = fork();
        if (pid == 0) {
            nshvc_input in;
            unsigned i;

            in_child = 1;
            close(fd[0]);
            keys_of((int)c, &in);
            for (i = 0; i < seg; i++)
                step(&in);
            r = d > 1 && g.level == level0 && !g.level_exiting && g.lives >= lives0 ? look(d - 1, NULL) : score();
            if (write(fd[1], &r, sizeof r) != sizeof r)
                _exit(1);
            _exit(0);
        }
        close(fd[1]);
        if (read(fd[0], &r, sizeof r) != sizeof r)
            r = -1e18;
        close(fd[0]);
        waitpid(pid, NULL, 0);
        if (r > top) {
            top = r;
            if (best)
                *best = (int)c;
        }
    }
    return top;
}

/* weighted choices for a random rollout */
static int random_combo(unsigned *seed)
{
    static const struct { int combo; unsigned weight; } w[] = {
        { 1, 28 }, { 3, 22 }, { 5, 5 }, { 2, 8 }, { 4, 5 }, { 9, 4 }, { 10, 3 }, { 6, 8 }, { 8, 5 }, { 0, 5 },
        { 14, 3 }, { 11, 3 }, { 12, 1 },
    };
    unsigned r = (unsigned)rand_r(seed) % 100, i;
    int flip = 0;

    if (way_at < way_n)
        flip = way_x[way_at] < (int)(g.actors[0].x >> 8);      /* the next waypoint is to the left: mirror the keys */
    for (i = 0; i < sizeof w / sizeof w[0]; i++) {
        if (r < w[i].weight) {
            int c = w[i].combo;

            if (flip)
                c = c == 1 ? 2 : c == 2 ? 1 : c == 3 ? 4 : c == 4 ? 3 : c == 6 ? 7 : c == 11 ? 12 : c == 12 ? 11 : c;
            return c;
        }
        r -= w[i].weight;
    }
    return 1;
}

/* random rollouts, each its own forked copy: the best one's choices go in queue (queued of them); returns its score */
static double explore(void)
{
    double top = -1e18;
    unsigned n, i;

    for (n = 0; n < rollouts; n++) {
        int fd[2];
        pid_t pid;
        struct { double score; int seq[MAXQ]; } r;

        if (pipe(fd))
            return top;
        pid = fork();
        if (pid == 0) {
            unsigned seed = n * 2654435761u + 12345u;
            double best = -1e18;
            nshvc_input in;

            in_child = 1;
            new_cells = 0;
            close(fd[0]);
            for (i = 0; i < rollout_len; i++) {
                unsigned k;

                r.seq[i] = random_combo(&seed);
                keys_of(r.seq[i], &in);
                for (k = 0; k < seg; k++) {
                    step(&in);
                    mark();
                }
                if (16.0 * new_cells + 0.25 * score() > best)
                    best = 16.0 * new_cells + 0.25 * score();
                if (g.lives < lives0 || g.level != level0 || g.level_exiting) {
                    if (g.level != level0 || g.level_exiting)
                        best = 1e6;
                    else
                        best = -1e6;
                    i++;
                    break;
                }
            }
            r.score = best;
            for (; i < MAXQ; i++)
                r.seq[i] = -1;
            if (write(fd[1], &r, sizeof r) != sizeof r)
                _exit(1);
            _exit(0);
        }
        close(fd[1]);
        if (read(fd[0], &r, sizeof r) != sizeof r)
            r.score = -1e18;
        close(fd[0]);
        waitpid(pid, NULL, 0);
        if (r.score > top) {
            top = r.score;
            queued = 0;
            for (i = 0; i < MAXQ && r.seq[i] >= 0; i++)
                queue[queued++] = r.seq[i];
        }
    }
    return top;
}

/*
 * The search by an archive of places (ALADDIN_DEV_BOT_GE=N, N copies at a time; _GE_ROUNDS, _GE_LEN): keeps, for each 32-pixel
 * cell Aladdin has stood in, the shortest choices that got there from the start; each round N copies take an entry (the less
 * tried the likelier, the one nearest the goal of the few drawn), play its choices again, then try random choices for _GE_LEN
 * segments and report every cell they stood in. The first choices to a level change (the exit) are played for real. The goal is
 * the waypoints' (ALADDIN_DEV_BOT_WAY) or the level's exit spots. ALADDIN_DEV_BOT_GE_DUMP=FILE: the cells (x y choices tries) at the end.
 */
typedef struct { unsigned len, visits; unsigned char *p; double prog; } GeEntry;
typedef struct { unsigned cell, nseg, exit; double prog; } GeRecord;
#define GE_CELLS (1u << 20)
#define GE_MAXREC 400
static GeEntry **ge_cell, **ge_list;
static unsigned ge_n, ge_cap, ge_par, ge_rounds = 400, ge_len = 24, ge_hurt = 3, ge_shift = 5, ge_short, ge_stall, ge_dfs_ok = 1;

static unsigned ge_cell_key(void)
{
    unsigned cx = (g.actors[0].x >> 8) >> ge_shift, cy = (g.actors[0].y >> 8) >> ge_shift;

    return (cx > 1023 ? 1023 : cx) * 1024 + (cy > 1023 ? 1023 : cy);
}

static double ge_health_weight;

static double ge_prog(void)
{
    if (goal_boss) {
        unsigned i;

        for (i = 1; i < ACTOR_RECORDS; i++)
            if (g.actors[i].state && g.actors[i].kind == 0x98)
                return 10000.0 - 1000.0 * g.actors[i].own.bytes[OWN_11];     /* the boss is there: its hits left */
    }
    return (way_n ? way_score() : g.level_spot_count ? -exit_distance() : (double)(g.actors[0].x >> 8)) + ge_health_weight * g.health;
}

/*
 * The depth-first helper of the archive search: from a place, tries key choices depth first (each node a copy of the game, the
 * choices that favour the goal's side first) for a way that gets closer to the goal by dfs_delta (or leaves the level); a state
 * seen before (a table the copies share) is not tried again; a branch stops when Aladdin loses a life or health, when it
 * makes no progress for dfs_stale segments, or at dfs_depth; the search stops after dfs_budget nodes. It finds the exact
 * chains of jumps (platforms that move, ropes, pegs) the random tries almost never hit.
 */
typedef struct {
    volatile unsigned found, n, nodes, exit, cell;
    volatile double prog;
    int path[160];
    unsigned char seen[1u << 22];
} DfsShared;
static DfsShared *dfs_sh;
static double dfs_base, dfs_delta = 24.0;
static unsigned dfs_budget = 8000, dfs_stale = 14, dfs_depth = 90;

static unsigned dfs_key(void)
{
    const Actor *a = &g.actors[0];
    unsigned h = (unsigned)(a->x >> 10) * 2654435761u;

    h ^= ((unsigned)(a->y >> 10) + 0x9E3779B9u) * 2246822519u;
    h ^= ((unsigned)(a->speed_x >> 7) & 0xFF) * 3266489917u;
    h ^= (((unsigned)(a->speed_y >> 7) & 0xFF) + 77u) * 668265263u;
    h ^= ((unsigned)a->resume * 374761393u) ^ ((unsigned)a->anim << 9);
    h ^= h >> 15;
    return h & ((1u << 22) - 1);
}

static void dfs(unsigned depth, int *path, double best, unsigned best_depth)
{
    static const int right[] = { 3, 1, 5, 11, 14, 9, 6, 4, 2, 0, 10, 12, 7, 8, 13 };
    unsigned c, key;
    double here;

    if (dfs_sh->found)
        return;
    here = ge_prog();
    if (g.level != level0 || g.level_exiting || here > dfs_base + dfs_delta) {
        if (__sync_bool_compare_and_swap(&dfs_sh->found, 0, 1)) {
            memcpy(dfs_sh->path, path, depth * sizeof path[0]);
            dfs_sh->n = depth;
            dfs_sh->exit = g.level != level0 || g.level_exiting;
            dfs_sh->cell = ge_cell_key();
            dfs_sh->prog = here;
        }
        return;
    }
    if (depth >= dfs_depth || dfs_sh->nodes >= dfs_budget)
        return;
    if (here > best) {
        best = here;
        best_depth = depth;
    }
    if (depth - best_depth > dfs_stale)
        return;
    key = dfs_key();
    if (dfs_sh->seen[key])
        return;
    dfs_sh->seen[key] = 1;
    dfs_sh->nodes++;
    for (c = 0; c < 15 && !dfs_sh->found; c++) {
        pid_t pid = fork();

        if (pid == 0) {
            nshvc_input in;
            unsigned i;

            path[depth] = right[c];
            keys_of(path[depth], &in);
            for (i = 0; i < seg; i++)
                step(&in);
            if (g.level != level0 || g.level_exiting || (g.lives >= lives0 && (unsigned)g.health + ge_hurt >= health0))
                dfs(depth + 1, path, best, best_depth);
            _exit(0);
        }
        waitpid(pid, NULL, 0);
    }
}

static GeEntry *ge_add(const unsigned char *base, unsigned blen, const unsigned char *suffix, unsigned slen, double prog)
{
    GeEntry *e = malloc(sizeof *e);

    if (ge_n == ge_cap) {
        ge_cap = ge_cap ? ge_cap * 2 : 1024;
        ge_list = realloc(ge_list, ge_cap * sizeof *ge_list);
    }
    e->len = blen + slen;
    e->visits = 0;
    e->prog = prog;
    e->p = malloc(e->len ? e->len : 1);
    memcpy(e->p, base, blen);
    memcpy(e->p + blen, suffix, slen);
    ge_list[ge_n++] = e;
    return e;
}

static void ge_replay(const GeEntry *e)
{
    nshvc_input in;
    unsigned i, k;

    for (i = 0; i < e->len; i++) {
        keys_of(e->p[i], &in);
        for (k = 0; k < seg; k++)
            step(&in);
    }
}

/* in a copy: play entry e again, then random choices; writes the report to fd */
static void ge_child(const GeEntry *e, unsigned seed, int fd)
{
    GeRecord rec[GE_MAXREC];
    unsigned char suffix[GE_MAXREC];
    unsigned n = 0, i, k;
    int c = 1;
    nshvc_input in;

    unsigned len = ge_len;
    int persist = (unsigned)rand_r(&seed) % 100 < 12;     /* one choice held all the way: for a wait, such as a scene that starts after a pause */

    in_child = 1;
    ge_replay(e);
    if (persist)
        len = ge_len * 6;
    else if (ge_short)
        len = 4 + (unsigned)rand_r(&seed) % ge_len;      /* mostly short tries: more of them from the places at the edge */
    for (i = 0; i < len && i < GE_MAXREC; i++) {
        if (i == 0 || (!persist && (unsigned)rand_r(&seed) % 100 < 45)) {
            c = (unsigned)rand_r(&seed) % 100 < 35 ? (int)((unsigned)rand_r(&seed) % COMBOS) : random_combo(&seed);
            if ((unsigned)rand_r(&seed) % 100 < 22)
                c = c == 1 ? 2 : c == 2 ? 1 : c == 3 ? 4 : c == 4 ? 3 : c == 6 ? 7 : c == 7 ? 6 : c == 11 ? 12 : c == 12 ? 11 : c;
        }
        suffix[i] = (unsigned char)c;
        keys_of(c, &in);
        for (k = 0; k < seg; k++)
            step(&in);
        if (g.level != level0 || g.level_exiting) {
            rec[n++] = (GeRecord){ 0, i + 1, 1, 0 };
            break;
        }
        if (g.lives < lives0 || (unsigned)g.health + ge_hurt < health0)
            break;
        rec[n++] = (GeRecord){ ge_cell_key(), i + 1, 0, ge_prog() };
    }
    if (write(fd, &n, sizeof n) == sizeof n && write(fd, rec, n * sizeof rec[0]) == (ssize_t)(n * sizeof rec[0]))
        if (write(fd, suffix, n) != (ssize_t)n)
            _exit(1);
    _exit(0);
}

static GeEntry *ge_pick(unsigned *seed)
{
    GeEntry *best = NULL;
    unsigned t, tries = 0;

    for (t = 0; t < 4 && tries < 400; tries++) {
        GeEntry *e = ge_list[(unsigned)rand_r(seed) % ge_n];

        if ((double)((unsigned)rand_r(seed) % 1000) / 1000.0 > 1.0 / (1.0 + 0.7 * e->visits))
            continue;
        t++;
        if (!best || e->prog > best->prog)
            best = e;
    }
    return best ? best : ge_list[0];
}

/* the search: returns whether it found the exit; the choices go in queue */
static int ge_search(void)
{
    unsigned seed = 1234567u, round, i, found_len = 0, exit_len = ~0u;
    unsigned char *found = NULL;
    double top = -1e18;
    unsigned top_cell = 0, stall = 0;
    GeEntry *top_entry = NULL;
    double top_seen = -1e18;

    ge_cell = calloc(GE_CELLS, sizeof *ge_cell);
    ge_cell[ge_cell_key()] = ge_add(NULL, 0, NULL, 0, ge_prog());
    for (round = 0; round < ge_rounds; round++) {
        struct { pid_t pid; int fd; GeEntry *e; } w[16];
        unsigned j, par = ge_par > 16 ? 16 : ge_par;

        for (j = 0; j < par; j++) {
            int fd[2];

            w[j].e = ge_pick(&seed);
            w[j].e->visits++;
            if (pipe(fd))
                return 0;
            w[j].pid = fork();
            if (w[j].pid == 0) {
                close(fd[0]);
                ge_child(w[j].e, (unsigned)rand_r(&seed) + j, fd[1]);
            }
            close(fd[1]);
            w[j].fd = fd[0];
        }
        for (j = 0; j < par; j++) {
            GeRecord rec[GE_MAXREC];
            unsigned char suffix[GE_MAXREC];
            unsigned n = 0;
            FILE *f = fdopen(w[j].fd, "r");

            if (f && fread(&n, sizeof n, 1, f) == 1 && n <= GE_MAXREC && fread(rec, sizeof rec[0], n, f) == n && fread(suffix, 1, n, f) == n) {
                for (i = 0; i < n; i++) {
                    unsigned len = w[j].e->len + rec[i].nseg;

                    if (rec[i].exit) {
                        if (len < exit_len) {
                            exit_len = len;
                            free(found);
                            found = malloc(len);
                            memcpy(found, w[j].e->p, w[j].e->len);
                            memcpy(found + w[j].e->len, suffix, rec[i].nseg);
                            found_len = len;
                        }
                        continue;
                    }
                    if (!ge_cell[rec[i].cell])
                        ge_cell[rec[i].cell] = ge_add(w[j].e->p, w[j].e->len, suffix, rec[i].nseg, rec[i].prog);
                    else if (len + 2 < ge_cell[rec[i].cell]->len) {
                        GeEntry *old = ge_cell[rec[i].cell];

                        old->len = len;
                        old->p = realloc(old->p, len);
                        memcpy(old->p, w[j].e->p, w[j].e->len);
                        memcpy(old->p + w[j].e->len, suffix, rec[i].nseg);
                    }
                    if (rec[i].prog > top) {
                        top = rec[i].prog;
                        top_cell = rec[i].cell;
                        top_entry = ge_cell[rec[i].cell];
                    }
                }
            }
            if (f)
                fclose(f);
            waitpid(w[j].pid, NULL, 0);
        }
        if (top > top_seen + 8.0) {
            top_seen = top;
            stall = 0;
        } else
            stall++;
        if (!found && ge_stall && stall >= ge_stall && top_entry && ge_dfs_ok) {
            /* stuck: the depth-first helper from the place that got furthest */
            int path[160];
            pid_t pid;

            stall = 0;
            if (!dfs_sh) {
                dfs_sh = mmap(NULL, sizeof *dfs_sh, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
                if (dfs_sh == MAP_FAILED)
                    dfs_sh = NULL;
            }
            if (dfs_sh) {
                memset((void *)dfs_sh, 0, sizeof *dfs_sh);
                pid = fork();
                if (pid == 0) {
                    in_child = 1;
                    ge_replay(top_entry);
                    dfs_base = ge_prog();
                    dfs(0, path, dfs_base, 0);
                    _exit(0);
                }
                waitpid(pid, NULL, 0);
                if (dfs_sh->found) {
                    GeEntry *e = ge_add(top_entry->p, top_entry->len, (const unsigned char *)"", 0, dfs_sh->prog);
                    unsigned k;

                    e->p = realloc(e->p, top_entry->len + dfs_sh->n + 1);
                    for (k = 0; k < dfs_sh->n; k++)
                        e->p[top_entry->len + k] = (unsigned char)dfs_sh->path[k];
                    e->len = top_entry->len + dfs_sh->n;
                    if (dfs_sh->exit) {
                        free(found);
                        found = malloc(e->len);
                        memcpy(found, e->p, e->len);
                        found_len = e->len;
                    } else {
                        if (!ge_cell[dfs_sh->cell])
                            ge_cell[dfs_sh->cell] = e;
                        top = dfs_sh->prog;
                        top_cell = dfs_sh->cell;
                        top_entry = e;
                        top_seen = top;
                    }
                    fprintf(stderr, "bot: the depth-first helper moved on %u segments (%u nodes)%s\n", dfs_sh->n, dfs_sh->nodes, dfs_sh->exit ? ": EXIT" : "");
                } else
                    fprintf(stderr, "bot: the depth-first helper found nothing in %u nodes\n", dfs_sh->nodes);
            }
        }
        if (round % 10 == 0 || found)
            fprintf(stderr, "bot: round %u: %u cells, best progress %.0f at (%u,%u)%s\n", round, ge_n, top, top_cell / 1024 << ge_shift, top_cell % 1024 << ge_shift, found ? ", EXIT found" : "");
        if (found && round >= 1)
            break;
    }
    if (getenv("ALADDIN_DEV_BOT_GE_DUMP")) {
        FILE *f = fopen(getenv("ALADDIN_DEV_BOT_GE_DUMP"), "w");
        unsigned k;

        for (k = 0; f && k < GE_CELLS; k++)
            if (ge_cell[k])
                fprintf(f, "%u %u %u %u\n", k / 1024 << ge_shift, k % 1024 << ge_shift, ge_cell[k]->len, ge_cell[k]->visits);
        if (f)
            fclose(f);
    }
    if (!found) {
        if (top_entry)
            for (queued = 0; queued < top_entry->len && queued < MAXQ; queued++)
                queue[queued] = top_entry->p[queued];     /* played for real: where it got furthest */
        return 0;
    }
    queued = 0;
    for (i = 0; i < found_len && i < MAXQ; i++)
        queue[queued++] = found[i];
    return 1;
}

/*
 * DevBotInput: the input to use for this update: false, the engine's own. u is the engine's update count.
 */
bool DevBotInput(uint64_t u, nshvc_input *in)
{
    static int on = -1;
    const char *v;

    if (on < 0) {
        on = 0;
        if ((v = getenv("ALADDIN_DEV_BOT")) && *v) {
            on = 1;
            start = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_SEG")) && *v)
                seg = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_DEPTH")) && *v)
                depth = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GOAL")) && *v) {
                goal_exit = !strcmp(v, "exit");
                goal_boss = !strcmp(v, "boss");
                goal_x = !strcmp(v, "x") - !strcmp(v, "-x");
                goal_y = !strcmp(v, "y") - !strcmp(v, "-y");
            }
            if ((v = getenv("ALADDIN_DEV_BOT_WAY")) && *v) {
                const char *q = v;
                int x, y, n;

                while (way_n < MAXWAY && sscanf(q, "%d,%d%n", &x, &y, &n) == 2) {
                    way_x[way_n] = x;
                    way_y[way_n++] = y;
                    q += n;
                    if (*q == ';')
                        q++;
                }
            }
            if ((v = getenv("ALADDIN_DEV_BOT_SWITCH")) && *v)
                switch_x = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GE")) && *v)
                ge_par = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GE_ROUNDS")) && *v)
                ge_rounds = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GE_HEALTH")) && *v)
                ge_health_weight = strtod(v, NULL);
            if ((v = getenv("ALADDIN_DEV_BOT_GE_STALL")) && *v)
                ge_stall = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GE_CELL")) && *v)
                ge_shift = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GE_SHORT")) && *v)
                ge_short = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GE_HURT")) && *v)
                ge_hurt = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_GE_LEN")) && *v)
                ge_len = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_ROLLOUTS")) && *v)
                rollouts = (unsigned)strtoul(v, NULL, 0);
            if ((v = getenv("ALADDIN_DEV_BOT_LEN")) && *v)
                rollout_len = (unsigned)strtoul(v, NULL, 0);
            if (rollout_len > MAXQ)
                rollout_len = MAXQ;
            if ((v = getenv("ALADDIN_DEV_BOT_OUT")) && *v)
                out = fopen(v, "w");
        }
    }
    if (!on || in_child || u < start)
        return false;
    if (!step)
        return false;
    mark();
    {
        static int vl = -1;

        if (g.level != vl) {
            vl = g.level;
            memset(visited, 0, sizeof visited);
        }
    }
    if (!held_left) {
        if (!step)
            return false;
        while (way_at < way_n && near_way(way_at)) {
            fprintf(stderr, "bot: waypoint %u (%d,%d) reached at update %llu\n", way_at, way_x[way_at], way_y[way_at], (unsigned long long)u);
            way_at++;
        }
        if (way_at > 0 && way_at < way_n && way_y[way_at - 1] >= 0 && way_y[way_at] >= 0 && way_y[way_at] < way_y[way_at - 1] && (int)(g.actors[0].y >> 8) > way_y[way_at - 1] + 90) {
            /* fell well below the last waypoint reached: start again from the nearest one before it */
            unsigned j, best = 0;
            long bd = 1L << 60;

            for (j = 0; j < way_at; j++) {
                long dx = (int)(g.actors[0].x >> 8) - way_x[j], dy = (int)(g.actors[0].y >> 8) - way_y[j], d = dx * dx + dy * dy;

                if (d < bd) {
                    bd = d;
                    best = j;
                }
            }
            fprintf(stderr, "bot: fell, back to waypoint %u at update %llu\n", best, (unsigned long long)u);
            way_at = best;
        }
        level0 = (uint8_t)g.level;
        lives0 = g.lives;
        health0 = g.health;
        if (g.level_exiting || g.level != level0)
            return false;
        if (ge_par) {
            static int ge_done;

            if (!ge_done) {
                ge_done = 1;
                if (ge_search())
                    fprintf(stderr, "bot: the exit's choices found at update %llu: %u segments\n", (unsigned long long)u, queued);
                else
                    fprintf(stderr, "bot: no exit found by the archive search; the furthest place is played: %u segments\n", queued);
            }
        }
        if (queued) {
            chosen = queue[0];
            memmove(queue, queue + 1, (--queued) * sizeof queue[0]);
        } else {
            static int last_level = -1;
            double here;

            if (g.level != last_level) {
                last_level = g.level;
                stuck_ref = -1e18;
            }
            here = score();
            if (here > stuck_ref + 3.0) {
                stuck_ref = here;
                stuck = 0;
                exploring = 0;
            } else
                stuck++;
            if (stuck >= 4 || exploring) {
                exploring = 1;
                explore();
                stuck = 0;
                chosen = queued ? queue[0] : 0;
                if (queued)
                    memmove(queue, queue + 1, (--queued) * sizeof queue[0]);
            } else
                look(depth, &chosen);
        }
        if (out) {
            unsigned i;

            fprintf(out, "%llu", (unsigned long long)u);
            for (i = 0; i < 4 && combos[chosen].keys[i]; i++)
                fprintf(out, " key=%02X", combos[chosen].keys[i]);
            fprintf(out, "\n");
            fflush(out);
        }
        held_left = seg;
    }
    keys_of(chosen, in);
    held_left--;
    return true;
}

void DevBotStep(void (*f)(const nshvc_input *))
{
    step = f;
}
#else
bool DevBotInput(uint64_t u, nshvc_input *in)
{
    (void)u;
    (void)in;
    return false;
}

void DevBotStep(void (*f)(const nshvc_input *))
{
    (void)f;
}
#endif
