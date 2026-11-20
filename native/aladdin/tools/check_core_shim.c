/*
 * check_core_shim.c: the project's bsnes core under the hook names
 * nativeshvc's oracle (tests/oracle/oracle.c) looks for.
 *
 * The core this project builds (setup.sh, tools/toolchains/bsnes) has the
 * DSP sample hook the oracle needs, but named snesbox_dsp_sample; the
 * oracle looks up sfxbox_dsp_sample (Stunt Race FX's name for the same
 * patch), and without it records the core's 48 kHz resampled output
 * instead of the DSP's own samples. This library is linked against the
 * core (it is its dependency, so dlsym on this library finds the core's
 * retro_* functions too) and gives the oracle:
 *   sfxbox_dsp_sample  each sample as the DSP makes it, by the core's
 *                      snesbox_dsp_sample
 *   sfxbox_memory      the core's snesbox_memory (the oracle's dumps)
 * Nothing in the core is changed or rebuilt.
 */
#include <stddef.h>
#include <stdint.h>

extern void (*snesbox_dsp_sample)(int16_t left, int16_t right);
extern uint8_t *snesbox_memory(const char *name, size_t *size);

void (*sfxbox_dsp_sample)(int16_t left, int16_t right);

static void forward(int16_t left, int16_t right)
{
    if (sfxbox_dsp_sample)
        sfxbox_dsp_sample(left, right);
}

__attribute__((constructor)) static void hook(void)
{
    snesbox_dsp_sample = forward;
}

uint8_t *sfxbox_memory(const char *name, size_t *size)
{
    return snesbox_memory(name, size);
}
