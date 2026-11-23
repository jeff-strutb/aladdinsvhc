/*
 * saves_check.c: the slot files (system/saves.c) outside the game, with a fake snapshot (2,000 bytes of a counter and a layout
 * mark) and the engine's file calls in a folder (saves_build.sh). Writes, reads back, and refuses, each with its result:
 *   saves_check DIR
 * Prints one line a check, "ok" or "FAIL"; exits 1 on any FAIL.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "nshvc/nshvc.h"
#include "system/saves.h"
#include "system/snapshot.h"
#include "system/state.h"

GameState g;
static const char *dir;
static uint8_t world[2000];
static uint32_t mark = 0x1234;

int snapshot_available(void) { return 1; }
size_t snapshot_max(void) { return sizeof world; }
size_t snapshot_save(uint8_t *b, size_t cap) { if (cap < sizeof world) return 0; memcpy(b, world, sizeof world); return sizeof world; }
int snapshot_load(const uint8_t *b, size_t n) { if (n != sizeof world) return 0; memcpy(world, b, n); return 1; }
uint32_t snapshot_layout_mark(void) { return mark; }
uint64_t nshvc_updates(void) { return 4321; }
const uint32_t *nshvc_picture_read(int *w, int *h) { (void)w; (void)h; return NULL; }
int nshvc_window_to_picture(float x, float y, int32_t *px, int32_t *py) { (void)x; (void)y; (void)px; (void)py; return 0; }

static void path(const char *name, char *out, size_t n) { snprintf(out, n, "%s/%s", dir, name); }
int nshvc_save_write(const char *name, const void *data, uint32_t size)
{
    char p[1024], t[1100];
    FILE *f;

    path(name, p, sizeof p);
    snprintf(t, sizeof t, "%s.new", p);
    if (!(f = fopen(t, "wb")))
        return -1;
    fwrite(data, 1, size, f);
    fclose(f);
    remove(p);
    rename(t, p);
    return 0;
}
int nshvc_save_read(const char *name, void *data, uint32_t size)
{
    char p[1024];
    FILE *f;
    size_t n;

    path(name, p, sizeof p);
    if (!(f = fopen(p, "rb")))
        return -1;
    n = fread(data, 1, size, f);
    fclose(f);
    return (int)n;
}
void nshvc_save_delete(const char *name) { char p[1024]; path(name, p, sizeof p); remove(p); }

static int fails;
static void say(const char *what, int ok) { printf("%s: %s\n", what, ok ? "ok" : "FAIL"); fails += !ok; }

static long size_of(const char *name)
{
    char p[1024];
    FILE *f;
    long n;

    path(name, p, sizeof p);
    if (!(f = fopen(p, "rb")))
        return -1;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fclose(f);
    return n;
}

static void patch(const char *name, long at, int byte)
{
    char p[1024];
    FILE *f;

    path(name, p, sizeof p);
    f = fopen(p, "r+b");
    fseek(f, at, SEEK_SET);
    fputc(byte, f);
    fclose(f);
}

static void cut(const char *name, long n)
{
    char p[1024];
    uint8_t *b = malloc((size_t)n);
    FILE *f;

    path(name, p, sizeof p);
    f = fopen(p, "rb");
    fread(b, 1, (size_t)n, f);
    fclose(f);
    f = fopen(p, "wb");
    fwrite(b, 1, (size_t)n, f);
    fclose(f);
    free(b);
}

int main(int argc, char **argv)
{
    static SaveCapture c;
    uint8_t keep[sizeof world];
    int i;

    if (argc < 2)
        return 2;
    dir = argv[1];
    for (i = 0; i < (int)sizeof world; i++)
        world[i] = (uint8_t)(i * 7 + 3);
    memcpy(keep, world, sizeof world);
    c.have = 1;
    c.sequence = 1; c.stage = 2; c.digits[0] = 7; c.digits[1] = 0; c.digits[2] = 1; c.updates = 99;
    for (i = 0; i < SAVE_PIC_W * SAVE_PIC_H; i++)
        c.pic[i] = 0xFF000000u | (uint32_t)i;

    saves_scan();
    say("all slots empty with no files", saves_info(0)->state == SAVE_EMPTY && saves_info(23)->state == SAVE_EMPTY);
    say("write slot 3", saves_write(3, &c) == 1);
    say("file size is 36 + 27648 + 2000 + 8", size_of("save_03") == 36 + 27648 + 2000 + 8);
    say("slot 3 reads as used, crop and caption kept",
        saves_info(3)->state == SAVE_USED && saves_info(3)->pic[1000] == c.pic[1000] && !strcmp(saves_info(3)->line1, "LEVEL 2.3") &&
        !strcmp(saves_info(3)->line2, "TIME 0:01:07"));
    memset(world, 0, sizeof world);
    say("load puts the snapshot back", saves_load(3) == 1 && !memcmp(world, keep, sizeof world));
    say("an empty slot does not load", saves_load(4) == 0);

    /* refusals: each makes a copy of slot 3 into slot 5 and damages it */
    {
        const char *names[] = { "wrong layout mark", "wrong size (a byte longer)", "cut short", "one bit changed in the snapshot",
                                "one bit changed in the crop", "bad magic" };
        for (i = 0; i < 6; i++) {
            char p3[1024], p5[1024], cmd[2200];

            path("save_03", p3, sizeof p3);
            path("save_05", p5, sizeof p5);
            snprintf(cmd, sizeof cmd, "cp '%s' '%s'", p3, p5);
            system(cmd);
            if (i == 0) mark = 0x9999;
            if (i == 1) { FILE *f = fopen(p5, "ab"); fputc(0, f); fclose(f); }
            if (i == 2) cut("save_05", size_of("save_05") - 100);
            if (i == 3) patch("save_05", 36 + 27648 + 500, 0x55);
            if (i == 4) patch("save_05", 36 + 100, 0x55);
            if (i == 5) patch("save_05", 0, 'X');
            saves_scan();
            memcpy(world, keep, sizeof world);
            memset(world, 0xEE, 16);
            {
                uint8_t before[sizeof world];

                memcpy(before, world, sizeof world);
                say(names[i], saves_info(5)->state == SAVE_UNREADABLE && saves_load(5) == 0 && !memcmp(before, world, sizeof world));
            }
            mark = 0x1234;
        }
    }
    /* a failed write leaves the old slot: the folder is made unwritable for the temporary name */
    {
        char p[1024];
        FILE *f;

        path("save_03.new", p, sizeof p);
        mkdir(p, 0755);                 /* the temporary's name is a folder: fopen fails */
        c.sequence = 3;
        say("write refused when the temporary cannot be made", saves_write(3, &c) == 0);
        saves_scan();
        say("old slot 3 intact after the failed write", saves_info(3)->state == SAVE_USED && !strcmp(saves_info(3)->line1, "LEVEL 2.3"));
        rmdir(p);
        (void)f;
    }
    printf("%s\n", fails ? "FAILED" : "all ok");
    return fails != 0;
}
