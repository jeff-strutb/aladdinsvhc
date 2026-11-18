/*
 * kind21.c: actor kind $21's start: by its $07, one of two figures that
 * play out a little scene, or a third that stands facing left.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorMoveX(void);

#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)  /* the player's x, whole pixels (word) */
#define V_146B 0x146Bu          /* part 0 clears it as it starts and counts it up halfway */
#define KIND21_28 0x28u         /* $FF */
#define KIND21_16 0x16u         /* word: $D06A */
#define KIND21_29 0x29u         /* word: $8909 */
#define FRAMES 0x40u            /* frames left of a step */
#define PARTS 0xA5F333u         /* the parts' code, by $07 (words, this bank) */

/* LDA #n, JSL AnimStart at `site`: its return address on the stack while
   it runs (AnimStart pushes and pulls there: PHB, PHA, PLB), as the
   original's. */
static void pose(uint32_t site, uint8_t n)
{
    a8_load(n);
    jsl_frame(site);
    abi_AnimStart();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* FRAMES = n, then each frame (BRK at brk_at) the step if any, until it counts down. */
static void for_frames(uint8_t n, uint32_t brk_at, void (*step)(void))
{
    a8_load(n);
    wr8(dpa(FRAMES), n);
    do {
        actor_yield(brk_at);
        if (step)
            step();
    } while (dec8_at(dpa(FRAMES)));
}

static void part0(void)
{
    wr8(dba(V_146B), 0);
    pose(0xA5F33Eu, 0x01);
    a8_load(0x40);
    wr8(dpa(FRAMES), 0x40);
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    wr8(dpa(ACTOR_SPEED_X), 0);
    a8_load(0x01);
    wr8(dpa(ACTOR_SPEED_X + 1), 0x01);
    wr8(dpa(ACTOR_SPEED_X + 2), 0);
    do
        actor_yield(0xA5F354u);
    while (dec8_at(dpa(FRAMES)));
    for_frames(0x80, 0xA5F35Eu, abi_ActorMoveX);
    pose(0xA5F36Au, 0x03);
    for_frames(0x80, 0xA5F372u, 0);
    (void)inc8_at(dba(V_146B));
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    pose(0xA5F381u, 0x1E);
    for_frames(0x10, 0xA5F389u, 0);
    pose(0xA5F391u, 0x02);
    a8_load(0x03);
    wr8(dpa(ACTOR_SPEED_X + 1), 0x03);
    for_frames(0x80, 0xA5F39Du, abi_ActorMoveX);
    for (;;)
        actor_yield(0xA5F3A7u);
}

static void part1(void)
{
    uint8_t x;

    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    for (;;) {
        pose(0xA5F3B1u, 0x0F);
        for (;;) {
            x = rd8(dba((uint16_t)PLAYER_X_AT));
            a8_load(x);
            if (x & 0x80)
                break;
            a8_load(0x08);
            actor_wait(0xA5F3BCu);
        }
        pose(0xA5F3C2u, 0x04);
        for (;;) {
            x = rd8(dba((uint16_t)PLAYER_X_AT));
            a8_load(x);
            if (!(x & 0x80))
                break;
            a8_load(0x08);
            actor_wait(0xA5F3CDu);
        }
    }
}

static void part2(void)
{
    pose(0xA5F3D3u, 0x00);
    wr8(dpa(FRAMES), 0);
    a8_load(0x01);
    wr8(dpa(ACTOR_FACING), 0x01);
    wr8(dpa(ACTOR_FLIP), 0x01);
    for (;;)
        actor_yield(0xA5F3DFu);
}

/*
 * @routine A5:F309 Kind21Start
 * @actor
 * WHAT IT DOES: Actor kind $21: by its $07, part 0 waits 64 frames, walks
 * left for 128, poses (3) for 128, counts V_146B up, turns right (pose
 * $1E) for 16 frames, then walks right faster (2) for 128 and stays; part 1
 * watches Aladdin's x: pose $0F while its bit 7 is clear, pose 4 while it
 * is set, checking every 8 frames; part 2 takes pose 0 facing left and
 * stays.
 * HOW: actor_08 bit 1 and actor_09 bits $90 set; KIND21_28 = $FF, its
 * sprite attributes $0220, KIND21_16 = $D06A, KIND21_29 = $8909; then by
 * $07 through .parts. Part 0: V_146B 0, AnimStart 1, facing and flip 1,
 * speed bytes 0, 1, 0 ($19-$1B), frame counts in FRAMES with BRK each frame and
 * ActorMoveX while walking; the second walk with $1A = 3. Part 1: facing
 * and flip 0; AnimStart $0F, COP 8 until bit 7 of the player's x's low
 * byte is set; AnimStart 4, COP 8 until it is clear; again. Part 2:
 * AnimStart 0, FRAMES 0, facing and flip 1, BRK each frame.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $21
 * in state $0C (the table at $83:8CAB, through the stub at $83:9226).
 * Exit: never returns.
 */
void abi_Kind21Start(void)
{
    uint8_t part, i;

    wr8(dpa(ACTOR_08), (uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_09), (uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    a8_load(0xFF);
    wr8(dpa(KIND21_28), 0xFF);
    op_rep(0x20);
    dp16_set(ACTOR_LOOK, 0x0220);
    dp16_set(KIND21_16, 0xD06A);
    a16_load(0x8909);
    dp16_set(KIND21_29, 0x8909);
    op_sep(0x20);
    part = rd8(dpa(ACTOR_07));
    i = (uint8_t)(part << 1);
    a8_load(i);
    flag_set(P_C, part & 0x80);
    x8_load(i);
    if (i == 0)
        part0();
    else if (i == 2)
        part1();
    else if (i == 4)
        part2();
    else
        jump_routine(0xA50000u | rd16((PARTS + i) & 0xFFFFFFu));
}
