/*
 * kind4c.c: actor kind $4C, the five marks at the end of levels $12 and $1C: each shows its
 * animation when the gem sets found reach its number, then goes; with $07 bit 4 set (no placement
 * has it) it waits, sets the level 0 and shows its animation for ever.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_video.h"

void abi_ActorEndPlaced(void);
void abi_ActorPaletteFromRom(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define K4C_COUNT 0x40u               /* frames of its animation left */
#define K4C_WAIT 0x41u                /* its long wait's two counters (bytes) */
#define K4C_FRAMES 0x88u              /* frames its animation shows */
#define K4C_SOUND_AT 0x50u            /* the count at which its sound plays */
#define K4C_SOUND 0x4Cu               /* its sound */
#define K4C_PALETTE_ROW 0x5Au         /* ActorPaletteFromRom's row for it */
#define K4C_GEM_SETS 0x0375u          /* the gem sets found */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine A5:F43D Kind4CStart
 * @actor
 * WHAT IT DOES: Actor kind $4C, one of the five placed at the end of
 * levels $12 and $1C (its $07 0-4 numbers them; bit 4 of $07 is set by
 * no placement). With $07 bit 4 clear it shows its animation 0 for 136
 * frames, with sound $4C halfway, when the gem sets found ($0375) less
 * 5 are at least its $07, else it goes at once; with $07 bit 4 set it
 * waits $A0 x $8C frames, sets the level ($035E) to 0 and then shows
 * animation 0 again and again, with a 32-frame wait between.
 * HOW: $08 bit 1 and $09 bit 4 set. $07 bit 0 clear: the palette byte
 * ($2C) 2 higher, and with $07's low four bits clear,
 * ActorPaletteFromRom with Y = $5A. $07 bit 4 clear: K4C_COUNT ($40) =
 * $88; $0375 less 5 borrowing, or less than $07: JML ActorEndPlaced;
 * else AnimStart 0, and each frame (BRK) AnimStep, K4C_COUNT down to 0,
 * sound $4C when it is $50 (AnimStart's frames first); then JML
 * ActorEndPlaced. $07 bit 4 set: K4C_WAIT (the bytes $41, $42) = $A0,
 * $8C; each frame (BRK) $41 down, and at 0 $42 down, until both are 0;
 * the level ($035E) cleared; then for ever K4C_COUNT = $88, AnimStart
 * 0, each frame (BRK) AnimStep and K4C_COUNT down to 0 with the sound
 * at $50, and a COP wait of $20 frames.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML
 * of kind $4C's stub (Kind4CEnter, $83:9245) from the actor loop in the
 * start state.
 * Exit: never returns: the first part ends in ActorEndPlaced, the
 * second goes on for ever.
 */
void abi_Kind4CStart(void)
{
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x10);
    sta8(ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_07));
    ACC8(lsr);
    if (IF_C)
        goto L_A5F461;
    lda8(ea_dp(ACTOR_LOOK + 1));
    clc();
    adc8_v(0x02);
    sta8(ea_dp(ACTOR_LOOK + 1));
    lda8(ea_dp(ACTOR_07));
    and8_v(0x0F);
    if (!IF_Z)
        goto L_A5F461;
    ldy8_v(K4C_PALETTE_ROW);
    jsl_call(0xA5F45Du, abi_ActorPaletteFromRom);
L_A5F461:
    lda8(ea_dp(ACTOR_07));
    and8_v(0x10);
    if (!IF_Z)
        goto L_A5F497;
    lda8_v(K4C_FRAMES);
    sta8(ea_dp(K4C_COUNT));
    sec();
    lda8(ea_abs(K4C_GEM_SETS));
    sbc8_v(0x05);
    if (!IF_C)
        goto L_A5F493;
    cmp8_v(rd8(ea_dp(ACTOR_07)));
    if (!IF_C)
        goto L_A5F493;
    lda8_v(0x00);
    jsl_call(0xA5F479u, abi_AnimStart);
L_A5F47D:
    lda8(ea_dp(K4C_COUNT));
    cmp8_v(K4C_SOUND_AT);
    if (!IF_Z)
        goto L_A5F489;
    lda8_v(K4C_SOUND);
    jsl_call(0xA5F485u, abi_SoundQueueAdd);
L_A5F489:
    actor_yield(0xA5F489u);
    jsl_call(0xA5F48Bu, abi_AnimStep);
    MEM8(dec, ea_dp(K4C_COUNT));
    if (!IF_Z)
        goto L_A5F47D;
L_A5F493:
    abi_ActorEndPlaced();
    return;
L_A5F497:
    lda8_v(0xA0);
    sta8(ea_dp(K4C_WAIT));
    lda8_v(0x8C);
    sta8(ea_dp(K4C_WAIT + 1));
L_A5F49F:
    actor_yield(0xA5F49Fu);
    MEM8(dec, ea_dp(K4C_WAIT));
    if (!IF_Z)
        goto L_A5F49F;
    MEM8(dec, ea_dp(K4C_WAIT + 1));
    if (!IF_Z)
        goto L_A5F49F;
    stz8(ea_abs(LEVEL));
L_A5F4AC:
    lda8_v(K4C_FRAMES);
    sta8(ea_dp(K4C_COUNT));
    lda8_v(0x00);
    jsl_call(0xA5F4B2u, abi_AnimStart);
L_A5F4B6:
    lda8(ea_dp(K4C_COUNT));
    cmp8_v(K4C_SOUND_AT);
    if (!IF_Z)
        goto L_A5F4C2;
    lda8_v(K4C_SOUND);
    jsl_call(0xA5F4BEu, abi_SoundQueueAdd);
L_A5F4C2:
    actor_yield(0xA5F4C2u);
    jsl_call(0xA5F4C4u, abi_AnimStep);
    MEM8(dec, ea_dp(K4C_COUNT));
    if (!IF_Z)
        goto L_A5F4B6;
    lda8_v(0x20);
    actor_wait(0xA5F4CEu);
    goto L_A5F4AC;
}
