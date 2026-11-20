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

#include "data/data.h"
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

static void update(const nshvc_input *input)
{
    NmiHandler();
    TaskScheduler(input);
    NmiPictureWork();
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
    return nshvc_run(&game, argc, argv);
}
