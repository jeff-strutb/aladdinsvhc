/*
 * main.c: the game's entry on the engine. The engine owns the loop and calls
 * update 60 times a second, each one frame of the original (plan, "The
 * loop"): the NMI's game work (the frame count, the tasks' countdown, the
 * sound command), every ready task's turn, then the NMI's picture work
 * (video_vblank: what the tasks queued, written to the engine's objects).
 * Over time that is the original's order, tasks, uploads, countdown, sound,
 * tasks; the picture draw sets between them is what the original showed
 * from that NMI on.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "audio/music.h"
#include "data/data.h"
#include "data/level_tables.h"
#include "nshvc/nshvc.h"
#include "system/system.h"
#include "system/task.h"
#include "system/unconverted.h"

static uint64_t pictures;

static void init(void)
{
    data_load();
    PowerOn();
    video_init();
}

/* ALADDIN_DEV_TRACE=N: every N updates, a line on stderr with where the play is (for planning a play's keys) */
static void trace_line(void)
{
    static unsigned every = ~0u;
    const char *v;

    if (every == ~0u) {
        v = getenv("ALADDIN_DEV_TRACE");
        every = v && *v ? (unsigned)strtoul(v, NULL, 0) : 0;
    }
    if (every && nshvc_updates() % every == 0)
        fprintf(stderr, "u=%" PRIu64 " level=%u x=%u y=%u health=%u lives=%u scroll=%u scrolly=%u exiting=%u\n",
                nshvc_updates(), (unsigned)g.level, (unsigned)(g.actors[0].x >> 8), (unsigned)(g.actors[0].y >> 8),
                (unsigned)g.health, (unsigned)g.lives, (unsigned)(g.layers[0].scroll_x >> 8), (unsigned)(g.layers[0].scroll_y >> 8),
                (unsigned)g.level_exiting);
    if (every) {
        static int spots_of = -1;

        if ((int)g.level != spots_of && g.level_spot_count) {
            unsigned i;

            spots_of = (int)g.level;
            fprintf(stderr, "level %u exit spots:", (unsigned)g.level);
            for (i = 0; i < g.level_spot_count; i++)
                fprintf(stderr, " (%u,%u)->%02X", (unsigned)spot_x(g.level_spots, i), (unsigned)spot_y(g.level_spots, i),
                        (unsigned)spot_leads_to(g.level_spots, i));
            fprintf(stderr, "\n");
        }
    }
    if (every && getenv("ALADDIN_DEV_ACTORS") && nshvc_updates() % every == 0) {
        unsigned i;

        for (i = 0; i < ACTOR_RECORDS; i++)
            if (g.actors[i].state)
                fprintf(stderr, "  actor %u kind=%02X state=%02X x=%d y=%d picture=%02X anim=%02X\n", i, g.actors[i].kind,
                        g.actors[i].state, (int)(g.actors[i].x >> 8), (int)(g.actors[i].y >> 8), g.actors[i].picture,
                        g.actors[i].anim);
    }
}

static void step(const nshvc_input *input)
{
    NmiHandler();
    TaskScheduler(input);
    NmiPictureWork();
}

static void update(const nshvc_input *input)
{
    nshvc_input bot;

    DevBotStep(step);
    step(DevBotInput(nshvc_updates(), &bot) ? &bot : input);
    trace_line();
    DevShot(nshvc_updates());
}

static void draw(float between)
{
    (void)between;
    pictures++;
    video_draw();
}

static void shutdown(void)
{
    printf("updates %" PRIu64 " pictures %" PRIu64 "\n", nshvc_updates(), pictures);
}

int main(int argc, char **argv)
{
    static nshvc_game game;

    game.title = "Aladdin";
    game.id = "aladdin";
    game.update_hz = 60;
    game.init = init;
    game.update = update;
    game.draw = draw;
    game.shutdown = shutdown;
    game.music.hz = MUSIC_HZ;
    game.music.tick = music_tick;
    game.music.queue_bytes = MUSIC_QUEUE_BYTES;
    return nshvc_run(&game, argc, argv);
}
