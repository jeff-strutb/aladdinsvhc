/*
 * dev_bot.c: a developer's player (not the game's): it plays a level by the pad's keys so a play can be made and
 * kept as an input file (native/checks/plays/). From update ALADDIN_DEV_BOT on, each SEG updates it tries every key
 * combination for those updates in a forked copy of the whole process, scores where Aladdin ends (further right,
 * alive, no health lost, or the level changed), applies the best for real and appends the update's keys to
 * ALADDIN_DEV_BOT_OUT in the input file's form. ALADDIN_DEV_BOT_SEG (updates a choice holds, default 16),
 * ALADDIN_DEV_BOT_DEPTH (choices looked ahead, default 1), ALADDIN_DEV_BOT_GOAL ("x" right, "-x" left, "y" down,
 * "-y" up: what the score follows). Linux only (fork).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifdef __unix__
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
};
#define COMBOS (sizeof combos / sizeof combos[0])

static void (*step)(const nshvc_input *);
static unsigned start, seg = 16, depth = 1;
static int goal_x = 1, goal_y = 0;
static FILE *out;
static int chosen = -1;
static unsigned held_left;
#define MAXQ 32
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

static double score(void)
{
    double s = goal_x * (double)(g.actors[0].x >> 8) + goal_y * (double)(g.actors[0].y >> 8);

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
        { 1, 30 }, { 3, 25 }, { 5, 5 }, { 2, 8 }, { 4, 5 }, { 9, 4 }, { 10, 3 }, { 6, 10 }, { 8, 5 }, { 0, 5 },
    };
    unsigned r = (unsigned)rand_r(seed) % 100, i;

    for (i = 0; i < sizeof w / sizeof w[0]; i++) {
        if (r < w[i].weight)
            return w[i].combo;
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
                goal_x = !strcmp(v, "x") - !strcmp(v, "-x");
                goal_y = !strcmp(v, "y") - !strcmp(v, "-y");
            }
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
        level0 = (uint8_t)g.level;
        lives0 = g.lives;
        health0 = g.health;
        if (g.level_exiting || g.level != level0)
            return false;
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
