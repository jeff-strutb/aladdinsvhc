/*
 * story_actor_9b.c: actor kind $9B, a figure of the story's scenes: by
 * its $07 one of four parts, each moving and posing on the story's cues and
 * starting the story's scene task with the next scene's number.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_TaskCreate(void);
void abi_ActorEndPlaced(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedXSetAccel(void);
void abi_ActorSpeedYSet(void);
void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_ActorMotionPreset(void);
void abi_ActorFlyAgainstNoRise(void);
void abi_ActorAccelerateXBack(void);

#define STORY_76 0x0076u    /* set to $40 (and $0077 to 0) as part 2 flies off */
#define STORY_77 0x0077u
#define STORY_78 0x0078u    /* counted up by part 2 once in place */
#define STORY_79 0x0079u    /* counted up by part 3 at its end; part 1 waits for it */
#define STORY_7A 0x007Au    /* counted up by part 1, cleared at its end; negative: parts 0 and 2 go on */
#define STORY_7B 0x007Bu    /* counted up by part 2; part 3 waits for it */
#define CUE_CF 0x00CFu      /* nonzero while the story's text is busy */
#define ACTOR_2C 0x2Cu      /* its sprite attributes' high byte */
#define FRAMES 0x40u        /* frames left of a step */
#define ROLES 0x84A51Cu     /* the parts' code, by $07 (words, this bank) */

/* TaskCreate at `site`, its five argument bytes after the JSL: its frame
   as the JSL pushes it; TaskCreate returns past them by its own RTL and
   the C goes on. */
static void task_create(uint32_t site)
{
    jsl_frame(site);
    abi_TaskCreate();
    cpu.unwound = 0;
}

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

static void pose(uint8_t n, uint32_t site)
{
    a8_load(n);
    jsl_call(site, abi_AnimStart);
}

static void wait_frames(uint8_t n, uint32_t cop_at)
{
    a8_load(n);
    actor_wait(cop_at);
}

static void set_bits(uint16_t field, uint8_t bits)  /* LDA #bits, TSB field */
{
    a8_load(bits);
    tsb_at(dpa(field), bits);
}

/* Each frame (BRK at brk_at) until the byte at addr is zero (want 0),
   nonzero (want 1) or negative (want 2). */
static void wait_for(uint16_t addr, int want, uint32_t brk_at)
{
    uint8_t v;

    for (;;) {
        actor_yield(brk_at);
        v = rd8(dba(addr));
        a8_load(v);
        if ((want == 0 && !v) || (want == 1 && v) || (want == 2 && (v & 0x80)))
            return;
    }
}

/* FRAMES = n, then each frame (BRK at brk_at) the step, until it counts down. */
static void for_frames(uint8_t n, uint32_t brk_at, void (*step)(void), uint32_t site, int jsr)
{
    a8_load(n);
    wr8(dpa(FRAMES), n);
    do {
        actor_yield(brk_at);
        if (jsr)
            call_frame((uint16_t)(site + 2), step);   /* JSR: its last byte */
        else
            jsl_call(site, step);
    } while (dec8_at(dpa(FRAMES)));
}

/* Its x and y pixels set (words). */
static void place(uint16_t x, uint16_t y)
{
    op_rep(0x20);
    a16_load(x);
    dp16_set(ACTOR_X + 1, x);
    a16_load(y);
    dp16_set(ACTOR_Y + 1, y);
    op_sep(0x20);
}

static void face(uint8_t f)                  /* STA (or STZ) facing and flip */
{
    wr8(dpa(ACTOR_FACING), f);
    wr8(dpa(ACTOR_FLIP), f);
}

static void part0(void)
{
    set_bits(ACTOR_2C, 0x0C);
    wait_for(STORY_7A, 2, 0x84A528u);
    set_bits(ACTOR_09, 0x80);
    pose(0x08, 0x84A535u);
    wait_frames(0x30, 0x84A53Bu);
    abi_ActorEndPlaced();
}

static void part1(void)
{
    set_bits(ACTOR_2C, 0x08);
    set_bits(ACTOR_08, 0x02);
    set_bits(ACTOR_09, 0x80);
    pose(0x01, 0x84A54Fu);
    wait_for(CUE_CF, 0, 0x84A553u);
    pose(0x02, 0x84A55Cu);
    wait_frames(0x18, 0x84A562u);
    pose(0x01, 0x84A566u);
    (void)inc8_at(dpa(ACTOR_FACING));
    (void)inc8_at(dpa(ACTOR_FLIP));
    a8_load(0x20);
    wr8(dpa(FRAMES), 0x20);
    y8_load(0x66);
    jsl_call(0x84A574u, abi_ActorSpeedXSet);
    do {
        actor_yield(0x84A578u);
        jsl_call(0x84A57Au, abi_ActorMoveX);
    } while (dec8_at(dpa(FRAMES)));
    wait_frames(0x20, 0x84A584u);
    pose(0x02, 0x84A588u);
    wait_frames(0x18, 0x84A58Eu);
    pose(0x01, 0x84A592u);
    face(0);
    task_create(0x84A59Au);                      /* scene 7 */
    wait_for(CUE_CF, 0, 0x84A5A3u);
    wait_frames(0x30, 0x84A5ACu);
    (void)inc8_at(dba(STORY_7A));
    wait_for(STORY_79, 1, 0x84A5B1u);
    pose(0x02, 0x84A5BAu);
    wait_frames(0x18, 0x84A5C0u);
    pose(0x01, 0x84A5C4u);
    (void)inc8_at(dpa(ACTOR_FLIP));
    wait_for(CUE_CF, 0, 0x84A5CAu);
    task_create(0x84A5D1u);                      /* scene 16 */
    wait_for(CUE_CF, 0, 0x84A5DAu);
    wait_frames(0x20, 0x84A5E3u);
    pose(0x02, 0x84A5E7u);
    wait_frames(0x18, 0x84A5EDu);
    pose(0x01, 0x84A5F1u);
    wr8(dpa(ACTOR_FLIP), 0);
    task_create(0x84A5F7u);                      /* scene 17 */
    wr8(dba(STORY_7A), 0);
    for (;;)
        actor_yield(0x84A603u);
}

static void part2(void)
{
    uint8_t v;

    set_bits(ACTOR_2C, 0x0C);
    set_bits(ACTOR_08, 0x02);
    set_bits(ACTOR_09, 0x80);
    (void)inc8_at(dpa(ACTOR_FACING));
    (void)inc8_at(dpa(ACTOR_FLIP));
    pose(0x04, 0x84A619u);
    wait_for(STORY_7A, 1, 0x84A61Du);
    y8_load(0x6E);
    jsl_call(0x84A626u, abi_ActorMotionPreset);
    for_frames(0x40, 0x84A62Eu, abi_ActorFlyAgainstNoRise, 0x84A630u, 1);
    wait_frames(0x20, 0x84A639u);
    task_create(0x84A63Bu);                      /* scene 8 */
    (void)inc8_at(dba(STORY_78));
    wait_for(STORY_7A, 2, 0x84A647u);
    pose(0x06, 0x84A650u);
    wait_frames(0x20, 0x84A656u);
    pose(0x07, 0x84A65Au);
    wait_frames(0x20, 0x84A660u);
    task_create(0x84A662u);                      /* scene 12 */
    wait_for(CUE_CF, 0, 0x84A66Bu);
    pose(0x05, 0x84A674u);
    wait_frames(0x10, 0x84A67Au);
    y8_load(0x87);
    jsl_call(0x84A67Eu, abi_ActorSpeedYSet);
    for_frames(0x50, 0x84A686u, abi_ActorMoveY, 0x84A688u, 0);
    pose(0x09, 0x84A692u);
    place(0x00E0, 0x0030);
    y8_load(0x6F);
    jsl_call(0x84A6A6u, abi_ActorMotionPreset);
    for_frames(0x60, 0x84A6AEu, abi_ActorFlyAgainstNoRise, 0x84A6B0u, 1);
    wait_frames(0x50, 0x84A6B9u);
    place(0x0020, 0x0030);
    face(0);
    y8_load(0x6F);
    jsl_call(0x84A6CFu, abi_ActorMotionPreset);
    for_frames(0x60, 0x84A6D7u, abi_ActorFlyAgainstNoRise, 0x84A6D9u, 1);
    wait_frames(0x30, 0x84A6E2u);
    place(0x00E8, 0x0050);
    pose(0x05, 0x84A6F4u);
    a8_load(0x40);
    y8_load(0x69);
    jsl_call(0x84A6FCu, abi_ActorSpeedXSetAccel);
    a8_load(0x01);
    face(0x01);
    for_frames(0x20, 0x84A70Au, abi_ActorMoveX, 0x84A70Cu, 0);
    /* slowing, until its speed turns back */
    do {
        actor_yield(0x84A714u);
        jsl_call(0x84A716u, abi_ActorAccelerateXBack);
        jsl_call(0x84A71Au, abi_ActorMoveX);
        v = rd8(dpa(ACTOR_SPEED_X + 2));
        a8_load(v);
    } while (!(v & 0x80));
    wait_frames(0x20, 0x84A724u);
    task_create(0x84A726u);                      /* scene 13 */
    (void)inc8_at(dba(STORY_7B));
    for (;;) {
        actor_yield(0x84A732u);
        v = rd8(dba(STORY_7A));
        a8_load(v);
        if (v)
            continue;
        v = rd8(dba(CUE_CF));
        a8_load(v);
        if (!v)
            break;
    }
    a8_load(0x40);
    wr8(dba(STORY_76), 0x40);
    wr8(dba(STORY_77), 0);
    y8_load(0x71);
    jsl_call(0x84A748u, abi_ActorMotionPreset);
    for (;;) {
        actor_yield(0x84A74Cu);
        call_frame(0xA750u, abi_ActorFlyAgainstNoRise);
    }
}

static void part3(void)
{
    uint8_t v;

    set_bits(ACTOR_2C, 0x0A);
    set_bits(ACTOR_08, 0x02);
    set_bits(ACTOR_09, 0x80);
    pose(0x03, 0x84A761u);
    for (;;) {
        actor_yield(0x84A765u);
        v = rd8(dba(STORY_7B));
        a8_load(v);
        if (!v)
            continue;
        v = rd8(dba(CUE_CF));
        a8_load(v);
        if (!v)
            break;
    }
    y8_load(0x66);
    jsl_call(0x84A773u, abi_ActorSpeedXSet);
    a8_load(0xFB);
    wr8(dpa(ACTOR_44), 0xFB);
    (void)inc8_at(dpa(ACTOR_42));
    for_frames(0x60, 0x84A781u, abi_ActorMoveX, 0x84A783u, 0);
    wr8(dpa(ACTOR_42), 0);
    wait_frames(0x20, 0x84A78Fu);
    task_create(0x84A791u);                      /* scene 15 */
    (void)inc8_at(dba(STORY_79));
    for (;;)
        actor_yield(0x84A79Du);
}

/*
 * @routine 84:A509 Kind9BStory
 * @actor
 * WHAT IT DOES: Actor kind $9B, a figure of the story's scenes: part 0
 * waits for STORY_7A to go negative, poses (8) and ends; part 1 turns,
 * walks, turns back and starts scenes 7, 16 and 17 between the story's
 * cues, counting STORY_7A up and clearing it at the end; part 2 flies in
 * and out on set motions, starts scenes 8, 12 and 13, counts STORY_78
 * and STORY_7B up, and flies on for good; part 3 waits for STORY_7B,
 * walks across, starts scene 15 and counts STORY_79 up.
 * HOW: actor_09 bit 4 set, actor_2c bits 1-3 cleared, actor_42 and 44
 * 0; then by its $07 through the table .roles. Each part sets its
 * actor_2c bits, actor_08 bit 1 and actor_09 bit 7, and runs its steps:
 * AnimStart poses, COP waits, BRK waits on STORY_7A, 79, 7B and CUE_CF,
 * motion presets and speeds, frame counts in actor_40, TaskCreate of
 * STORY_SCENE_TASK in slot 4 with the scene's number. Part 0 ends in
 * ActorEndPlaced; the others yield for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $9B
 * in state $0C (the table at $83:8DAB, through the stub at $83:93E5).
 * Exit: never returns.
 */
void abi_Kind9BStory(void)
{
    uint8_t part, i;

    set_bits(ACTOR_09, 0x10);
    a8_load(0x0E);
    trb_at(dpa(ACTOR_2C), 0x0E);
    wr8(dpa(ACTOR_42), 0);
    wr8(dpa(ACTOR_44), 0);
    part = rd8(dpa(ACTOR_07));
    i = (uint8_t)(part << 1);
    a8_load(i);
    flag_set(P_C, part & 0x80);
    x8_load(i);
    switch (i) {
    case 0: part0(); break;
    case 2: part1(); break;
    case 4: part2(); break;
    case 6: part3(); break;
    default: jump_routine(0x840000u | rd16((ROLES + i) & 0xFFFFFFu)); break;
    }
}
