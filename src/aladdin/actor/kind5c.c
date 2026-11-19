/*
 * kind5c.c: actor kind $5C of level 8, the Genie's head that leads
 * Aladdin across: once he is right of x $08A0 it flies in to x $0900 and
 * then along a path (one by his lives) of points, bobbing about its line;
 * at each point it makes the next of its children (clouds, kinds $5D, $8B,
 * $8C) and, after the first, waits until he has landed on the last one;
 * at the path's end it flies off and ends. Out of view, it ends; with
 * Aladdin out of health it only bobs. Its wait is Kind5CWait and its bob
 * ActorBobAboutLine (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorMotionPreset(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_ActorFlyAgainst(void);
void abi_ActorCreateChild(void);
void abi_ActorEndPlaced(void);
void abi_ActorPictureSend(void);
void abi_ActorBobAboutLine(void);
void abi_Kind5CUnhittable(void);
void abi_Kind5CSpeedPick(void);
void abi_Kind5CMove(void);
void abi_Kind5CFace(void);
void abi_Kind5CPathByLives(void);

#define K5C_PATH 0x16u          /* its path: points of 4 bytes (x word, the child's kind, 0), $FFFF ends it (word) */
#define K5C_FIRST_DONE 0x18u    /* nonzero once its first child is made (it waits for the next ones) */
#define K5C_CHILD 0x37u         /* its last child's record (word) */
#define K5C_LINE 0x40u          /* the y (word) it bobs about */
#define K5C_TARGET 0x42u        /* the point it flies to: x pixels (word) */
#define K5C_TOP_SPEED 0x44u     /* its speed's limit by distance (word) */
#define K5C_SPEED 0x46u         /* its speed across (word: fraction, pixels) */
#define K5C_STEP 0x48u          /* its speed's change a frame by distance */
#define K5C_HEADING 0x49u       /* the way it moves: 0 right, 1 left */
#define K5C_AT 0x4Au            /* its point's offset in the path */
#define K5C_GONE 0x4Eu          /* counted up as it ends */
#define K5C_LIVES 0x0364u       /* Aladdin's lives left */
#define K5C_BOB 0x13u           /* ActorBobAboutLine's state */
#define K5C_CHILD_LANDED 0x40u  /* a child's: nonzero once Aladdin has landed on it */
#define K5C_START_X 0x08A0u     /* Aladdin's x it waits for, and its own x as it comes in */
#define K5C_IN_X 0x0900u        /* the x it flies in to */
#define K5C_LOOK 0x890Bu        /* its K5C_29 (word) */
#define K5C_29 0x29u            /* set to K5C_LOOK at its start; purpose not yet established (word) */
#define K5C_PATHS 0xD428u       /* bank $80: its path by Kind5CPathByLives (words) */
#define K5C_SPEEDS 0xD408u      /* bank $80: by distance / 32, its speed limit (word) and step */
#define K5C_IN_PRESET 0x45u     /* ActorMotionPreset's preset for its flight in */
#define K5C_OUT_PRESET 0x46u    /* and for its flight off */
#define K5C_TURN_STEP 0x08u     /* its speed's change a frame while it turns about */
#define K5C_NEAR 0x0080u        /* distances from here on take the farthest speed */
#define K5C_FAR 0x0070u         /* the distance they are taken as */
#define K5C_TAKEN 0x000Cu       /* Kind5CMove: what it takes from the speed */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:E000 Kind5CStart
 * @actor
 * WHAT IT DOES: Actor kind $5C: waits until Aladdin is right of x $08A0,
 * then comes in at that x above the view and flies in to x $0900; there it
 * bobs about its line and follows its path (by his lives): to each point,
 * then (from the second point on, once he has landed on its last child)
 * its poses, the point's child made (its $07 the point's number), the next
 * point; at the path's end it waits 176 frames, flies off until out of
 * view, and ends.
 * HOW: 16-bit: each frame (BRK first) until $08A0 is below his x; x
 * pixels $08A0, $29 $890B, y pixels $FFFF; 8-bit: $12 $18, $08 2,
 * K5C_SPEED, K5C_STEP, K5C_HEADING, K5C_GONE and K5C_FIRST_DONE 0;
 * K5C_PATH from K5C_PATHS by Kind5CPathByLives; K5C_AT 0; $21 $50, $09
 * $90, redraw; ActorMotionPreset $45, AnimStart 0; each frame (BRK first)
 * ActorFlyAgainst until its x is $0900 or more. AnimStart 1; flip 1;
 * K5C_LINE its y; K5C_BOB up; COP $3F. Each point: K5C_TARGET from it;
 * each frame (BRK first) Kind5CFace, Kind5CSpeedPick, Kind5CMove until its
 * x reaches it. First point: K5C_FIRST_DONE up; else each frame (BRK
 * first) until its child's K5C_CHILD_LANDED is set. AnimStart 2, COP $0F,
 * AnimStart 3; each frame (BRK first) until ActorCreateChild with the
 * point's kind makes one ($07 = K5C_AT / 4): K5C_CHILD it (16-bit X
 * stored), the child's $37-$38 ours, its K5C_CHILD_LANDED 0; COP $0F; the
 * next point, until its x word is $FFFF. Then COP $B0, K5C_BOB 0,
 * AnimStart 0, ActorMotionPreset $46; each frame (BRK first)
 * ActorFlyAgainst while $09 bit 6 is set; Kind5CUnhittable, COP $FF,
 * K5C_GONE up, JMP ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $5C in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind5CStart(void)
{
    op_rep(0x20);
    do {
        actor_yield(0x83E002u);
        lda16_v(K5C_START_X);
        cmp16_v(rd16(ea_abs(PLAYER + ACTOR_X + 1)));
    } while (IF_C);
    sta16(ea_dp(ACTOR_X + 1));
    lda16_v(K5C_LOOK);
    sta16(ea_dp(K5C_29));
    lda16_v(0xFFFF);
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    lda8_v(0x18);
    sta8(ea_dp(ACTOR_12));
    lda8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    stz8(ea_dp(K5C_SPEED));
    stz8(ea_dp(K5C_SPEED + 1));
    stz8(ea_dp(K5C_STEP));
    stz8(ea_dp(K5C_HEADING));
    stz8(ea_dp(K5C_GONE));
    stz8(ea_dp(K5C_FIRST_DONE));
    call_frame(0xE030, abi_Kind5CPathByLives);
    ACC8(asl);
    tax8();
    lda8(ea_absx(K5C_PATHS));
    sta8(ea_dp(K5C_PATH));
    lda8(ea_absx(K5C_PATHS + 1));
    sta8(ea_dp(K5C_PATH + 1));
    stz8(ea_dp(K5C_AT));
    lda8_v(0x50);
    sta8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0xFF);
    sta8(ea_dp(ACTOR_REDRAW));
    ldy8_v(K5C_IN_PRESET);
    jsl_call(0x83E04Du, abi_ActorMotionPreset);
    lda8_v(0x00);
    jsl_call(0x83E053u, abi_AnimStart);
    do {
        actor_yield(0x83E057u);
        jsl_call(0x83E059u, abi_ActorFlyAgainst);
        op_rep(0x20);
        lda16(ea_dp(ACTOR_X + 1));
        cmp16_v(K5C_IN_X);
        op_sep(0x20);
    } while (!IF_C);
    lda8_v(0x01);
    jsl_call(0x83E06Au, abi_AnimStart);
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_FLIP));
    lda8(ea_dp(ACTOR_Y + 1));
    sta8(ea_dp(K5C_LINE));
    lda8(ea_dp(ACTOR_Y + 2));
    sta8(ea_dp(K5C_LINE + 1));
    MEM8(inc, ea_dp(K5C_BOB));
    lda8_v(0x3F);
    actor_wait(0x83E07Eu);
    do {
        /* to the point */
        ldy8(ea_dp(K5C_AT));
        lda8(ea_dpiy(K5C_PATH));
        sta8(ea_dp(K5C_TARGET));
        iny8();
        lda8(ea_dpiy(K5C_PATH));
        sta8(ea_dp(K5C_TARGET + 1));
        for (;;) {
            actor_yield(0x83E08Bu);
            call_frame(0xE08F, abi_Kind5CFace);
            call_frame(0xE092, abi_Kind5CSpeedPick);
            call_frame(0xE095, abi_Kind5CMove);
            ldy8(ea_dp(K5C_AT));
            op_rep(0x20);
            lda16(ea_dp(ACTOR_X + 1));
            cmp16_v(rd16(ea_dpiy(K5C_PATH)));
            op_sep(0x20);
            if (IF_C)
                break;
            op_sep(0x20);
        }
        lda8(ea_dp(K5C_FIRST_DONE));
        if (IF_Z) {
            MEM8(inc, ea_dp(K5C_FIRST_DONE));
        } else {
            do {
                actor_yield(0x83E0AEu);
                op_rep(0x10);
                ldx16(ea_dp(K5C_CHILD));
                lda8(ea_absx(K5C_CHILD_LANDED));
                op_sep(0x10);
            } while (IF_Z);
        }
        /* the point's child */
        lda8_v(0x02);
        jsl_call(0x83E0BDu, abi_AnimStart);
        lda8_v(0x0F);
        actor_wait(0x83E0C3u);
        lda8_v(0x03);
        jsl_call(0x83E0C7u, abi_AnimStart);
        do {
            actor_yield(0x83E0CBu);
            lda8(ea_dp(K5C_AT));
            pha8();
            ACC8(lsr);
            ACC8(lsr);
            sta8(ea_dp(ACTOR_07));
            ply8();
            iny8();
            iny8();
            lda8(ea_dpiy(K5C_PATH));
            jsl_call(0x83E0D9u, abi_ActorCreateChild);
        } while (IF_C);
        stx16(ea_dp(K5C_CHILD));
        tdc();
        sta8(ea_absx(K5C_CHILD));
        xba();
        sta8(ea_absx(K5C_CHILD + 1));
        stz8(ea_absx(K5C_CHILD_LANDED));
        op_sep(0x10);
        lda8_v(0x0F);
        actor_wait(0x83E0F0u);
        ldy8(ea_dp(K5C_AT));
        iny8();
        iny8();
        iny8();
        iny8();
        sty8(ea_dp(K5C_AT));
        lda8(ea_dpiy(K5C_PATH));
        cmp8_v(0xFF);
    } while (!IF_Z);
    /* the path's end: off and away */
    lda8_v(0xB0);
    actor_wait(0x83E102u);
    stz8(ea_dp(K5C_BOB));
    lda8_v(0x00);
    jsl_call(0x83E108u, abi_AnimStart);
    ldy8_v(K5C_OUT_PRESET);
    jsl_call(0x83E10Eu, abi_ActorMotionPreset);
    do {
        actor_yield(0x83E112u);
        jsl_call(0x83E114u, abi_ActorFlyAgainst);
        bit8_v(rd8(ea_dp(ACTOR_09)));
    } while (IF_V);
    call_frame(0xE11E, abi_Kind5CUnhittable);
    lda8_v(0xFF);
    actor_wait(0x83E121u);
    MEM8(inc, ea_dp(K5C_GONE));
    abi_ActorEndPlaced();
}

/*
 * @routine 83:E128 Kind5CGone
 * @actor
 * WHAT IT DOES: Kind $5C out of the view: it can no longer be hit, waits
 * 31 frames and ends for good.
 * HOW: Kind5CUnhittable; K5C_GONE up; COP $1F; JMP ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's RTI, Kind5CWait having made it its resume address.
 * Exit: never returns.
 */
void abi_Kind5CGone(void)
{
    call_frame(0xE12A, abi_Kind5CUnhittable);
    MEM8(inc, ea_dp(K5C_GONE));
    lda8_v(0x1F);
    actor_wait(0x83E12Fu);
    abi_ActorEndPlaced();
}

/*
 * @routine 83:E134 Kind5CUnhittable
 * WHAT IT DOES: Makes the actor one that cannot be hit and is not tested:
 * $09 bit 7 and $08 bit 3 cleared.
 * HOW: ASL then LSR of $09; $08 AND $F7.
 * Entry: M 8-bit; D = the actor's record; called by JSR. Exit: A = the
 * new $08; N and Z from it; C = bit 7 of $09 before.
 */
void abi_Kind5CUnhittable(void)
{
    MEM8(asl, ea_dp(ACTOR_09));
    MEM8(lsr, ea_dp(ACTOR_09));
    lda8(ea_dp(ACTOR_08));
    and8_v(0xF7);
    sta8(ea_dp(ACTOR_08));
}

/*
 * @routine 83:E13F Kind5CSpeedPick
 * WHAT IT DOES: Picks kind $5C's speed limit and step by its distance to
 * its point: the nearer, the slower.
 * HOW: 16-bit: |its x less K5C_TARGET|, $80 or more taken as $70; / 4
 * AND $1C picks the entry of K5C_SPEEDS: K5C_TOP_SPEED its word, K5C_STEP
 * its next byte.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; X = the entry's offset; Y = the step.
 */
void abi_Kind5CSpeedPick(void)
{
    op_rep(0x20);
    sec();
    lda16(ea_dp(ACTOR_X + 1));
    sbc16_v(rd16(ea_dp(K5C_TARGET)));
    if (IF_N) {
        eor16_v(0xFFFF);
        ACC16(inc);
    }
    cmp16_v(K5C_NEAR);
    if (IF_C)
        lda16_v(K5C_FAR);
    ACC16(lsr);
    ACC16(lsr);
    and16_v(0x001C);
    tax8();
    lda16(ea_absx(K5C_SPEEDS));
    sta16(ea_dp(K5C_TOP_SPEED));
    ldy8(ea_absx(K5C_SPEEDS + 2));
    sty8(ea_dp(K5C_STEP));
    op_sep(0x20);
}

/*
 * @routine 83:E167 Kind5CMove
 * WHAT IT DOES: Moves kind $5C across by its speed: heading the way it
 * faces, its speed grows by its step up to its limit (or drops to it);
 * facing the other way, its speed drops by 8 a frame until it turns
 * about.
 * HOW: 16-bit X. Facing ($14) as K5C_HEADING: limit below the speed:
 * the speed = the limit; else the speed + the step (16-bit by bytes), at
 * most the limit. Else (turning, $000C = 8) or above the limit ($000C =
 * the step): the speed less $000C; below 0: speed 0, K5C_HEADING the
 * facing. Then its 24-bit x plus the speed (heading right) or less it.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
 * M 8-bit, X 8-bit; A = its x's top byte; C from its addition.
 */
void abi_Kind5CMove(void)
{
    op_rep(0x10);
    lda8(ea_dp(ACTOR_FACING));
    cmp8_v(rd8(ea_dp(K5C_HEADING)));
    if (!IF_Z) {
        lda8_v(K5C_TURN_STEP);
        goto less;
    }
    ldx16(ea_dp(K5C_TOP_SPEED));
    cpx16_v(rd16(ea_dp(K5C_SPEED)));
    if (!IF_C) {
        lda8(ea_dp(K5C_STEP));
        goto less;
    }
    clc();
    lda8(ea_dp(K5C_SPEED));
    adc8_v(rd8(ea_dp(K5C_STEP)));
    sta8(ea_dp(K5C_SPEED));
    lda8(ea_dp(K5C_SPEED + 1));
    adc8_v(0x00);
    sta8(ea_dp(K5C_SPEED + 1));
    cpx16_v(rd16(ea_dp(K5C_SPEED)));
    if (!IF_C)
        stx16(ea_dp(K5C_SPEED));
    goto move;
less:
    sta8(ea_abs(K5C_TAKEN));
    sec();
    lda8(ea_dp(K5C_SPEED));
    sbc8_v(rd8(ea_abs(K5C_TAKEN)));
    sta8(ea_dp(K5C_SPEED));
    lda8(ea_dp(K5C_SPEED + 1));
    sbc8_v(0x00);
    sta8(ea_dp(K5C_SPEED + 1));
    lda8_v(0x00);
    sbc8_v(0x00);
    if (IF_N) {
        stz8(ea_dp(K5C_SPEED));
        stz8(ea_dp(K5C_SPEED + 1));
        lda8(ea_dp(ACTOR_FACING));
        sta8(ea_dp(K5C_HEADING));
    }
move:
    op_sep(0x10);
    lda8(ea_dp(K5C_HEADING));
    if (IF_Z) {
        clc();
        lda8(ea_dp(ACTOR_X));
        adc8_v(rd8(ea_dp(K5C_SPEED)));
        sta8(ea_dp(ACTOR_X));
        lda8(ea_dp(ACTOR_X + 1));
        adc8_v(rd8(ea_dp(K5C_SPEED + 1)));
        sta8(ea_dp(ACTOR_X + 1));
        lda8(ea_dp(ACTOR_X + 2));
        adc8_v(0x00);
        sta8(ea_dp(ACTOR_X + 2));
        return;
    }
    sec();
    lda8(ea_dp(ACTOR_X));
    sbc8_v(rd8(ea_dp(K5C_SPEED)));
    sta8(ea_dp(ACTOR_X));
    lda8(ea_dp(ACTOR_X + 1));
    sbc8_v(rd8(ea_dp(K5C_SPEED + 1)));
    sta8(ea_dp(ACTOR_X + 1));
    lda8(ea_dp(ACTOR_X + 2));
    sbc8_v(0x00);
    sta8(ea_dp(ACTOR_X + 2));
}

/*
 * @routine 83:E1DE Kind5CFace
 * WHAT IT DOES: Faces kind $5C toward its point: $14 0 when the point is
 * at or right of its x, 1 when left.
 * HOW: 16-bit: X 0; K5C_TARGET below its x pixels: X 1; STX $14.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
 * M 8-bit; X = the facing; C from the compare.
 */
void abi_Kind5CFace(void)
{
    op_rep(0x20);
    ldx8_v(0x00);
    lda16(ea_dp(K5C_TARGET));
    cmp16_v(rd16(ea_dp(ACTOR_X + 1)));
    if (!IF_C)
        inx8();
    stx8(ea_dp(ACTOR_FACING));
    op_sep(0x20);
}

/*
 * @routine 83:E22A Kind5CPathByLives
 * WHAT IT DOES: Kind $5C's path (and its children's) by Aladdin's lives:
 * 0 with one, 1 with two or three, 2 with four or more.
 * HOW: A 0; X = lives less one: zero, done; A up; below 3, done; A up.
 * Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: A = the path;
 * X = lives less one; N, Z and C from the last DEX, INC or CPX.
 */
void abi_Kind5CPathByLives(void)
{
    lda8_v(0x00);
    ldx8(ea_abs(K5C_LIVES));
    dex8();
    if (IF_Z)
        return;
    ACC8(inc);
    cpx8_v(0x03);
    if (!IF_C)
        return;
    ACC8(inc);
}

/*
 * @routine 83:E282 Kind5CDone
 * @actor
 * WHAT IT DOES: Kind $5C with Aladdin out of health: its pose 4, and only
 * its bob from then on, animated and drawn.
 * HOW: AnimStart 4, ActorPictureSend; each frame (BRK first)
 * ActorBobAboutLine, AnimStep, ActorPictureSend.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's RTI, Kind5CWait having made it its resume address.
 * Exit: never returns.
 */
void abi_Kind5CDone(void)
{
    lda8_v(0x04);
    jsl_call(0x83E284u, abi_AnimStart);
    jsl_call(0x83E288u, abi_ActorPictureSend);
    for (;;) {
        actor_yield(0x83E28Cu);
        call_frame(0xE290, abi_ActorBobAboutLine);
        jsl_call(0x83E291u, abi_AnimStep);
        jsl_call(0x83E295u, abi_ActorPictureSend);
    }
}
