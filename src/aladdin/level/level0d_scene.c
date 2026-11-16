/*
 * level0d_scene.c: level $0D's task, which its screen set-up starts
 * ($81:8BA7, through the table by level at $81:8802): it keeps Aladdin
 * from rising too high until he reaches a spot, then takes him over and
 * moves the camera's limits on through a short scene.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"
#include "wram.h"
#include "wram_state.h"
#include "wram_video.h"

void abi_SongPlay(void);
void abi_SoundQueueF6(void);
void abi_TaskExit(void);
void abi_TaskWait(void);

#define CUE_BD 0x00BDu          /* the scene's cue, set by an actor */
#define CUE_BE 0x00BEu          /* set by the task once the camera has moved on */
#define LIMIT_1A40 0x1A40u      /* two words the task sets as the scene goes on ($0A00, then $0900 and $7FFF at $1A42) */
#define LIMIT_1A42 0x1A42u
#define V_083D 0x083Du
#define SCROLL_X_HIGH (LAYER0 + LAYER_SCROLL_X + 1)  /* the play field's scroll x, high byte */
#define PLAYER_X (PLAYER + ACTOR_X + 1)              /* Aladdin's x, whole pixels (word) */
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)              /* and y (word) */

/* The task waits `frames` frames, its JSL returning at `site`. */
static void wait_frames(uint32_t site, uint8_t frames)
{
    a8_load(frames);
    jsl_frame(site);
    abi_TaskWait();
}

/*
 * @routine 80:A501 Level0DScene
 * WHAT IT DOES: Level $0D's scene. Each frame Aladdin is kept from going
 * above y $18 (when his y's low byte is $18 or less it is set to $0018)
 * until he, alive, is at x $09F0 or more and y $78 or more; then he is given
 * scripted code, sound command $F6 2 is queued, the camera moves on until
 * the scroll x reaches $0Axx, song $25 plays, and on cue $BD the task
 * waits 240 frames, queues $F6 2 again, waits 192 frames, opens the
 * limits ($1A42 = $7FFF) and ends.
 * HOW: X 16-bit; LIMIT_1A42 = $0900, CUE_BD and CUE_BE 0. Each frame
 * (TaskWait 1): PLAYER_Y's low byte $18 or less: PLAYER_Y = $0018; until
 * PLAYER_X >= $09F0, PLAYER_Y >= $0078 and HEALTH nonzero. X 8-bit; the
 * player's $03 word $CB3B and $07 5; SoundQueueF6 2; $1A43 $0A; $083D 4.
 * Each frame until SCROLL_X_HIGH >= $0A: CUE_BE up, $083D 0, LIMIT_1A40 =
 * $0A00; 1 frame; SongPlay $25. Each frame until CUE_BD: 240 frames,
 * SoundQueueF6 2, 192 frames; LIMIT_1A42 = $7FFF; TaskExit.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; a
 * task's entry (TaskCreate slot $48 at $81:8BA7). Exit: never returns
 * (TaskExit).
 */
void abi_Level0DScene(void)
{
    uint16_t x, y;
    uint8_t v;

    op_rep(0x10);
    a8_load(0x09);
    wr8(dba(LIMIT_1A42 + 1), 0x09);
    wr8(dba(LIMIT_1A42), 0);
    wr8(dba(CUE_BD), 0);
    wr8(dba(CUE_BE), 0);

    /* kept low until he is at the spot, alive */
    for (;;) {
        wait_frames(0x80A513u, 0x01);
        a8_load(0x18);
        v = rd8(dba((uint16_t)PLAYER_Y));
        cmp8_of(0x18, v);
        if (v <= 0x18) {
            wr8(dba((uint16_t)PLAYER_Y), 0x18);
            wr8(dba((uint16_t)(PLAYER_Y + 1)), 0);
        }
        x = rd16(dba((uint16_t)PLAYER_X));
        cpu.x = x;
        cmp16_of(x, 0x09F0);
        if (x < 0x09F0)
            continue;
        y = rd16(dba((uint16_t)PLAYER_Y));
        cpu.x = y;
        cmp16_of(y, 0x0078);
        if (y < 0x0078)
            continue;
        v = rd8(dba((uint16_t)HEALTH));
        a8_load(v);
        if (v)
            break;
    }
    op_sep(0x10);
    op_rep(0x20);
    a16_load(0xCB3B);
    wr16(dba((uint16_t)(PLAYER + ACTOR_03)), 0xCB3B);
    op_sep(0x20);
    a8_load(0x05);
    wr8(dba((uint16_t)(PLAYER + ACTOR_07)), 0x05);
    a8_load(0x02);
    abi_SoundQueueF6();
    a8_load(0x0A);
    wr8(dba(LIMIT_1A42 + 1), 0x0A);
    a8_load(0x04);
    wr8(dba(V_083D), 0x04);

    /* the camera moves on to $0Axx */
    do {
        wait_frames(0x80A55Cu, 0x01);
        v = rd8(dba((uint16_t)SCROLL_X_HIGH));
        a8_load(v);
        cmp8_of(v, 0x0A);
    } while (v < 0x0A);
    (void)inc8_at(dba(CUE_BE));
    wr8(dba(V_083D), 0);
    a8_load(0x0A);
    wr8(dba(LIMIT_1A40 + 1), 0x0A);
    wr8(dba(LIMIT_1A40), 0);
    wait_frames(0x80A577u, 0x01);
    a8_load(0x25);
    abi_SongPlay();

    /* on cue $BD */
    do {
        wait_frames(0x80A583u, 0x01);
        v = rd8(dba(CUE_BD));
        a8_load(v);
    } while (!v);
    wait_frames(0x80A58Eu, 0xF0);
    a8_load(0x02);
    abi_SoundQueueF6();
    wait_frames(0x80A59Au, 0xC0);
    a8_load(0xFF);
    wr8(dba(LIMIT_1A42), 0xFF);
    a8_load(0x7F);
    wr8(dba(LIMIT_1A42 + 1), 0x7F);
    abi_TaskExit();
}
