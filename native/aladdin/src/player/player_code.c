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
            return a->kind == 0x01 ? player_unconverted(a, "PlayerCarpetCrash (81:E7E9)") : player_states_code(a);
        return player_unconverted(a, "his third entry (state $C0 up)");
    case 1:
        return player_starts_code(a);
    case 2:
        return player_states_code(a);
    case 3:
        return player_unconverted(a, "the held states (81:D59A-81:DA14, player_held.c)");
    case 4:
        return player_unconverted(a, "the exits (81:CBF9-81:E7B1, player_exit.c)");
    case 5:
        return player_unconverted(a, "the scenes' states (81:DDC1-81:E154, scene_starts.c)");
    case 6:
        return player_unconverted(a, "the scene figures' states (81:DFBB, 81:E23E, 81:E27A)");
    default:
        return player_unconverted(a, "a resume point past his blocks");
    }
}

int PlayerGoTo(Actor *a, uint16_t at)
{
    a->resume = at;
    return PlayerCode(a);
}
