/* main.c: the game's entry on the engine. The engine owns the loop and calls
   update 60 times a second and draw once a picture. The screen is the
   original's 256 x 224 and shows nothing yet. */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "nshvc/nshvc.h"

static uint64_t pictures;

static void update(const nshvc_input *input)
{
    (void)input;
}

static void draw(float between)
{
    nshvc_screen scr;

    (void)between;
    pictures++;
    memset(&scr, 0, sizeof scr);
    scr.blank = 1;
    nshvc_screen_set(&scr);
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
    game.update = update;
    game.draw = draw;
    game.shutdown = shutdown;
    return nshvc_run(&game, argc, argv);
}
