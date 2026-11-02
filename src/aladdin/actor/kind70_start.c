/*
 * kind70_start.c: the start of actor kind $70 (and of kind $6E in state
 * $4C): a scripted actor that either rises, flies off and carries the
 * escort actor once it meets it, or waits to be hit and then cues the
 * scene on. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_AnimStart(void);

#define KIND70_CARRYING 0x40u  /* frames of the first flight; then nonzero while it carries the escort */
#define SCENE_CUE 0x146Cu  /* counted up by scripted actors as the scene goes on */
#define SCENE_CUE_C2 0x00C2u  /* counted up when the hit one has waited; another actor waits for it */

/*
 * @routine 83:F296 Kind70Start
 * @actor
 * WHAT IT DOES: A scripted actor's start. $07 zero: it rises (animations
 * 0, 1, 2 with waits), flies off for $28 frames, then flies on under
 * gravity until out of range; when the escort actor, not rising, comes
 * within 8 pixels across and 16 down of it, it counts SCENE_CUE up and
 * carries the escort 30 pixels to its left from then on. $07 nonzero: it
 * faces left and waits until it can be hit, counts SCENE_CUE up, waits $80
 * frames, counts SCENE_CUE_C2 up and plays animations 4 and 5, then stays.
 * HOW: Bits $90 set in $09. First way: animation 0, COP $20; animation 1,
 * COP $17; position +16 x, -13 y; animation 2, ActorMotionPreset $4F; $28
 * frames of ActorFly; gravity $18; then each frame ActorFlyAgainst and
 * ActorEndOutOfRange (box 0); carrying: the escort's x = its x - 30, y =
 * its y; not: the escort's speed y not negative, (x - 24) - its x below 8
 * and (y + 16) - its y below 16, then SCENE_CUE and $40 up. Second way:
 * facing and flip 1, animation 3; BRK until bit 6 of $09; SCENE_CUE up,
 * COP $80; SCENE_CUE_C2 up, animation 4, COP $20, animation 5, then BRK
 * for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $70 in state $0C and kind $6E in state
 * $4C. Exit: never returns: its first way ends in ActorEndOutOfRange.
 */
void abi_Kind70Start(void)
{
    lda8_v(0x90);  /* 83F296 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83F298 tsb */
    lda8(ea_dp(ACTOR_07));  /* 83F29A lda */
    if (IF_Z) goto L_83F2A1;  /* 83F29C beq */
    goto L_83F339;  /* 83F29E jmp */
L_83F2A1:
    stz8(ea_dp(KIND70_CARRYING));  /* 83F2A1 stz */
    lda8_v(0x00);  /* 83F2A3 lda */
    abi_AnimStart();  /* 83F2A5 jsl */
    lda8_v(0x20);  /* 83F2A9 lda */
    actor_wait(0x83F2ABu);  /* 83F2AB cop */
    lda8_v(0x01);  /* 83F2AD lda */
    abi_AnimStart();  /* 83F2AF jsl */
    lda8_v(0x17);  /* 83F2B3 lda */
    actor_wait(0x83F2B5u);  /* 83F2B5 cop */
    op_rep(0x21);  /* 83F2B7 rep */
    lda16(ea_dp(ACTOR_X + 1));  /* 83F2B9 lda */
    adc16_v(0x0010);  /* 83F2BB adc */
    sta16(ea_dp(ACTOR_X + 1));  /* 83F2BE sta */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83F2C0 lda */
    sec();  /* 83F2C2 sec */
    sbc16_v(0x000D);  /* 83F2C3 sbc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83F2C6 sta */
    op_sep(0x20);  /* 83F2C8 sep */
    lda8_v(0x02);  /* 83F2CA lda */
    abi_AnimStart();  /* 83F2CC jsl */
    ldy8_v(0x4F);  /* 83F2D0 ldy */
    abi_ActorMotionPreset();  /* 83F2D2 jsl */
    lda8_v(0x28);  /* 83F2D6 lda */
    sta8(ea_dp(KIND70_CARRYING));  /* 83F2D8 sta */
L_83F2DA:
    actor_yield(0x83F2DAu);  /* 83F2DA brk */
    abi_ActorFly();  /* 83F2DC jsl */
    MEM8(dec, ea_dp(KIND70_CARRYING));  /* 83F2E0 dec */
    if (!IF_Z) goto L_83F2DA;  /* 83F2E2 bne */
    lda8_v(0x18);  /* 83F2E4 lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* 83F2E6 sta */
L_83F2E8:
    actor_yield(0x83F2E8u);  /* 83F2E8 brk */
    abi_ActorFlyAgainst();  /* 83F2EA jsl */
    ldx8_v(0x00);  /* 83F2EE ldx */
    push16(0xF2F2); abi_ActorEndOutOfRange(); cpu.s = (uint16_t)(cpu.s + 2);  /* 83F2F0 jsr */
    lda8(ea_dp(KIND70_CARRYING));  /* 83F2F3 lda */
    if (IF_Z) goto L_83F30B;  /* 83F2F5 beq */
    op_rep(0x20);  /* 83F2F7 rep */
    clc();  /* 83F2F9 clc */
    lda16(ea_dp(ACTOR_X + 1));  /* 83F2FA lda */
    adc16_v(0xFFE2);  /* 83F2FC adc */
    sta16(ea_abs(ACTOR_ESCORT + ACTOR_X + 1));  /* 83F2FF sta */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83F302 lda */
    sta16(ea_abs(ACTOR_ESCORT + ACTOR_Y + 1));  /* 83F304 sta */
L_83F307:
    op_sep(0x20);  /* 83F307 sep */
    goto L_83F2E8;  /* 83F309 bra */
L_83F30B:
    lda8(ea_abs(ACTOR_ESCORT + ACTOR_SPEED_Y + 2));  /* 83F30B lda */
    if (IF_N) goto L_83F2E8;  /* 83F30E bmi */
    op_rep(0x20);  /* 83F310 rep */
    clc();  /* 83F312 clc */
    lda16(ea_dp(ACTOR_X + 1));  /* 83F313 lda */
    adc16_v(0xFFE8);  /* 83F315 adc */
    sec();  /* 83F318 sec */
    sbc16_v(rd16(ea_abs(ACTOR_ESCORT + ACTOR_X + 1)));  /* 83F319 sbc */
    cmp16_v(0x0008);  /* 83F31C cmp */
    if (IF_C) goto L_83F307;  /* 83F31F bcs */
    clc();  /* 83F321 clc */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83F322 lda */
    adc16_v(0x0010);  /* 83F324 adc */
    sec();  /* 83F327 sec */
    sbc16_v(rd16(ea_abs(ACTOR_ESCORT + ACTOR_Y + 1)));  /* 83F328 sbc */
    cmp16_v(0x0010);  /* 83F32B cmp */
    if (IF_C) goto L_83F307;  /* 83F32E bcs */
    op_sep(0x20);  /* 83F330 sep */
    MEM8(inc, ea_abs(SCENE_CUE));  /* 83F332 inc */
    MEM8(inc, ea_dp(KIND70_CARRYING));  /* 83F335 inc */
    goto L_83F2E8;  /* 83F337 bra */
L_83F339:
    lda8_v(0x01);  /* 83F339 lda */
    sta8(ea_dp(ACTOR_FACING));  /* 83F33B sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83F33D sta */
    lda8_v(0x03);  /* 83F33F lda */
    abi_AnimStart();  /* 83F341 jsl */
L_83F345:
    actor_yield(0x83F345u);  /* 83F345 brk */
    lda8(ea_dp(ACTOR_09));  /* 83F347 lda */
    bit8_i(0x40);  /* 83F349 bit */
    if (IF_Z) goto L_83F345;  /* 83F34B beq */
    MEM8(inc, ea_abs(SCENE_CUE));  /* 83F34D inc */
    lda8_v(0x80);  /* 83F350 lda */
    actor_wait(0x83F352u);  /* 83F352 cop */
    MEM8(inc, ea_abs(SCENE_CUE_C2));  /* 83F354 inc */
    lda8_v(0x04);  /* 83F357 lda */
    abi_AnimStart();  /* 83F359 jsl */
    lda8_v(0x20);  /* 83F35D lda */
    actor_wait(0x83F35Fu);  /* 83F35F cop */
    lda8_v(0x05);  /* 83F361 lda */
    abi_AnimStart();  /* 83F363 jsl */
L_83F367:
    actor_yield(0x83F367u);  /* 83F367 brk */
    goto L_83F367;  /* 83F369 bra */
}
