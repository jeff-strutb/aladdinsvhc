/*
 * kind33.c: actor kind $33, a platform swinging under the placed one:
 * the placed one ($07 0) queues its kind's graphics when it holds its tile
 * slot alone, makes the platform ($07 3) and then shows the frame that
 * matches the platform's direction; the platform swings to and fro about
 * the placed one's position and carries the player standing on it.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"
#include "actor_anim.h"
#include "wram_video.h"

void abi_ActorSpawn(void);
void abi_AnimStart(void);
void abi_AnimResume(void);
void abi_ActorEndPlaced(void);
void abi_ActorHoldPlayer(void);
void abi_ActorMoveByVector(void);
void abi_PlayerLandOnActor(void);
void abi_Kind33Graphics(void);
void abi_Kind33Hub(void);
void abi_Kind33Swing(void);
void abi_Kind33PartnerState(void);
void abi_Kind33SwingMove(void);
void abi_Kind33Look(void);
void abi_Kind33GraphicsQueue(void);

#define K33_PARTNER 0x40u       /* the pair's other record (word): the placed one's platform, the platform's placed one */
#define K33_PHASE 0x42u         /* the platform: 0 swinging one way, then 1 the other */
#define K33_PAST 0x43u          /* the platform: nonzero once this swing is slowing */
#define K33_ACCEL 0x44u         /* the platform: taken from or added to the speed each frame (word) */
#define K33_SPEED 0x46u         /* the platform: its angle's change a frame (word) */
#define K33_ANGLE 0x48u         /* the platform: its angle (word); the high byte its direction */
#define K33_HUB_X 0x4Au         /* the platform: the placed one's x pixels, where it swings from (word) */
#define K33_HUB_Y 0x4Cu         /* and its y pixels (word) */
#define K33_RIDDEN 0x4Eu        /* the platform: nonzero while the player stands on it */
#define K33_SHOWN 0x4Fu         /* the placed one: the frame it shows */
#define K33_HOLD_AT 0x13u       /* ActorHoldPlayer's offset for the player */
#define K33_OLD_X 0x000Eu       /* the platform's x pixels before its move (word) */
#define K33_OLD_Y 0x0010u       /* and its y pixels */
#define K33_FRAMES 0xD199u      /* bank $80: the placed one's frame by the platform's direction / 8 */
#define K33_UPLOADS 0xD1AAu     /* bank $80: its graphics' two uploads, 6 bytes each: tile, source offset, bytes (words) */
#define K33_CELL_ATTRS 0x7FE766u /* kind $33's cell: its attribute word (bits 0-8 its first tile) */
#define K33_CELL_GFX 0x7FE566u  /* kind $33's cell: its graphics' address in bank $7F (word) */

/* A JSL at `site` to fn: its return address on the stack while fn runs,
   as the original's. */
static void jsl_call(uint32_t site, void (*fn)(void))
{
    jsl_frame(site);
    fn();
    cpu.s = (uint16_t)(cpu.s + 3);
}

/*
 * @routine 83:B966 Kind33Start
 * @actor
 * WHAT IT DOES: Actor kind $33's start: the placed one ($07 0) goes to
 * Kind33Graphics, its platform ($07 nonzero) to Kind33Swing.
 * HOW: $07 zero: JMP Kind33Graphics; else JMP Kind33Swing.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $33 in state $0C (the table at $83:8CAB).
 * Exit: never returns.
 */
void abi_Kind33Start(void)
{
    lda8(ea_dp(ACTOR_07));
    if (IF_Z) {
        abi_Kind33Graphics();
        return;
    }
    abi_Kind33Swing();
}

/*
 * @routine 83:B970 Kind33Hub
 * @actor
 * WHAT IT DOES: The placed one of kind $33 after its graphics: makes its
 * platform ($07 3, swinging from its position) and then each frame shows
 * the frame of animation 0 that matches the platform's direction.
 * HOW: $21 = 0; $07 = 3; ActorSpawn with its own hit box ($06); made (N
 * clear): K33_PARTNER = the platform, the platform's K33_PARTNER = it,
 * K33_HUB_X/Y its x and y pixels, its look ours. 8-bit; Kind33Look; BRK.
 * Then each frame (BRK first): the platform's direction ($14) / 8 picks
 * from K33_FRAMES into ACTOR_FRAME; a frame other than K33_SHOWN: kept
 * there, AnimResume 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
 * jump from Kind33Graphics. Exit: never returns.
 */
void abi_Kind33Hub(void)
{
    stz8(ea_dp(ACTOR_SPAWN_AT));
    lda8_v(0x03);
    sta8(ea_dp(ACTOR_07));
    lda8(ea_dp(ACTOR_06));
    jsl_call(0x83B978u, abi_ActorSpawn);
    if (!IF_N) {
        op_rep(0x20);
        txa16();
        sta16(ea_dp(K33_PARTNER));
        tdc();
        sta16(ea_absx(K33_PARTNER));
        lda16(ea_dp(ACTOR_X + 1));
        sta16(ea_absx(K33_HUB_X));
        lda16(ea_dp(ACTOR_Y + 1));
        sta16(ea_absx(K33_HUB_Y));
        lda16(ea_dp(ACTOR_LOOK));
        sta16(ea_absx(ACTOR_LOOK));
        op_sep(0x20);
    }
    op_sep(0x30);
    call_frame(0xB99C, abi_Kind33Look);
    actor_yield(0x83B99Du);
    for (;;) {
        actor_yield(0x83B99Fu);
        op_rep(0x10);
        ldx16(ea_dp(K33_PARTNER));
        lda8(ea_absx(ACTOR_FACING));
        op_sep(0x10);
        ACC8(lsr);
        ACC8(lsr);
        ACC8(lsr);
        tax8();
        lda8(ea_absx(K33_FRAMES));
        sta8(ea_dp(ACTOR_FRAME));
        cmp8_v(rd8(ea_dp(K33_SHOWN)));
        if (IF_Z)
            continue;
        sta8(ea_dp(K33_SHOWN));
        lda8_v(0x00);
        jsl_call(0x83B9BBu, abi_AnimResume);
    }
}

/*
 * @routine 83:B9C1 Kind33Swing
 * @actor
 * WHAT IT DOES: Kind $33's platform: swings about the placed one, faster
 * towards the bottom of each swing and slower past it, one way and back,
 * then stops and starts over; it carries the player standing on it and
 * may take hold of him; it ends when the placed one's record is free.
 * HOW: $08 bit 0 and $09 bit 4 set; AnimStart 1; K33_RIDDEN, K33_SHOWN,
 * $21 and K33_HOLD_AT cleared; y pixels $8080; K33_PHASE 0, K33_PAST 1,
 * K33_ACCEL 9, K33_SPEED $020A, K33_ANGLE $4AFD. Each frame (BRK first):
 * Kind33PartnerState zero: JMP ActorEndPlaced. Kind33SwingMove;
 * ActorHoldPlayer. Phase 0: K33_ANGLE += K33_SPEED (16-bit); phase 1: -=.
 * K33_PAST clear and the direction below $44 (phase 0) or at least $48
 * (phase 1): K33_SPEED += K33_ACCEL, next frame. K33_PAST clear
 * otherwise: K33_PAST up. Then K33_SPEED -= K33_ACCEL; not negative, next
 * frame; negative in phase 0: phase 1, K33_PAST 0; in phase 1: phase,
 * past, speed and angle 0, K33_ACCEL 9.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
 * jump from Kind33Start. Exit: never returns.
 * (The JMP back to the stop at $83:BA76-BA78, after the JMP at $83:BA73,
 * is reached by nothing: no branch or jump targets it. It is not filed.)
 */
void abi_Kind33Swing(void)
{
    lda8_v(0x01);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x10);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x01);
    jsl_call(0x83B9CBu, abi_AnimStart);
    stz8(ea_dp(K33_RIDDEN));
    stz8(ea_dp(K33_SHOWN));
    stz8(ea_dp(ACTOR_SPAWN_AT));
    stz8(ea_dp(K33_HOLD_AT));
    lda8_v(0x80);
    sta8(ea_dp(ACTOR_Y + 1));
    sta8(ea_dp(ACTOR_Y + 2));
    stz8(ea_dp(K33_PHASE));
    lda8_v(0x01);
    sta8(ea_dp(K33_PAST));
    lda8_v(0x09);
    sta8(ea_dp(K33_ACCEL));
    stz8(ea_dp(K33_ACCEL + 1));
    lda8_v(0x0A);
    sta8(ea_dp(K33_SPEED));
    lda8_v(0x02);
    sta8(ea_dp(K33_SPEED + 1));
    lda8_v(0xFD);
    sta8(ea_dp(K33_ANGLE));
    lda8_v(0x4A);
    sta8(ea_dp(K33_ANGLE + 1));
    goto frame;
stop:
    stz8(ea_dp(K33_PHASE));
    stz8(ea_dp(K33_PAST));
    stz8(ea_dp(K33_SPEED));
    stz8(ea_dp(K33_SPEED + 1));
    lda8_v(0x09);
    sta8(ea_dp(K33_ACCEL));
    stz8(ea_dp(K33_ACCEL + 1));
    stz8(ea_dp(K33_ANGLE));
    stz8(ea_dp(K33_ANGLE + 1));
frame:
    actor_yield(0x83BA0Du);
    call_frame(0xBA11, abi_Kind33PartnerState);
    if (IF_Z) {
        abi_ActorEndPlaced();
        return;
    }
    call_frame(0xBA19, abi_Kind33SwingMove);
    call_frame(0xBA1C, abi_ActorHoldPlayer);
    lda8(ea_dp(K33_PHASE));
    if (!IF_Z)
        goto back;
    op_rep(0x21);
    lda16(ea_dp(K33_ANGLE));
    adc16_v(rd16(ea_dp(K33_SPEED)));
    sta16(ea_dp(K33_ANGLE));
    op_sep(0x20);
    lda8(ea_dp(K33_PAST));
    if (!IF_Z)
        goto slower;
    lda8(ea_dp(K33_ANGLE + 1));
    cmp8_v(0x44);
    if (!IF_C)
        goto faster;
    MEM8(inc, ea_dp(K33_PAST));
    goto slower;
back:
    op_rep(0x20);
    lda16(ea_dp(K33_ANGLE));
    sec();
    sbc16_v(rd16(ea_dp(K33_SPEED)));
    sta16(ea_dp(K33_ANGLE));
    op_sep(0x20);
    lda8(ea_dp(K33_PAST));
    if (!IF_Z)
        goto slower;
    lda8(ea_dp(K33_ANGLE + 1));
    cmp8_v(0x48);
    if (IF_C)
        goto faster;
    MEM8(inc, ea_dp(K33_PAST));
    goto slower;
faster:
    op_rep(0x20);
    lda16(ea_dp(K33_SPEED));
    adc16_v(rd16(ea_dp(K33_ACCEL)));
    sta16(ea_dp(K33_SPEED));
    op_sep(0x20);
    goto frame;
slower:
    op_rep(0x20);
    lda16(ea_dp(K33_SPEED));
    sec();
    sbc16_v(rd16(ea_dp(K33_ACCEL)));
    sta16(ea_dp(K33_SPEED));
    op_sep(0x20);
    if (!IF_N)
        goto frame;
    lda8(ea_dp(K33_PHASE));
    if (!IF_Z)
        goto stop;
    MEM8(inc, ea_dp(K33_PHASE));
    stz8(ea_dp(K33_PAST));
    goto frame;
}

/*
 * @routine 83:BA79 Kind33PartnerState
 * WHAT IT DOES: Reads the state of the pair's other record (K33_PARTNER).
 * HOW: 16-bit X = K33_PARTNER; A low = its state byte; X 8-bit again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: A low = the state; N and Z from it; X = K33_PARTNER's low
 * byte; M 8-bit, X 8-bit; the rest unchanged.
 */
void abi_Kind33PartnerState(void)
{
    op_rep(0x10);
    ldx16(ea_dp(K33_PARTNER));
    lda8(ea_absx(ACTOR_STATE));
    op_sep(0x10);
}

/*
 * @routine 83:BA83 Kind33SwingMove
 * WHAT IT DOES: Puts the platform where its angle sets it about the placed
 * one, and the player with it: lands him on it when he falls onto it,
 * carries him by its move while he stands on it, and marks when he is off.
 * HOW: Its x and y pixels kept in K33_OLD_X/Y; x and y pixels =
 * K33_HUB_X/Y, their fractions 0; the facing = the angle's high byte;
 * ActorMoveByVector with set 4. K33_RIDDEN clear: PlayerLandOnActor with
 * box 0; landed (C clear): K33_RIDDEN up. Set: the player's x and y
 * pixels moved by its new place less the old; PlayerLandOnActor with box
 * 0; off (C set): K33_RIDDEN = 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: M 8-bit, X 8-bit; C as PlayerLandOnActor leaves it.
 */
void abi_Kind33SwingMove(void)
{
    op_rep(0x20);
    lda16(ea_dp(ACTOR_X + 1));
    sta16(ea_abs(K33_OLD_X));
    lda16(ea_dp(ACTOR_Y + 1));
    sta16(ea_abs(K33_OLD_Y));
    lda16(ea_dp(K33_HUB_X));
    sta16(ea_dp(ACTOR_X + 1));
    lda16(ea_dp(K33_HUB_Y));
    sta16(ea_dp(ACTOR_Y + 1));
    op_sep(0x20);
    stz8(ea_dp(ACTOR_X));
    stz8(ea_dp(ACTOR_Y));
    lda8(ea_dp(K33_ANGLE + 1));
    sta8(ea_dp(ACTOR_FACING));
    ldx8_v(0x04);
    jsl_call(0x83BAA3u, abi_ActorMoveByVector);
    lda8(ea_dp(K33_RIDDEN));
    if (IF_Z) {
        ldy8_v(0x00);
        jsl_call(0x83BAADu, abi_PlayerLandOnActor);
        if (IF_C)
            return;
        MEM8(inc, ea_dp(K33_RIDDEN));
        return;
    }
    op_rep(0x20);
    lda16(ea_dp(ACTOR_X + 1));
    sec();
    sbc16_v(rd16(ea_abs(K33_OLD_X)));
    clc();
    adc16_v(rd16(ea_abs(PLAYER + ACTOR_X + 1)));
    sta16(ea_abs(PLAYER + ACTOR_X + 1));
    lda16(ea_dp(ACTOR_Y + 1));
    sec();
    sbc16_v(rd16(ea_abs(K33_OLD_Y)));
    clc();
    adc16_v(rd16(ea_abs(PLAYER + ACTOR_Y + 1)));
    sta16(ea_abs(PLAYER + ACTOR_Y + 1));
    op_sep(0x20);
    ldy8_v(0x00);
    jsl_call(0x83BAD7u, abi_PlayerLandOnActor);
    if (!IF_C)
        return;
    stz8(ea_dp(K33_RIDDEN));
}

/*
 * @routine 83:BAE0 Kind33Look
 * WHAT IT DOES: The placed one of kind $33's look: $08 bit 1 and $09 bits
 * 7 and 4 set, animation 0 from its start.
 * HOW: TSB $02 on $08, TSB $90 on $09; AnimStart with A = 0.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
 * JSR. Exit: as AnimStart.
 */
void abi_Kind33Look(void)
{
    lda8_v(0x02);
    tsb8(ea_dp(ACTOR_08));
    lda8_v(0x90);
    tsb8(ea_dp(ACTOR_09));
    lda8_v(0x00);
    jsl_call(0x83BAEAu, abi_AnimStart);
}

/*
 * @routine 83:BB2F Kind33Graphics
 * @actor
 * WHAT IT DOES: The placed one of kind $33 first: when it holds its tile
 * slot alone it queues its kind's graphics (two uploads, a frame apart);
 * then Kind33Hub.
 * HOW: The slot's holders ($15E8 by $39) less one: zero: X = 0,
 * Kind33GraphicsQueue, BRK, X = 6, Kind33GraphicsQueue, JMP Kind33Hub;
 * else BRK, JMP Kind33Hub.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
 * jump from Kind33Start. Exit: never returns.
 */
void abi_Kind33Graphics(void)
{
    ldx8(ea_dp(ACTOR_TILE_SLOT));
    lda8(ea_absx(ACTOR_TILE_SLOT_USERS));
    ACC8(dec);
    if (IF_Z) {
        ldx8_v(0x00);
        call_frame(0xBB3B, abi_Kind33GraphicsQueue);
        actor_yield(0x83BB3Cu);
        ldx8_v(0x06);
        call_frame(0xBB42, abi_Kind33GraphicsQueue);
        abi_Kind33Hub();
        return;
    }
    actor_yield(0x83BB46u);
    abi_Kind33Hub();
}

/*
 * @routine 83:BB4B Kind33GraphicsQueue
 * WHAT IT DOES: Queues one of kind $33's graphics uploads (X = 0 or 6) for
 * the vertical blank: a VRAM queue record.
 * HOW: 16-bit. Y = the queue's count (its low byte) times 8. The VRAM
 * address: (K33_CELL_ATTRS AND $01FF) plus the upload's tile word, times
 * 16, plus $6000; the source: K33_CELL_GFX plus its offset word; bank
 * $7F; the byte count its third word (written over the bank's high
 * byte); VMAIN $80. The count up.
 * Entry: M 8-bit, X 8-bit; X = the upload's offset in K33_UPLOADS; DB =
 * $80; called by JSR. Exit: M 8-bit, X 8-bit; A low = $80; Y = the
 * record's offset; N and Z from the count's INC.
 */
void abi_Kind33GraphicsQueue(void)
{
    op_rep(0x30);
    lda16(ea_abs(VRAM_QUEUE_COUNT));
    and16_v(0x00FF);
    ACC16(asl);
    ACC16(asl);
    ACC16(asl);
    tay16();
    lda16(ea_long(K33_CELL_ATTRS));
    and16_v(0x01FF);
    clc();
    adc16_v(rd16(ea_absx(K33_UPLOADS)));
    ACC16(asl);
    ACC16(asl);
    ACC16(asl);
    ACC16(asl);
    clc();
    adc16_v(0x6000);
    sta16(ea_absy(VRAM_QUEUE));
    clc();
    lda16(ea_long(K33_CELL_GFX));
    adc16_v(rd16(ea_absx(K33_UPLOADS + 2)));
    sta16(ea_absy(VRAM_QUEUE + 2));
    lda16_v(0x007F);
    sta16(ea_absy(VRAM_QUEUE + 4));
    lda16(ea_absx(K33_UPLOADS + 4));
    sta16(ea_absy(VRAM_QUEUE + 5));
    op_sep(0x20);
    lda8_v(0x80);
    sta8(ea_absy(VRAM_QUEUE + 7));
    op_sep(0x10);
    MEM8(inc, ea_abs(VRAM_QUEUE_COUNT));
}
