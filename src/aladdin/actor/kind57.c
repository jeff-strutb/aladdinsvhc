/*
 * kind57.c: actor kind $57 of level 3, the guard at the market's end:
 * it waits until Aladdin is right of x $0528, shows itself, then in rounds
 * stands, and when he is near steps at him (charging and slashing) or
 * walks off; stomped it hops back and flashes, with no health left it is
 * knocked away; shot it stops a round. Its wait is Kind57Wait
 * (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);
void abi_ActorNearPlayerY(void);
void abi_ActorNearPlayerX(void);
void abi_SoundQueueAdd(void);
void abi_ActorSpeedXSet(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSnapToGround(void);
void abi_ActorHopFromTable(void);
void abi_ActorKnockedAway(void);
void abi_Kind57Round(void);
void abi_Kind57Hurt(void);

#define K57_HEALTH 0x11u        /* stomps left: 4 at its start */
#define K57_LOOKS 0x16u         /* its look table (word) */
#define K57_29 0x29u            /* set to K57_29_START at its start; purpose not yet established (word) */
#define K57_STEPS 0x40u         /* frames left of a move */
#define K57_TOUCH 0x42u         /* Kind57Wait: 0 touching hurts Aladdin; positive: stomp or hurt; negative: nothing */
#define K57_MODE 0x43u          /* Kind57Wait: 0 in play; 1 still; 2 up posed by facing */
#define K57_FACES 0x44u         /* Kind57Wait: nonzero, it turns to face Aladdin */
#define K57_LOOK_TABLE 0xD3B8u  /* its look table's address */
#define K57_29_START 0x8913u    /* K57_29's value at its start */
#define K57_PLAYER_X_HIGH 0x08E8u /* Aladdin's x pixels, high byte */
#define K57_PLAYER_X_LOW 0x08E7u  /* and low byte */
#define K57_HEALTH_P 0x0367u    /* Aladdin's health */
#define K57_C0 0x00C0u          /* nonzero: it goes on hurting (its hurt round again) */
#define K57_KNOCKED 0x00BDu     /* counted up as one is knocked away */
#define K57_NEAR_Y 0x08u        /* ActorNearPlayerY's span */
#define K57_NEAR_X 0x06u        /* ActorNearPlayerX's span */
#define K57_STEP_SPEED 0x3Fu    /* ActorSpeedXSet's entry for its step at him */
#define K57_WALK_SPEED 0x33u    /* and for its walk */
#define K57_STEP_SOUND 0x20u    /* the sound of its step at him */
#define K57_HURT_SOUND 0x21u    /* the sound of its hurt round */
#define K57_STOMP_SOUND 0x0Fu   /* the sound as it is stomped */
#define K57_HOP_SOUND 0x22u     /* the sound between its hops back */
#define K57_SHOT_SOUND 0x17u    /* the sound as it is shot */
#define K57_STOMPED 0x0Cu       /* its state after a stomp: started again */
#define K57_KNOCK_PICTURE 0x09u /* ActorKnockedAway's animation */
#define K57_KNOCK_SOUND 0x45u   /* its sound */
#define K57_KNOCK_PRESET 0x03u  /* its motion preset */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* K57_STEPS frames of moving (BRK at brk_at first): ActorMoveX,
   ActorOutOfWallProbe4, ActorSnapToGround (JSLs at brk_at + 2, + 6,
   + 10). */
static void k57_moves(uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        jsl_call(brk_at + 2, abi_ActorMoveX);
        jsl_call(brk_at + 6, abi_ActorOutOfWallProbe4);
        jsl_call(brk_at + 10, abi_ActorSnapToGround);
        MEM8(dec, ea_dp(K57_STEPS));
    } while (!IF_Z);
}

/*
 * @routine 83:D9BA Kind57Start
 * @actor
 * WHAT IT DOES: Actor kind $57: shows itself once Aladdin is right of x
 * $0528 (its poses 2 and 1), then goes on in its rounds.
 * HOW: $08 bit 1 and $09 bits 7 and 4 set; redraw; K57_HEALTH 4; its look
 * table $D3B8, $29 $8913; facing, flip, K57_TOUCH, K57_FACES, K57_MODE 0;
 * AnimStart 0. Each frame (BRK first) until Aladdin's x high byte is 5 and
 * its low byte $28 or more. COP $18, AnimStart 2, COP $28, AnimStart 1,
 * COP $90; on into Kind57Round.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $57 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind57Start(void)
{
    lda8(ea_dp(ACTOR_08));
    ora8_v(0x02);
    sta8(ea_dp(ACTOR_08));
    lda8(ea_dp(ACTOR_09));
    ora8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    lda8_v(0xFF);
    sta8(ea_dp(ACTOR_REDRAW));
    lda8_v(0x04);
    sta8(ea_dp(K57_HEALTH));
    op_rep(0x20);
    lda16_v(K57_LOOK_TABLE);
    sta16(ea_dp(K57_LOOKS));
    lda16_v(K57_29_START);
    sta16(ea_dp(K57_29));
    op_sep(0x20);
    stz8(ea_dp(ACTOR_FACING));
    stz8(ea_dp(ACTOR_FLIP));
    stz8(ea_dp(K57_TOUCH));
    stz8(ea_dp(K57_FACES));
    stz8(ea_dp(K57_MODE));
    lda8_v(0x00);
    jsl_call(0x83D9E8u, abi_AnimStart);
    for (;;) {
        actor_yield(0x83D9ECu);
        lda8(ea_abs(K57_PLAYER_X_HIGH));
        cmp8_v(0x05);
        if (!IF_Z)
            continue;
        lda8(ea_abs(K57_PLAYER_X_LOW));
        cmp8_v(0x28);
        if (IF_C)
            break;
    }
    lda8_v(0x18);
    actor_wait(0x83D9FEu);
    lda8_v(0x02);
    jsl_call(0x83DA02u, abi_AnimStart);
    lda8_v(0x28);
    actor_wait(0x83DA08u);
    lda8_v(0x01);
    jsl_call(0x83DA0Cu, abi_AnimStart);
    lda8_v(0x90);
    actor_wait(0x83DA12u);
    abi_Kind57Round();
}

/*
 * @routine 83:DA14 Kind57Round
 * @actor
 * WHAT IT DOES: A round of kind $57: it stands facing Aladdin (pose 3, 24
 * frames); then each frame while he has health it looks for him: near in
 * height and across it steps at him (poses 5 and 6, sound $20, 8 frames
 * of moving), rests 32 frames and starts a round again; near in height
 * only, it walks (pose 4, 48 frames) and starts a round again.
 * HOW: K57_FACES 1, K57_TOUCH 0; AnimStart 3; COP $18. Each frame (BRK
 * first) with Aladdin's health nonzero: ActorNearPlayerY 8 C set: the
 * walk; ActorNearPlayerX 6 C set: the walk; else K57_TOUCH 1, AnimStart
 * 5, COP $18, K57_FACES 0, AnimStart 6, SoundQueueAdd $20, K57_STEPS 8,
 * ActorSpeedXSet $3F; K57_STEPS frames (BRK first) of ActorMoveX,
 * ActorOutOfWallProbe4, ActorSnapToGround; COP $20; again. The walk:
 * K57_STEPS $30, ActorSpeedXSet $33, AnimStart 4, K57_FACES 0; K57_STEPS
 * frames of the same moves; again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached from
 * Kind57Start (on into it) and by jumps from its other states.
 * Exit: never returns.
 */
void abi_Kind57Round(void)
{
    for (;;) {
        lda8_v(0x01);
        sta8(ea_dp(K57_FACES));
        stz8(ea_dp(K57_TOUCH));
        lda8_v(0x03);
        jsl_call(0x83DA1Cu, abi_AnimStart);
        lda8_v(0x18);
        actor_wait(0x83DA22u);
        do {
            actor_yield(0x83DA24u);
            lda8(ea_abs(K57_HEALTH_P));
        } while (IF_Z);
        ldy8_v(K57_NEAR_Y);
        jsl_call(0x83DA2Du, abi_ActorNearPlayerY);
        if (!IF_C) {
            ldy8_v(K57_NEAR_X);
            jsl_call(0x83DA35u, abi_ActorNearPlayerX);
        }
        if (IF_C) {
            /* the walk */
            lda8_v(0x30);
            sta8(ea_dp(K57_STEPS));
            ldy8_v(K57_WALK_SPEED);
            jsl_call(0x83DA7Fu, abi_ActorSpeedXSet);
            lda8_v(0x04);
            jsl_call(0x83DA85u, abi_AnimStart);
            stz8(ea_dp(K57_FACES));
            k57_moves(0x83DA8Bu);
            continue;
        }
        lda8_v(0x01);
        sta8(ea_dp(K57_TOUCH));
        lda8_v(0x05);
        jsl_call(0x83DA41u, abi_AnimStart);
        lda8_v(0x18);
        actor_wait(0x83DA47u);
        stz8(ea_dp(K57_FACES));
        lda8_v(0x06);
        jsl_call(0x83DA4Du, abi_AnimStart);
        lda8_v(K57_STEP_SOUND);
        jsl_call(0x83DA53u, abi_SoundQueueAdd);
        lda8_v(0x08);
        sta8(ea_dp(K57_STEPS));
        ldy8_v(K57_STEP_SPEED);
        jsl_call(0x83DA5Du, abi_ActorSpeedXSet);
        k57_moves(0x83DA61u);
        lda8_v(0x20);
        actor_wait(0x83DA75u);
    }
}

/*
 * @routine 83:DAA0 Kind57Hurt
 * @actor
 * WHAT IT DOES: Kind $57 hurt: it walks back in pose $0C for 128 frames
 * with sound $21, again while $00C0 is set; then a round again.
 * HOW: K57_FACES 1, K57_TOUCH 0; ActorSpeedXSet $33; K57_STEPS $80;
 * SoundQueueAdd $21; AnimStart $0C; K57_STEPS frames (BRK first) of
 * ActorMoveX, ActorOutOfWallProbe4, ActorSnapToGround; $00C0 nonzero:
 * again; else JMP Kind57Round.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * jumps from its second and third states.
 * Exit: never returns.
 */
void abi_Kind57Hurt(void)
{
    do {
        lda8_v(0x01);
        sta8(ea_dp(K57_FACES));
        stz8(ea_dp(K57_TOUCH));
        ldy8_v(K57_WALK_SPEED);
        jsl_call(0x83DAA8u, abi_ActorSpeedXSet);
        lda8_v(0x80);
        sta8(ea_dp(K57_STEPS));
        lda8_v(K57_HURT_SOUND);
        jsl_call(0x83DAB2u, abi_SoundQueueAdd);
        lda8_v(0x0C);
        jsl_call(0x83DAB8u, abi_AnimStart);
        k57_moves(0x83DABCu);
        lda8(ea_abs(K57_C0));
    } while (!IF_Z);
    abi_Kind57Round();
}

/*
 * @routine 83:DB43 Kind57Second
 * @actor
 * WHAT IT DOES: Kind $57 stomped (its second state): with health left it
 * plays sound $0F, poses, hops back twice (sound $22 between), flashes 32
 * frames and goes on hurt (Kind57Hurt); with none left it is knocked away
 * (counted at $00BD).
 * HOW: K57_HEALTH zero: $00BD up, ActorKnockedAway with A = 9, X = $45,
 * Y = 3 (JMP). Else K57_MODE up; SoundQueueAdd $0F; $09 bit 7 set; state
 * $0C; AnimStart 7; COP 8; K57_MODE up; ActorHopFromTable $23,
 * SoundQueueAdd $22, ActorHopFromTable $28; K57_MODE down; K57_STEPS $20
 * frames (BRK first) of $08 bit 3 flipped; $08 bit 3 set; K57_MODE 0; JMP
 * Kind57Hurt.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $57 in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind57Second(void)
{
    lda8(ea_dp(K57_HEALTH));
    if (IF_Z) {
        MEM8(inc, ea_abs(K57_KNOCKED));
        lda8_v(K57_KNOCK_PICTURE);
        ldx8_v(K57_KNOCK_SOUND);
        ldy8_v(K57_KNOCK_PRESET);
        abi_ActorKnockedAway();
        return;
    }
    MEM8(inc, ea_dp(K57_MODE));
    lda8_v(K57_STOMP_SOUND);
    jsl_call(0x83DB4Bu, abi_SoundQueueAdd);
    lda8_v(0x80);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(K57_STOMPED);
    sta8(ea_dp(ACTOR_STATE));
    lda8_v(0x07);
    jsl_call(0x83DB59u, abi_AnimStart);
    lda8_v(0x08);
    actor_wait(0x83DB5Fu);
    MEM8(inc, ea_dp(K57_MODE));
    ldx8_v(0x23);
    call_frame(0xDB67, abi_ActorHopFromTable);
    lda8_v(K57_HOP_SOUND);
    jsl_call(0x83DB6Au, abi_SoundQueueAdd);
    ldx8_v(0x28);
    call_frame(0xDB72, abi_ActorHopFromTable);
    MEM8(dec, ea_dp(K57_MODE));
    lda8_v(0x20);
    sta8(ea_dp(K57_STEPS));
    do {
        actor_yield(0x83DB79u);
        lda8(ea_dp(ACTOR_08));
        eor8_v(0x08);
        sta8(ea_dp(ACTOR_08));
        MEM8(dec, ea_dp(K57_STEPS));
    } while (!IF_Z);
    lda8_v(0x08);
    tsb8(ea_dp(ACTOR_08));
    stz8(ea_dp(K57_MODE));
    abi_Kind57Hurt();
}

/*
 * @routine 83:DB9A Kind57Third
 * @actor
 * WHAT IT DOES: Kind $57 shot (its third state): sound $17; while $00C0
 * is set it goes on hurt; else it stands still in pose $0A for 32 frames,
 * touching him does nothing, and a round again.
 * HOW: SoundQueueAdd $17; $00C0 nonzero: JMP Kind57Hurt. K57_FACES 0,
 * K57_TOUCH $FF; AnimStart $0A; COP $20; JMP Kind57Round.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $57 in a state with bits 7 and 6 set
 * (the table at $83:90A1). Exit: never returns.
 */
void abi_Kind57Third(void)
{
    lda8_v(K57_SHOT_SOUND);
    jsl_call(0x83DB9Cu, abi_SoundQueueAdd);
    lda8(ea_abs(K57_C0));
    if (!IF_Z) {
        abi_Kind57Hurt();
        return;
    }
    stz8(ea_dp(K57_FACES));
    lda8_v(0xFF);
    sta8(ea_dp(K57_TOUCH));
    lda8_v(0x0A);
    jsl_call(0x83DBB0u, abi_AnimStart);
    lda8_v(0x20);
    actor_wait(0x83DBB6u);
    abi_Kind57Round();
}
