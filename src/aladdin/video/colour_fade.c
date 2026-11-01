/*
 * colour_fade.c: fading the screen by colour math.
 *
 * A fade task takes a fixed grey from every layer (colour subtraction) and
 * steps the grey's level a frame or more apart; at 31 the screen is black.
 * The task's parameter byte is the frames between steps.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "wram.h"

void abi_TaskWait(void);

static uint32_t abs_db(uint16_t addr)
{
    return ((uint32_t)cpu.db << 16) | addr;
}

static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* CGADSUB for the fade: subtract, on BG1-4 and the sprites. */
#define FADE_CGADSUB 0x9F
/* WOBJSEL for the fade: colour window 2 on. */
#define FADE_WOBJSEL 0x80

/*
 * @routine 81:9F66 ColourFadeBegin
 * WHAT IT DOES: Sets colour math up for a fade by subtraction, and halves
 * the fade task's frames between steps (at least 1).
 * HOW: CGADSUB's shadow = $9F, CGWSEL's = 0, WOBJSEL's = $80; the task's
 * parameter shifted right, 0 becoming 1.
 * Entry: M 8-bit, X 8-bit; Y = the task's slot; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: A low byte = the new
 * parameter; N clear, Z clear; C = the parameter's old bit 0; X, Y, V
 * unchanged.
 */
void abi_ColourFadeBegin(void)
{
    uint8_t a;

    set_a8(FADE_CGADSUB);
    wr8(abs_db(CGADSUB_SHADOW), FADE_CGADSUB);
    wr8(abs_db(CGWSEL_SHADOW), 0);
    set_a8(FADE_WOBJSEL);
    wr8(abs_db(WOBJSEL_SHADOW), FADE_WOBJSEL);
    a = rd8(abs_db((uint16_t)(TASK_PARAM + cpu.y)));
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a & 1));
    a >>= 1;
    if (a == 0)
        a++;
    set_a8(a);
    wr8(abs_db((uint16_t)(TASK_PARAM + cpu.y)), a);
}

/*
 * @routine 81:9F7E ColourFadeStep
 * WHAT IT DOES: Sets the fade's grey to level X and waits the task's
 * frames between steps.
 * HOW: The fixed colour's red, green and blue shadows each = X with its
 * channel bit; then TaskWait for the task's parameter.
 * Entry: M 8-bit, X 8-bit; X = the level (0-31); Y = the task's slot; DB a
 * bank whose $0000-$1FFF is work RAM; called by JSR from a task.
 * Exit: when the task resumes, the registers as the wait restores them.
 */
void abi_ColourFadeStep(void)
{
    set_a8((uint8_t)(cpu.x | 0x20));
    wr8(abs_db(COLDATA_RED), (uint8_t)cpu.a);
    set_a8((uint8_t)(cpu.x | 0x40));
    wr8(abs_db(COLDATA_GREEN), (uint8_t)cpu.a);
    set_a8((uint8_t)(cpu.x | 0x80));
    wr8(abs_db(COLDATA_BLUE), (uint8_t)cpu.a);
    set_a8(rd8(abs_db((uint16_t)(TASK_PARAM + cpu.y))));
    jsl_frame(0x819F93);
    abi_TaskWait();
}
