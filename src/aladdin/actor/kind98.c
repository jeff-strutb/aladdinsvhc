/*
 * kind98.c: actor kind $98 (stage a5 s2), the flier that bobs along the
 * top of the screen, drops kind $9A, spawns kind $23, takes six hits and
 * throws kind $A5 after each; its wait is Kind98WaitWork. The stub at
 * $83:93C2 (kind_wait.c) calls that.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorBobY(void);
void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_ActorOutOfGround(void);
void abi_ActorPictureSend(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSpawnFacing(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedYClear(void);
void abi_ActorSpeedYSet(void);
void abi_ActorSpeedYSetGravity(void);
void abi_ActorStompOrHurtLong(void);
void abi_ActorTurnsShotBackLong(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_RandomFromList(void);
void abi_SoundQueueAdd(void);
void abi_Kind98Throw(void);

#define ACTOR_PROBES 0x16u      /* its probe list's address (word) */
#define K98_29 0x29u            /* set to $891B as it starts (word) */
#define K98_FRAMES 0x40u        /* frames left of a wait */
#define K98_STEPS 0x42u         /* frames left of a move; throws left (the $07 of each kind $A5) */
#define K98_RIGHT 0x44u         /* 0: it flies right across next, else left */
#define K98_HIT 0x46u           /* nonzero while hit: its wait does not hurt or turn shots */
#define K98_TURNS 0x48u         /* nonzero: the next turn at the left edge spawns kind $23 */
#define K98_HITS 0x11u          /* hits left (6 as it starts); counted down by the hit code */
#define K98_PROBES 0xD90Au      /* (data bank $80) its probes */
#define K98_DONE 0x1B14u        /* cleared as it starts, counted up when it is beaten */
#define SCROLL_X_HELD 0x1A10u   /* nonzero: no follow across (word) */
#define SCROLL_Y_HELD 0x1A12u   /* the same up and down (word) */
#define CUE_BD 0x00BDu          /* the scene's cue: set when it is beaten */
#define K98_TOP 0x38            /* the y it bobs at (pixels) */
#define K98_RANDOM_LIST 0x3E    /* RandomFromList's list: its way, 0 or 2 */
#define K98_WAYS 0xA193u        /* (bank $84) its ways by the pick */
#define K98_DROP_KIND 0x9A
#define K98_SPAWN_KIND 0x23
#define K98_THROW_KIND 0xA5

/* A JSL at `site` to fn, its return address on the stack while fn runs, as
   the original's (frame_step.c's model). */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* `frames` frames (BRK at brk_at) of ActorBobY (its JSL at brk_at + 2),
   counted down in `count`. */
static void bob_for(uint8_t frames, uint32_t count, uint32_t brk_at)
{
    lda8_v(frames);
    sta8(ea_dp(count));
    do {
        actor_yield(brk_at);
        jsl_call(brk_at + 2, abi_ActorBobY);
        MEM8(dec, ea_dp(count));
    } while (!IF_Z);
}

/* `frames` frames (BRK at brk_at) of ActorMoveX and ActorBobY, counted
   down in K98_STEPS. */
static void move_for(uint8_t frames, uint32_t brk_at)
{
    lda8_v(frames);
    sta8(ea_dp(K98_STEPS));
    do {
        actor_yield(brk_at);
        jsl_call(brk_at + 2, abi_ActorMoveX);
        jsl_call(brk_at + 6, abi_ActorBobY);
        MEM8(dec, ea_dp(K98_STEPS));
    } while (!IF_Z);
}

/* Its speed across $0180 along facing K98_RIGHT (84:A197 and 84:A35A). */
static void speed_across(void)
{
    lda8(ea_dp(K98_RIGHT));
    sta8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_SPEED_X + 2));
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_SPEED_X + 1));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_SPEED_X));
}

void abi_Kind98Cycle(void);
void abi_Kind98Flight(void);
void abi_Kind98StartSpawn(void);
void abi_Kind98StartEnd(void);

/*
 * @routine 84:A167 Kind98Cycle
 * @actor
 * WHAT IT DOES: Kind $98's cycle: back at the top, it bobs $60 frames and
 * by a random pick flies across (dropping a kind $9A) or spawns a kind $23.
 * HOW: y K98_TOP (fraction 0), $49 0, ActorSpeedYClear, gravity 6,
 * ActorSpeedXClear; $60 frames (BRK) ActorBobY; RandomFromList with X =
 * K98_RANDOM_LIST, JMP (K98_WAYS,X): way 2 is Kind98StartSpawn; way 0:
 * facing K98_RIGHT, speed across $0180; $20 frames ActorMoveX and
 * ActorBobY; AnimStart 3, SoundQueueAdd $2F; $10 frames; ActorSpawnFacing
 * kind $9A; $20 frames; AnimStart 2; on into Kind98Flight.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached from
 * Kind98Start's end and by Kind98StartEnd's jump (84:A2A3). Exit: never
 * returns.
 */
void abi_Kind98Cycle(void)
{
    lda8_v(K98_TOP);
    sta8(ea_dp(ACTOR_Y + 1));
    stz8(ea_dp(ACTOR_Y));
    stz8(ea_dp(ACTOR_49));
    jsl_call(0x84A16Fu, abi_ActorSpeedYClear);
    lda8_v(0x06);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    jsl_call(0x84A177u, abi_ActorSpeedXClear);
    bob_for(0x60, K98_FRAMES, 0x84A17Fu);
    ldx8_v(K98_RANDOM_LIST);
    jsl_call(0x84A18Bu, abi_RandomFromList);
    tax8();
    switch (cpu.x) {  /* 84A190 jmp (K98_WAYS,x) */
    case 0: break;
    case 2: abi_Kind98StartSpawn(); return;
    default:  /* past the table: its next bytes */
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(K98_WAYS + cpu.x)));
        return;
    }
    speed_across();
    move_for(0x20, 0x84A1A9u);
    lda8_v(0x03);
    jsl_call(0x84A1B9u, abi_AnimStart);
    lda8_v(0x2F);
    jsl_call(0x84A1BFu, abi_SoundQueueAdd);
    move_for(0x10, 0x84A1C7u);
    lda8_v(K98_DROP_KIND);
    jsl_call(0x84A1D7u, abi_ActorSpawnFacing);
    move_for(0x20, 0x84A1DFu);
    lda8_v(0x02);
    jsl_call(0x84A1EFu, abi_AnimStart);
    abi_Kind98Flight();
}

/*
 * @routine 84:A1F3 Kind98Flight
 * @actor
 * WHAT IT DOES: Kind $98's flight across: each frame it moves and bobs
 * until it reaches the far side, then turns (Kind98StartEnd), or spawns a
 * kind $23 first (Kind98StartSpawn) when it last turned after a hit.
 * HOW: Each frame (BRK first) ActorMoveX and ActorBobY until, K98_RIGHT
 * set, its x + $60 is below $20 (K98_RIGHT 0, JMP to Kind98StartEnd), or,
 * K98_RIGHT 0, at or past $E0 (K98_RIGHT up; K98_TURNS zero: JMP to
 * Kind98StartEnd; else on at Kind98StartSpawn). The JMP at 84:A20F, after
 * the first JMP, is never reached.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached from
 * Kind98Cycle's way 0 and by Kind98Hit's jump (84:A36A). Exit: never
 * returns.
 */
void abi_Kind98Flight(void)
{
    for (;;) {
        actor_yield(0x84A1F3u);
        jsl_call(0x84A1F5u, abi_ActorMoveX);
        jsl_call(0x84A1F9u, abi_ActorBobY);
        lda8(ea_dp(K98_RIGHT));
        if (!IF_Z) {
            lda8(ea_dp(ACTOR_X + 1));
            clc();
            adc8_v(0x60);
            cmp8_v(0x20);
            if (IF_C)
                continue;
            stz8(ea_dp(K98_RIGHT));
            abi_Kind98StartEnd();
            return;
            /* 84A20F jmp $A177: never reached */
        }
        lda8(ea_dp(ACTOR_X + 1));
        clc();
        adc8_v(0x60);
        cmp8_v(0xE0);
        if (!IF_C)
            continue;
        MEM8(inc, ea_dp(K98_RIGHT));
        lda8(ea_dp(K98_TURNS));
        if (!IF_Z) {
            abi_Kind98StartSpawn();
            return;
        }
        abi_Kind98StartEnd();
        return;
    }
}

/*
 * @routine 84:A224 Kind98StartSpawn
 * @actor
 * WHAT IT DOES: Kind $98's second way: it spawns a kind $23, then turns.
 * HOW: K98_TURNS 0; SoundQueueAdd $47; AnimStart 4; $10 frames (BRK)
 * ActorBobY; ActorSpawnFacing Y = 0 kind $23; $1A frames; AnimStart 2; JMP
 * to Kind98StartEnd.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * Kind98Cycle's JMP (K98_WAYS,X) and Kind98Flight's branch. Exit: never
 * returns.
 */
void abi_Kind98StartSpawn(void)
{
    stz8(ea_dp(K98_TURNS));
    lda8_v(0x47);
    jsl_call(0x84A228u, abi_SoundQueueAdd);
    lda8_v(0x04);
    jsl_call(0x84A22Eu, abi_AnimStart);
    bob_for(0x10, K98_FRAMES, 0x84A236u);
    ldy8_v(0x00);
    lda8_v(K98_SPAWN_KIND);
    jsl_call(0x84A244u, abi_ActorSpawnFacing);
    bob_for(0x1A, K98_FRAMES, 0x84A24Cu);
    lda8_v(0x02);
    jsl_call(0x84A258u, abi_AnimStart);
    abi_Kind98StartEnd();
}

/*
 * @routine 84:A25F Kind98StartEnd
 * @actor
 * WHAT IT DOES: Kind $98's turn: it bobs, faces the middle of the screen,
 * flies back up to its height and goes the other way next.
 * HOW: $20 frames (BRK) ActorBobY; SoundQueueAdd $50; facing 0, up if x +
 * $60 - $80 leaves no borrow; ActorMotionPreset Y = $70; AnimStart 2; each
 * frame (BRK first) ActorFlyAgainst until its y is at most K98_TOP (y
 * K98_TOP, fraction 0); K98_RIGHT ^= 1; JMP to Kind98Cycle.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the JMPs at 84:A20C, 84:A221 and 84:A25C. Exit: never returns.
 */
void abi_Kind98StartEnd(void)
{
    bob_for(0x20, K98_STEPS, 0x84A263u);
    lda8_v(0x50);
    jsl_call(0x84A26Fu, abi_SoundQueueAdd);
    stz8(ea_dp(ACTOR_FACING));
    lda8(ea_dp(ACTOR_X + 1));
    clc();
    adc8_v(0x60);
    sec();
    sbc8_v(0x80);
    if (IF_C)
        MEM8(inc, ea_dp(ACTOR_FACING));
    ldy8_v(0x70);
    jsl_call(0x84A283u, abi_ActorMotionPreset);
    lda8_v(0x02);
    jsl_call(0x84A289u, abi_AnimStart);
    do {
        actor_yield(0x84A28Du);
        jsl_call(0x84A28Fu, abi_ActorFlyAgainst);
        lda8_v(K98_TOP);
        cmp8_v(rd8(ea_dp(ACTOR_Y + 1)));
    } while (!IF_C);
    sta8(ea_dp(ACTOR_Y + 1));
    stz8(ea_dp(ACTOR_Y));
    lda8(ea_dp(K98_RIGHT));
    eor8_v(0x01);
    sta8(ea_dp(K98_RIGHT));
    abi_Kind98Cycle();
}

/*
 * @routine 84:A112 Kind98Start
 * @actor
 * WHAT IT DOES: Actor kind $98: holds the screen still, waits $80 frames,
 * then over and over bobs at the top for $60 frames and by a random pick
 * either flies across (dropping a kind $9A on the way) or, when it last
 * turned after a hit, spawns a kind $23; then bobs $20 frames, turns to
 * face the middle of the screen and flies back up to its height, and goes
 * the other way next.
 * HOW: K98_HITS 6; K98_DONE and K98_TURNS 0; both scroll holds 1; COP $80;
 * $08 bits 0-1 and $09 bits $90 set; 16-bit probes K98_PROBES and K98_29
 * $891B; its x pixels $80, y pixels K98_TOP; K98_RIGHT 1, K98_HIT and
 * facing 0; SoundQueueAdd $46; AnimStart 0; COP $20; AnimStart 2; on into
 * Kind98Cycle (84:A167), whose ways go on in Kind98Flight,
 * Kind98StartSpawn and Kind98StartEnd.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop for kind $98 in state $0C (the stub at $83:93BE). Exit:
 * never returns.
 */
void abi_Kind98Start(void)
{
    lda8_v(0x06);
    sta8(ea_dp(K98_HITS));
    stz8(ea_abs(K98_DONE));
    stz8(ea_dp(K98_TURNS));
    lda8_v(0x01);
    sta8(ea_abs(SCROLL_X_HELD));
    sta8(ea_abs(SCROLL_Y_HELD));
    lda8_v(0x80);
    actor_wait(0x84A125u);
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x03);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    op_rep(0x20);
    lda16_v(K98_PROBES);
    sta16(ea_dp(ACTOR_PROBES));
    lda16_v(0x891B);
    sta16(ea_dp(K98_29));
    op_sep(0x20);
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_X + 1));
    lda8_v(K98_TOP);
    sta8(ea_dp(ACTOR_Y + 1));
    lda8_v(0x01);
    sta8(ea_dp(K98_RIGHT));
    stz8(ea_dp(K98_HIT));
    stz8(ea_dp(ACTOR_FACING));
    lda8_v(0x46);
    jsl_call(0x84A153u, abi_SoundQueueAdd);
    lda8_v(0x00);
    jsl_call(0x84A159u, abi_AnimStart);
    lda8_v(0x20);
    actor_wait(0x84A15Fu);
    lda8_v(0x02);
    jsl_call(0x84A163u, abi_AnimStart);
    abi_Kind98Cycle();
}

/*
 * @routine 84:A2A6 Kind98Hit
 * @actor
 * WHAT IT DOES: Kind $98 hit: sound $0F and its hit pose for 8 frames.
 * With no hits left it is beaten: the scene's cue, K98_DONE up, its last
 * pose with sound $46, $30 frames and it ends. Else it falls to the
 * ground, shows its hurt pose $30 frames, rises back to the top, bobs $10
 * frames, throws three kind $A5 ($10 and $30 frames about each) and flies
 * across again, to the right or left as before, spawning kind $23 at the
 * next turn.
 * HOW: K98_HIT up; $09 bit 7 set (TSB); SoundQueueAdd $0F; AnimStart 5;
 * COP 8. K98_HITS zero: CUE_BD 1, K98_DONE up, AnimStart 1, SoundQueueAdd
 * $46, COP $30, JML ActorEndPlaced. Else ActorSpeedXClear;
 * ActorSpeedYSetGravity Y = $8D, A = K98_TOP; each frame (BRK) ActorFly
 * and ActorOutOfGround until that raises it; AnimStart 6; COP $30; $08 bit
 * 4 set (TSB); AnimStart 2; ActorSpeedYSet Y = $90; each frame ActorMoveY
 * until its y is at most K98_TOP; y K98_TOP, $49 0, ActorSpeedYClear,
 * gravity 6, $08 bit 4 cleared, K98_HIT 0; $10 frames ActorBobY. K98_STEPS
 * 3: each time AnimStart 7, $10 frames ActorBobY, Kind98Throw, $30
 * frames. AnimStart 2; state $0C; facing K98_RIGHT, speed across $0180;
 * K98_TURNS up; JMP to the flight (84:A1F3) in Kind98Start.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop for kind $98 hit (the stub at $83:93C7). Exit: never
 * returns.
 */
void abi_Kind98Hit(void)
{
    MEM8(inc, ea_dp(K98_HIT));
    lda8_v(0x80);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x0F);
    jsl_call(0x84A2AEu, abi_SoundQueueAdd);
    lda8_v(0x05);
    jsl_call(0x84A2B4u, abi_AnimStart);
    lda8_v(0x08);
    actor_wait(0x84A2BAu);
    lda8(ea_dp(K98_HITS));
    if (IF_Z) {
        lda8_v(0x01);
        sta8(ea_abs(CUE_BD));
        MEM8(inc, ea_abs(K98_DONE));
        lda8_v(0x01);
        jsl_call(0x84A377u, abi_AnimStart);
        lda8_v(0x46);
        jsl_call(0x84A37Du, abi_SoundQueueAdd);
        lda8_v(0x30);
        actor_wait(0x84A383u);
        abi_ActorEndPlaced();
        return;
    }
    jsl_call(0x84A2C3u, abi_ActorSpeedXClear);
    ldy8_v(0x8D);
    lda8_v(K98_TOP);
    jsl_call(0x84A2CBu, abi_ActorSpeedYSetGravity);
    do {
        actor_yield(0x84A2CFu);
        jsl_call(0x84A2D1u, abi_ActorFly);
        jsl_call(0x84A2D5u, abi_ActorOutOfGround);
    } while (IF_Z);
    lda8_v(0x06);
    jsl_call(0x84A2DDu, abi_AnimStart);
    lda8_v(0x30);
    actor_wait(0x84A2E3u);
    lda8_v(0x10);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x02);
    jsl_call(0x84A2EBu, abi_AnimStart);
    ldy8_v(0x90);
    jsl_call(0x84A2F1u, abi_ActorSpeedYSet);
    do {
        actor_yield(0x84A2F5u);
        jsl_call(0x84A2F7u, abi_ActorMoveY);
        lda8_v(K98_TOP);
        cmp8_v(rd8(ea_dp(ACTOR_Y + 1)));
    } while (!IF_C);
    sta8(ea_dp(ACTOR_Y + 1));
    stz8(ea_dp(ACTOR_49));
    jsl_call(0x84A305u, abi_ActorSpeedYClear);
    lda8_v(0x06);
    sta8(ea_dp(ACTOR_ACCEL_Y));
    lda8(ea_dp(ACTOR_08));
    and8_v(0xEF);
    sta8(ea_dp(ACTOR_08));
    stz8(ea_dp(K98_HIT));
    bob_for(0x10, K98_FRAMES, 0x84A319u);
    lda8_v(0x03);
    sta8(ea_dp(K98_STEPS));
    do {
        lda8_v(0x07);
        jsl_call(0x84A329u, abi_AnimStart);
        bob_for(0x10, K98_FRAMES, 0x84A331u);
        call_frame(0xA33Du, abi_Kind98Throw);
        bob_for(0x30, K98_FRAMES, 0x84A342u);
        MEM8(dec, ea_dp(K98_STEPS));
    } while (!IF_Z);
    lda8_v(0x02);
    jsl_call(0x84A352u, abi_AnimStart);
    lda8_v(0x0C);
    sta8(ea_dp(ACTOR_STATE));
    speed_across();
    MEM8(inc, ea_dp(K98_TURNS));
    abi_Kind98Flight();
}

/*
 * @routine 84:A389 Kind98Throw
 * WHAT IT DOES: Kind $98 throws a kind $A5, its $07 the throws left, with
 * sound $3E.
 * HOW: SoundQueueAdd $3E; $07 = K98_STEPS; spawn place 0;
 * ActorCreateChild8 with A = $A5.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR from Kind98Hit. Exit: M and X 8-bit; as ActorCreateChild8 leaves
 * A, X, Y and the flags.
 */
void abi_Kind98Throw(void)
{
    lda8_v(0x3E);
    jsl_call(0x84A38Bu, abi_SoundQueueAdd);
    lda8(ea_dp(K98_STEPS));
    sta8(ea_dp(ACTOR_07));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(K98_THROW_KIND);
    jsl_call(0x84A397u, abi_ActorCreateChild8);
}

/*
 * @routine 84:A39C Kind98WaitWork
 * WHAT IT DOES: What kind $98 does each frame it waits: its animation
 * steps and its picture is sent; it turns to Aladdin; unless hit, Aladdin
 * coming down on it or touching it, and his shots turning on it, are
 * dealt with.
 * HOW: AnimStep; ActorPictureSend; ActorSideOfPlayer into its flip ($15);
 * K98_HIT zero: ActorStompOrHurtLong, ActorTurnsShotBackLong.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSL
 * from the kind's stub ($83:93C2) through the actor loop's wait table.
 * Exit: RTL; as the last routine called leaves A, X, Y and the flags.
 */
void abi_Kind98WaitWork(void)
{
    jsl_call(0x84A39Cu, abi_AnimStep);
    jsl_call(0x84A3A0u, abi_ActorPictureSend);
    jsl_call(0x84A3A4u, abi_ActorSideOfPlayer);
    sta8(ea_dp(ACTOR_FLIP));
    lda8(ea_dp(K98_HIT));
    if (IF_Z) {
        jsl_call(0x84A3AEu, abi_ActorStompOrHurtLong);
        jsl_call(0x84A3B2u, abi_ActorTurnsShotBackLong);
    }
}
