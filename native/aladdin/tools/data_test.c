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
    printf("\n");
    return 0;
}
