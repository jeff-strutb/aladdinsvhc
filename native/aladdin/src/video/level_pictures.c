/*
 * level_pictures.c: reads level_pictures.dat, the file of every level's picture (video/level_picture.c writes it when the game is
 * built; the format is there). The whole file is refused when its size, format, picture size or hash is not right.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/beaten.h"
#include "nshvc/nshvc.h"
#include "video/level_picture.h"

#define FILE_NAME "level_pictures.dat"
#define FILE_VERSION 1u
#define PIXELS (LEVEL_PICTURE_W * LEVEL_PICTURE_H)

static uint32_t fnv(uint32_t h, const uint8_t *p, size_t n)
{
    while (n--)
        h = (h ^ *p++) * 16777619u;
    return h;
}

static uint32_t get32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint32_t *pictures;      /* BEATEN_LEVELS pictures in the list's order */
static unsigned pictured[BEATEN_LEVELS];
static int pictures_read;

static void pictures_load(void)
{
    nshvc_asset a;
    size_t size = 20 + (size_t)BEATEN_LEVELS * (4 + 4 * PIXELS) + 4, got = 0;
    uint8_t *raw;
    uint32_t n, sum;

    pictures_read = 1;
    a = nshvc_asset_open(FILE_NAME);
    if (!a.id)
        return;
    raw = malloc(size + 1);
    if (!raw) {
        nshvc_asset_close(a);
        return;
    }
    while (got < size + 1 && (n = nshvc_asset_read(a, raw + got, (uint32_t)(size + 1 - got))) != 0)
        got += n;
    nshvc_asset_close(a);
    sum = got == size ? fnv(2166136261u, raw, size - 4) : 0;
    if (got != size || memcmp(raw, "LPIC", 4) || get32(raw + 4) != FILE_VERSION || get32(raw + 8) != BEATEN_LEVELS ||
        get32(raw + 12) != LEVEL_PICTURE_W || get32(raw + 16) != LEVEL_PICTURE_H || get32(raw + size - 4) != sum) {
        fprintf(stderr, "%s is not valid: run tools/make_pictures.sh\n", FILE_NAME);
        free(raw);
        return;
    }
    pictures = malloc((size_t)BEATEN_LEVELS * PIXELS * sizeof *pictures);
    if (pictures)
        for (unsigned i = 0; i < BEATEN_LEVELS; i++) {
            const uint8_t *at = raw + 20 + (size_t)i * (4 + 4 * PIXELS);

            pictured[i] = get32(at);
            for (unsigned k = 0; k < PIXELS; k++)
                pictures[(size_t)i * PIXELS + k] = get32(at + 4 + 4 * k);
        }
    free(raw);
}

const uint32_t *LevelPictureOf(unsigned level)
{
    if (!pictures_read)
        pictures_load();
    if (pictures)
        for (unsigned i = 0; i < BEATEN_LEVELS; i++)
            if (pictured[i] == level)
                return pictures + (size_t)i * PIXELS;
    return NULL;
}
