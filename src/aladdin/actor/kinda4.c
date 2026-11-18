/*
 * kinda4.c: actor kind $A4, figures of a scene with the level-end screen's
 * cue ($00AE, as kind $A8's) and its step ($00B5): by its $07 one blinks
 * while the cue is set, one drops and shakes the scene's y ($00B0), one
 * rises, flies and leaves a child, and two only pose once and end. Its
 * wait is KindA4WaitWork (kind_waits_84.c). The kind that spawns it, $59
 * (83:DD10), is placed on no saved stage and made by no spawn found in the
 * ROM, so scripts make kind $A4 itself.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_ActorSpeedYSetGravity(void);
void abi_ActorFallAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorFly(void);
void abi_ActorSpawnSameLook(void);
void abi_ActorEndPlaced(void);

#define KA4_COUNT 0x40u         /* way 2: frames of its flight */
#define KA4_LEFT 0x41u          /* frames left of a step; way 1's shake: the table entry */
#define CUE_AE 0x00AEu          /* the scene's cue (as kind $A8's): nonzero, it shows */
#define SCENE_STEP 0x00B5u      /* the scene's step: negative or 2 lets ways 1 and 2 go on */
#define SCENE_Y 0x00B0u         /* the scene's y that way 1 shakes (word) */
#define KA4_SHAKES 0xD972u      /* (data bank $80) way 1's shake, entry 8 down to 0 (words) */
#define KA4_WAYS 0xAA43u        /* in bank $84: KindA4Start's ways (words), by X */
#define KA4_WAY4 0x10           /* a $07 of $10 or more takes way 4 (its low 4 bits kept) */

/* $07 0 ($84:AA4D): shows while CUE_AE is set, steady 127 frames, then
   blinking every 32; for good. */
static void pulse(void)
{
    lda8_v(0x00);
    abi_AnimStart();
    for (;;) {                                  /* $84:AA53 */
        lda8_v(0x08);
        trb8(ea_dp(ACTOR_08));
        for (;;) {
            actor_yield(0x84AA57u);
            lda8(ea_abs(CUE_AE));
            if (!IF_Z)
                break;
        }
        lda8_v(0x7F);
        sta8(ea_dp(KA4_LEFT));
        do {
            actor_yield(0x84AA64u);
            lda8(ea_abs(CUE_AE));
            if (IF_Z)
                goto off;
            MEM8(dec, ea_dp(KA4_LEFT));
        } while (!IF_Z);
        for (;;) {
            lda8_v(0x20);
            sta8(ea_dp(KA4_LEFT));
            do {
                actor_yield(0x84AA73u);
                lda8(ea_abs(CUE_AE));
                if (IF_Z)
                    goto off;
                MEM8(dec, ea_dp(KA4_LEFT));
            } while (!IF_Z);
            lda8(ea_dp(ACTOR_08));
            eor8_v(0x08);
            sta8(ea_dp(ACTOR_08));
        }
    off:;
    }
}

/* Each frame (BRK at brk_at) until SCENE_STEP is negative or 2. */
static void step_wait(uint32_t brk_at)
{
    for (;;) {
        actor_yield(brk_at);
        lda8(ea_abs(SCENE_STEP));
        if (IF_N)
            return;
        cmp8_v(0x02);
        if (IF_Z)
            return;
    }
}

/* $07 2 ($84:AA86). */
static void drop(void)
{
    step_wait(0x84AA86u);
    lda8_v(0x43);
    actor_wait(0x84AA93u);
    lda8_v(0x41);
    abi_SoundQueueAdd();
    lda8_v(0x01);
    abi_AnimStart();
    lda8_v(0xFF);
    ldy8_v(0x8A);
    abi_ActorSpeedYSetGravity();
    lda8_v(0x20);
    sta8(ea_dp(KA4_LEFT));
    do {
        actor_yield(0x84AAADu);
        abi_ActorFallAgainst();
        MEM8(dec, ea_dp(KA4_LEFT));
    } while (!IF_Z);
    lda8_v(0x0A);
    actor_wait(0x84AAB9u);
    lda8_v(0x05);
    sta8(ea_dp(KA4_LEFT));
    do {
        actor_yield(0x84AABFu);
        op_rep(0x21);
        lda16(ea_abs(SCENE_Y));
        adc16_v(0x0010);
        sta16(ea_abs(SCENE_Y));
        op_sep(0x20);
        MEM8(dec, ea_dp(KA4_LEFT));
    } while (!IF_Z);
    lda8_v(0x08);
    sta8(ea_dp(KA4_LEFT));
    do {
        actor_yield(0x84AAD6u);
        lda8(ea_dp(KA4_LEFT));
        ACC8(asl);
        tax8();
        op_rep(0x21);
        lda16(ea_abs(SCENE_Y));
        adc16_v(rd16(ea_absx(KA4_SHAKES)));
        sta16(ea_abs(SCENE_Y));
        op_sep(0x20);
        lda8(ea_dp(KA4_LEFT));
        cmp8_v(0x05);
        if (IF_Z) {
            lda8_v(0x3A);
            abi_SoundQueueAdd();
        }
        MEM8(dec, ea_dp(KA4_LEFT));
    } while (!IF_N);
    lda8_v(0x3A);
    abi_SoundQueueAdd();
    abi_ActorEndPlaced();
}

/* $07 4 ($84:AB03). */
static void rise(void)
{
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_Y + 2));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_X + 1));
    lda8_v(0x8A);
    sta8(ea_dp(ACTOR_Y + 1));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_08));
    ldy8_v(0x74);
    abi_ActorMotionPreset();
    stz8(ea_dp(ACTOR_SPAWN_AT));
    step_wait(0x84AB1Bu);
    lda8_v(0x98);
    actor_wait(0x84AB28u);
    stz8(ea_dp(ACTOR_Y + 2));
    lda8_v(0x02);
    abi_AnimStart();
    lda8_v(0x43);
    abi_SoundQueueAdd();
    lda8_v(0x14);
    actor_wait(0x84AB3Au);
    lda8_v(0x03);
    abi_AnimStart();
    lda8(ea_dp(ACTOR_Y + 1));
    sec();
    sbc8_v(0x04);
    sta8(ea_dp(ACTOR_Y + 1));
    lda8_v(0x42);
    abi_SoundQueueAdd();
    lda8_v(0x11);
    sta8(ea_dp(KA4_COUNT));
    do {
        abi_ActorFly();
        actor_yield(0x84AB57u);
        MEM8(dec, ea_dp(KA4_COUNT));
    } while (!IF_Z);
    lda8_v(KA4_WAY4);
    sta8(ea_dp(ACTOR_07));
    lda8(ea_dp(ACTOR_06));
    abi_ActorSpawnSameLook();
    abi_ActorEndPlaced();
}

/* $07 6 ($84:AB6B). */
static void pose(void)
{
    stz8(ea_dp(ACTOR_LOOK));
    lda8_v(0x05);
    abi_AnimStart();
    lda8_v(0x0E);
    actor_wait(0x84AB75u);
    abi_ActorEndPlaced();
}

/* $07 $10 up ($84:AB7B). */
static void pose_once(void)
{
    lda8_v(0x2D);
    abi_SoundQueueAdd();
    lda8_v(0x04);
    abi_AnimStart();
    lda8_v(0x34);
    sta8(ea_dp(ACTOR_X + 1));
    lda8_v(0x16);
    sta8(ea_dp(ACTOR_Y + 1));
    lda8_v(0x30);
    actor_wait(0x84AB91u);
    abi_ActorEndPlaced();
}

/*
 * @routine 84:AA24 KindA4Start
 * @actor
 * WHAT IT DOES: Actor kind $A4, a figure of a scene, by its $07 (0, 2, 4, 6;
 * $10 or more for the last): 0 shows while CUE_AE is set, steady for 127
 * frames, then blinking every 32, for good; 2 waits for SCENE_STEP,
 * moves 32 frames with its pull reversed and shakes SCENE_Y, then ends;
 * 4 is put at y $018A (x low byte $80) with a preset motion, and once
 * SCENE_STEP comes poses, flies for 17 frames and leaves a child of its
 * kind ($07 $10), then ends; 6 and $10 up pose once and end.
 * HOW: $08 bits 0 (TSB) and $09 bits $90 set; the palette byte ($2C) 6.
 * $07 $10 or more: $07 AND $0F, X = 8; else X = $07. JMP through the table
 * at $84:AA43 by X.
 * 0: AnimStart 0. Again: $08 bit 3 cleared; each frame (BRK) until
 * CUE_AE; KA4_LEFT $7F frames (BRK), back to again if CUE_AE clears;
 * then for good: $20 frames (BRK, the same test), $08 bit 3 flipped.
 * 2: each frame (BRK) until SCENE_STEP is negative or 2; COP $43;
 * SoundQueueAdd $41; AnimStart 1; ActorSpeedYSetGravity with A = $FF,
 * Y = $8A; $20 frames (BRK) of ActorFallAgainst; COP $0A; 5 frames (BRK
 * first): SCENE_Y + $10 (REP #$21: carry clear); then KA4_LEFT 8 down to
 * 0, each frame (BRK first): SCENE_Y + KA4_SHAKES entry KA4_LEFT, and
 * SoundQueueAdd $3A at entry 5; SoundQueueAdd $3A; ActorEndPlaced.
 * 4: y high byte 1, x low byte $80, y low byte $8A, $08 = 1;
 * ActorMotionPreset $74; spawn place 0; each frame (BRK) until
 * SCENE_STEP is negative or 2; COP $98; y high byte 0; AnimStart 2;
 * SoundQueueAdd $43; COP $14; AnimStart 3; y low byte - 4; SoundQueueAdd
 * $42; KA4_COUNT $11 frames of ActorFly (then BRK); $07 = $10;
 * ActorSpawnSameLook with A = its $06; ActorEndPlaced.
 * 6: look's low byte ($2B) 0; AnimStart 5; COP $0E; ActorEndPlaced.
 * $10 up: SoundQueueAdd $2D; AnimStart 4; x low byte $34, y low byte
 * $16; COP $30; ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $A4 in state $0C (the table at $83:8DAB,
 * through the stub at $83:942C). Exit: never returns.
 */
void abi_KindA4Start(void)
{
    lda8_v(0x00);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x06);
    sta8(ea_dp(ACTOR_LOOK + 1));
    lda8(ea_dp(ACTOR_07));
    cmp8_v(KA4_WAY4);
    if (IF_C) {
        and8_v(0x0F);
        sta8(ea_dp(ACTOR_07));
        ldx8_v(0x08);
    } else {
        ldx8(ea_dp(ACTOR_07));
    }
    switch (cpu.x) {
    case 0x00: pulse(); break;
    case 0x02: drop(); break;
    case 0x04: rise(); break;
    case 0x06: pose(); break;
    case 0x08: pose_once(); break;
    default:
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(KA4_WAYS + cpu.x)));
        break;
    }
}
