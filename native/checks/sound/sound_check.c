/*
 * sound_check.c: Aladdin's native sound driver against the original's, voice
 * event for voice event, tick for tick.
 *
 *   sound_check APU.BIN [--report N]
 *
 * APU.BIN is a run of the original (snesbox --apulog): the APU RAM at power-on,
 * then each SPC700 write, 8 bytes: the DSP samples made before it (32 bits),
 * the DSP's cycle in its sample, the address (16 bits), the byte. From it this
 * reads what the original's driver did, and when:
 *   - its start (timer 0's target, $FA = $40): the native driver starts on the
 *     data sound.c builds from cart.dat, as the game's power-on does;
 *   - each command it took (its echo of the command on port 3, $F7, written
 *     once the command has run): the same command to the native driver. A
 *     song upload ($FA) is the song whose list's bytes the original received,
 *     given by SongPlay; a fade ($F5, $F6) has the speed the original
 *     stored ($C7-$C8); the port 1 value is 0, as the game leaves it;
 *   - each tick (its step of $B0 after the timer's count, $B1, is written
 *     nonzero): the native driver's tick.
 * Before each tick both sides' voices are compared: what the DSP's registers
 * hold on the original (built from its writes to $F2/$F3) against what the
 * native driver last gave the engine (this program stands in for the
 * engine's sound calls and keeps what they were given):
 *   per voice: its sample (by its start and loop), its envelope (ADSR's four
 *   values when ADSR1's bit 7 is set, else GAIN), its pitch (14 bits), its
 *   left and right volume, its echo; the whole mix: main volume, echo volume,
 *   feedback, delay, filter, echo writes, mute, noise rate;
 *   and the key-ons (each with the sample it starts) and key-offs since the
 *   last comparison.
 * Not compared: ESA, the echo buffer's place (the engine has no call for it;
 * the original's overlaps none of the sound data), and when inside a tick
 * each write happened (the original's spread over the tick's SPC700 time; the
 * native driver's calls all come at the tick's start).
 *
 * Prints a line per difference (up to --report N, default 20), then the
 * totals; exits 1 when anything differs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nshvc/nshvc.h"
#include "audio/driver.h"
#include "audio/music.h"
#include "audio/sound.h"
#include "data/data.h"

/* ---- the engine's calls, kept ---- */

typedef struct {
    uint16_t start, loop;       /* the sample, by its place in the sound data */
    int adsr_on, a, dr, sl, sr, gain;
    int pitch, left, right, echo;
} Voice;

typedef struct {
    Voice v[8];
    int main_l, main_r, evol_l, evol_r, efb, edl, fir[8], writes, mute, noise;
    uint8_t kon, koff;
    uint16_t kon_start[8], kon_loop[8];
} Mix;

static Mix nat;

#define SAMPLES_MAX 256
static struct { const uint8_t *p; uint32_t size, loop; } loaded[SAMPLES_MAX];
static uint32_t nloaded;
static const uint8_t *data_base;

static void sample_place(nshvc_sample s, uint16_t *start, uint16_t *loop)
{
    *start = *loop = 0xFFFF;
    if (s.id >= 1 && s.id <= nloaded && data_base) {
        *start = (uint16_t)(loaded[s.id - 1].p - data_base);
        *loop = (uint16_t)(*start + loaded[s.id - 1].loop);
    }
}

nshvc_sample nshvc_sample_load(const void *data, uint32_t size, nshvc_sample_format f, uint32_t loop_start)
{
    nshvc_sample r = { 0 };
    (void)f;
    if (nloaded < SAMPLES_MAX) {
        loaded[nloaded].p = data;
        loaded[nloaded].size = size;
        loaded[nloaded].loop = loop_start;
        r.id = ++nloaded;
    }
    return r;
}

void nshvc_voice_sample(uint32_t v, nshvc_sample s) { sample_place(s, &nat.v[v].start, &nat.v[v].loop); }
void nshvc_voice_key_on(uint32_t v, nshvc_sample s)
{
    sample_place(s, &nat.v[v].start, &nat.v[v].loop);
    nat.kon |= (uint8_t)(1u << v);
    nat.kon_start[v] = nat.v[v].start;
    nat.kon_loop[v] = nat.v[v].loop;
}
void nshvc_voice_key_off(uint32_t v) { nat.koff |= (uint8_t)(1u << v); }
void nshvc_voice_pitch(uint32_t v, uint16_t p) { nat.v[v].pitch = p & 0x3FFF; }
void nshvc_voice_volume(uint32_t v, int8_t l, int8_t r) { nat.v[v].left = l; nat.v[v].right = r; }
void nshvc_voice_adsr(uint32_t v, uint8_t a, uint8_t dr, uint8_t sl, uint8_t sr)
{
    nat.v[v].adsr_on = 1; nat.v[v].a = a; nat.v[v].dr = dr; nat.v[v].sl = sl; nat.v[v].sr = sr;
}
void nshvc_voice_gain(uint32_t v, nshvc_gain_mode mode, uint8_t value)
{
    nat.v[v].adsr_on = 0;
    nat.v[v].gain = mode == NSHVC_GAIN_DIRECT ? (value & 0x7F) : (0x80 | (int)mode << 5 | (value & 31));
}
void nshvc_voice_options(uint32_t v, uint8_t noise, uint8_t echo, uint8_t pmod) { (void)noise; (void)pmod; nat.v[v].echo = echo != 0; }
void nshvc_sound_volume(int8_t l, int8_t r) { nat.main_l = l; nat.main_r = r; }
void nshvc_sound_noise_rate(uint8_t rate) { nat.noise = rate & 31; }
void nshvc_echo_writes(int on) { nat.writes = on != 0; }
void nshvc_sound_mute(int on) { nat.mute = on != 0; }
void nshvc_echo(uint8_t delay, int8_t feedback, int8_t l, int8_t r, const int8_t f[8])
{
    int i;
    nat.edl = delay & 15; nat.efb = feedback; nat.evol_l = l; nat.evol_r = r;
    for (i = 0; i < 8; i++)
        nat.fir[i] = f[i];
}

/* messages: kept, then taken by music_take as the audio thread would */
static uint8_t sent[16][MUSIC_MESSAGE_MAX];
static uint32_t sent_size[16], nsent;
void nshvc_music_send(const void *m, uint32_t size)
{
    if (nsent < 16 && size <= MUSIC_MESSAGE_MAX) {
        memcpy(sent[nsent], m, size);
        sent_size[nsent++] = size;
    }
}
uint32_t nshvc_music_receive(void *buffer, uint32_t max) { (void)buffer; (void)max; return 0; }

static const char *cart_path = "cart.dat";
static FILE *asset_f;
nshvc_asset nshvc_asset_open(const char *path)
{
    nshvc_asset a = { 0 };
    (void)path;
    if ((asset_f = fopen(cart_path, "rb")) != NULL)
        a.id = 1;
    return a;
}
uint32_t nshvc_asset_read(nshvc_asset a, void *p, uint32_t size) { (void)a; return (uint32_t)fread(p, 1, size, asset_f); }
void nshvc_asset_close(nshvc_asset a) { (void)a; fclose(asset_f); }

/* ---- the original's DSP, from its writes ---- */

static uint8_t ram[0x10000], dsp[128];
static Mix org;

static void org_voices(void)
{
    int v, i;

    for (v = 0; v < 8; v++) {
        const uint8_t *r = &dsp[v * 16];
        Voice *o = &org.v[v];
        uint16_t e = (uint16_t)(dsp[0x5D] * 0x100 + r[4] * 4);

        o->start = (uint16_t)(ram[e] | ram[(uint16_t)(e + 1)] << 8);
        o->loop = (uint16_t)(ram[(uint16_t)(e + 2)] | ram[(uint16_t)(e + 3)] << 8);
        o->adsr_on = (r[5] & 0x80) != 0;
        o->a = o->adsr_on ? r[5] & 15 : 0;
        o->dr = o->adsr_on ? (r[5] >> 4) & 7 : 0;
        o->sl = o->adsr_on ? r[6] >> 5 : 0;
        o->sr = o->adsr_on ? r[6] & 31 : 0;
        o->gain = r[7];
        o->pitch = (r[2] | r[3] << 8) & 0x3FFF;
        o->left = (int8_t)r[0];
        o->right = (int8_t)r[1];
        o->echo = (dsp[0x4D] >> v) & 1;
    }
    org.main_l = (int8_t)dsp[0x0C]; org.main_r = (int8_t)dsp[0x1C];
    org.evol_l = (int8_t)dsp[0x2C]; org.evol_r = (int8_t)dsp[0x3C];
    org.efb = (int8_t)dsp[0x0D]; org.edl = dsp[0x7D] & 15;
    for (i = 0; i < 8; i++)
        org.fir[i] = (int8_t)dsp[i * 16 + 0x0F];
    org.writes = !(dsp[0x6C] & 0x20);
    org.mute = (dsp[0x6C] & 0x40) != 0;
    org.noise = dsp[0x6C] & 31;
}

/* ---- the comparison ---- */

static long ticks, differ_ticks, kons, koffs, commands, reported, report_max = 20;
static uint32_t sample_now;

static void say(const char *what, int v, long a, long b)
{
    if (reported++ < report_max) {
        if (v >= 0)
            printf("tick %ld (sample %u): voice %d %s: original %ld native %ld\n", ticks, sample_now, v, what, a, b);
        else
            printf("tick %ld (sample %u): %s: original %ld native %ld\n", ticks, sample_now, what, a, b);
    }
}

static int compare(void)
{
    int v, i, bad = 0;

#define CMP(w, v, a, b) do { if ((long)(a) != (long)(b)) { say(w, v, (long)(a), (long)(b)); bad = 1; } } while (0)
    org_voices();
    for (v = 0; v < 8; v++) {
        const Voice *o = &org.v[v], *n = &nat.v[v];
        CMP("sample start", v, o->start, n->start);
        CMP("sample loop", v, o->loop, n->loop);
        CMP("ADSR on", v, o->adsr_on, n->adsr_on);
        if (o->adsr_on) {
            CMP("attack", v, o->a, n->a); CMP("decay", v, o->dr, n->dr);
            CMP("sustain level", v, o->sl, n->sl); CMP("sustain rate", v, o->sr, n->sr);
        } else {
            CMP("GAIN", v, o->gain, n->gain);
        }
        CMP("pitch", v, o->pitch, n->pitch);
        CMP("left", v, o->left, n->left);
        CMP("right", v, o->right, n->right);
        CMP("echo", v, o->echo, n->echo);
    }
    CMP("main left", -1, org.main_l, nat.main_l); CMP("main right", -1, org.main_r, nat.main_r);
    CMP("echo left", -1, org.evol_l, nat.evol_l); CMP("echo right", -1, org.evol_r, nat.evol_r);
    CMP("feedback", -1, org.efb, nat.efb); CMP("delay", -1, org.edl, nat.edl);
    for (i = 0; i < 8; i++)
        CMP("filter", -1, org.fir[i], nat.fir[i]);
    CMP("echo writes", -1, org.writes, nat.writes); CMP("mute", -1, org.mute, nat.mute);
    CMP("noise rate", -1, org.noise, nat.noise);
    CMP("key-ons", -1, org.kon, nat.kon);
    CMP("key-offs", -1, org.koff, nat.koff);
    for (v = 0; v < 8; v++)
        if (org.kon & nat.kon & (1u << v)) {
            CMP("key-on sample start", v, org.kon_start[v], nat.kon_start[v]);
            CMP("key-on sample loop", v, org.kon_loop[v], nat.kon_loop[v]);
        }
#undef CMP
    for (v = 0; v < 8; v++) {
        kons += (org.kon >> v) & 1;
        koffs += (org.koff >> v) & 1;
    }
    org.kon = org.koff = nat.kon = nat.koff = 0;
    return bad;
}

/* the messages sound.c sent, taken; uploads too unless `keep_uploads` is 0 */
static void take_sent(int keep_uploads)
{
    uint32_t i;
    for (i = 0; i < nsent; i++)
        if (keep_uploads || sent[i][0] != MUSIC_UPLOAD)
            music_take(sent[i], sent_size[i]);
    nsent = 0;
}

/* the song whose list's block is what the original received at $0D20 */
static int song_received(void)
{
    unsigned list;
    for (list = 16; list < 55; list++) {
        int n = sound_list_blocks(list);
        SoundBlock b;
        if (n != 1)
            continue;
        b = sound_list_block(list, 0);
        if (b.to == DATA_SONG_BLOCKS && !memcmp(&ram[b.to], b.bytes, b.length))
            return (int)list;
    }
    return -1;
}

int main(int argc, char **argv)
{
    FILE *f;
    uint8_t r[8];
    int started = 0, in_command = 0, tick_due = 0, i;
    long songs = 0;

    if (argc < 2) {
        fprintf(stderr, "usage: sound_check APU.BIN [--report N] [--cart CART.DAT]\n");
        return 2;
    }
    for (i = 2; i + 1 < argc; i++) {
        if (!strcmp(argv[i], "--report"))
            report_max = atol(argv[i + 1]);
        if (!strcmp(argv[i], "--cart"))
            cart_path = argv[i + 1];
    }
    if (!(f = fopen(argv[1], "rb")) || fread(ram, 1, sizeof ram, f) != sizeof ram) {
        fprintf(stderr, "sound_check: cannot read %s\n", argv[1]);
        return 2;
    }
    data_load();
    while (fread(r, 1, 8, f) == 8) {
        uint32_t s = (uint32_t)(r[0] | r[1] << 8 | r[2] << 16 | (uint32_t)r[3] << 24);
        uint16_t a = (uint16_t)(r[5] | r[6] << 8);
        uint8_t v = r[7];

        sample_now = s;
        if (a == 0x00B1 && v)
            tick_due = 1;     /* the timer counted: the next step of $B0 starts a tick */
        if (started && a == 0x00B0 && !in_command && tick_due) {
            tick_due = 0;
            ticks++;
            differ_ticks += compare();
            driver_tick();
        }
        ram[a] = v;
        if (a == 0x00F3)
            dsp[ram[0xF2] & 0x7F] = v;
        if (a == 0x00F3 && (ram[0xF2] & 0x7F) == 0x4C) {
            int k;
            org_voices();
            org.kon |= v;
            for (k = 0; k < 8; k++)
                if (v & (1u << k)) {
                    org.kon_start[k] = org.v[k].start;
                    org.kon_loop[k] = org.v[k].loop;
                }
        }
        if (a == 0x00F3 && (ram[0xF2] & 0x7F) == 0x5C)
            org.koff |= v;
        if (a == 0x00FA && v == 0x40) {
            /* the driver's start: the game's power-on load (its two set
               uploads come as the original's own $F9s) */
            SoundSetsLoad();
            data_base = NULL;
            for (i = 0; i < (int)nsent; i++)
                if (sent[i][0] == MUSIC_START) {
                    MusicStart m;
                    memcpy(&m, sent[i], sizeof m);
                    data_base = m.data->bytes;
                }
            take_sent(0);
            started = 1;
            in_command = 0;
            continue;
        }
        if (!started)
            continue;
        if (a == 0x00B2)
            in_command = 1;
        if (a == 0x00F7) {
            in_command = 0;
            if (!v)
                continue;   /* the start's own write: no command is 0 */
            commands++;
            if (v == 0xFA) {
                int list = song_received();
                if (list < 0) {
                    printf("sample %u: a song upload that matches no song's list\n", s);
                    return 1;
                }
                SongPlay((uint8_t)list);
                take_sent(1);
                songs++;
            } else if (v == 0xF5 || v == 0xF6) {
                driver_command(v, (uint16_t)(ram[0xC7] | ram[0xC8] << 8), 0);
            } else {
                driver_command(v, 0, 0);
            }
        }
    }
    fclose(f);
    printf("%ld ticks compared, %ld with a difference; %ld commands (%ld songs); %ld key-ons, %ld key-offs: %s\n",
           ticks, differ_ticks, commands, songs, kons, koffs, differ_ticks ? "DIFFER" : "same");
    return differ_ticks ? 1 : 0;
}
