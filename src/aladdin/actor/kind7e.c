/*
 * kind7e.c: actor kind $7E, the platforms Aladdin bounces on (stage a4
 * s1): by its $07 one rises and falls in turn, drifts across and back
 * aslant, or is the hidden centre of two or three that circle it. Its wait
 * (the bounce) is Kind7EWaitWork in kind_waits_84.c.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorMotionReset(void);
void abi_ActorAccelerateX(void);
void abi_ActorAccelerateY(void);
void abi_ActorDecelerateX(void);
void abi_ActorDecelerateY(void);
void abi_ActorMoveX(void);
void abi_ActorStepYBack(void);
void abi_ActorStepY(void);
void abi_ActorMoveAlong(void);
void abi_ActorSpawn(void);
void abi_ActorEndPlaced(void);
void abi_ActorEndOutOfRangeLong(void);
void abi_AnimStart(void);
void abi_Kind7EStart(void);
void abi_Kind7EStep(void);
void abi_Kind7EShow(void);
void abi_Kind7ERing(void);

#define K7E_DOWN 0x40u          /* 0: it moves against its vertical speed (ActorStepYBack), else with it */
#define K7E_LEFT 0x41u          /* frames left of its glide; for a centre, children left to make, less one */
#define K7E_BOUNCE 0x42u        /* frames left of a bounce (the player held on it; Kind7EWaitWork) */
#define K7E_CENTRE_X 0x43u      /* a circler: the x it circles (pixels, word) */
#define K7E_CENTRE_Y 0x45u      /* and the y (pixels, word) */
#define K7E_STAYS 0x47u         /* nonzero: never freed out of range (a circler) */
#define K7E_RADIUS 0x48u        /* a circler's distance from its centre; a centre's for its children */
#define K7E_ANGLE 0x49u         /* a circler: its direction from the centre, 0-$1FF (word) */
#define K7E_CENTRE 0x4Bu        /* a circler: its centre's record (word) */
#define K7E_TURN 0x4Du          /* a circler: 0 its angle counts up, 1 down */
#define K7E_GLIDE 0x4Eu         /* frames it glides at full speed */
#define K7E_NEXT 0x4Fu          /* a centre: the entry of K7E_ANGLES its next child starts at */
#define K7E_ANGLES 0xD828u      /* (data bank $80) the children's first angles (words) */
#define K7E_WAYS 0x91AEu        /* in bank $84: Kind7EStart's ways by $07 (words) */
#define K7E_GRAVITY 0x06        /* its vertical acceleration (ActorMotionReset) */
#define K7E_TOP_SPEED 0xC0      /* the vertical speed's fraction at which it stops speeding up */
#define K7E_TURN_WAIT 0x20      /* frames it waits each time it turns */
#define K7E_RANGE_BOX 0x00      /* a centre: its box for ActorEndOutOfRangeLong */

/*
 * @routine 83:92A8 Kind7EEnter
 * @actor
 * WHAT IT DOES: Where actors of kind $7E go on from the actor loop in
 * their start state (the table at $83:8CAB): Kind7EStart.
 * HOW: JML to Kind7EStart ($84:91A5).
 * Entry: D = the actor's record; reached by the actor loop's jump (JMP
 * (table,X)) by kind. Exit: never returns: as Kind7EStart.
 */
void abi_Kind7EEnter(void)
{
    abi_Kind7EStart();
}

/* $84:91D1 on, for good: speed up to the top speed, glide, slow to a
   stop, turn and wait; again. */
static void rise_and_fall(void)
{
    for (;;) {
        for (;;) {
            actor_yield(0x8491D1u);
            abi_ActorAccelerateX();
            abi_ActorAccelerateY();
            lda8(ea_dp(ACTOR_SPEED_Y));
            cmp8_v(K7E_TOP_SPEED);
            if (IF_C)
                break;
            call_frame(0x91E3u, abi_Kind7EStep);
        }
        lda8(ea_dp(K7E_GLIDE));
        sta8(ea_dp(K7E_LEFT));
        do {
            actor_yield(0x8491EAu);
            call_frame(0x91EEu, abi_Kind7EStep);
            MEM8(dec, ea_dp(K7E_LEFT));
        } while (!IF_Z);
        for (;;) {
            actor_yield(0x8491F3u);
            abi_ActorDecelerateX();
            abi_ActorDecelerateY();
            lda8(ea_dp(ACTOR_SPEED_Y));
            if (IF_Z)
                break;
            call_frame(0x9203u, abi_Kind7EStep);
        }
        lda8(ea_dp(K7E_DOWN));
        eor8_v(0x01);
        sta8(ea_dp(K7E_DOWN));
        lda8(ea_dp(ACTOR_FACING));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
        lda8_v(K7E_TURN_WAIT);
        actor_wait(0x849214u);
    }
}

/* $07 0 ($84:91C0). */
static void rise(void)
{
    call_frame(0x91C2u, abi_Kind7EShow);
    lda8_v(K7E_GRAVITY);
    abi_ActorMotionReset();
    stz8(ea_dp(K7E_DOWN));
    stz8(ea_dp(K7E_BOUNCE));
    lda8_v(0x36);
    sta8(ea_dp(K7E_GLIDE));
    rise_and_fall();
}

/* $07 1-4, its $07 already the children's ($84:9222, $84:9245 on): the
   children made, then for good it ends once out of range ($84:9231). */
static void ring(uint8_t radius, uint8_t left, uint8_t next)
{
    lda8_v(radius);
    sta8(ea_dp(K7E_RADIUS));
    lda8_v(left);
    sta8(ea_dp(K7E_LEFT));
    lda8_v(next);
    sta8(ea_dp(K7E_NEXT));
    call_frame(0x9230u, abi_Kind7ERing);
    for (;;) {
        actor_yield(0x849231u);
        ldx8_v(K7E_RANGE_BOX);
        jsl_frame(0x849235u);
        abi_ActorEndOutOfRangeLong();
        cpu.s = (uint16_t)(cpu.s + 3);
    }
}

/* $07 5 and 6, the facing set ($84:9255 on). */
static void aslant(void)
{
    call_frame(0x9257u, abi_Kind7EShow);
    stz8(ea_dp(K7E_DOWN));
    stz8(ea_dp(K7E_BOUNCE));
    lda8_v(0x44);
    sta8(ea_dp(K7E_GLIDE));
    lda8_v(K7E_GRAVITY);
    abi_ActorMotionReset();
    lda8_v(0x03);
    sta8(ea_dp(ACTOR_ACCEL_X));
    rise_and_fall();
}

/* $07 7 and 8, K7E_TURN set ($84:927B on): circles its centre's place
   until the centre's record is free. */
static void circle(void)
{
    call_frame(0x927Du, abi_Kind7EShow);
    stz8(ea_dp(K7E_BOUNCE));
    MEM8(inc, ea_dp(K7E_STAYS));
    for (;;) {
        actor_yield(0x849282u);
        op_rep(0x10);
        ldx16(ea_dp(K7E_CENTRE));
        lda8(ea_absx(ACTOR_STATE));
        if (IF_Z)
            break;
        ldx16(ea_dp(K7E_CENTRE_X));
        stx16(ea_dp(ACTOR_X + 1));
        stz8(ea_dp(ACTOR_X));
        ldx16(ea_dp(K7E_CENTRE_Y));
        stx16(ea_dp(ACTOR_Y + 1));
        stz8(ea_dp(ACTOR_Y));
        ldy16(ea_dp(K7E_ANGLE));
        lda8(ea_dp(K7E_RADIUS));
        abi_ActorMoveAlong();
        lda8(ea_dp(K7E_TURN));
        ACC8(lsr);
        op_rep(0x20);
        lda16(ea_dp(K7E_ANGLE));
        ACC16(inc);
        if (IF_C) {
            ACC16(dec);
            ACC16(dec);
        }
        and16_v(0x01FF);
        sta16(ea_dp(K7E_ANGLE));
        op_sep(0x20);
    }
    op_sep(0x10);
    abi_ActorEndPlaced();
}

/*
 * @routine 84:91A5 Kind7EStart
 * @actor
 * WHAT IT DOES: Actor kind $7E, a platform Aladdin bounces on, by its $07:
 * 0 rises, glides, slows to a stop, waits 32 frames and comes back down,
 * and so on for good; 5 and 6 do the same aslant, rightward or leftward
 * first; 1 to 4 make two or three children of kind $7E ($07 7 or 8) set
 * round it, and stay unseen where they were placed until out of range;
 * 7 and 8 (the children) circle their centre's place, one way or the
 * other, until the centre's record is free.
 * HOW: K7E_STAYS 0; JMP through the table at $84:91AE by $07 times 2.
 * 0: Kind7EShow, ActorMotionReset with gravity 6, K7E_DOWN and
 * K7E_BOUNCE 0, K7E_GLIDE $36. 5 and 6: facing 0 or 1, Kind7EShow,
 * K7E_DOWN and K7E_BOUNCE 0, K7E_GLIDE $44, ActorMotionReset 6,
 * acceleration across 3. Then for good: each frame (BRK first)
 * ActorAccelerateX and ActorAccelerateY, Kind7EStep, until the vertical
 * speed's fraction reaches $C0; K7E_GLIDE frames (BRK first) of
 * Kind7EStep; each frame (BRK first) ActorDecelerateX and
 * ActorDecelerateY, Kind7EStep, until the fraction is 0; K7E_DOWN and the
 * facing flipped; COP for 32 frames.
 * 1 and 3: $07 = 7; 2 and 4: $07 = 8. 1 and 2: K7E_RADIUS $B2, three
 * children from entry 2 down; 3 and 4: K7E_RADIUS $74, two from entry 4
 * down (Kind7ERing). Then each frame (BRK first) ActorEndOutOfRangeLong
 * by box 0.
 * 7 and 8: K7E_TURN 0 or 1; Kind7EShow, K7E_BOUNCE 0, K7E_STAYS up. Each
 * frame (BRK first), 16-bit X: K7E_CENTRE's state 0: X 8-bit,
 * ActorEndPlaced. Else its place at K7E_CENTRE_X/Y (fractions 0), then
 * ActorMoveAlong direction K7E_ANGLE at K7E_RADIUS; the angle one up (or
 * down by K7E_TURN), AND $1FF.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $7E in state $0C (the table at $83:8CAB,
 * through the stub at $83:92A8). Exit: never returns.
 */
void abi_Kind7EStart(void)
{
    stz8(ea_dp(K7E_STAYS));
    lda8(ea_dp(ACTOR_07));
    ACC8(asl);
    tax8();
    switch (cpu.x) {
    case 0x00:
        rise();
        break;
    case 0x02:
    case 0x04:
        lda8_v(cpu.x == 0x02 ? 0x07 : 0x08);
        sta8(ea_dp(ACTOR_07));
        ring(0xB2, 0x02, 0x02);
        break;
    case 0x06:
    case 0x08:
        lda8_v(cpu.x == 0x06 ? 0x07 : 0x08);
        sta8(ea_dp(ACTOR_07));
        ring(0x74, 0x01, 0x04);
        break;
    case 0x0A:
        stz8(ea_dp(ACTOR_FACING));
        aslant();
        break;
    case 0x0C:
        lda8_v(0x01);
        sta8(ea_dp(ACTOR_FACING));
        aslant();
        break;
    case 0x0E:
        stz8(ea_dp(K7E_TURN));
        circle();
        break;
    case 0x10:
        lda8_v(0x01);
        sta8(ea_dp(K7E_TURN));
        circle();
        break;
    default:
        jump_routine(0x840000u | rd16(0x840000u | (uint16_t)(K7E_WAYS + cpu.x)));
        break;
    }
}

/*
 * @routine 84:92BC Kind7EStep
 * WHAT IT DOES: Kind $7E's step: across by its speed the way it faces, and
 * up or down by its vertical speed as K7E_DOWN says.
 * HOW: ActorMoveX; K7E_DOWN zero: ActorStepYBack, else ActorStepY.
 * Entry: M 8-bit; D = the actor's record; called by JSR from
 * Kind7EStart. Exit: as ActorStepYBack or ActorStepY leave them.
 */
void abi_Kind7EStep(void)
{
    abi_ActorMoveX();
    lda8(ea_dp(K7E_DOWN));
    if (IF_Z)
        abi_ActorStepYBack();
    else
        abi_ActorStepY();
}

/*
 * @routine 84:92CE Kind7EShow
 * WHAT IT DOES: Shows a kind $7E with animation 0, its wait (the bounce)
 * to run each frame it waits.
 * HOW: $08 bit 1 and $09 bits $90 set (TSB); AnimStart 0.
 * Entry: M 8-bit; D = the actor's record; called by JSR from
 * Kind7EStart. Exit: as AnimStart leaves them.
 */
void abi_Kind7EShow(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x00);
    abi_AnimStart();
}

/*
 * @routine 84:92DD Kind7ERing
 * WHAT IT DOES: Makes a kind $7E centre's children, K7E_LEFT + 1 of them,
 * each to circle the centre's place at K7E_RADIUS from its own first angle.
 * HOW: Each time: ActorSpawn with A = its kind; one made (N clear, X =
 * the child): its look ($2B, $2C) copied; 16-bit, K7E_CENTRE = this
 * record, K7E_ANGLE = K7E_ANGLES entry K7E_NEXT, K7E_CENTRE_X/Y = this
 * one's place; 8-bit, K7E_RADIUS copied. Then (made or not) K7E_NEXT
 * and K7E_LEFT down, again while K7E_LEFT is not negative.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR from Kind7EStart. Exit: M and X 8-bit; K7E_LEFT = $FF.
 */
void abi_Kind7ERing(void)
{
    do {
        lda8(ea_dp(ACTOR_06));
        abi_ActorSpawn();
        if (!IF_N) {
            lda8(ea_dp(ACTOR_LOOK));
            sta8(ea_absx(ACTOR_LOOK));
            lda8(ea_dp(ACTOR_LOOK + 1));
            sta8(ea_absx(ACTOR_LOOK + 1));
            op_rep(0x20);
            tdc();
            sta16(ea_absx(K7E_CENTRE));
            lda16(ea_dp(K7E_NEXT));
            and16_v(0x00FF);
            ACC16(asl);
            tay16();
            lda16(ea_absy(K7E_ANGLES));
            sta16(ea_absx(K7E_ANGLE));
            lda16(ea_dp(ACTOR_X + 1));
            sta16(ea_absx(K7E_CENTRE_X));
            lda16(ea_dp(ACTOR_Y + 1));
            sta16(ea_absx(K7E_CENTRE_Y));
            op_sep(0x20);
            lda8(ea_dp(K7E_RADIUS));
            sta8(ea_absx(K7E_RADIUS));
        }
        op_sep(0x30);
        MEM8(dec, ea_dp(K7E_NEXT));
        MEM8(dec, ea_dp(K7E_LEFT));
    } while (!IF_N);
}
