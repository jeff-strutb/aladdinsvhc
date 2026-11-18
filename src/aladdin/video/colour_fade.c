/*
 * colour_fade.c: fading the screen by colour math.
 *
 * A fade task takes a fixed grey from every layer (colour subtraction) and
 * steps the grey's level a frame or more apart; at 31 the screen is black.
 * The task's parameter byte is the frames between steps.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"

void abi_TaskWait(void);
void abi_TaskExit(void);
void abi_ScreenFullBright(void);
void abi_ColourFadeBegin(void);
void abi_ColourFadeBeginWith(void);
void abi_ColourFadeBeginKeep(void);
void abi_ColourFadeStep(void);

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
 * WHAT IT DOES: Sets colour math up for a fade by subtraction on every
 * layer and the sprites, and halves the fade task's frames between steps
 * (at least 1).
 * HOW: A = $9F, on into ColourFadeBeginWith.
 * Entry: M 8-bit, X 8-bit; Y = the task's slot; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: as ColourFadeBeginKeep.
 */
void abi_ColourFadeBegin(void)
{
    set_a8(FADE_CGADSUB);
    abi_ColourFadeBeginWith();
}

/*
 * @routine 81:9F68 ColourFadeBeginWith
 * WHAT IT DOES: Sets colour math up for a fade by subtraction on the
 * layers A names (CGADSUB's bits), and halves the fade task's frames
 * between steps.
 * HOW: CGADSUB's shadow = A, then on into ColourFadeBeginKeep.
 * Entry: as ColourFadeBegin, A low byte = CGADSUB. Exit: as
 * ColourFadeBeginKeep.
 */
void abi_ColourFadeBeginWith(void)
{
    wr8(abs_db(CGADSUB_SHADOW), (uint8_t)cpu.a);
    abi_ColourFadeBeginKeep();
}

/*
 * @routine 81:9F6B ColourFadeBeginKeep
 * WHAT IT DOES: Sets colour math up for a fade with CGADSUB as it is, and
 * halves the fade task's frames between steps (at least 1).
 * HOW: CGWSEL's shadow = 0, WOBJSEL's = $80; the task's parameter shifted
 * right, 0 becoming 1.
 * Entry: M 8-bit, X 8-bit; Y = the task's slot; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: A low byte = the new
 * parameter; N clear, Z clear; C = the parameter's old bit 0; X, Y, V
 * unchanged.
 */
void abi_ColourFadeBeginKeep(void)
{
    uint8_t a;

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



/* JSR ColourFadeStep from `site`: a call that can wait, its return
   address pushed. */
static void fade_step(uint16_t ret)
{
    push16(ret);
    abi_ColourFadeStep();
    cpu.s = (uint16_t)(cpu.s + 2);
}

/* The fade's end (81:9F5F): the colour window off, and TaskExit. */
static void fade_end(void)
{
    wr8(dba(WOBJSEL_SHADOW), 0);
    abi_TaskExit();
}

/* ColourFadeInTask from 81:9F20: the grey from 31 down to 0, then the end. */
static void fade_in_steps(void)
{
    x8_load(0x1F);
    do {
        fade_step(0x9F24);
        x8_load((uint8_t)(cpu.x - 1));
    } while (!(cpu.x & 0x80));
    fade_end();
}

/* ColourFadeInTask from 81:9F11: the grey at 31 for one step, a frame's
   wait, ScreenFullBright, then the steps down. */
static void fade_in(void)
{
    x8_load(0x1F);
    fade_step(0x9F15);
    a8_load(0x01);
    jsl_frame(0x819F18u);
    abi_TaskWait();
    abi_ScreenFullBright();
    fade_in_steps();
}

/* ColourFadeOutTask from 81:9F55: the grey from 0 up to 31, then the end. */
static void fade_out(void)
{
    x8_load(0x00);
    do {
        fade_step(0x9F59);
        x8_load((uint8_t)(cpu.x + 1));
        cmp8_of((uint8_t)cpu.x, 0x20);
    } while (cpu.x != 0x20);
    fade_end();
}

/*
 * @routine 81:9F0E ColourFadeInTask
 * WHAT IT DOES: A task that fades the screen in from black by colour
 * math, then ends.
 * HOW: ColourFadeBegin; the grey at 31 (black) for one step, one frame's
 * wait, then ScreenFullBright; the grey steps from 31 down to 0
 * (ColourFadeStep); then ColourFadeOutTask's end: the colour window off
 * and TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInTask(void)
{
    abi_ColourFadeBegin();
    fade_in();
}
/*
 * @routine 81:9F52 ColourFadeOutTask
 * WHAT IT DOES: A task that fades the screen out to black by colour math,
 * then ends.
 * HOW: ColourFadeBegin; the grey steps from 0 up to 31 (ColourFadeStep);
 * then the colour window off (WOBJSEL's shadow 0) and TaskExit.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeOutTask(void)
{
    abi_ColourFadeBegin();
    fade_out();
}

/* Fade tasks no code creates: each runs only where cov_unused_tasks.box
   creates it. */

/*
 * @routine 81:9EDD ColourFadeInSprites
 * WHAT IT DOES: A task that fades the sprites alone in from black by
 * colour math (CGADSUB $90), then ends. No code creates it.
 * HOW: A = $90; ColourFadeBeginWith; then as ColourFadeInTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInSprites(void)
{
    a8_load(0x90);
    abi_ColourFadeBeginWith();
    fade_in();
}

/*
 * @routine 81:9EE4 ColourFadeInBg1
 * WHAT IT DOES: A task that fades layer 1 alone in from black by colour
 * math (CGADSUB $81), then ends. No code creates it.
 * HOW: A = $81, on into ColourFadeInSprites past its load (81:9EDF);
 * ColourFadeBeginWith; then as ColourFadeInTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInBg1(void)
{
    a8_load(0x81);
    abi_ColourFadeBeginWith();
    fade_in();
}

/*
 * @routine 81:9EE8 ColourFadeInBg2
 * WHAT IT DOES: A task that fades layer 2 alone in from black by colour
 * math (CGADSUB $82), then ends. No code creates it.
 * HOW: A = $82, on into ColourFadeInSprites past its load (81:9EDF);
 * ColourFadeBeginWith; then as ColourFadeInTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInBg2(void)
{
    a8_load(0x82);
    abi_ColourFadeBeginWith();
    fade_in();
}

/*
 * @routine 81:9EEC ColourFadeInBg3
 * WHAT IT DOES: A task that fades layer 3 alone in from black by colour
 * math (CGADSUB $84), then ends. No code creates it.
 * HOW: A = $84, on into ColourFadeInSprites past its load (81:9EDF);
 * ColourFadeBeginWith; then as ColourFadeInTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInBg3(void)
{
    a8_load(0x84);
    abi_ColourFadeBeginWith();
    fade_in();
}

/*
 * @routine 81:9EF0 ColourFadeInBg1Sprites
 * WHAT IT DOES: A task that fades layer 1 and the sprites in from black
 * by colour math (CGADSUB $91), then ends. No code creates it.
 * HOW: A = $91, on into ColourFadeInSprites past its load (81:9EDF);
 * ColourFadeBeginWith; then as ColourFadeInTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInBg1Sprites(void)
{
    a8_load(0x91);
    abi_ColourFadeBeginWith();
    fade_in();
}

/*
 * @routine 81:9EF4 ColourFadeInBg1Lit
 * WHAT IT DOES: A task that fades layer 1 in from black by colour math
 * (CGADSUB $81), the screen turned on by clearing forced blank (not
 * ScreenFullBright), then ends. No code creates it.
 * HOW: A = $81; ColourFadeBeginWith; the grey at 31 for one step, a
 * frame's wait; INIDISP_SHADOW's forced blank bit cleared (its low four
 * bits kept); then as ColourFadeInTask from its steps down (81:9F20).
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInBg1Lit(void)
{
    a8_load(0x81);
    abi_ColourFadeBeginWith();
    x8_load(0x1F);
    fade_step(0x9EFD);
    a8_load(0x01);
    jsl_frame(0x819F00u);
    abi_TaskWait();
    a8_load((uint8_t)(rd8(dba(INIDISP_SHADOW)) & 0x0F));
    wr8(dba(INIDISP_SHADOW), (uint8_t)cpu.a);
    fade_in_steps();
}

/*
 * @routine 81:9F2A ColourFadeInKeep
 * WHAT IT DOES: A task that fades the screen in from black by colour
 * math with CGADSUB as it is, then ends. No code creates it.
 * HOW: ColourFadeBeginKeep; then as ColourFadeInTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeInKeep(void)
{
    abi_ColourFadeBeginKeep();
    fade_in();
}

/*
 * @routine 81:9F2F ColourFadeOutSprites
 * WHAT IT DOES: A task that fades the sprites alone out to black by
 * colour math (CGADSUB $90), then ends. No code creates it.
 * HOW: A = $90; ColourFadeBeginWith; then as ColourFadeOutTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeOutSprites(void)
{
    a8_load(0x90);
    abi_ColourFadeBeginWith();
    fade_out();
}

/*
 * @routine 81:9F36 ColourFadeOutBg1
 * WHAT IT DOES: A task that fades layer 1 alone out to black by colour
 * math (CGADSUB $81), then ends. No code creates it.
 * HOW: A = $81; ColourFadeBeginWith; then as ColourFadeOutTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeOutBg1(void)
{
    a8_load(0x81);
    abi_ColourFadeBeginWith();
    fade_out();
}

/*
 * @routine 81:9F3D ColourFadeOutBg2
 * WHAT IT DOES: A task that fades layer 2 alone out to black by colour
 * math (CGADSUB $82), then ends. No code creates it.
 * HOW: A = $82; ColourFadeBeginWith; then as ColourFadeOutTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeOutBg2(void)
{
    a8_load(0x82);
    abi_ColourFadeBeginWith();
    fade_out();
}

/*
 * @routine 81:9F44 ColourFadeOutBg3
 * WHAT IT DOES: A task that fades layer 3 alone out to black by colour
 * math (CGADSUB $84), then ends. No code creates it.
 * HOW: A = $84; ColourFadeBeginWith; then as ColourFadeOutTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeOutBg3(void)
{
    a8_load(0x84);
    abi_ColourFadeBeginWith();
    fade_out();
}

/*
 * @routine 81:9F4B ColourFadeOutBg1Sprites
 * WHAT IT DOES: A task that fades layer 1 and the sprites out to black
 * by colour math (CGADSUB $91), then ends. No code creates it.
 * HOW: A = $91; ColourFadeBeginWith; then as ColourFadeOutTask after its
 * ColourFadeBegin.
 * Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
 * parameter = the frames between steps (halved); DB a bank whose
 * $0000-$1FFF is work RAM. Exit: none: the task ends.
 */
void abi_ColourFadeOutBg1Sprites(void)
{
    a8_load(0x91);
    abi_ColourFadeBeginWith();
    fade_out();
}
