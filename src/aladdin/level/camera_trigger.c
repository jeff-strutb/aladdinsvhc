/*
 * camera_trigger.c: a level's task (created at $80:8943) that waits for
 * the camera to reach two places: at the first it frees the actors, at
 * the second (the player alive) it sets the player and the escort going
 * again and starts an actor of kind $6B.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_ActorSpawnAt(void);
void abi_ActorsFreeAll(void);
void abi_CameraTriggerTask(void);
void abi_SoundQueueF6(void);
void abi_TaskExit(void);
void abi_TaskWait(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"


#define CUE_BD 0x0000BDu        /* the scene's cues, cleared as it starts */
#define CUE_BE 0x0000BEu
#define CUE_BF 0x0000BFu
#define PLAY_SCREEN_X (LAYER0 + LAYER_SCROLL_X + 1)  /* the play field's screen across and down */
#define PLAY_SCREEN_Y (LAYER0 + LAYER_SCROLL_Y + 1)
#define V_03FA 0x03FAu
#define V_1A10 0x1A10u
#define PLACE_X 0x002Cu         /* where ActorSpawnAt places the actor: x, then y (words) */
#define PLACE_Y 0x002Eu

/* The task's wait for the next frame (A = 1 frame), returning at `site`. */
static void next_frame(uint32_t site)
{
    a8_load(0x01);
    jsl_frame(site);
    abi_TaskWait();
}

/*
 * @routine 80:A22D CameraTriggerTask
 * WHAT IT DOES: Waits for the camera to reach x $0Bxx, y $02xx and frees
 * every actor; then waits for x $0Fxx with the player alive, gives the
 * escort and the player new code, starts an actor of kind $6B at ($0F38,
 * $03B0) and queues sound command $F6 with parameter 2, and ends.
 * HOW: $00BD-$00BF cleared. Each frame (TaskWait 1) until the high bytes
 * of the scroll x and y are $0B and 2: ActorsFreeAll. Each frame until
 * the scroll x's high byte is $0F and the health nonzero: $03FA up; the
 * escort's $03 word $E554 and $07 5; $1A10 up; ActorsFreeAll; the
 * player's $07 9 and $03 word $CB3B; ActorSpawnAt of kind $6B (its $07
 * 1) at the spawn words; SoundQueueF6 2; TaskExit.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
 * task's entry. Exit: never returns (TaskExit).
 */
void abi_CameraTriggerTask(void)
{
    uint8_t across, down, health;

    wr8(dba(CUE_BD), 0);
    wr8(dba(CUE_BE), 0);
    wr8(dba(CUE_BF), 0);
    /* until the play field is at screen $0B across, 2 down: every actor goes */
    for (;;) {
        next_frame(0x80A238u);
        across = rd8(dba(PLAY_SCREEN_X));
        a8_load(across);
        cmp8_of(across, 0x0B);
        if (across != 0x0B)
            continue;
        down = rd8(dba(PLAY_SCREEN_Y));
        a8_load(down);
        cmp8_of(down, 0x02);
        if (down == 0x02)
            break;
    }
    abi_ActorsFreeAll();
    /* until screen $0F across with the player alive: the scene */
    for (;;) {
        next_frame(0x80A250u);
        across = rd8(dba(PLAY_SCREEN_X));
        a8_load(across);
        cmp8_of(across, 0x0F);
        if (across != 0x0F)
            continue;
        health = rd8(dba(HEALTH));
        a8_load(health);
        if (health)
            break;
    }
    wr8(dba(V_03FA), (uint8_t)(rd8(dba(V_03FA)) + 1));
    wr16(dba(ACTOR_ESCORT + ACTOR_03), 0xE554);    /* the escort's code goes on at $E554 */
    a8_load(0x05);
    wr8(dba(ACTOR_ESCORT + ACTOR_07), 0x05);
    wr8(dba(V_1A10), (uint8_t)(rd8(dba(V_1A10)) + 1));
    abi_ActorsFreeAll();
    a8_load(0x09);
    wr8(dba(PLAYER + ACTOR_07), 0x09);              /* the player started as $07 = 9 */
    wr16(dba(PLAYER + ACTOR_03), 0xCB3B);
    wr16(dba(PLACE_X), 0x0F38);                    /* a kind $6B ($07 1) at (3896, 944) */
    wr16(dba(PLACE_Y), 0x03B0);
    cpu.a = 0x03B0;
    a8_load(0x6B);
    op_xba();
    a8_load(0x01);
    abi_ActorSpawnAt();
    op_sep(0x30);
    a8_load(0x02);
    abi_SoundQueueF6();
    abi_TaskExit();
}
