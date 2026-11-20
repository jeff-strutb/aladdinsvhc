/*
 * record.c: records the original, played on the project's bsnes core, for
 * the native game's checks (docs/aladdin/native/CHECK.md, PLAN.md section 6).
 * Recordings find bugs in the converted game; they are never its target.
 *
 *   record --core SO --rom ROM --inputs INPUTS --frames N --out DIR
 *          [--shot-every K] [--shots F,F...] [--voices]
 *
 * INPUTS is the engine's inputs.txt (native/aladdin/tools/check_inputs.py
 * makes one from an input-only script): the update number, then each pad's
 * buttons in hex. Update U is the core's frame U + 1 (frames counted from 1),
 * its buttons set before the frame runs. N frames are run.
 *
 * DIR receives, all little-endian:
 *   state.bin    the machine at marks, each as changes from the mark before
 *                (power-on's machine counts as all zero): at the end of
 *                every frame (kind 0, with the frame counted from 1) and,
 *                with the core of native/checks/core, at each picture's start,
 *                the PPU at line 0 (kind 4, with the update it falls in, the
 *                frame counted from 0; the CPU synchronizes the PPU before
 *                each PPU write, so video memory then is the picture's).
 *                Each mark: the frame, the kind and 0 (32 bits each), then
 *                for each 64-byte block that changed its offset (32 bits)
 *                and its bytes, ended by FFFFFFFF. As sfxbox --native writes
 *                them for Stunt Race FX. The machine is the
 *                core's snesbox_core_snapshot: work RAM 128K, a zero 64K
 *                (Super FX RAM, none here), VRAM 64K, OAM 544, CGRAM 512,
 *                APU RAM 64K. So work RAM, the sprite table, the palette and
 *                video memory are known at each frame's end.
 *   ppu.bin      every write that reaches a PPU register ($2100-$213F), by
 *                the CPU, DMA or HDMA, 12 bytes: the update it lands in (the
 *                frame counted from 0), the line and the dot (16 bits each),
 *                the register's low byte, the value, two zeros. With the
 *                state, the per-line picture state. Needs the core built with
 *                native/checks/core (its snesbox_ppu_write); without it this
 *                file is not written.
 *   pictures.txt each picture's start (the PPU at line 0): "UPDATE SAMPLES",
 *                the DSP samples made so far (needs the same core).
 *   sound.raw    every sample the DSP makes, 16-bit stereo, left first, at
 *                its 32,040 Hz.
 *   apu.bin      the APU RAM after power-on (65,536 bytes), then each write
 *                the SPC700 makes (APU RAM, its registers, and the DSP's
 *                through $F2/$F3), 8 bytes: the DSP samples made before it
 *                (32 bits), the DSP's cycle within the sample (0-31), the
 *                address (16 bits), the byte. The DSP writes with their
 *                sample time, the voice events' source.
 *   ports.bin    each S-CPU (0) or DMA (2) write to an APU port ($2140-$2143
 *                and their mirrors to $217F), 12 bytes: the update, the DSP
 *                samples made so far (32 bits each), the port (0-3), the
 *                value, the kind, a zero. The port commands.
 *   frames.txt   per frame: "FRAME SAMPLES", the DSP samples made by its end.
 *   shots/F.ppm  the core's picture at the end of frame F (counted from 1),
 *                as the oracle writes it: its lines once, 256 wide unless a
 *                pixel pair differs. Every K frames (--shot-every) and at
 *                the frames --shots lists.
 *   voices.bin   with --voices: with each DSP sample, each voice's state (8 x
 *                5 32-bit values: envelope level and mode, interpolation
 *                position, BRR block address and offset). 160 bytes a sample.
 */
#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

struct retro_game_info { const char *path; const void *data; size_t size; const char *meta; };
struct retro_variable { const char *key; const char *value; };
enum { ENV_GET_CAN_DUPE = 3, ENV_GET_SYSTEM_DIRECTORY = 9, ENV_SET_PIXEL_FORMAT = 10, ENV_GET_VARIABLE = 15,
       ENV_GET_SAVE_DIRECTORY = 31 };
enum { PIX_0RGB1555 = 0, PIX_XRGB8888 = 1, PIX_RGB565 = 2 };

/* the core's harness hooks (tools/toolchains/patches/bsnes, native/checks/core) */
struct callbacks {
    void (*insn)(uint32_t pc);
    void (*write)(uint32_t addr, uint8_t data, int kind);
    void (*read)(uint32_t addr, uint8_t data);
    void (*interrupt)(uint32_t resume_pc, uint16_t s);
    void (*timing)(int kind, int clocks, uint32_t addr);
};
#define SNAPSHOT (0x20000 + 0x10000 + 0x10000 + 544 + 512 + 0x10000)

static void (*p_init)(void);
static void (*p_set_environment)(bool (*)(unsigned, void *));
static void (*p_set_video_refresh)(void (*)(const void *, unsigned, unsigned, size_t));
static void (*p_set_audio_sample)(void (*)(int16_t, int16_t));
static void (*p_set_audio_sample_batch)(size_t (*)(const int16_t *, size_t));
static void (*p_set_input_poll)(void (*)(void));
static void (*p_set_input_state)(int16_t (*)(unsigned, unsigned, unsigned, unsigned));
static bool (*p_load_game)(const struct retro_game_info *);
static void (*p_run)(void);
static void (*p_snapshot)(uint8_t *);
static uint8_t *(*p_memory)(const char *, size_t *);

static const char *out;
static char sysdir[4096] = ".";
static unsigned pixfmt = PIX_0RGB1555;
static uint32_t frame[512 * 512];
static unsigned fw, fh;
static uint16_t pad[2];
static uint32_t updates;           /* frames run so far: the update being run */
static uint32_t samples;           /* DSP samples made so far */
static FILE *state_f, *ppu_f, *pictures_f, *sound_f, *apu_f, *ports_f, *frames_f, *voices_f;
static uint8_t now[SNAPSHOT], last[SNAPSHOT];

static void die(const char *m, const char *a)
{
    fprintf(stderr, "record: %s%s\n", m, a ? a : "");
    exit(1);
}

static FILE *open_out(const char *name)
{
    char p[4200];
    FILE *f;
    snprintf(p, sizeof p, "%s/%s", out, name);
    if (!(f = fopen(p, "wb")))
        die("cannot write ", p);
    return f;
}

static void put32(uint8_t *b, uint32_t v)
{
    b[0] = (uint8_t)v; b[1] = (uint8_t)(v >> 8); b[2] = (uint8_t)(v >> 16); b[3] = (uint8_t)(v >> 24);
}

static bool env(unsigned cmd, void *data)
{
    switch (cmd) {
    case ENV_GET_SYSTEM_DIRECTORY:
    case ENV_GET_SAVE_DIRECTORY:
        *(const char **)data = sysdir;
        return true;
    case ENV_SET_PIXEL_FORMAT:
        pixfmt = *(const unsigned *)data;
        return true;
    case ENV_GET_CAN_DUPE:
        *(bool *)data = true;
        return true;
    case ENV_GET_VARIABLE: {
        /* the oracle's settings (nativeshvc tests/oracle/oracle.c): accuracy
           first, no overclock, no overscan */
        struct retro_variable *v = (struct retro_variable *)data;
        static const char *fixed[][2] = {
            { "bsnes_ppu_fast", "OFF" }, { "bsnes_dsp_fast", "OFF" }, { "bsnes_coprocessor_delayed_sync", "OFF" },
            { "bsnes_cpu_overclock", "100" }, { "bsnes_cpu_sfx_overclock", "100" },
            { "bsnes_cpu_sa1_overclock", "100" }, { "bsnes_sfx_overclock", "100" }, { "bsnes_sa1_overclock", "100" },
            { "bsnes_ppu_show_overscan", "OFF" }, { "bsnes_blur_emulation", "OFF" }, { "bsnes_entropy", "None" },
            { "bsnes_hotfixes", "OFF" }, { "bsnes_cpu_fastmath", "OFF" }, { "bsnes_ppu_deinterlace", "OFF" },
            { "bsnes_ppu_no_sprite_limit", "OFF" }, { "bsnes_ppu_no_vram_blocking", "OFF" },
            { "bsnes_mode7_scale", "1x" }, { "bsnes_mode7_perspective", "OFF" }, { "bsnes_mode7_supersample", "OFF" },
            { "bsnes_mode7_mosaic", "OFF" }, { "bsnes_dsp_cubic", "OFF" }, { "bsnes_dsp_echo_shadow", "OFF" },
            { "bsnes_coprocessor_prefer_hle", "OFF" }, { "bsnes_video_filter", "None" }, { "bsnes_run_ahead_frames", "OFF" },
            { "bsnes_aspect_ratio", "8:7" },
        };
        unsigned i;
        for (i = 0; i < sizeof fixed / sizeof fixed[0]; i++)
            if (!strcmp(v->key, fixed[i][0])) {
                v->value = fixed[i][1];
                return true;
            }
        return false;
    }
    }
    return false;
}

static void video(const void *data, unsigned w, unsigned h, size_t pitch)
{
    unsigned x, y;
    if (!data)
        return;
    if (w > 512) w = 512;
    if (h > 512) h = 512;
    fw = w;
    fh = h;
    for (y = 0; y < h; y++) {
        const uint8_t *row = (const uint8_t *)data + y * pitch;
        for (x = 0; x < w; x++) {
            unsigned r, g, b;
            if (pixfmt == PIX_XRGB8888) {
                uint32_t c = ((const uint32_t *)row)[x];
                r = (c >> 19) & 31; g = (c >> 11) & 31; b = (c >> 3) & 31;
            } else {
                uint16_t p = ((const uint16_t *)row)[x];
                if (pixfmt == PIX_RGB565) {
                    r = p >> 11 & 31; g = p >> 6 & 31; b = p & 31;
                } else {
                    r = p >> 10 & 31; g = p >> 5 & 31; b = p & 31;
                }
            }
            frame[y * 512 + x] = (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
        }
    }
}

/* the oracle's shot: the lines once (the core doubles them), 256 wide unless
   a pixel pair differs somewhere */
static void shot(uint32_t n)
{
    char p[4200];
    unsigned x, y, ys = 1, xs = 1, ow, oh;
    FILE *f;
    snprintf(p, sizeof p, "%s/shots/%u.ppm", out, (unsigned)n);
    if (!(f = fopen(p, "wb")))
        die("cannot write ", p);
    if (fh >= 448) {
        ys = 2;
        for (y = 0; y + 1 < fh && ys == 2; y += 2)
            if (memcmp(&frame[y * 512], &frame[(y + 1) * 512], fw * 4))
                ys = 1;
    }
    if (fw == 512) {
        xs = 2;
        for (y = 0; y < fh && xs == 2; y++)
            for (x = 0; x < fw; x += 2)
                if (frame[y * 512 + x] != frame[y * 512 + x + 1]) {
                    xs = 1;
                    break;
                }
    }
    ow = fw / xs;
    oh = fh / ys;
    fprintf(f, "P6\n%u %u\n255\n", ow, oh);
    for (y = 0; y < oh; y++)
        for (x = 0; x < ow; x++) {
            uint32_t c = frame[y * ys * 512 + x * xs];
            uint8_t rgb[3] = { (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c };
            fwrite(rgb, 1, 3, f);
        }
    fclose(f);
}

static void audio(int16_t l, int16_t r) { (void)l; (void)r; }
static size_t audio_batch(const int16_t *d, size_t n) { (void)d; return n; }
static void poll(void) {}

static int16_t input(unsigned port, unsigned dev, unsigned idx, unsigned id)
{
    unsigned i, m = 0;
    (void)idx;
    if (dev != 1 || port > 1)                 /* RETRO_DEVICE_JOYPAD, two pads */
        return 0;
    for (i = 0; i < 12; i++)                  /* libretro's B Y SELECT START UP DOWN LEFT RIGHT A X L R */
        if (pad[port] & (0x8000u >> i))
            m |= 1u << i;
    if (id == 256)                            /* RETRO_DEVICE_ID_JOYPAD_MASK */
        return (int16_t)m;
    return id < 12 ? (int16_t)(m >> id & 1) : 0;
}

static void dsp_sample(int16_t l, int16_t r)
{
    int16_t b[2] = { l, r };
    fwrite(b, 2, 2, sound_f);
    samples++;
}

static void dsp_voices(const int32_t *st) { fwrite(st, 4, 40, voices_f); }

static void apu_write(uint16_t address, uint8_t data, int phase)
{
    uint8_t rec[8];
    put32(rec, samples);
    rec[4] = (uint8_t)phase;
    rec[5] = (uint8_t)address;
    rec[6] = (uint8_t)(address >> 8);
    rec[7] = data;
    fwrite(rec, 1, 8, apu_f);
}

static void cpu_write(uint32_t addr, uint8_t data, int kind)
{
    uint32_t bank = addr >> 16 & 0xFF, lo = addr & 0xFFFF;
    uint8_t rec[12];
    if ((bank >= 0x40 && bank < 0x80) || bank >= 0xC0 || lo < 0x2140 || lo > 0x217F)
        return;
    put32(rec, updates);
    put32(rec + 4, samples);
    rec[8] = (uint8_t)(lo & 3);
    rec[9] = data;
    rec[10] = (uint8_t)kind;
    rec[11] = 0;
    fwrite(rec, 1, 12, ports_f);
}

static void ppu_write(uint16_t address, uint8_t data, int line, int dot)
{
    uint8_t rec[12];
    put32(rec, updates);
    rec[4] = (uint8_t)line; rec[5] = (uint8_t)(line >> 8);
    rec[6] = (uint8_t)dot; rec[7] = (uint8_t)(dot >> 8);
    rec[8] = (uint8_t)address;
    rec[9] = data;
    rec[10] = rec[11] = 0;
    fwrite(rec, 1, 12, ppu_f);
}

static void mark(uint32_t n, uint32_t kind)
{
    uint32_t head[3] = { n, kind, 0 }, end = 0xFFFFFFFFu, at;
    p_snapshot(now);
    fwrite(head, 4, 3, state_f);
    for (at = 0; at < SNAPSHOT; at += 64) {
        uint32_t k = SNAPSHOT - at < 64 ? SNAPSHOT - at : 64;
        if (memcmp(now + at, last + at, k)) {
            fwrite(&at, 4, 1, state_f);
            fwrite(now + at, 1, k, state_f);
            memcpy(last + at, now + at, k);
        }
    }
    fwrite(&end, 4, 1, state_f);
}

static void ppu_picture(void)
{
    fprintf(pictures_f, "%u %u\n", (unsigned)updates, (unsigned)samples);
    mark(updates, 4);
}

/* the inputs: up to 1<<20 changes */
static uint32_t ch_u[1 << 20];
static uint16_t ch_p[1 << 20][2];
static size_t nch;

static void read_inputs(const char *path)
{
    char line[512];
    FILE *f = fopen(path, "r");
    if (!f)
        die("cannot read ", path);
    while (fgets(line, sizeof line, f)) {
        char *h = strchr(line, '#');
        unsigned long u;
        unsigned a = 0, b = 0;
        int k;
        if (h)
            *h = 0;
        k = sscanf(line, "%lu %x %x", &u, &a, &b);
        if (k < 1)
            continue;
        if (nch == sizeof ch_u / sizeof ch_u[0])
            die("too many input changes in ", path);
        if (nch && u <= ch_u[nch - 1])
            die("input updates out of order in ", path);
        ch_u[nch] = (uint32_t)u;
        ch_p[nch][0] = (uint16_t)(k > 1 ? a : 0);
        ch_p[nch][1] = (uint16_t)(k > 2 ? b : 0);
        nch++;
    }
    fclose(f);
}

static uint8_t romdata[8 << 20];
static unsigned char *shot_at;

int main(int argc, char **argv)
{
    const char *corepath = NULL, *rom = NULL, *inputs = NULL, *shots = NULL;
    long frames = -1, every = 0;
    int voices = 0, i;
    size_t c = 0;
    void *core;
    struct retro_game_info gi;
    FILE *f;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--core") && i + 1 < argc) corepath = argv[++i];
        else if (!strcmp(argv[i], "--rom") && i + 1 < argc) rom = argv[++i];
        else if (!strcmp(argv[i], "--inputs") && i + 1 < argc) inputs = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) out = argv[++i];
        else if (!strcmp(argv[i], "--shot-every") && i + 1 < argc) every = atol(argv[++i]);
        else if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!strcmp(argv[i], "--voices")) voices = 1;
        else
            die("usage: record --core SO --rom ROM --inputs INPUTS --frames N --out DIR "
                "[--shot-every K] [--shots F,F...] [--voices]", NULL);
    }
    if (!corepath || !rom || !inputs || frames < 1 || !out)
        die("--core, --rom, --inputs, --frames and --out are needed", NULL);
    read_inputs(inputs);
    {
        char p[4200];
        snprintf(p, sizeof p, "%s/shots", out);
        if (mkdir(out, 0777) && errno != EEXIST)
            die("cannot make ", out);
        if (mkdir(p, 0777) && errno != EEXIST)
            die("cannot make ", p);
    }
    shot_at = (unsigned char *)calloc((size_t)frames + 1, 1);
    if (!shot_at)
        die("out of memory", NULL);
    for (const char *s = shots; s && *s;) {
        char *e;
        long n = strtol(s, &e, 10);
        if (e == s)
            die("bad --shots ", shots);
        if (n >= 1 && n <= frames)
            shot_at[n] = 1;
        s = *e ? e + 1 : e;
    }
    if (every > 0)
        for (long n = every; n <= frames; n += every)
            shot_at[n] = 1;

    if (!(core = dlopen(corepath, RTLD_NOW | RTLD_LOCAL)))
        die("cannot load the core: ", dlerror());
#define LOAD(p, name) do { *(void **)&p = dlsym(core, name); if (!p) die("the core has no ", name); } while (0)
    LOAD(p_init, "retro_init");
    LOAD(p_set_environment, "retro_set_environment");
    LOAD(p_set_video_refresh, "retro_set_video_refresh");
    LOAD(p_set_audio_sample, "retro_set_audio_sample");
    LOAD(p_set_audio_sample_batch, "retro_set_audio_sample_batch");
    LOAD(p_set_input_poll, "retro_set_input_poll");
    LOAD(p_set_input_state, "retro_set_input_state");
    LOAD(p_load_game, "retro_load_game");
    LOAD(p_run, "retro_run");
    LOAD(p_snapshot, "snesbox_core_snapshot");
    LOAD(p_memory, "snesbox_memory");

    p_set_environment(env);
    p_init();
    p_set_video_refresh(video);
    p_set_audio_sample(audio);
    p_set_audio_sample_batch(audio_batch);
    p_set_input_poll(poll);
    p_set_input_state(input);

    state_f = open_out("state.bin");
    sound_f = open_out("sound.raw");
    apu_f = open_out("apu.bin");
    ports_f = open_out("ports.bin");
    frames_f = open_out("frames.txt");
    {
        void (**ds)(int16_t, int16_t) = (void (**)(int16_t, int16_t))dlsym(core, "snesbox_dsp_sample");
        void (**aw)(uint16_t, uint8_t, int) = (void (**)(uint16_t, uint8_t, int))dlsym(core, "snesbox_apu_write");
        void (**pw)(uint16_t, uint8_t, int, int) = (void (**)(uint16_t, uint8_t, int, int))dlsym(core, "snesbox_ppu_write");
        void (**pp)(void) = (void (**)(void))dlsym(core, "snesbox_ppu_picture");
        void (*set)(const struct callbacks *) = (void (*)(const struct callbacks *))dlsym(core, "snesbox_set_callbacks");
        static struct callbacks cb;
        if (!ds || !aw || !set)
            die("the core has no snesbox_dsp_sample, snesbox_apu_write or snesbox_set_callbacks", NULL);
        *ds = dsp_sample;
        *aw = apu_write;
        cb.write = cpu_write;
        set(&cb);
        if (pw && pp) {
            ppu_f = open_out("ppu.bin");
            pictures_f = open_out("pictures.txt");
            *pw = ppu_write;
            *pp = ppu_picture;
        } else
            fprintf(stderr, "record: the core has no PPU write hook (native/checks/core): no ppu.bin\n");
        if (voices) {
            void (**dv)(const int32_t *) = (void (**)(const int32_t *))dlsym(core, "snesbox_dsp_voices");
            if (!dv)
                die("the core has no snesbox_dsp_voices", NULL);
            voices_f = open_out("voices.bin");
            *dv = dsp_voices;
        }
    }

    if (!(f = fopen(rom, "rb")))
        die("cannot read ", rom);
    memset(&gi, 0, sizeof gi);
    gi.path = rom;
    gi.data = romdata;
    gi.size = fread(romdata, 1, sizeof romdata, f);
    fclose(f);
    if (!p_load_game(&gi))
        die("the core did not load ", rom);
    {
        size_t n;
        uint8_t *apu = p_memory("apuram", &n);
        if (!apu || n != 0x10000)
            die("the core gives no APU RAM", NULL);
        fwrite(apu, 1, n, apu_f);
    }

    for (updates = 0; updates < (uint32_t)frames; updates++) {
        while (c < nch && ch_u[c] <= updates) {
            pad[0] = ch_p[c][0];
            pad[1] = ch_p[c][1];
            c++;
        }
        p_run();
        mark(updates + 1, 0);
        fprintf(frames_f, "%u %u\n", (unsigned)(updates + 1), (unsigned)samples);
        if (shot_at[updates + 1])
            shot(updates + 1);
    }

    fclose(state_f);
    fclose(sound_f);
    fclose(apu_f);
    fclose(ports_f);
    fclose(frames_f);
    if (ppu_f) fclose(ppu_f);
    if (pictures_f) fclose(pictures_f);
    if (voices_f) fclose(voices_f);
    printf("recorded %ld frames, %u samples%s\n", frames, (unsigned)samples, ppu_f ? "" : " (no ppu.bin)");
    return 0;
}
