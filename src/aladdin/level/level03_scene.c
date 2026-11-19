/*
 * level03_scene.c: level 3's task, which its screen set-up starts
 * (Level03ScreenSetup, $81:8908): it waits for Aladdin at a spot, takes him
 * over and moves the camera's limits through the scene, as level $0D's and
 * $10's, with a copier check and a checksum of its own bytes. Written
 * instruction for instruction from the assembly (level03_scene.asm), every
 * JSL with its return frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "wram_video.h"

void abi_ActorsFreeAll(void);
void abi_SongPlay(void);
void abi_SoundQueueF6(void);
void abi_TaskExit(void);
void abi_TaskWait(void);

#define CUE_BD 0x00BDu          /* the scene's cues, set by its actors */
#define CUE_BE 0x00BEu
#define CUE_BF 0x00BFu
#define SCENE_SUM 0x00C0u       /* the checksum of the task's bytes, less $EC (0 when untouched) */
#define LIMIT_1A40 0x1A40u      /* the camera's limits the task moves on */
#define LIMIT_1A42 0x1A42u
#define V_083D 0x083Du
#define SCENE_ON 0x03FAu        /* counted up as the scene ends */
#define V_141C 0x141Cu
#define HEALTH 0x0367u          /* the player's health */
#define PLAYER_X (PLAYER + ACTOR_X + 1)  /* Aladdin's x pixels (the low byte tested) */
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)  /* his y pixels */
#define SCROLL_X_HIGH (LAYER0 + LAYER_SCROLL_X + 1)  /* the play field's scroll x, high byte */
#define COPIER_COUNT 0x700F10u  /* a byte a copier's cartridge RAM keeps (a retail cartridge has none) */
#define SCENE_CODE 0xCB3Bu      /* the player's code word for the scene */
#define COMPANION_CODE 0xE554u  /* the escort actor's code word for the scene */
#define SUM_FROM 0x80A167u      /* the first of the bytes summed (its copier check) */
#define SUM_BYTES 15

/* A JSL at `site`: its return address on the stack while the callee runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* LDA #frames, JSL TaskWait at `site`: the task waits that many frames
   (TaskWait returns through the frame itself). */
static void wait_frames(uint32_t site, uint8_t frames)
{
    lda8_v(frames);
    jsl_frame(site);
    abi_TaskWait();
}

/*
 * @routine 80:A12D Level03Scene
 * WHAT IT DOES: Level 3's scene. LIMIT_1A42's high byte 4, its low 0, the
 * cues and SCENE_SUM 0; each frame until the screen is past $04xx and
 * Aladdin, alive, stands at y $B0 with x $F0 or more (low bytes): sound
 * command $F6 2; then unless a copier's RAM keeps its count, every actor
 * freed, his and the companion's code words set, his $07 8, theirs 4, the
 * limit 5 and $083D 4. Each frame until the screen is past $05xx; the
 * limit's high byte at $1A41 5; 48 frames; song $21; SCENE_SUM = the sum of
 * its own 15 bytes from 80:A167, less $EC. Each frame until CUE_BD; sound
 * command $F6 2; 128 frames; each frame until x $F0 or more, V_141C 0 and
 * y $B0 or more: SCENE_ON up, the limit's high byte $7F, his code word
 * again, his $07 $0D; each frame until CUE_BE; song $1E; TaskExit.
 * HOW: As above; the copier check: COPIER_COUNT up (long) and compared:
 * equal (the RAM kept it) skips to the second wait. The sum: X 0 to 14,
 * from A = 0, CLC and ADC each byte (long, X); SEC, SBC #$EC.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
 * task's entry (TaskCreate slot 3 at $81:8917). Exit: never returns
 * (TaskExit).
 */
void abi_Level03Scene(void)
{
    lda8_v(0x04);
    sta8(ea_abs(LIMIT_1A42 + 1));
    stz8(ea_abs(LIMIT_1A42));
    stz8(ea_abs(CUE_BD));
    stz8(ea_abs(CUE_BE));
    stz8(ea_abs(CUE_BF));
    stz8(ea_abs(SCENE_SUM));
    for (;;) {
        wait_frames(0x80A143u, 0x01);
        lda8(ea_abs(SCROLL_X_HIGH));
        cmp8_v(0x04);
        if (!IF_C)
            continue;
        lda8(ea_abs(PLAYER_Y));
        cmp8_v(0xB0);
        if (!IF_Z)
            continue;
        lda8(ea_abs(PLAYER_X));
        cmp8_v(0xF0);
        if (!IF_C)
            continue;
        lda8(ea_abs(HEALTH));
        if (!IF_Z)
            break;
    }
    lda8_v(0x02);
    jsl_call(0x80A163u, abi_SoundQueueF6);
    lda8(ea_long(COPIER_COUNT));
    ACC8(inc);
    sta8(ea_long(COPIER_COUNT));
    cmp8_v(rd8(ea_long(COPIER_COUNT)));
    if (!IF_Z) {
        jsl_call(0x80A176u, abi_ActorsFreeAll);
        op_rep(P_M);
        lda16_v(SCENE_CODE);
        sta16(ea_abs(PLAYER + ACTOR_03));
        lda16_v(COMPANION_CODE);
        sta16(ea_abs(ACTOR_ESCORT + ACTOR_03));
        op_sep(P_M);
        lda8_v(0x08);
        sta8(ea_abs(PLAYER + ACTOR_07));
        lda8_v(0x04);
        sta8(ea_abs(ACTOR_ESCORT + ACTOR_07));
        lda8_v(0x05);
        sta8(ea_abs(LIMIT_1A42 + 1));
        lda8_v(0x04);
        sta8(ea_abs(V_083D));
    }
    do {
        wait_frames(0x80A1A0u, 0x01);
        lda8(ea_abs(SCROLL_X_HIGH));
        cmp8_v(0x05);
    } while (!IF_C);
    lda8_v(0x05);
    sta8(ea_abs(LIMIT_1A40 + 1));
    wait_frames(0x80A1B2u, 0x30);
    lda8_v(0x21);
    jsl_call(0x80A1B8u, abi_SongPlay);
    ldx8_v(0x00);
    ldy8_v(SUM_BYTES);
    lda8_v(0x00);
    do {
        clc();
        adc8_v(rd8(ea_longx(SUM_FROM)));
        inx8();
        dey8();
    } while (!IF_Z);
    sec();
    sbc8_v(0xEC);
    sta8(ea_abs(SCENE_SUM));
    do {
        wait_frames(0x80A1D3u, 0x01);
        lda8(ea_abs(CUE_BD));
    } while (IF_Z);
    lda8_v(0x02);
    jsl_call(0x80A1DEu, abi_SoundQueueF6);
    wait_frames(0x80A1E4u, 0x80);
    for (;;) {
        wait_frames(0x80A1EAu, 0x01);
        lda8(ea_abs(PLAYER_X));
        cmp8_v(0xF0);
        if (!IF_C)
            continue;
        lda8(ea_abs(V_141C));
        if (!IF_Z)
            continue;
        lda8(ea_abs(PLAYER_Y));
        cmp8_v(0xB0);
        if (IF_C)
            break;
    }
    MEM8(inc, ea_abs(SCENE_ON));
    lda8_v(0x7F);
    sta8(ea_abs(LIMIT_1A42 + 1));
    op_rep(P_M);
    lda16_v(SCENE_CODE);
    sta16(ea_abs(PLAYER + ACTOR_03));
    op_sep(P_M);
    lda8_v(0x0D);
    sta8(ea_abs(PLAYER + ACTOR_07));
    do {
        wait_frames(0x80A21Au, 0x01);
        lda8(ea_abs(CUE_BE));
    } while (IF_Z);
    lda8_v(0x1E);
    jsl_call(0x80A225u, abi_SongPlay);
    abi_TaskExit();
}
