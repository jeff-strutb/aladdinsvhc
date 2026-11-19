/*
 * jump_pose.c: the poses of Aladdin's jump, stepped once a frame while he
 * is in the air: up, at the top, falling, and a roll that starts over.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_PlayerMoveStart(void);
void abi_SoundQueueAdd(void);

#define POSE_STEP ACTOR_40      /* the step the jump's pose is at (0-8) */
#define POSE_TIMER ACTOR_41     /* frames left in a timed step */
#define TOP_Y ACTOR_42          /* y (pixels, word) where he was as the rise ended */
#define STEPS 0x81D1FCu         /* the steps' code, by POSE_STEP (words, this bank) */

static void move_start(uint8_t move)
{
    a8_load(move);
    abi_PlayerMoveStart();
}

static void step_on(void)
{
    (void)inc8_at(dpa(POSE_STEP));
}

/*
 * @routine 81:D1F5 PlayerJumpPoseStep
 * WHAT IT DOES: Steps the pose of Aladdin's jump by POSE_STEP: step 0
 * starts move 4 for 14 frames; step 1 then starts move $0D and keeps the
 * height reached; step 2 starts move $0E once he is lower than that; step
 * 3 holds. Step 6 starts move $11 and goes back to step 1 for $12 frames;
 * step 7 starts move $0D after them; step 8 starts move $0E once he falls
 * and goes on at step 3.
 * HOW: Through .steps by POSE_STEP. 0: PlayerMoveStart 4, step on,
 * POSE_TIMER $0E. 1: POSE_TIMER down; at 0 PlayerMoveStart $0D, y pixels
 * to TOP_Y, step on. 2: (16-bit A) TOP_Y not negative and not below y:
 * nothing; else PlayerMoveStart $0E, step on. 3: nothing. 4 and 5, which
 * no script runs: SoundQueueAdd 2, PlayerMoveStart 9, POSE_TIMER $10, step
 * on. 6: PlayerMoveStart $11, POSE_STEP 1, POSE_TIMER $12. 7: POSE_TIMER
 * down; at 0 PlayerMoveStart $0D, step on. 8: rising (the vertical speed
 * negative): nothing; else PlayerMoveStart $0E, POSE_STEP 3.
 * Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
 * JSR from the player's code (81:D06E). Exit: M and X 8-bit; A, X and the
 * flags as the step's last instruction left them (X = POSE_STEP * 2
 * where the step calls nothing).
 */
void abi_PlayerJumpPoseStep(void)
{
    uint8_t step = rd8(dpa(POSE_STEP)), i, v;
    uint16_t top, y;

    i = (uint8_t)(step << 1);
    a8_load(i);
    flag_set(P_C, step & 0x80);
    x8_load(i);
    switch (i) {
    case 0:
        move_start(0x04);
        step_on();
        a8_load(0x0E);
        wr8(dpa(POSE_TIMER), 0x0E);
        return;
    case 2:
        if (dec8_at(dpa(POSE_TIMER)))
            return;
        move_start(0x0D);
        v = rd8(dpa(ACTOR_Y + 1));
        a8_load(v);
        wr8(dpa(TOP_Y), v);
        v = rd8(dpa(ACTOR_Y + 2));
        a8_load(v);
        wr8(dpa(TOP_Y + 1), v);
        step_on();
        return;
    case 4:
        op_rep(P_M);
        top = dp16(TOP_Y);
        a16_load(top);
        if (!(top & 0x8000)) {
            y = dp16(ACTOR_Y + 1);
            cmp16_of(top, y);
            if (top >= y) {
                op_sep(P_M);
                return;
            }
        }
        op_sep(P_M);
        move_start(0x0E);
        step_on();
        op_sep(P_M);
        return;
    case 6:
        return;
    case 8:
    case 10:
        /* no script runs these steps */
        a8_load(0x02);
        jsl_frame(0x81D247u);
        abi_SoundQueueAdd();
        cpu.s = (uint16_t)(cpu.s + 3);
        move_start(0x09);
        a8_load(0x10);
        wr8(dpa(POSE_TIMER), 0x10);
        step_on();
        return;
    case 12:
        move_start(0x11);
        a8_load(0x01);
        wr8(dpa(POSE_STEP), 0x01);
        a8_load(0x12);
        wr8(dpa(POSE_TIMER), 0x12);
        return;
    case 14:
        if (dec8_at(dpa(POSE_TIMER)))
            return;
        move_start(0x0D);
        step_on();
        return;
    case 16:
        v = rd8(dpa(ACTOR_SPEED_Y + 2));
        a8_load(v);
        if (v & 0x80)
            return;
        move_start(0x0E);
        a8_load(0x03);
        wr8(dpa(POSE_STEP), 0x03);
        return;
    default:
        /* past the table: where the bytes after it send the jump */
        jump_routine(0x810000u | rd16((STEPS + i) & 0xFFFFFFu));
        return;
    }
}
