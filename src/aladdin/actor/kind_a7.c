/*
 * kind_a7.c: actor kind $A7, a figure of the ending: it waits (for a cue,
 * by its $07), is placed, fades its palette up out of black with colour
 * math, and then stays, or (with $07 set) rises 64 pixels and stays.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "wram.h"
#include "wram_video.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_PaletteFadeStep(void);
void abi_PaletteSave(void);

#define CHAIN_HEAD 0x1B14u      /* cleared, then awaited by a $07 figure (set by another actor) */
#define KA7_FADE 0x40u          /* the fade's step: -$20 up to 8 */
#define KA7_TOP 0x42u           /* a $07 figure: the y it rises to (word) */
#define KA7_PLACES 0xD99Eu      /* (data bank) by $07 * 4: x, y (words; on the screen for $07 > 0) */
#define SUB_LAYERS 0x039Au      /* layers kept for the subscreen while the fade runs */
#define SCROLL_X (LAYER0 + LAYER_SCROLL_X)
#define SCROLL_Y (LAYER0 + LAYER_SCROLL_Y)

static void wait(uint8_t n, uint32_t cop_at)
{
    a8_load(n);
    actor_wait(cop_at);
}

/*
 * @routine 84:AC66 KindA7Start
 * @actor
 * WHAT IT DOES: Actor kind $A7, a figure of the ending. $07 0: three
 * waits of 255 frames; $07 set: CHAIN_HEAD cleared, then awaited, and 96
 * frames. Then pose 0 at its place (KA7_PLACES by $07; for $07 set, from
 * the screen's corner), its sprite palette faded up from black in steps
 * of 6 frames (4 for $07 set) under colour math, and: $07 0 waits 255 and
 * 192 frames and stays; $07 set waits 255 and 152 frames, rises 64 pixels
 * half a pixel a frame, and stays.
 * HOW: $07 set ($84:AC74): COP $60 after CHAIN_HEAD. AnimStart 0; X =
 * $07 * 4; $0C, $0F from KA7_PLACES (+ the scroll for X > 0, $84:AC95);
 * ACTOR_09 $20; PaletteSave of one palette (8 + its palette bits);
 * KA7_FADE $E0, PaletteFadeStep; CGWSEL 2, CGADSUB $12, SUB_LAYERS $10;
 * each step COP 6 (4: $84:ACE5), KA7_FADE up, PaletteFadeStep, until 8;
 * CGWSEL $30, CGADSUB 0; ACTOR_09 bits 4-5 cleared. $07 0: COP $FF, COP
 * $C0, BRK for good. $07 set ($84:AD19): COP $FF, COP $98, KA7_TOP = y -
 * $40; each frame (BRK) y less $0080 (24-bit, from $0E) until it is below
 * KA7_TOP; BRK for good. The $07 set paths no script meets yet: written
 * from their bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds KA7_PLACES (bank $84); reached by
 * the actor loop's jump for kind $A7 in state $0C (the table at $83:8DAB,
 * through the stub at $83:9442). Exit: never returns.
 */
void abi_KindA7Start(void)
{
    uint8_t v07 = rd8(dpa(ACTOR_07)), at, step;
    uint16_t y, sc;

    a8_load(v07);
    if (v07) {
        wr8(dba(CHAIN_HEAD), 0);
        do {
            actor_yield(0x84AC6Du);
            a8_load(rd8(dba(CHAIN_HEAD)));
        } while (!(uint8_t)cpu.a);
        wait(0x60, 0x84AC76u);
    } else {
        wait(0xFF, 0x84AC7Cu);
        wait(0xFF, 0x84AC80u);
        wait(0xFF, 0x84AC84u);
    }
    a8_load(0x00);
    abi_AnimStart();

    /* its place */
    op_rep(0x20);
    at = (uint8_t)(rd8(dpa(ACTOR_07)) << 2);
    a16_load((uint16_t)(rd16(dpa(ACTOR_07)) << 2));
    x8_load(at);
    if (at) {
        flag_set(P_C, 0);
        sc = rd16(dba((uint16_t)SCROLL_X));
        a16_load(op_adc16(sc, rd16(dba((uint16_t)(KA7_PLACES + at))), &cpu.p));
        dp16_set(ACTOR_X + 1, cpu.a);
        flag_set(P_C, 0);
        sc = rd16(dba((uint16_t)SCROLL_Y));
        a16_load(op_adc16(sc, rd16(dba((uint16_t)(KA7_PLACES + 2 + at))), &cpu.p));
        dp16_set(ACTOR_Y + 1, cpu.a);
    } else {
        a16_load(rd16(dba((uint16_t)(KA7_PLACES + at))));
        dp16_set(ACTOR_X + 1, cpu.a);
        a16_load(rd16(dba((uint16_t)(KA7_PLACES + 2 + at))));
        dp16_set(ACTOR_Y + 1, cpu.a);
    }
    op_sep(0x20);

    /* the fade up out of black */
    a8_load(0x20);
    wr8(dpa(ACTOR_09), 0x20);
    a8_load((uint8_t)(((rd8(dpa(ACTOR_LOOK + 1)) & 0x0E) >> 1) | 0x08));
    flag_set(P_C, 0);
    y8_load(0x01);
    abi_PaletteSave();
    a8_load(0xE0);
    wr8(dpa(KA7_FADE), 0xE0);
    y8_load(0x01);
    abi_PaletteFadeStep();
    a8_load(0x02);
    wr8(dba(CGWSEL_SHADOW), 0x02);
    a8_load(0x12);
    wr8(dba(CGADSUB_SHADOW), 0x12);
    a8_load(0x10);
    wr8(dba(SUB_LAYERS), 0x10);
    do {
        a8_load(0x06);
        x8_load(rd8(dpa(ACTOR_07)));
        if ((uint8_t)cpu.x)
            a8_load(0x04);
        actor_wait(0x84ACE7u);
        step = inc8_at(dpa(KA7_FADE));
        a8_load(step);
        y8_load(0x01);
        abi_PaletteFadeStep();
        step = rd8(dpa(KA7_FADE));
        a8_load(step);
        cmp8_of(step, 0x08);
    } while (step != 0x08);
    a8_load(0x30);
    wr8(dba(CGWSEL_SHADOW), 0x30);
    wr8(dba(CGADSUB_SHADOW), 0);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) & 0xCF));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    a8_load(rd8(dpa(ACTOR_07)));
    if (!(uint8_t)cpu.a) {
        wait(0xFF, 0x84AD0Fu);
        wait(0xC0, 0x84AD13u);
        for (;;)
            actor_yield(0x84AD15u);
    }

    /* a $07 figure rises 64 pixels */
    wait(0xFF, 0x84AD1Bu);
    wait(0x98, 0x84AD1Fu);
    op_rep(0x20);
    flag_set(P_C, 1);
    a16_load(op_sbc16(dp16(ACTOR_Y + 1), 0x0040, &cpu.p));
    dp16_set(KA7_TOP, cpu.a);
    op_sep(0x20);
    do {
        actor_yield(0x84AD2Du);
        flag_set(P_C, 1);
        a8_load(op_sbc8(rd8(dpa(ACTOR_Y)), 0x80, &cpu.p));
        wr8(dpa(ACTOR_Y), (uint8_t)cpu.a);
        a8_load(op_sbc8(rd8(dpa(ACTOR_Y + 1)), 0x00, &cpu.p));
        wr8(dpa(ACTOR_Y + 1), (uint8_t)cpu.a);
        a8_load(op_sbc8(rd8(dpa(ACTOR_Y + 2)), 0x00, &cpu.p));
        wr8(dpa(ACTOR_Y + 2), (uint8_t)cpu.a);
        op_rep(0x20);
        y = dp16(ACTOR_Y + 1);
        a16_load(y);
        cmp16_of(y, dp16(KA7_TOP));
        op_sep(0x20);
    } while (cpu.p & P_C);
    for (;;)
        actor_yield(0x84AD4Cu);
}
