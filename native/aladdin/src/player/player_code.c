/*
 * player_code.c: Aladdin's code, kinds $00-$03 and $07 (the record's
 * entries for them: start 83:91F4 Kind00Start, second 83:91FD Kind00Second
 * and 83:9201 Kind01Second, the stubs that jump to PlayerStart,
 * PlayerKnockback and PlayerCarpetCrash): one function that goes on where
 * his record's resume point says, each of player/'s files numbering its
 * own points in a block (player.h, player_own.h).
 */
#include <stdio.h>

#include "player/player_own.h"

/* a state not converted yet: its name once, and his record waits a frame
   where it is */
int player_unconverted(Actor *a, const char *name)
{
    static const char *told[64];
    unsigned i;

    (void)a;
    for (i = 0; i < sizeof told / sizeof told[0] && told[i]; i++)
        if (told[i] == name)
            return 1;
    if (i < sizeof told / sizeof told[0])
        told[i] = name;
    fprintf(stderr, "player: %s not converted\n", name);
    return 1;
}

int PlayerCode(Actor *a)
{
    switch (a->resume >> 8) {
    case 0:
        if (a->resume == ACTOR_START)
            return player_starts_code(a);
        if (a->resume == ACTOR_START_SECOND)
            return a->kind == 0x01 ? PlayerGoTo(a, P_CARPET_CRASH) : player_states_code(a);
        return player_unconverted(a, "his third entry (state $C0 up)");
    case 1:
        return player_starts_code(a);
    case 2:
        return player_states_code(a);
    case 3:
        return player_held_code(a);
    case 4:
        return player_exit_code(a);
    case 5:
    case 6:
        return player_scene_code(a);
    case 7:
        return player_carpet_code(a);
    default:
        return player_unconverted(a, "a resume point past his blocks");
    }
}

int PlayerGoTo(Actor *a, uint16_t at)
{
    a->resume = at;
    return PlayerCode(a);
}
