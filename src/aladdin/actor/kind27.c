/*
 * kind27.c: actor kind $27: a figure that crosses the view and closes
 * the level, a still pose, or one that waits for the first to be touched
 * and then flies about; both leave a trail. Written from the bytes where
 * no script runs them; named and described by hand.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAccelerateX(void);
void abi_ActorCreateChild8(void);
void abi_ActorDecelerateX(void);
void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorMove(void);
void abi_ActorMoveByVector(void);
void abi_ActorMoveY(void);
void abi_ActorSpawn(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedYClear(void);
void abi_ActorSpeedYSet(void);
void abi_AnimStart(void);
void abi_Kind27Trail(void);
void abi_Kind27Waiter(void);
void abi_LevelTablesClear(void);
void abi_PaletteFadeStep(void);
void abi_PaletteSave(void);
void abi_SoundQueueAdd(void);
void abi_TaskCreate(void);

#define K27_FRAMES 0x40u          /* frames left (the still pose: its frames) */
#define K27_STEP 0x41u            /* bit 3: the palette fade's step; the waiter: rounds left */
#define K27_ROUNDS 0x41u
#define K27_OPEN 0x42u            /* nonzero: Kind27Wait lets it be touched */
#define K27_V_036C 0x036Cu        /* counted up as one of bit 3's figures is touched */
#define K27_VIEW_X 0x1773u        /* the view's left edge (pixels, word) */
#define K27_VIEW_Y 0x1777u        /* its top (pixels, word) */
#define K27_CLOCK 0x032Du         /* the frame clock's high byte */
#define K27_CHECK 0x702000u       /* the copier check's byte: work RAM on a copier, none on the cartridge */
#define K27_CLEARED 0x08EFu       /* + 1 to + $2C: the player's record bytes a copier loses */
#define CHILD_07 0x0007u          /* a child's fields (absolute, by its record) */
#define CHILD_40 0x0040u
#define CHILD_LOOK 0x002Bu

/* A JSL at `site`, its return address on the stack while the callee runs. */
#define JSL(site, fn) do { jsl_frame(site); fn(); cpu.s = (uint16_t)(cpu.s + 3); } while (0)

static void vector_1a(void) { ldx8_v(0x1A); JSL(0x839EBCu, abi_ActorMoveByVector); }
static void vector_16(void) { ldx8_v(0x16); JSL(0x839F90u, abi_ActorMoveByVector); }

/* `n` frames (BRK at brk_at first) of Kind27Trail (its JSR right after the
   BRK) and `step` (its JSL at step_at). */
static void trail_frames(uint8_t n, uint32_t brk_at, void (*step)(void), uint32_t step_at)
{
    lda8_v(n);
    sta8(ea_dp(K27_FRAMES));
    do {
        actor_yield(brk_at);
        call_frame((uint16_t)(brk_at + 4), abi_Kind27Trail);
        if (step == vector_1a || step == vector_16) {
            step();                       /* LDX, then its JSL at step_at */
        } else {
            JSL(step_at, step);
        }
        MEM8(dec, ea_dp(K27_FRAMES));
    } while (!IF_Z);
}


/* The figure of bit 3. */
static void crosser(void)
{
    stz8(ea_abs(K27_V_036C));
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    MEM8(inc, ea_dp(K27_OPEN));
    op_rep(P_M);
    clc();
    lda16(ea_abs(K27_VIEW_X));
    adc16_v(0xFFF0);
    sta16(ea_dp(ACTOR_X + 1));
    clc();
    lda16(ea_abs(K27_VIEW_Y));
    adc16_v(0x0020);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(P_M);
    lda8_v(0x0C);
    JSL(0x839E79u, abi_AnimStart);
    ldy8_v(0x2A);
    JSL(0x839E7Fu, abi_ActorSpeedXSet);
    lda8_v(0x38);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    JSL(0x839E87u, abi_ActorSpeedYClear);
    lda8_v(0x18);
    sta8(ea_dp(K27_FRAMES));
    do {                                  /* flies down */
        actor_yield(0x839E8Fu);
        call_frame(0x9E93, abi_Kind27Trail);
        JSL(0x839E94u, abi_ActorFly);
        op_rep(P_M);
        lda16_v(0x0300);
        cmp16_v(rd16(ea_dp(ACTOR_SPEED_Y)));
        if (!IF_C) {
            sta16(ea_dp(ACTOR_SPEED_Y));
            ldx8_v(0x00);
            stx8(ea_dp(ACTOR_ACCEL_Y));
        }
        op_sep(P_M);
        MEM8(dec, ea_dp(K27_FRAMES));
    } while (!IF_Z);
    lda8_v(0x00);
    do {                                  /* circles */
        sta8(ea_dp(ACTOR_FACING));
        trail_frames(0x03, 0x839EB5u, vector_1a, 0x839EBCu);
        lda8(ea_dp(ACTOR_FACING));
        ACC8(dec);
        and8_v(0x0F);
    } while (!IF_Z);
    stz8(ea_dp(ACTOR_FACING));
    trail_frames(0x0C, 0x839ED1u, abi_ActorMove, 0x839ED6u);
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(0x85);
    JSL(0x839EE2u, abi_ActorCreateChild8);
    lda8_v(0x20);
    actor_wait(0x839EE8u);
    JSL(0x839EEAu, abi_LevelTablesClear);
    lda8_v(0x00);
    ldy8_v(0x10);
    JSL(0x839EF2u, abi_PaletteSave);
    lda8_v(0x1F);
    sta8(ea_dp(K27_FRAMES));
    stz8(ea_dp(K27_STEP));
    do {                                  /* the palettes fade */
        actor_yield(0x839EFCu);
        lda8(ea_dp(K27_STEP));
        ldy8_v(0x10);
        JSL(0x839F02u, abi_PaletteFadeStep);
        MEM8(inc, ea_dp(K27_STEP));
        MEM8(dec, ea_dp(K27_FRAMES));
    } while (!IF_Z);
    jsl_frame(0x839F0Cu);
    abi_TaskCreate();
    cpu.unwound = 0;
    abi_ActorEndPlaced();
}

/*
 * @routine 83:9E3A Kind27Start
 * @actor
 * WHAT IT DOES: Actor kind $27, by its $07: with bit 3 a figure that
 * comes in at the view's top left and flies down, circles, crosses,
 * leaves a figure (hit box $85), and then the level's palettes fade out
 * and the screen darkens before it ends; with bit 4 a still pose ($0D)
 * for its $40 frames, then the end; else Kind27Waiter. Each moving frame
 * it leaves a trail (Kind27Trail).
 * HOW: ACTOR_09 bits $90 set (TSB); ACTOR_LOOK $0200; facing and flip 1;
 * K27_OPEN 0. Bit 3: V_036C 0; facing and flip 0; K27_OPEN up; x pixels
 * = VIEW_X - 16, y pixels = VIEW_Y + 32; AnimStart $0C; ActorSpeedXSet
 * with entry $2A; ACTOR_ACCEL_Y $38; ActorSpeedYClear; 24 frames (BRK
 * first) of Kind27Trail and ActorFly, its vertical speed held to $0300
 * at most (then ACTOR_ACCEL_Y 0); the direction ($14) 0, then 15 down to
 * 1, 3 frames each of Kind27Trail and ActorMoveByVector (X = $1A);
 * direction 0, 12 frames of Kind27Trail and ActorMove; ACTOR_SPAWN_AT 0,
 * ActorCreateChild8 with A = $85; COP $20; LevelTablesClear; PaletteSave
 * (A = 0, Y = $10); 31 frames (BRK first) of PaletteFadeStep by K27_STEP
 * (0 up), Y = $10; TaskCreate (slot 2, parameter 4, BrightnessDownTask);
 * ActorEndPlaced. Bit 4: ACTOR_08 bit 0 set (TSB); AnimStart $0D; COP
 * with its $40; ActorEndPlaced. Neither: on at Kind27Waiter. No script
 * runs bit 3: written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the speed
 * table's bank); reached by the actor loop's jump for kind $27 in state
 * $0C (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind27Start(void)
{
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    op_rep(P_M);
    lda16_v(0x0200);
    sta16(ea_dp(ACTOR_LOOK));
    op_sep(P_M);
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_FACING));
    sta8(ea_dp(ACTOR_FLIP));
    stz8(ea_dp(K27_OPEN));
    lda8(ea_dp(ACTOR_07));
    bit8_i(0x08);
    if (!IF_Z) {
        crosser();
        return;
    }
    bit8_i(0x10);
    if (IF_Z) {
        abi_Kind27Waiter();
        return;
    }
    lda8_v(0x01);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x0D);
    JSL(0x839F23u, abi_AnimStart);
    lda8(ea_dp(K27_FRAMES));
    actor_wait(0x839F29u);
    abi_ActorEndPlaced();
}

/*
 * @routine 83:9F2F Kind27Waiter
 * @actor
 * WHAT IT DOES: Kind $27 with neither bit 3 nor bit 4 of $07: waits
 * until one of bit 3's figures has been touched (V_036C), then rises 48
 * frames, circles in five steps, and flies off against the walls,
 * leaving a trail, again and again three times before it stays. A copier
 * check comes first: when the count at $70:2000 stays written (work RAM
 * there, which the cartridge does not have), bytes $08F0-$091B of the
 * player's record are cleared.
 * HOW: Each frame: ACTOR_08 bit 1 set (TSB), BRK, its bit 3 cleared
 * (TRB), until V_036C is nonzero; V_036C 0. The byte at K27_CHECK up and
 * compared; equal: X = $2C down to 1, 0 to $08EF + X. ACTOR_08 bit 3 set
 * (TSB); AnimStart $0C; ActorSpeedYSet with entry $51; 48 frames (BRK
 * first) of Kind27Trail and ActorMoveY; K27_OPEN up; K27_ROUNDS 3. A
 * round: the direction 4 down to 0, 8 frames each of Kind27Trail and
 * ActorMoveByVector (X = $16); direction 0; ActorMotionPreset with Y =
 * $81; ACTOR_ACCEL_X $18; frames (BRK first) of Kind27Trail,
 * ActorAccelerateX and ActorFlyAgainst while its vertical speed's top
 * byte is negative; then frames of Kind27Trail and ActorFlyAgainst, for
 * good when K27_ROUNDS is 0, else with ActorDecelerateX until its
 * horizontal speed's two high bytes add to 0; K27_ROUNDS down, the next
 * round. No script runs it: written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
 * a branch from Kind27Start. Exit: never returns.
 */
void abi_Kind27Waiter(void)
{
    do {
        lda8_v(0x02);
        tsb8(ea_dp(ACTOR_08));
        actor_yield(0x839F33u);
        lda8_v(0x08);
        trb8(ea_dp(ACTOR_08));
        lda8(ea_abs(K27_V_036C));
    } while (IF_Z);
    stz8(ea_abs(K27_V_036C));
    lda8(K27_CHECK);
    ACC8(inc);
    sta8(K27_CHECK);
    cmp8_v(rd8(K27_CHECK));
    if (IF_Z) {                           /* the count stayed written: a copier */
        ldx8_v(0x2C);
        lda8_v(0x00);
        do {
            sta8(ea_absx(K27_CLEARED));
            dex8();
        } while (!IF_Z);
    }
    lda8_v(0x08);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x0C);
    JSL(0x839F60u, abi_AnimStart);
    ldy8_v(0x51);
    JSL(0x839F66u, abi_ActorSpeedYSet);
    trail_frames(0x30, 0x839F6Eu, abi_ActorMoveY, 0x839F73u);
    MEM8(inc, ea_dp(K27_OPEN));
    lda8_v(0x03);
    sta8(ea_dp(K27_ROUNDS));
    for (;;) {
        lda8_v(0x04);
        do {                              /* circles in five steps */
            sta8(ea_dp(ACTOR_FACING));
            trail_frames(0x08, 0x839F89u, vector_16, 0x839F90u);
            lda8(ea_dp(ACTOR_FACING));
            ACC8(dec);
            and8_v(0x07);
            cmp8_v(0x07);
        } while (!IF_Z);
        stz8(ea_dp(ACTOR_FACING));
        ldy8_v(0x81);
        JSL(0x839FA5u, abi_ActorMotionPreset);
        lda8_v(0x18);
        sta8(ea_dp(ACTOR_ACCEL_X));
        do {
            actor_yield(0x839FADu);
            call_frame(0x9FB1, abi_Kind27Trail);
            JSL(0x839FB2u, abi_ActorAccelerateX);
            JSL(0x839FB6u, abi_ActorFlyAgainst);
            lda8(ea_dp(ACTOR_SPEED_Y + 2));
        } while (!IF_N);
        for (;;) {
            actor_yield(0x839FBEu);
            call_frame(0x9FC2, abi_Kind27Trail);
            JSL(0x839FC3u, abi_ActorFlyAgainst);
            lda8(ea_dp(K27_ROUNDS));
            if (IF_Z)
                continue;
            JSL(0x839FCBu, abi_ActorDecelerateX);
            clc();
            lda8(ea_dp(ACTOR_SPEED_X));
            adc8_v(rd8(ea_dp(ACTOR_SPEED_X + 1)));
            if (IF_Z)
                break;
        }
        MEM8(dec, ea_dp(K27_ROUNDS));
    }
}

/*
 * @routine 83:9FDA Kind27Trail
 * WHAT IT DOES: Kind $27's trail: every 16 frames of the clock a new
 * figure of its hit box at its spawn point 0, sort $10, for 32 frames,
 * in its own look; every other time also a sound $37 when it can be hit.
 * HOW: K27_CLOCK low 4 bits nonzero: RTS. Bit 4 clear and ACTOR_09 bit 6
 * set: SoundQueueAdd $37. ACTOR_SPAWN_AT 0; ActorSpawn with its hit box;
 * spawned: the child's $07 = $10, $40 = $20, ACTOR_LOOK = its own; X
 * 8-bit. No script runs it: written from its bytes.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; called by
 * JSR from Kind27Start and Kind27Waiter. Exit: M 8-bit, X 8-bit; not
 * now: A low = the clock byte, Z clear; none free: as ActorSpawn (N
 * set); spawned: A low = its look's high byte.
 */
void abi_Kind27Trail(void)
{
    lda8(ea_abs(K27_CLOCK));
    bit8_i(0x0F);
    if (!IF_Z)
        return;
    bit8_i(0x10);
    if (IF_Z) {
        lda8(ea_dp(ACTOR_09));
        bit8_i(0x40);
        if (!IF_Z) {
            lda8_v(0x37);
            JSL(0x839FEDu, abi_SoundQueueAdd);
        }
    }
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8(ea_dp(ACTOR_06));
    JSL(0x839FF5u, abi_ActorSpawn);
    if (IF_N)
        return;
    lda8_v(0x10);
    sta8(ea_absx(CHILD_07));
    lda8_v(0x20);
    sta8(ea_absx(CHILD_40));
    lda8(ea_dp(ACTOR_LOOK));
    sta8(ea_absx(CHILD_LOOK));
    lda8(ea_dp(ACTOR_LOOK + 1));
    sta8(ea_absx(CHILD_LOOK + 1));
    op_sep(P_X);
}
