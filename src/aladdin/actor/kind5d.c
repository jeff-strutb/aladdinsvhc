/*
 * kind5d.c: actor kind $5D of level 8, a cloud the Genie's head
 * (kind $5C) makes: it rises to its height (by Aladdin's lives and its
 * number), leaving sparkles; when Aladdin lands on it, it tells its maker,
 * sinks under him and rises again, and again when he leaves it. Once its
 * maker is gone it starts its timed end. Its wait is Kind5DWait
 * (kind_waits_rest.c).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_AnimStart(void);
void abi_ActorMotionPreset(void);
void abi_PlayerLandOnActor(void);
void abi_SoundQueueAdd(void);
void abi_ActorKeepSpot(void);
void abi_ActorFlyAgainst(void);
void abi_ActorFly(void);
void abi_PlayerMoveWithActor(void);
void abi_ActorSpeedYSet(void);
void abi_ActorMoveY(void);
void abi_ActorCreateChild8(void);
void abi_Kind5CPathByLives(void);
void abi_Kind5DRise(void);
void abi_Kind5DParentCheck(void);
void abi_Kind5DLandTest(void);
void abi_Kind5DSinkStep(void);
void abi_Kind5DRiseStep(void);
void abi_Kind5DParentGone(void);

#define K5D_PARENT 0x37u        /* its maker's record (word) */
#define K5D_LANDED 0x40u        /* counted up when Aladdin first lands on it (its maker waits for it) */
#define K5D_COUNT 0x41u         /* Kind5DRise: frames to its next sparkle */
#define K5D_HEIGHT_AT 0x42u     /* Kind5DRise: its height's offset in K5D_HEIGHTS */
#define K5D_BACK 0x43u          /* Kind5DRise: its caller's return address while it rises (3 bytes) */
#define K5D_47 0x47u            /* cleared as it rises; purpose not yet established */
#define K5D_ON 0x48u            /* nonzero when Aladdin landed on it this frame */
#define K5D_WAS_ON 0x49u        /* K5D_ON as the frame before (Kind5DWait keeps it) */
#define K5D_GO 0x4Au            /* nonzero once he has landed: Kind5DWait starts its end */
#define K5D_STATE 0x13u         /* Kind5DWait's step: 2 starts its end at once */
#define K5D_PARENT_GONE 0x4Eu   /* its maker's: nonzero once it has gone */
#define K5D_HEIGHTS 0xD4A8u     /* bank $80: by path the offset of its heights, there by twice its number the y it rises to (words) */
#define K5D_BOX 0x50u           /* PlayerLandOnActor's box */
#define K5D_LAND_SOUND 0x29u    /* the sound as he lands on it */
#define K5D_RISE_SOUND 0x28u    /* the sound as it starts to rise */
#define K5D_RISE_SPEED 0x45u    /* ActorSpeedYSet's entry for its rise */
#define K5D_SINK 0x49u          /* ActorMotionPreset's preset as it sinks under him */
#define K5D_BACK_UP 0x4Au       /* and as it rises again */
#define K5D_SPARKLE 0x5Fu       /* the kind of its sparkles */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/* Kind5DStart's wait for a change in Aladdin's landing: each frame (BRK
   at brk_at first) Kind5DParentCheck and Kind5DLandTest (JSRs at
   brk_at + 2 and + 5) until K5D_ON differs from K5D_WAS_ON. */
static void k5d_wait_change(uint32_t brk_at)
{
    do {
        actor_yield(brk_at);
        call_frame((uint16_t)(brk_at + 4), abi_Kind5DParentCheck);
        call_frame((uint16_t)(brk_at + 7), abi_Kind5DLandTest);
        lda8(ea_dp(K5D_ON));
        cmp8_v(rd8(ea_dp(K5D_WAS_ON)));
    } while (IF_Z);
}

/* Kind5DStart's sink and rise: ActorMotionPreset $49 (JSL at
   preset_at), each frame (BRK first) Kind5DSinkStep (JSR after it) while
   its speed's top byte is not negative; preset $4A (JSL at back_at), each
   frame Kind5DRiseStep while it is. */
static void k5d_sink_rise(uint32_t preset_at, uint32_t back_at)
{
    ldy8_v(K5D_SINK);
    jsl_call(preset_at, abi_ActorMotionPreset);
    do {
        actor_yield(preset_at + 4);
        call_frame((uint16_t)(preset_at + 8), abi_Kind5DSinkStep);
    } while (!IF_N);
    ldy8_v(K5D_BACK_UP);
    jsl_call(back_at, abi_ActorMotionPreset);
    do {
        actor_yield(back_at + 4);
        call_frame((uint16_t)(back_at + 8), abi_Kind5DRiseStep);
    } while (IF_N);
}

/*
 * @routine 83:E29B Kind5DStart
 * @actor
 * WHAT IT DOES: Actor kind $5D, a cloud: rises (Kind5DRise), then waits
 * for Aladdin to land on it; then marks it for its maker, sinks under him
 * and rises back, and waits for him to leave it; then sinks and rises
 * once more, and waits for him again.
 * HOW: Kind5DRise (JSL); $08 1, $09 $90; K5D_ON, K5D_WAS_ON, K5D_GO and
 * K5D_STATE 0; AnimStart 0. Each frame (BRK first) Kind5DParentCheck and
 * Kind5DLandTest until K5D_ON differs from K5D_WAS_ON. K5D_LANDED up,
 * K5D_GO 1; ActorMotionPreset $49 and Kind5DSinkStep each frame (BRK
 * first) while its vertical speed's top byte is not negative; preset $4A
 * and Kind5DRiseStep each frame while it is. Then the same wait for a
 * change, the same sink and rise, and the first wait again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $5D in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind5DStart(void)
{
    jsl_call(0x83E29Bu, abi_Kind5DRise);
    lda8_v(0x01);
    sta8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    sta8(ea_dp(ACTOR_09));
    stz8(ea_dp(K5D_ON));
    stz8(ea_dp(K5D_WAS_ON));
    stz8(ea_dp(K5D_GO));
    stz8(ea_dp(K5D_STATE));
    lda8_v(0x00);
    jsl_call(0x83E2B1u, abi_AnimStart);
    for (;;) {
        k5d_wait_change(0x83E2B5u);
        MEM8(inc, ea_dp(K5D_LANDED));
        lda8_v(0x01);
        sta8(ea_dp(K5D_GO));
        k5d_sink_rise(0x83E2CBu, 0x83E2D8u);
        k5d_wait_change(0x83E2E3u);
        k5d_sink_rise(0x83E2F3u, 0x83E300u);
    }
}

/*
 * @routine 83:E30E Kind5DIdle
 * @actor
 * WHAT IT DOES: Kind $5D in its timed end: does nothing each frame until
 * its wait ends it.
 * HOW: BRK, again.
 * Entry: D = the actor's record; reached by the actor loop's RTI,
 * Kind5DWait having made it its resume address. Exit: never returns.
 */
void abi_Kind5DIdle(void)
{
    for (;;)
        actor_yield(0x83E30Eu);
}

/*
 * @routine 83:E312 Kind5DLandTest
 * WHAT IT DOES: Tells whether Aladdin lands on the cloud this frame: then
 * sound $29 and K5D_ON 1, C clear; else K5D_ON 0, C set.
 * HOW: K5D_ON 0; PlayerLandOnActor with box $50; landed (C clear):
 * SoundQueueAdd $29, K5D_ON up, C clear.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: C as above; M 8-bit, X 8-bit.
 */
void abi_Kind5DLandTest(void)
{
    stz8(ea_dp(K5D_ON));
    ldy8_v(K5D_BOX);
    jsl_call(0x83E316u, abi_PlayerLandOnActor);
    if (IF_C)
        return;
    lda8_v(K5D_LAND_SOUND);
    jsl_call(0x83E31Eu, abi_SoundQueueAdd);
    MEM8(inc, ea_dp(K5D_ON));
    clc();
}

/* Kind5DSinkStep and Kind5DRiseStep: Kind5DLandTest (JSR at at), its C
   kept (PHP); ActorKeepSpot and fly (JSLs at at + 4, at + 8); landed:
   PlayerMoveWithActor (JSL at at + 15); A = its speed's top byte. */
static void k5d_step(uint32_t at, void (*fly)(void))
{
    call_frame((uint16_t)(at + 2), abi_Kind5DLandTest);
    php();
    jsl_call(at + 4, abi_ActorKeepSpot);
    jsl_call(at + 8, fly);
    plp();
    if (!IF_C)
        jsl_call(at + 15, abi_PlayerMoveWithActor);
    lda8(ea_dp(ACTOR_SPEED_Y + 2));
}

/*
 * @routine 83:E326 Kind5DSinkStep
 * WHAT IT DOES: A frame of the cloud sinking: Aladdin landed on it is
 * carried with it.
 * HOW: Kind5DLandTest, its C kept (PHP); ActorKeepSpot, ActorFlyAgainst;
 * landed: PlayerMoveWithActor. A = its vertical speed's top byte.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: A = $1F; N and Z from it.
 */
void abi_Kind5DSinkStep(void)
{
    k5d_step(0x83E326u, abi_ActorFlyAgainst);
}

/*
 * @routine 83:E33C Kind5DRiseStep
 * WHAT IT DOES: A frame of the cloud rising back: Aladdin landed on it is
 * carried with it.
 * HOW: As Kind5DSinkStep, with ActorFly.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: A = $1F; N and Z from it.
 */
void abi_Kind5DRiseStep(void)
{
    k5d_step(0x83E33Cu, abi_ActorFly);
}

/*
 * @routine 83:E3AD Kind5DParentCheck
 * WHAT IT DOES: Once the cloud's maker has gone, starts the cloud's timed
 * end (Kind5DWait's step 2), if it has not started.
 * HOW: Kind5DParentGone nonzero and K5D_STATE below 2: K5D_STATE 2.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
 * M 8-bit, X 8-bit; A as the last load.
 */
void abi_Kind5DParentCheck(void)
{
    call_frame(0xE3AF, abi_Kind5DParentGone);
    if (IF_Z)
        return;
    lda8(ea_dp(K5D_STATE));
    cmp8_v(0x02);
    if (IF_C)
        return;
    lda8_v(0x02);
    sta8(ea_dp(K5D_STATE));
}

/*
 * @routine 83:E3BD Kind5DParentGoneLong
 * WHAT IT DOES: Kind5DParentGone for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as Kind5DParentGone; called by JSL. Exit: as Kind5DParentGone.
 */
void abi_Kind5DParentGoneLong(void)
{
    call_frame(0xE3BF, abi_Kind5DParentGone);
}

/*
 * @routine 83:E3C1 Kind5DParentGone
 * WHAT IT DOES: Reads whether the actor's maker (K5D_PARENT) has gone: its
 * K5D_PARENT_GONE.
 * HOW: 16-bit X = K5D_PARENT; A low = its byte at $4E; X 8-bit again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: A low = the byte; N and Z
 * from it; X = K5D_PARENT's low byte.
 */
void abi_Kind5DParentGone(void)
{
    op_rep(0x10);
    ldx16(ea_dp(K5D_PARENT));
    lda8(ea_absx(K5D_PARENT_GONE));
    op_sep(0x10);
}

/*
 * @routine 83:E3CB Kind5DRise
 * WHAT IT DOES: The cloud's rise: from where its maker made it, up to its
 * height (by Aladdin's lives and its number), with sound $28 and a
 * sparkle (kind $5F) every 4 frames; then back to its caller.
 * HOW: Its caller's return address (3 bytes) pulled into K5D_BACK.
 * SoundQueueAdd $28; K5D_HEIGHT_AT = K5D_HEIGHTS by Kind5CPathByLives
 * plus twice its $07; $21, $07, K5D_LANDED, K5D_COUNT and K5D_47 0;
 * ActorSpeedYSet $45. Each frame (BRK first) ActorMoveY until its y is
 * K5D_HEIGHTS's word at K5D_HEIGHT_AT or more; K5D_COUNT down past 0:
 * ActorCreateChild8 kind $5F, K5D_COUNT 3, $07 bit 0 flipped. Then
 * K5D_BACK pushed back, RTL.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSL. Exit: by RTL to its caller; M 8-bit, X 8-bit.
 */
void abi_Kind5DRise(void)
{
    pla8();
    sta8(ea_dp(K5D_BACK));
    pla8();
    sta8(ea_dp(K5D_BACK + 1));
    pla8();
    sta8(ea_dp(K5D_BACK + 2));
    lda8_v(K5D_RISE_SOUND);
    jsl_call(0x83E3D6u, abi_SoundQueueAdd);
    call_frame(0xE3DC, abi_Kind5CPathByLives);
    tax8();
    lda8(ea_dp(ACTOR_07));
    ACC8(asl);
    clc();
    adc8_v(rd8(ea_absx(K5D_HEIGHTS)));
    sta8(ea_dp(K5D_HEIGHT_AT));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    stz8(ea_dp(ACTOR_07));
    stz8(ea_dp(K5D_LANDED));
    stz8(ea_dp(K5D_COUNT));
    stz8(ea_dp(K5D_47));
    ldy8_v(K5D_RISE_SPEED);
    jsl_call(0x83E3F3u, abi_ActorSpeedYSet);
    for (;;) {
        actor_yield(0x83E3F7u);
        jsl_call(0x83E3F9u, abi_ActorMoveY);
        op_rep(0x20);
        ldx8(ea_dp(K5D_HEIGHT_AT));
        lda16(ea_dp(ACTOR_Y + 1));
        cmp16_v(rd16(ea_absx(K5D_HEIGHTS)));
        op_sep(0x20);
        if (IF_C)
            break;
        MEM8(dec, ea_dp(K5D_COUNT));
        if (!IF_N)
            continue;
        lda8_v(K5D_SPARKLE);
        jsl_call(0x83E410u, abi_ActorCreateChild8);
        lda8_v(0x03);
        sta8(ea_dp(K5D_COUNT));
        lda8(ea_dp(ACTOR_07));
        eor8_v(0x01);
        sta8(ea_dp(ACTOR_07));
    }
    lda8(ea_dp(K5D_BACK + 2));
    pha8();
    lda8(ea_dp(K5D_BACK + 1));
    pha8();
    lda8(ea_dp(K5D_BACK));
    pha8();
}
