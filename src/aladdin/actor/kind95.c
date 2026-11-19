/*
 * kind95.c: actor kind $95, figures of the story's scenes: by its $07 one
 * of fifteen parts, each a short piece of motion (a fall, a fly-past, a
 * bounce, a hop in place) with its poses and sounds, some starting the
 * story's scene task.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveY(void);
void abi_ActorSpeedUpMove(void);
void abi_ActorSpeedXFrom84(void);
void abi_ActorSpeedYFrom84(void);
void abi_ActorStepYBack(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_TaskCreate(void);
void abi_Kind95Hops(void);

#define K95_FRAMES 0x40u        /* frames left of a step (a word in part 14) */
#define K95_STEP 0x41u          /* part 8: its step; the hops: how many are left */
#define K95_HOP 0x4Cu           /* the hops: which set, then its index by 2 */
#define K95_BACK 0x4Eu          /* the hops: their caller's return address, kept while they wait */
#define SPEED_X_HIGH (ACTOR_SPEED_X + 2)
#define SPEED_Y_HIGH (ACTOR_SPEED_Y + 2)
#define STORY_PAGE 0x00CFu      /* the story's page: nonzero while a page is up */
#define PARTS 0x9C0Au           /* the parts' code, by $07 AND $0F (words, this bank) */
#define PART8_STEPS 0x9DD1u     /* part 8: what follows each step, by K95_STEP (words) */
#define PART8_FRAMES 0xD8F6u    /* part 8: each step's frames, by K95_STEP */
#define HOP_POSE 0xD8B6u        /* the hops, by set: the pose */
#define HOP_DELAY 0xD8BAu       /* the frames before the first */
#define HOP_COUNT 0xD8BEu       /* how many */
#define HOP_UP 0xD8C2u          /* by set * 2: the rise's frames, its speed entry */
#define HOP_DOWN 0xD8CAu        /* by set * 2: the fall's frames, its speed entry */

static void pose(uint8_t n, uint32_t site)
{
    a8_load(n);
    jsl_frame(site); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
}

static void wait(uint8_t frames, uint32_t cop_at)
{
    a8_load(frames);
    actor_wait(cop_at);
}

static void speed_x(uint8_t entry)
{
    y8_load(entry);
    abi_ActorSpeedXFrom84();
}

static void speed_y(uint8_t entry)
{
    y8_load(entry);
    abi_ActorSpeedYFrom84();
}

/* K95_FRAMES = n, then each frame (BRK at brk_at) the step, until it counts down. */
static void for_frames(uint8_t n, uint32_t brk_at, void (*step)(void))
{
    a8_load(n);
    wr8(dpa(K95_FRAMES), n);
    do {
        actor_yield(brk_at);
        step();
    } while (dec8_at(dpa(K95_FRAMES)));
}

static void stay(uint32_t brk_at)
{
    for (;;)
        actor_yield(brk_at);
}

static void sound(uint8_t n)
{
    a8_load(n);
    abi_SoundQueueAdd();
}

/* The story's scene task (slot 4) with scene `scene`, once the page is down. */
static void story_wait_page(uint32_t brk_at)
{
    uint8_t page;

    do {
        actor_yield(brk_at);
        page = rd8(dba(STORY_PAGE));
        a8_load(page);
    } while (page);
}

/* Part 0: pose 0, 16 frames, sound $43, then it speeds off through poses
   1 to 4, 16 frames each, and ends. */
static void part0(void)
{
    uint8_t n;

    pose(0x00, 0x849C4Cu);
    wait(0x10, 0x849C52u);
    sound(0x43);
    a8_load(0x40);
    wr8(dpa(ACTOR_ACCEL_X), 0x40);
    speed_x(0x0C);
    speed_y(0x0F);
    n = 0x01;
    a8_load(n);
    for (;;) {
        wr8(dpa(K95_STEP), n);
        jsl_frame(0x849C6Cu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        for_frames(0x10, 0x849C74u, abi_ActorSpeedUpMove);
        n = (uint8_t)(rd8(dpa(K95_STEP)) + 1);
        a8_load(n);
        cmp8_of(n, 0x05);
        if (n >= 0x05)
            break;
    }
    abi_ActorEndPlaced();
}

/* Part 2 (part 1 facing left): pose 0, 40 frames, then 216 frames of
   speeding along, bouncing back (speed y turned round, the facing turned)
   whenever its speed x is $C0 or more; then 88 frames falling; it ends. */
static void part2(void)
{
    pose(0x00, 0x849C8Du);
    wait(0x28, 0x849C93u);
    a8_load(0xD8);
    wr8(dpa(K95_FRAMES), 0xD8);
    speed_y(0x1E);
    speed_x(0x21);
    a8_load(0x09);
    wr8(dpa(ACTOR_ACCEL_X), 0x09);
    do {
        uint8_t high;

        actor_yield(0x849CA7u);
        abi_ActorSpeedUpMove();
        high = rd8(dpa(SPEED_X_HIGH));
        a8_load(high);
        if (!(high & 0x80)) {
            uint16_t sx;

            op_rep(0x20);
            sx = dp16(ACTOR_SPEED_X);
            a16_load(sx);
            cmp16_of(sx, 0x00C0);
            if (sx >= 0x00C0) {
                uint16_t sy = (uint16_t)(dp16(ACTOR_SPEED_Y) ^ 0xFFFF);

                sy = (uint16_t)(sy + 1);
                a16_load(sy);
                dp16_set(ACTOR_SPEED_Y, sy);
                op_sep(0x20);
                a8_load((uint8_t)(rd8(dpa(SPEED_Y_HIGH)) ^ 0xFF));
                wr8(dpa(SPEED_Y_HIGH), (uint8_t)cpu.a);
                speed_x(0x21);
                a8_load((uint8_t)(rd8(dpa(ACTOR_FACING)) ^ 0x01));
                wr8(dpa(ACTOR_FACING), (uint8_t)cpu.a);
            }
        }
        op_sep(0x20);
    } while (dec8_at(dpa(K95_FRAMES)));
    speed_y(0x15);
    for_frames(0x58, 0x849CE4u, abi_ActorMoveY);
    abi_ActorEndPlaced();
}

/* Part 4 (part 3 facing left): pose 0, 136 frames, then 48 frames flying
   up, its fall held to $0200; part 3 then waits for the story's page to
   go and starts the story's scene task with scene 6; it ends. */
static void part4(void)
{
    pose(0x00, 0x849CF6u);
    wait(0x88, 0x849CFCu);
    speed_y(0x1B);
    speed_x(0x12);
    a8_load(0x20);
    wr8(dpa(ACTOR_ACCEL_Y), 0x20);
    a8_load(0x30);
    wr8(dpa(K95_FRAMES), 0x30);
    do {
        uint8_t high;

        actor_yield(0x849D10u);
        abi_ActorFly();
        high = rd8(dpa(SPEED_Y_HIGH));
        a8_load(high);
        if (!(high & 0x80)) {
            op_rep(0x20);
            a16_load(0x0200);
            cmp16_of(0x0200, dp16(ACTOR_SPEED_Y));
            if (0x0200 < dp16(ACTOR_SPEED_Y))
                dp16_set(ACTOR_SPEED_Y, 0x0200);
        }
        op_sep(0x20);
    } while (dec8_at(dpa(K95_FRAMES)));
    a8_load(rd8(dpa(ACTOR_07)));
    cmp8_of((uint8_t)cpu.a, 0x03);
    if ((uint8_t)cpu.a == 0x03) {
        story_wait_page(0x849D31u);
        jsl_frame(0x849D38u); abi_TaskCreate(); cpu.unwound = 0;
    }
    abi_ActorEndPlaced();
}

/* Part 8's stretch: from its pose on, each step's frames flying with its
   fall held to $E0, then what the step table says. */
static void part8_steps(uint8_t first)
{
    uint8_t step, n;

    a8_load(first);
    for (;;) {
        jsl_frame(0x849DADu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);
        step = rd8(dpa(K95_STEP));
        x8_load(step);
        n = rd8(dba((uint16_t)(PART8_FRAMES + step)));
        a8_load(n);
        wr8(dpa(K95_FRAMES), n);
        do {
            actor_yield(0x849DB8u);
            abi_ActorFly();
            a8_load(0xE0);
            cmp8_of(0xE0, rd8(dpa(ACTOR_SPEED_Y)));
            if (0xE0 < rd8(dpa(ACTOR_SPEED_Y)))
                wr8(dpa(ACTOR_SPEED_Y), 0xE0);
        } while (dec8_at(dpa(K95_FRAMES)));
        step = rd8(dpa(K95_STEP));
        a8_load((uint8_t)(step << 1));
        flag_set(P_C, step & 0x80);
        x8_load((uint8_t)(step << 1));
        if (step < 5)
            (void)inc8_at(dpa(K95_STEP));
        switch (step) {
        case 0:
            a8_load(0x0A);
            break;
        case 1:
            a8_load(0x10);
            wr8(dpa(ACTOR_SPEED_X), 0x10);
            a8_load(0x0B);
            break;
        case 2:
            a8_load(0x20);
            wr8(dpa(ACTOR_SPEED_X), 0x20);
            a8_load(0x0C);
            break;
        case 3:
            a8_load(0x03);
            tsb_at(dpa(ACTOR_08), 0x03);
            for_frames(0x60, 0x849E01u, abi_ActorStepYBack);
            wait(0x60, 0x849E0Du);
            pose(0x0D, 0x849E11u);
            op_rep(0x20);
            dp16_set(ACTOR_X + 1, 0x00A0);
            a16_load(0x006C);
            dp16_set(ACTOR_Y + 1, 0x006C);
            op_sep(0x20);
            (void)inc8_at(dpa(ACTOR_FACING));
            a8_load(0x0D);
            break;
        case 4:
            a8_load((uint8_t)((rd8(dpa(ACTOR_08)) & 0xFC) ^ 0x02));
            wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
            a8_load(0x0E);
            break;
        default:                        /* 5: the last */
            abi_ActorEndPlaced();
            return;
        }
    }
}

/* Part 8: the hops of set 3, 240 frames, the story's scene task with
   scene 1; once the page is down it flies across for 29 frames, is put at
   (144, 144), waits 128 frames, and plays out its six steps. */
static void part8(void)
{
    x8_load(0x03);
    push16(0x9D64);
    abi_Kind95Hops();
    cpu.s = (uint16_t)(cpu.s + 2);
    wait(0xF0, 0x849D67u);
    jsl_frame(0x849D69u); abi_TaskCreate(); cpu.unwound = 0;
    story_wait_page(0x849D72u);
    y8_load(0x61);
    abi_ActorMotionPreset();
    a8_load(0x1D);
    wr8(dpa(K95_FRAMES), 0x1D);
    (void)inc8_at(dpa(ACTOR_FACING));
    do {
        actor_yield(0x849D85u);
        abi_ActorFly();
    } while (dec8_at(dpa(K95_FRAMES)));
    op_rep(0x20);
    dp16_set(ACTOR_X + 1, 0x0090);
    a16_load(0x0090);
    dp16_set(ACTOR_Y + 1, 0x0090);
    op_sep(0x20);
    wait(0x80, 0x849D9Fu);
    wr8(dpa(ACTOR_FACING), 0);
    y8_load(0x66);
    abi_ActorMotionPreset();
    wr8(dpa(K95_STEP), 0);
    part8_steps(0x09);
}

/* Parts 5-7: the hops of set 0-2, then it stays. */
static void hops_then_stay(uint8_t set, uint16_t back, uint32_t brk_at)
{
    x8_load(set);
    push16(back);
    abi_Kind95Hops();
    cpu.s = (uint16_t)(cpu.s + 2);
    stay(brk_at);
}

/* Part 14: off the screen's left, it waits $06F0 frames hidden, then is
   part 0. */
static void part14(void)
{
    op_rep(0x10);
    a8_load(0x80);
    trb_at(dpa(ACTOR_09), 0x80);
    cpu.x = 0x0468;
    nz16_of(cpu.x);
    dp16_set(ACTOR_X + 1, 0x0468);
    cpu.x = 0x0116;
    nz16_of(cpu.x);
    dp16_set(ACTOR_Y + 1, 0x0116);
    cpu.x = 0x06F0;
    nz16_of(cpu.x);
    do {
        dp16_set(K95_FRAMES, cpu.x);
        actor_yield(0x849C3Du);
        cpu.x = (uint16_t)(dp16(K95_FRAMES) - 1);
        nz16_of(cpu.x);
    } while (cpu.x);
    a8_load(0x80);
    tsb_at(dpa(ACTOR_09), 0x80);
    op_sep(0x10);
    part0();
}

/*
 * @routine 84:9BE3 Kind95Start
 * @actor
 * WHAT IT DOES: Actor kind $95, a figure of the story's scenes: by its
 * $07 (low four bits) one of fifteen parts: a figure that speeds off
 * through four poses (0, or 14 after a long wait off screen); one that
 * bounces along and falls (2, 1 facing left); one that flies up (4, 3
 * facing left, which then starts the story's scene task with scene 6);
 * hops in place, then still (5-7); hops, the scene task with scene 1, then
 * a flight in six steps (8); a still pose (9-11); a sound after 16 frames
 * (12); a sound every $12C frames (13). Unless its $07 bit 4 is set it
 * takes palette 5.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set, facing and flip 0;
 * $07 bit 4 clear: ACTOR_LOOK's palette bits ($2C AND $0E) = $0A. Then
 * JMP through PARTS by $07 AND $0F. Waits by COP, frames by BRK with
 * K95_FRAMES counting; motion by ActorSpeedUpMove, ActorMoveY, ActorFly
 * and ActorStepYBack with speeds from the bank's entries
 * (ActorSpeedXFrom84, ActorSpeedYFrom84) or ActorMotionPreset; the hops
 * are Kind95Hops; the scene task is TaskCreate (slot 4, scene, $81:F332,
 * the opening story's) once STORY_PAGE is 0; part 8's steps go on through
 * PART8_STEPS by K95_STEP. The parts that end do so by ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $95 in state $0C (the table at $83:8DAB,
 * through the stub at $83:93A3). Exit: never returns.
 */
void abi_Kind95Start(void)
{
    uint8_t which, part;

    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    wr8(dpa(ACTOR_FACING), 0);
    wr8(dpa(ACTOR_FLIP), 0);
    which = rd8(dpa(ACTOR_07));
    a8_load(which);
    flag_set(P_Z, !(which & 0x10));
    if (!(which & 0x10)) {
        a8_load(0x0E);
        trb_at(dpa(ACTOR_LOOK + 1), 0x0E);
        a8_load(0x0A);
        tsb_at(dpa(ACTOR_LOOK + 1), 0x0A);
    }
    part = (uint8_t)(rd8(dpa(ACTOR_07)) & 0x0F);
    a8_load((uint8_t)(part << 1));
    flag_set(P_C, 0);
    x8_load((uint8_t)(part << 1));
    switch (part) {
    case 0:
        part0();
        break;
    case 1:
        (void)inc8_at(dpa(ACTOR_FACING));
        part2();
        break;
    case 2:
        part2();
        break;
    case 3:
        (void)inc8_at(dpa(ACTOR_FACING));
        part4();
        break;
    case 4:
        part4();
        break;
    case 5:
        hops_then_stay(0x00, 0x9D49, 0x849D4Au);
        break;
    case 6:
        hops_then_stay(0x01, 0x9D52, 0x849D53u);
        break;
    case 7:
        hops_then_stay(0x02, 0x9D5B, 0x849D5Cu);
        break;
    case 8:
        part8();
        break;
    case 9:
        pose(0x08, 0x849E3Eu);
        stay(0x849E42u);
        break;
    case 10:
        pose(0x05, 0x849E48u);
        stay(0x849E4Cu);
        break;
    case 11:
        pose(0x06, 0x849E52u);
        stay(0x849E56u);
        break;
    case 12:
        wait(0x10, 0x849E5Cu);
        sound(0x05);
        abi_ActorEndPlaced();
        break;
    case 13:
        for (;;) {
            wait(0x2D, 0x849E6Au);
            sound(0x3E);
            wait(0xFF, 0x849E74u);
        }
    case 14:
        part14();
        break;
    default:                            /* 15: past the table's 15 words (it reads part 14's first bytes, $10C2); no $07 names it */
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(PARTS + cpu.x)));
        break;
    }
}

/*
 * @routine 84:9E78 Kind95Hops
 * @actor
 * WHAT IT DOES: Kind $95's hops in place: set X's pose, a wait, then its
 * count of hops, each a rise and a fall for the set's frames at the set's
 * speeds; then back to the caller.
 * HOW: Its return address pulled into K95_BACK (so its waits leave the
 * actor's stack as the loop expects); K95_HOP = X; AnimStart HOP_POSE by
 * X; K95_STEP = HOP_COUNT by X; COP HOP_DELAY by X; K95_HOP doubled. Each
 * hop: K95_FRAMES from HOP_UP, Y the byte after, ActorSpeedYFrom84, each
 * frame (BRK) ActorMoveY until it counts down; the same from HOP_DOWN;
 * K95_STEP down by one. K95_BACK pushed back, RTS.
 * Entry: M 8-bit, X 8-bit; X = the set (0-3); D = the actor's record; DB
 * = $84; called by JSR from Kind95Start's parts 5-8. Exit: M 8-bit, X
 * 8-bit; A = K95_BACK's low byte; X = K95_HOP; Y and C as the last
 * ActorMoveY leaves them; N and Z from the last count.
 */
void abi_Kind95Hops(void)
{
    uint16_t back;
    uint8_t set, at;

    op_rep(0x20);
    back = pull16();
    a16_load(back);
    dp16_set(K95_BACK, back);
    op_sep(0x20);
    set = (uint8_t)cpu.x;
    wr8(dpa(K95_HOP), set);
    pose(rd8(dba((uint16_t)(HOP_POSE + set))), 0x849E84u);
    x8_load(rd8(dpa(K95_HOP)));
    a8_load(rd8(dba((uint16_t)(HOP_COUNT + cpu.x))));
    wr8(dpa(K95_STEP), (uint8_t)cpu.a);
    wait(rd8(dba((uint16_t)(HOP_DELAY + cpu.x))), 0x849E92u);
    wr8(dpa(K95_HOP), (uint8_t)(rd8(dpa(K95_HOP)) << 1));
    flag_set(P_C, 0);                   /* the ASL's: bit 7 of a set below 4 */
    do {
        at = rd8(dpa(K95_HOP));
        x8_load(at);
        a8_load(rd8(dba((uint16_t)(HOP_UP + at))));
        wr8(dpa(K95_FRAMES), (uint8_t)cpu.a);
        speed_y(rd8(dba((uint16_t)(HOP_UP + 1 + at))));
        do {
            actor_yield(0x849EA3u);
            abi_ActorMoveY();
        } while (dec8_at(dpa(K95_FRAMES)));
        at = rd8(dpa(K95_HOP));
        x8_load(at);
        a8_load(rd8(dba((uint16_t)(HOP_DOWN + at))));
        wr8(dpa(K95_FRAMES), (uint8_t)cpu.a);
        speed_y(rd8(dba((uint16_t)(HOP_DOWN + 1 + at))));
        do {
            actor_yield(0x849EBAu);
            abi_ActorMoveY();
        } while (dec8_at(dpa(K95_FRAMES)));
    } while (dec8_at(dpa(K95_STEP)));
    op_rep(0x20);
    a16_load(dp16(K95_BACK));
    push16(cpu.a);
    op_sep(0x20);
}
