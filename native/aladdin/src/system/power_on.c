/*
 * power_on.c: the game's start: the state as the original's power-on
 * leaves it, and the first task.
 */
#include <string.h>

#include "system/state.h"
#include "system/system.h"
#include "system/task.h"

/*
 * @routine 80:8000 PowerOn
 * WHAT IT DOES: The game's start: work RAM cleared, the kept options set
 * afresh, the NMI on, the first task (slot 0, BootTask) created; the
 * scheduler then runs each update (system/main.c).
 * HOW: The original kept the options over a console reset when work RAM
 * still held its signature ($0000 = $0123), the cartridge's title and the
 * options' sum. The native game starts only from power-on, so it always
 * takes the cold start: the sum $80, the options 0, the cheat byte ($0009)
 * 0. The cold start's other writes ($0000, $0004, $0005, $000A, the direct
 * page's $5F) are read nowhere in the game, so they have no field; the
 * hardware's set-up is gone.
 */
void PowerOn(void)
{
    memset(&g, 0, sizeof g);
    g.blank = 1;
    g.brightness = 15;
    g.kept_options_sum = 0x80;
    tasks_init();
    NmiEnable();
}
