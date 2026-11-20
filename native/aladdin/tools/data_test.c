/*
 * data_test.c: runs data/'s readers on a cart.dat outside the engine and
 * prints what they give, for data_check.py to hold against the ROM. The
 * engine's asset calls are stood in for by stdio (test only).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "data/data.h"
#include "data/video_tables.h"
#include "data/input_tables.h"

static FILE *asset_file;

nshvc_asset nshvc_asset_open(const char *path)
{
    nshvc_asset a = {0};
    const char *dir = getenv("DATA_TEST_DIR");
    char full[4096];

    snprintf(full, sizeof full, "%s/%s", dir ? dir : ".", path);
    asset_file = fopen(full, "rb");
    a.id = asset_file ? 1 : 0;
    return a;
}

uint32_t nshvc_asset_read(nshvc_asset a, void *data, uint32_t size)
{
    (void)a;
    return (uint32_t)fread(data, 1, size, asset_file);
}

void nshvc_asset_close(nshvc_asset a)
{
    (void)a;
    fclose(asset_file);
}

static uint8_t unpacked[0x80000];

static uint32_t sum_of(PackedGraphics p)
{
    uint32_t n = unpack_graphics(p, unpacked), j, sum = 0;

    for (j = 0; j < n; j++)
        sum = sum * 31u + unpacked[j];
    return sum ^ n;
}

int main(int argc, char **argv)
{
    unsigned i, b;

    data_load();
    for (i = 1; i < (unsigned)argc; i++) {         /* the sound lists asked for */
        unsigned list = (unsigned)strtoul(argv[i], 0, 0);
        int n = sound_list_blocks(list);

        printf("list %u blocks %d start %04X", list, n, sound_list_start(list));
        for (b = 0; n > 0 && b < (unsigned)n; b++) {
            SoundBlock k = sound_list_block(list, b);
            uint32_t sum = 0, j;

            for (j = 0; j < k.length; j++)
                sum = sum * 31u + k.bytes[j];
            printf(" %04X:%u:%08X", k.to, k.length, sum);
        }
        printf("\n");
    }
    printf("sine");
    for (i = 0; i < 64; i++)
        printf(" %u", quarter_sine_step(i));
    printf("\ndemo");
    for (i = 0; i < 0x82; i += 2) {
        DemoStep s = demo_step_at((uint16_t)i);
        printf(" %02X/%u", s.buttons, s.frames);
    }
    printf("\npalettes");
    for (i = 0; i < PALETTE_LOADS; i++) {
        uint32_t sum = 0, j, n = palette_load_count((uint8_t)i);

        for (j = 0; j < n; j++)
            sum = sum * 31u + palette_load_colour((uint8_t)i, (uint16_t)j);
        printf(" %u/%u/%08X", (unsigned)n, palette_load_first((uint8_t)i), sum);
    }
    printf("\nlevels");
    for (i = 0; i < LEVEL_GRAPHICS; i++) {
        unsigned n = level_vram_parts((uint16_t)i), k = level_kept_parts((uint16_t)i);

        printf(" %u:", i);
        for (b = 0; b < n; b++) {
            LevelVramPart p = level_vram_part((uint16_t)i, b);
            printf("v%04X/%u/%08X", p.to, p.bytes, sum_of(p.packed));
        }
        for (b = 0; b < k; b++)
            printf("k%08X", sum_of(level_kept_part((uint16_t)i, b)));
    }
    printf("\nsets");
    for (i = 0; i < UNPACK_SETS; i++) {
        UnpackSetEntry u = unpack_set((uint8_t)i);
        printf(" %04X/%u/%08X", u.to, u.packed.groups, sum_of(u.packed));
    }
    printf("\nmaps");
    for (i = 0; i < LEVEL_MAPS; i++) {
        unsigned n = level_map_screens((uint16_t)i), l, j, k;
        uint32_t sum = 0;

        for (b = 0; b < n; b++) {
            uint8_t sc = level_map_screen((uint16_t)i, b);
            for (j = 0; j < 128; j += 2) {
                uint16_t blk = (uint16_t)(level_screen_list_byte((uint16_t)i, sc, j) | level_screen_list_byte((uint16_t)i, sc, j + 1) << 8);
                for (k = 0; k < 4; k++)
                    sum = sum * 31u + level_block_tile((uint16_t)i, blk, k);
            }
            for (j = 128; j < 132; j++)
                sum = sum * 31u + level_screen_list_byte((uint16_t)i, sc, j);
        }
        printf(" %u:%u/%02X/%08X", i, n, level_map_layers((uint16_t)i), sum);
        for (l = 0; l < level_map_layout_count((uint16_t)i); l++) {
            unsigned c = level_map_layout_columns((uint16_t)i, l), r = level_map_layout_rows((uint16_t)i, l);
            uint32_t ls = 0;
            for (j = 0; j < c * r; j++)
                ls = ls * 31u + level_map_layout_screen((uint16_t)i, l, j);
            printf("L%ux%u/%08X", c, r, ls);
        }
    }
    printf("\n");
    return 0;
}
