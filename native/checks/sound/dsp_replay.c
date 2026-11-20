/*
 * dsp_replay.c: the original's own DSP register writes, each at the sample
 * the original made it, given to the engine: how far the engine's mixer is
 * from bsnes's DSP when the writes come at the same samples. A test bench
 * for sound_play's result, not part of the game.
 *
 *   dsp_replay --apu APU.BIN --headless --updates N --sound OUT.RAW
 *
 * Each write to $F3 (the register $F2 names) becomes the engine call for it,
 * before the sample after the one it was made in: a voice's VOL, PITCH (both
 * bytes as they stand), SRCN (the sample the directory entry names, as
 * sound.c loads them), ADSR1, ADSR2 and GAIN (the envelope ADSR1's bit 7
 * chooses), KON, KOF, MVOL, EVOL, EFB, EDL, the filter, EON and FLG. ESA has
 * no call; PMON and NON stay 0 (the driver writes them 0 once).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "audio/driver.h"
#include "audio/music.h"
#include "audio/sound.h"
#include "data/data.h"

typedef struct { uint32_t sample; uint8_t reg, value; } Write;

static Write *w;
static uint32_t nw, at, samples;
static const char *apu_path;
static uint8_t dsp[128];
static nshvc_sample source[DRIVER_SOURCES];
static int8_t fir[8];

static void load(void)
{
    static uint8_t ram[0x10000];
    FILE *f = fopen(apu_path, "rb");
    uint8_t r[8];
    uint32_t room = 0;

    if (!f || fread(ram, 1, sizeof ram, f) != sizeof ram) {
        fprintf(stderr, "dsp_replay: cannot read %s\n", apu_path);
        exit(2);
    }
    while (fread(r, 1, 8, f) == 8) {
        uint16_t a = (uint16_t)(r[5] | r[6] << 8);
        if (a == 0x00F3) {
            if (nw == room) {
                room = room ? room * 2 : 65536;
                w = (Write *)realloc(w, room * sizeof *w);
                if (!w)
                    exit(70);
            }
            w[nw].sample = (uint32_t)(r[0] | r[1] << 8 | r[2] << 16 | (uint32_t)r[3] << 24);
            w[nw].reg = ram[0xF2] & 0x7F;
            w[nw].value = r[7];
            nw++;
        }
        ram[a] = r[7];
    }
    fclose(f);
}

static void envelope(int v)
{
    uint8_t a1 = dsp[v * 16 + 5], a2 = dsp[v * 16 + 6], g = dsp[v * 16 + 7];
    if (a1 & 0x80)
        nshvc_voice_adsr((uint32_t)v, a1 & 15, (a1 >> 4) & 7, a2 >> 5, a2 & 31);
    else if (!(g & 0x80))
        nshvc_voice_gain((uint32_t)v, NSHVC_GAIN_DIRECT, g & 0x7F);
    else
        nshvc_voice_gain((uint32_t)v, (nshvc_gain_mode)(g >> 5), g & 31);
}

static void put(uint8_t reg, uint8_t value)
{
    int v = reg >> 4, i;
    dsp[reg] = value;
    switch (reg & 0x0F) {
    case 0x0: case 0x1:
        nshvc_voice_volume((uint32_t)v, (int8_t)dsp[v * 16], (int8_t)dsp[v * 16 + 1]);
        return;
    case 0x2: case 0x3:
        nshvc_voice_pitch((uint32_t)v, (uint16_t)((dsp[v * 16 + 2] | dsp[v * 16 + 3] << 8) & 0x3FFF));
        return;
    case 0x4:
        if (value < DRIVER_SOURCES)
            nshvc_voice_sample((uint32_t)v, source[value]);
        return;
    case 0x5: case 0x6: case 0x7:
        envelope(v);
        return;
    case 0xF:
        fir[v] = (int8_t)value;
        nshvc_echo(dsp[0x7D] & 15, (int8_t)dsp[0x0D], (int8_t)dsp[0x2C], (int8_t)dsp[0x3C], fir);
        return;
    }
    switch (reg) {
    case 0x4C:
        for (i = 0; i < 8; i++)
            if (value & (1u << i) && dsp[i * 16 + 4] < DRIVER_SOURCES)
                nshvc_voice_key_on((uint32_t)i, source[dsp[i * 16 + 4]]);
        break;
    case 0x5C:
        for (i = 0; i < 8; i++)
            if (value & (1u << i))
                nshvc_voice_key_off((uint32_t)i);
        break;
    case 0x0C: case 0x1C:
        nshvc_sound_volume((int8_t)dsp[0x0C], (int8_t)dsp[0x1C]);
        break;
    case 0x2C: case 0x3C: case 0x0D: case 0x7D:
        nshvc_echo(dsp[0x7D] & 15, (int8_t)dsp[0x0D], (int8_t)dsp[0x2C], (int8_t)dsp[0x3C], fir);
        break;
    case 0x4D:
        for (i = 0; i < 8; i++)
            nshvc_voice_options((uint32_t)i, 0, (value >> i) & 1, 0);
        break;
    case 0x6C:
        nshvc_echo_writes(!(value & 0x20));
        nshvc_sound_mute((value & 0x40) != 0);
        nshvc_sound_noise_rate(value & 31);
        break;
    }
}

/* writes made during sample s, before sample s + 1 */
static void tick(void)
{
    while (at < nw && w[at].sample < samples) {
        put(w[at].reg, w[at].value);
        at++;
    }
    samples++;
}

static void init(void)
{
    data_load();
    load();
    SoundSetsLoad();            /* the samples, as the game loads them */
    memcpy(source, sound_data()->source, sizeof source);
    printf("dsp_replay: %u DSP writes\n", nw);
}

static void update(const nshvc_input *in) { (void)in; }
static void draw(float between) { (void)between; }

int main(int argc, char **argv)
{
    static nshvc_game g;
    int i;
    for (i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], "--apu"))
            apu_path = argv[i + 1];
    if (!apu_path) {
        fprintf(stderr, "usage: dsp_replay --apu APU.BIN --headless --updates N --sound OUT.RAW\n");
        return 2;
    }
    g.title = "DSP replay";
    g.id = "dsp-replay";
    g.update_hz = 60;
    g.init = init;
    g.update = update;
    g.draw = draw;
    g.music.hz = MUSIC_HZ;
    g.music.tick = tick;
    g.music.queue_bytes = MUSIC_QUEUE_BYTES;
    return nshvc_run(&g, argc, argv);
}
