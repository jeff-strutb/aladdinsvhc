/*
 * level10_scene.c: level $10's task, which its screen set-up starts
 * (Level10ScreenSetup, $81:8C14): it waits for Aladdin to reach a spot,
 * then takes him over and moves the camera's limits on through a short
 * scene, as level $0D's (level0d_scene.c). Written instruction for
 * instruction from the assembly (level10_scene.asm), every JSL with its
 * return frame.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorsFreeAll(void);
void abi_SongPlay(void);
void abi_SoundQueueF6(void);
void abi_TaskExit(void);
void abi_TaskWait(void);

#define CUE_BD 0x00BDu          /* the scene's cue, set by an actor */
#define LIMIT_1A40 0x1A40u      /* two words the task sets as the scene goes on */
#define LIMIT_1A42 0x1A42u
#define V_083D 0x083Du
#define V_1A10 0x1A10u
#define PLAYER_X (PLAYER + ACTOR_X + 1)  /* Aladdin's x, whole pixels (word) */
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)  /* and y (word) */
#define SCENE_CODE 0xCB3Bu      /* the player's code word for the scene */
#define SCENE_SONG 0x21

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
 * @routine 80:A5AC Level10Scene
 * WHAT IT DOES: Level $10's scene. Each frame until Aladdin is at x $1190
 * or more with his y's low byte $A8 or more; then sound command $F6 $10
 * is queued, every actor freed, he is given scripted code, the camera's
 * limits move on and song $21 plays; on cue $BD the task queues $F6 $10
 * again, waits 32 frames, sets the limits for the scene's end and ends.
 * HOW: LIMIT_1A42 = $10A0 (byte by byte), CUE_BD 0. Each frame (TaskWait
 * 1): until PLAYER_X >= $1190 (16-bit) and PLAYER_Y's low byte >= $A8.
 * SoundQueueF6 $10; ActorsFreeAll; the player's $03 word SCENE_CODE, his
 * $07 8; LIMIT_1A42 = $11A0; $083D 4, V_1A10 1; 64 frames; SongPlay
 * SCENE_SONG. Each frame until CUE_BD; SoundQueueF6 $10; 32 frames;
 * LIMIT_1A40 = $11A0, LIMIT_1A42's high byte $7F; V_1A10 and $083D 0;
 * TaskExit.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
 * task's entry (TaskCreate slot 3 at $81:8C25). Exit: never returns
 * (TaskExit).
 */
void abi_Level10Scene(void)
{
    lda8_v(0xA0);
    sta8(ea_abs(LIMIT_1A42));
    lda8_v(0x10);
    sta8(ea_abs(LIMIT_1A42 + 1));
    stz8(ea_abs(CUE_BD));
    for (;;) {
        wait_frames(0x80A5BBu, 0x01);
        op_rep(P_M);
        lda16(ea_abs(PLAYER_X));
        cmp16_v(0x1190);
        op_sep(P_M);
        if (!IF_C)
            continue;
        lda8(ea_abs(PLAYER_Y));
        cmp8_v(0xA8);
        if (IF_C)
            break;
    }
    lda8_v(0x10);
    jsl_call(0x80A5D4u, abi_SoundQueueF6);
    jsl_call(0x80A5D8u, abi_ActorsFreeAll);
    op_rep(P_M);
    lda16_v(SCENE_CODE);
    sta16(ea_abs(PLAYER + ACTOR_03));
    op_sep(P_M);
    lda8_v(0x08);
    sta8(ea_abs(PLAYER + ACTOR_07));
    lda8_v(0xA0);
    sta8(ea_abs(LIMIT_1A42));
    lda8_v(0x11);
    sta8(ea_abs(LIMIT_1A42 + 1));
    lda8_v(0x04);
    sta8(ea_abs(V_083D));
    lda8_v(0x01);
    sta8(ea_abs(V_1A10));
    wait_frames(0x80A601u, 0x40);
    lda8_v(SCENE_SONG);
    jsl_call(0x80A607u, abi_SongPlay);
    do {
        wait_frames(0x80A60Du, 0x01);
        lda8(ea_abs(CUE_BD));
    } while (IF_Z);
    lda8_v(0x10);
    jsl_call(0x80A618u, abi_SoundQueueF6);
    wait_frames(0x80A61Eu, 0x20);
    lda8_v(0xA0);
    sta8(ea_abs(LIMIT_1A40));
    lda8_v(0x11);
    sta8(ea_abs(LIMIT_1A40 + 1));
    lda8_v(0x7F);
    sta8(ea_abs(LIMIT_1A42 + 1));
    stz8(ea_abs(V_1A10));
    stz8(ea_abs(V_083D));
    abi_TaskExit();
}
