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
#include "video/view.h"

static uint64_t pictures;

static void init(void)
{
    data_load();
    view_init();
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
        fprintf(stderr, "u=%" PRIu64 " level=%u x=%u y=%u health=%u lives=%u scroll=%u scrolly=%u exiting=%u xmin=%u xmax=%u kind=%u setup=%u figures=%u\n",
                nshvc_updates(), (unsigned)g.level, (unsigned)(g.actors[0].x >> 8), (unsigned)(g.actors[0].y >> 8),
                (unsigned)g.health, (unsigned)g.lives, (unsigned)(g.layers[0].scroll_x >> 8), (unsigned)(g.layers[0].scroll_y >> 8),
                (unsigned)g.level_exiting, (unsigned)g.scroll_x_min, (unsigned)g.scroll_x_max, (unsigned)g.scroll_kind, (unsigned)g.layer_setup_mode, (unsigned)g.figures_touched);
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
    if (every && getenv("ALADDIN_DEV_VIDEO") && nshvc_updates() % every == 0)
        fprintf(stderr, "  video mode=%u bg3p=%u t16=%u map=%u,%u,%u tiles=%u,%u,%u,%u lines=%02X mode7=%u split=%u layers=%02X/%02X "
                "blend=%u/%u/%u/%u/%02X/%u/%u win=%02X/%02X mosaic=%u/%u fixed=%u,%u,%u\n",
                (unsigned)g.mode, (unsigned)g.bg3_priority, (unsigned)g.tile_size_16, (unsigned)g.bg_map[0], (unsigned)g.bg_map[1],
                (unsigned)g.bg_map[2], (unsigned)g.bg_tiles[0], (unsigned)g.bg_tiles[1], (unsigned)g.bg_tiles[2],
                (unsigned)g.bg_tiles[3], (unsigned)g.line_effects_on, (unsigned)g.mode7_how, (unsigned)g.split_on,
                (unsigned)g.main_layers, (unsigned)g.sub_layers, (unsigned)g.blend_prevent, (unsigned)g.blend_clip,
                (unsigned)g.blend_use_sub, (unsigned)g.blend_direct_colour, (unsigned)g.blend_layers, (unsigned)g.blend_half,
                (unsigned)g.blend_subtract, (unsigned)g.window_main, (unsigned)g.window_sub, (unsigned)g.mosaic_size,
                (unsigned)g.mosaic_layers, (unsigned)g.fixed_red, (unsigned)g.fixed_green, (unsigned)g.fixed_blue);
    if (every && getenv("ALADDIN_DEV_ACTORS") && nshvc_updates() % every == 0) {
        unsigned i;

        for (i = 0; i < ACTOR_RECORDS; i++)
            if (g.actors[i].state)
            {
                unsigned b;

                fprintf(stderr, "  actor %u kind=%02X state=%02X x=%d y=%d picture=%02X anim=%02X", i, g.actors[i].kind,
                        g.actors[i].state, (int)(g.actors[i].x >> 8), (int)(g.actors[i].y >> 8), g.actors[i].picture,
                        g.actors[i].anim);
                if (i == 0)                                           /* his test box: centre, half-sizes, box number */
                    fprintf(stderr, " tb=%d,%d,%u,%u,%02X", (int)(int16_t)g.test_box_x, (int)(int16_t)g.test_box_y,
                            (unsigned)g.test_box_half_w, (unsigned)g.test_box_half_h, (unsigned)g.player_box);
                if (getenv("ALADDIN_DEV_ACTORS")[0] == '2') {         /* =2: the kind's own bytes too */
                    fprintf(stderr, " own=");
                    for (b = 0; b < 32; b++)
                        fprintf(stderr, "%02X", g.actors[i].own.bytes[b]);
                }
                fprintf(stderr, " resume=%X\n", (unsigned)g.actors[i].resume);
            }
    }
}

static void step(const nshvc_input *input)
{
    NmiHandler();
    TaskScheduler(input);
    NmiPictureWork();
}

/*
 * ALADDIN_DEV_RECORD=FILE: the keys and controller buttons a person plays, as an --input file (a line "UPDATE key=XX ...
 * controller=N,BITS" at each update where they change), so a play by hand can be kept and replayed (native/checks/plays/run.sh).
 */
static void record_input(const nshvc_input *input)
{
    static FILE *f;
    static int known;
    static nshvc_input last;
    static uint16_t last_buttons[8];
    unsigned i, k, n = 0;
    int changed = 0;

    if (!known) {
        const char *v = getenv("ALADDIN_DEV_RECORD");

        known = 1;
        if (v && *v)
            f = fopen(v, "w");
    }
    if (!f)
        return;
    for (i = 0; i < 8; i++) {
        changed |= input->keys[i] != last.keys[i];
        changed |= input->controller[i].on && input->controller[i].buttons != last_buttons[i];
    }
    if (!changed)
        return;
    fprintf(f, "%" PRIu64, nshvc_updates());
    for (i = 0; i < 8; i++)
        for (k = 0; k < 32; k++)
            if (input->keys[i] >> k & 1u)
                fprintf(f, " key=%02X", i * 32 + k), n++;
    for (i = 0; i < 8; i++)
        if (input->controller[i].on && input->controller[i].buttons)
            fprintf(f, " controller=%u,%X", i, (unsigned)input->controller[i].buttons);
    fprintf(f, "\n");
    fflush(f);
    last = *input;
    for (i = 0; i < 8; i++)
        last_buttons[i] = input->controller[i].buttons;
}

static void update(const nshvc_input *input)
{
    nshvc_input bot;

    if (DevMapDumpHeld()) {
        NmiPictureWork();
        return;
    }
    record_input(input);

    DevBotStep(step);
    step(DevBotInput(nshvc_updates(), &bot) ? &bot : input);
    trace_line();
    DevShot(nshvc_updates());
    DevMapDump(nshvc_updates());
    DevMapDumpLevel(nshvc_updates());
    DevDump(nshvc_updates());
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
