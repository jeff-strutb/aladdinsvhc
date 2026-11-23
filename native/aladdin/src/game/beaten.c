/*
 * beaten.c: the beaten levels' record and the start of a chosen level. Nothing of the original's.
 *
 * The file "levels" (the engine's save folder, beside "options"): SIZE bytes: its size, the version, the beaten levels as a
 * 24 bit mask (bit n: level n) little endian, and a check byte (0xA5 xor the five bytes before it). A file of another size,
 * version or check, or with a bit that is no level of the list, is left out whole and nothing is beaten. The engine
 * writes it to a temporary name and renames it (nshvc_save_write).
 */
#include <string.h>

#include "data/level_tables.h"
#include "data/game_tables.h"
#include "game/beaten.h"
#include "game/game.h"
#include "nshvc/nshvc.h"
#include "system/state.h"

#define SIZE 6
#define VERSION 1
#define PARTS 7                     /* parts 0-6; the part after the last is the ending */
#define PART_ENDING 7

static const char NAME[] = "levels";
static unsigned listed;
static uint8_t list_part[BEATEN_LEVELS], list_stage[BEATEN_LEVELS], list_level[BEATEN_LEVELS];
static uint32_t mask;
static int loaded;

/* The list: from part 0, a part's stages (up to the part's last stage, the level select's own limits) and then the part the
   game's own table says comes next, until the ending. */
static void list_make(void)
{
    unsigned part = 0, guard;

    if (listed)
        return;
    for (guard = 0; part < PARTS && guard < PARTS; guard++) {
        unsigned last = level_select_last_stage((uint8_t)part), stage;

        for (stage = 0; stage <= last && listed < BEATEN_LEVELS; stage++) {
            list_part[listed] = (uint8_t)part;
            list_stage[listed] = (uint8_t)stage;
            list_level[listed] = progress_level((uint8_t)(progress_first((uint8_t)part) + stage));
            listed++;
        }
        part = progress_next_sequence((uint8_t)part);
        if (part >= PART_ENDING)
            break;
    }
}

unsigned BeatenListLevel(unsigned i) { list_make(); return list_level[i % BEATEN_LEVELS]; }
unsigned BeatenListPart(unsigned i) { list_make(); return list_part[i % BEATEN_LEVELS]; }
unsigned BeatenListStage(unsigned i) { list_make(); return list_stage[i % BEATEN_LEVELS]; }

unsigned BeatenListOf(unsigned level)
{
    unsigned i;

    list_make();
    for (i = 0; i < listed; i++)
        if (list_level[i] == level)
            return i;
    return BEATEN_LEVELS;
}

static uint32_t all_bits(void)
{
    uint32_t bits = 0;
    unsigned i;

    list_make();
    for (i = 0; i < listed; i++)
        bits |= 1u << list_level[i];
    return bits;
}

static uint8_t check_of(const uint8_t *f)
{
    uint8_t c = 0xA5;
    unsigned i;

    for (i = 0; i < SIZE - 1; i++)
        c ^= f[i];
    return c;
}

static void load(void)
{
    uint8_t f[SIZE + 1];
    uint32_t m;

    loaded = 1;
    mask = 0;
    if (nshvc_save_read(NAME, f, sizeof f) != SIZE || f[0] != SIZE || f[1] != VERSION || f[5] != check_of(f))
        return;
    m = (uint32_t)f[2] | (uint32_t)f[3] << 8 | (uint32_t)f[4] << 16;
    if (m & ~all_bits())
        return;
    mask = m;
}

static void save(void)
{
    uint8_t f[SIZE];

    f[0] = SIZE;
    f[1] = VERSION;
    f[2] = (uint8_t)mask;
    f[3] = (uint8_t)(mask >> 8);
    f[4] = (uint8_t)(mask >> 16);
    f[5] = check_of(f);
    nshvc_save_write(NAME, f, sizeof f);
}

int BeatenIs(unsigned level)
{
    if (!loaded)
        load();
    return level < 24 && (mask >> level & 1u);
}

unsigned BeatenCount(void)
{
    unsigned i, n = 0;

    list_make();
    for (i = 0; i < listed; i++)
        n += (unsigned)BeatenIs(list_level[i]);
    return n;
}

unsigned BeatenFind(unsigned i)
{
    unsigned k, n = 0;

    list_make();
    for (k = 0; k < listed; k++)
        if (BeatenIs(list_level[k]) && n++ == i)
            return k;
    return BEATEN_LEVELS;
}

void LevelBeaten(void)
{
    unsigned level = g.level;

    if (!loaded)
        load();
    if (level >= 24 || !(all_bits() >> level & 1u) || (mask >> level & 1u))
        return;
    mask |= 1u << level;
    save();
}

void LevelSelectStart(unsigned i)
{
    list_make();
    if (i >= listed)
        return;
    NewGameCounts();
    g.progress_sequence = list_part[i];
    g.progress_stage = list_stage[i];
    g.game_begun = 0;
}
