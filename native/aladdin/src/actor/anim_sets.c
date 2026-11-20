/*
 * anim_sets.c: the animation sets a level's actors need, loaded once each.
 *
 * Each actor kind uses one animation set. Loading a level marks the sets of
 * the kinds it places and of the 32 kinds every level has, lists each set
 * once, and copies each listed set's picture list and animation table into
 * g.actor_pictures and g.anim_data. A kind's place is its set's place in the
 * list.
 */
#include <stdio.h>
#include <string.h>

#include "actor/actor_own.h"
#include "data/actor_tables.h"

#define NO_SET 0xFF
#define ALWAYS_KINDS 32
#define NOT_LISTED 4        /* the place of a kind whose set is not listed */

/*
 * @routine 81:9D4C AnimSetsFill
 * WHAT IT DOES: Sets every actor kind's animation set to `set`.
 * Also 81:9D4A AnimSetsClear, the same with $FF: no set.
 */
void AnimSetsFill(uint8_t set)
{
    memset(g.anim_set_of_kind, set, sizeof g.anim_set_of_kind);
}

/*
 * @routine 81:9D0C AnimSetsOfLevel
 * WHAT IT DOES: Marks the animation sets of the kinds the level places.
 */
static void AnimSetsOfLevel(void)
{
    unsigned i, n = placement_count(g.level);

    for (i = 0; i < n; i++) {
        uint8_t kind = placement_at(g.level, i).kind;

        g.anim_set_of_kind[kind] = kind_anim_set(kind);
    }
}

/*
 * @routine 81:906B AnimSetsAlways
 * WHAT IT DOES: Marks the animation sets of the 32 kinds every level has.
 */
static void AnimSetsAlways(void)
{
    int kind;

    for (kind = ALWAYS_KINDS - 1; kind >= 0; kind--)
        g.anim_set_of_kind[kind] = kind_anim_set((uint8_t)kind);
}

/*
 * @routine 81:902C AnimSetsList
 * WHAT IT DOES: Lists each animation set the kinds use, once, in kind
 * order. Returns how many (81:9079 AnimSetsListEnd's count).
 */
static unsigned AnimSetsList(uint8_t *sets)
{
    unsigned n = 0, i;
    int kind;

    for (kind = 0; kind < 256; kind++) {
        uint8_t s = g.anim_set_of_kind[kind];

        if (s == NO_SET)
            continue;
        for (i = 0; i < n && sets[i] != s; i++)
            ;
        if (i == n)
            sets[n++] = s;
    }
    return n;
}

/*
 * @routine 81:9086 AnimSetsPlaces
 * WHAT IT DOES: For each kind, its set's place in the list; 4 when it is
 * not listed.
 */
static void AnimSetsPlaces(const uint8_t *sets, unsigned n)
{
    int kind;
    unsigned i;

    for (kind = 0; kind < 256; kind++) {
        g.anim_set_place[kind] = NOT_LISTED;
        for (i = 0; i < n; i++)
            if (g.anim_set_of_kind[kind] == sets[i]) {
                g.anim_set_place[kind] = (uint8_t)i;
                break;
            }
    }
}

/*
 * @routine 81:8F47 AnimSetsLoad
 * WHAT IT DOES: Loads the level's animation sets: marks and lists the sets
 * its kinds use, and copies each listed set's picture list and animation
 * table into g.actor_pictures and g.anim_data, one after another; each set's
 * start goes to g.anim_set_pictures and g.anim_set_table by its place.
 */
void AnimSetsLoad(void)
{
    uint8_t sets[256];
    unsigned n, i;
    uint32_t at;

    AnimSetsFill(NO_SET);
    AnimSetsOfLevel();
    AnimSetsAlways();
    n = AnimSetsList(sets);
    if (n > sizeof g.anim_set_pictures / sizeof g.anim_set_pictures[0]) {
        fprintf(stderr, "actor: level %u lists %u animation sets, more than %u\n", g.level, n,
                (unsigned)(sizeof g.anim_set_pictures / sizeof g.anim_set_pictures[0]));
        n = sizeof g.anim_set_pictures / sizeof g.anim_set_pictures[0];
    }
    for (i = 0, at = 0; i < n; i++) {
        uint16_t bytes;
        const uint8_t *from = anim_set_pictures(sets[i], &bytes);

        g.anim_set_pictures[i] = (uint16_t)at;
        if (at + bytes > sizeof g.actor_pictures) {
            fprintf(stderr, "actor: level %u's picture lists pass %u bytes\n", g.level, (unsigned)sizeof g.actor_pictures);
            break;
        }
        memcpy(&g.actor_pictures[at], from, bytes);
        at += bytes;
    }
    for (i = 0, at = 0; i < n; i++) {
        uint16_t words, w;
        const uint8_t *from = anim_set_table(sets[i], &words);

        g.anim_set_table[i] = (uint16_t)(2 * at);
        if (at + words > sizeof g.anim_data / sizeof g.anim_data[0]) {
            fprintf(stderr, "actor: level %u's animation tables pass %u words\n", g.level,
                    (unsigned)(sizeof g.anim_data / sizeof g.anim_data[0]));
            break;
        }
        for (w = 0; w < words; w++)
            g.anim_data[at + w] = (uint16_t)(from[2 * w] | from[2 * w + 1] << 8);
        at += words;
    }
    AnimSetsPlaces(sets, n);
}
