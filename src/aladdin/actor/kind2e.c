/*
 * kind2e.c: actor kind $2E, the figures of level 5's closing scene (the
 * scene's task places them, $80:A2A8): by its $07, one that walks a few
 * steps on a cue, starts the window waves and sinks away; one that stands;
 * one that keeps beside Aladdin on a cue; one that turns up at its place
 * on another.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "actor.h"

void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);
void abi_TaskCreate(void);

#define K2E_16 0x16u            /* word: $D14E */
#define K2E_FACE 0x40u          /* part 0: its facing, flipped each step; then frames left */
#define K2E_STEPS 0x41u         /* part 0: steps left */
#define CUE_146B 0x146Bu        /* part 0 goes once this is set (cleared as it starts) */
#define CUE_BE 0x00BEu          /* part 2 goes on this cue */
#define CUE_C0 0x00C0u          /* part 3 goes on this cue (set by the scene's task) */
#define PLAYER_2F (PLAYER + 0x2F) /* Aladdin's byte $2F: the offset pair part 2 keeps by */
#define BESIDE_X 0xD152u        /* (data bank) by PLAYER_2F * 2: the x offset from Aladdin (word) */
#define BESIDE_Y 0xD160u        /* the y offset (word) */
#define PLAYER_X (PLAYER + ACTOR_X + 1)
#define PLAYER_Y (PLAYER + ACTOR_Y + 1)

static void stay(uint32_t brk_at)
{
    for (;;)
        actor_yield(brk_at);
}

static void face(uint8_t f)
{
    if (f) {
        a8_load(f);
        wr8(dpa(ACTOR_FACING), f);
        wr8(dpa(ACTOR_FLIP), f);
    } else {
        wr8(dpa(ACTOR_FACING), 0);
        wr8(dpa(ACTOR_FLIP), 0);
    }
}

static void wait_cue(uint16_t cue, uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        a8_load(rd8(dba(cue)));
    } while (!(uint8_t)cpu.a);
}

static void place(uint16_t x, uint16_t y)
{
    op_rep(0x20);
    a16_load(x);
    dp16_set(ACTOR_X + 1, x);
    a16_load(y);
    dp16_set(ACTOR_Y + 1, y);
    op_sep(0x20);
}

/* Part 0: faces right; on CUE_146B six steps across at speed 2, turning
   each step; window effect 6 started; 42 frames; then 144 frames sinking
   at speed 1; it stays. */
static void part0(void)
{
    uint8_t f;

    face(0x01);
    wr8(dpa(K2E_FACE), 0);
    a8_load(0x00);
    abi_AnimStart();
    wr8(dba(CUE_146B), 0);
    wait_cue(CUE_146B, 0x83AB11u);
    wr8(dpa(ACTOR_SPEED_X), 0);
    a8_load(0x02);
    wr8(dpa(ACTOR_SPEED_X + 1), 0x02);
    wr8(dpa(ACTOR_SPEED_X + 2), 0);
    a8_load(0x06);
    wr8(dpa(K2E_STEPS), 0x06);
    do {
        actor_yield(0x83AB24u);
        abi_ActorMoveX();
        f = (uint8_t)(rd8(dpa(K2E_FACE)) ^ 0x01);
        a8_load(f);
        wr8(dpa(ACTOR_FACING), f);
        wr8(dpa(K2E_FACE), f);
    } while (dec8_at(dpa(K2E_STEPS)));
    jsl_frame(0x83AB36u); abi_TaskCreate(); cpu.unwound = 0;
    a8_load(0x2A);
    actor_wait(0x83AB41u);
    a8_load(0x90);
    wr8(dpa(K2E_FACE), 0x90);
    wr8(dpa(ACTOR_SPEED_Y), 0);
    a8_load(0x01);
    wr8(dpa(ACTOR_SPEED_Y + 1), 0x01);
    wr8(dpa(ACTOR_SPEED_Y + 2), 0);
    do {
        actor_yield(0x83AB4Fu);
        abi_ActorMoveY();
    } while (dec8_at(dpa(K2E_FACE)));
    stay(0x83AB59u);
}

/* Part 2: faces left; on CUE_BE it is put at ($0E80, $0148), then each
   frame beside Aladdin by the offsets his byte $2F picks. */
static void part2(void)
{
    uint8_t at;
    uint16_t off;

    face(0x00);
    a8_load(0x00);
    abi_AnimStart();
    wait_cue(CUE_BE, 0x83AB75u);
    place(0x0E80, 0x0148);
    for (;;) {
        actor_yield(0x83AB8Au);
        at = (uint8_t)(rd8(dba((uint16_t)PLAYER_2F)) << 1);
        a8_load(at);
        x8_load(at);
        op_rep(0x20);
        flag_set(P_C, 0);
        off = rd16(dba((uint16_t)(BESIDE_X + at)));
        a16_load(op_adc16(off, rd16(dba((uint16_t)PLAYER_X)), &cpu.p));
        dp16_set(ACTOR_X + 1, cpu.a);
        flag_set(P_C, 0);
        off = rd16(dba((uint16_t)(BESIDE_Y + at)));
        a16_load(op_adc16(off, rd16(dba((uint16_t)PLAYER_Y)), &cpu.p));
        dp16_set(ACTOR_Y + 1, cpu.a);
        op_sep(0x20);
    }
}

/* Part 3: faces left in pose 1; on CUE_C0 it is put at ($0DA0, $0238) in
   pose 2 with sound 5, and stays. */
static void part3(void)
{
    face(0x00);
    a8_load(0x01);
    abi_AnimStart();
    wait_cue(CUE_C0, 0x83ABB3u);
    place(0x0DA0, 0x0238);
    a8_load(0x02);
    abi_AnimStart();
    a8_load(0x05);
    abi_SoundQueueAdd();
    stay(0x83ABD4u);
}

/*
 * @routine 83:AADC Kind2EStart
 * @actor
 * WHAT IT DOES: Actor kind $2E, a figure of level 5's closing scene, by
 * its $07: 0 faces right and, on CUE_146B, takes six steps across turning
 * each step, starts window effect 6 and sinks away for 144 frames; 1
 * faces left and stays; 2 faces left and, on CUE_BE, keeps beside Aladdin
 * by the offsets his byte $2F picks; 3 faces left in pose 1 and, on
 * CUE_C0, turns up at ($0DA0, $0238) in pose 2 with sound 5.
 * HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; K2E_16 $D14E; JMP through
 * its four parts by $07. Part 0: CUE_146B cleared and awaited; speed x
 * $0200, six frames of BRK, ActorMoveX and the facing flipped; TaskCreate
 * (slot 6, parameter $0C, WindowEffectTask); COP $2A; speed y $0100, 144
 * frames of BRK and ActorMoveY. Part 2: from ($0E80, $0148), each frame x
 * = BESIDE_X + Aladdin's x, y = BESIDE_Y + his y, by PLAYER_2F * 2.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM and that holds the offsets (bank $83); reached
 * by the actor loop's jump for kind $2E in state $0C (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind2EStart(void)
{
    uint8_t part;

    a8_load((uint8_t)(rd8(dpa(ACTOR_08)) | 0x02));
    wr8(dpa(ACTOR_08), (uint8_t)cpu.a);
    a8_load((uint8_t)(rd8(dpa(ACTOR_09)) | 0x90));
    wr8(dpa(ACTOR_09), (uint8_t)cpu.a);
    op_rep(0x20);
    a16_load(0xD14E);
    dp16_set(K2E_16, 0xD14E);
    op_sep(0x20);
    part = rd8(dpa(ACTOR_07));
    a8_load((uint8_t)(part << 1));
    flag_set(P_C, part & 0x80);
    x8_load((uint8_t)(part << 1));
    switch (part) {
    case 0:
        part0();
        break;
    case 1:
        face(0x00);
        a8_load(0x00);
        abi_AnimStart();
        stay(0x83AB67u);
        break;
    case 2:
        part2();
        break;
    case 3:
        part3();
        break;
    default:                            /* past the table's four words: no $07 names it */
        jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(0xAAF8 + cpu.x)));
        break;
    }
}
