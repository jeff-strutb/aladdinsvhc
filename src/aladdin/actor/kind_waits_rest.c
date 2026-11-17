/*
 * kind_waits_rest.c: the wait routines of the actor kinds whose waits
 * had no C (the actor loop's wait table, 83:8F4F, reaches them by JSR each
 * frame an actor of the kind waits): most animate, then end the actor when
 * it is out of range (ActorEndOutOfRange, by its box); two do nothing.
 * Written from their bytes.
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorBouncesOrHurtsPlayer(void);
void abi_ActorDownedByHarmlessShot(void);
void abi_ActorDownedByShot(void);
void abi_ActorEndDropReturn(void);
void abi_ActorEndOutOfRange(void);
void abi_ActorEndsShot(void);
void abi_ActorHurtsPlayerOnTouch(void);
void abi_ActorPictureSend(void);
void abi_ActorStompOrHurt16(void);
void abi_ActorTestCarries(void);
void abi_ActorTurnsShotBack(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define K2B_4B 0x4Bu                /* cleared while bit 4 of ACTOR_07 is set */
#define K2B_STAYS 0x4Eu             /* nonzero: it is never ended out of range */
#define K2C_FRAMES 0x43u            /* frames to its next sound, from 6 */
#define K2C_SOUNDS 0x44u            /* nonzero: it plays sound $40 every 6 frames */
#define CARRYING 0x42u              /* nonzero: it carries (ActorTestCarries) */
#define K41_ACTIVE 0x42u            /* nonzero: it turns shots back and Aladdin can stand on it */
#define K41_FRAMES 0x43u            /* frames before he can stand on it again */
#define K41_Y 0x0Fu                 /* its y (word), raised 8 for the stand-on test */
#define K41_Y_LIMIT 0x4Au           /* below this y (word) it may end */
#define K41_X_LIMIT 0x48u           /* the x (word) its side is judged by */
#define K41_SIDE 0x14u              /* 0: it ends left of K41_X_LIMIT; 1: right */
#define HEALTH 0x0367u              /* the player's health */
#define K5A_SHAPE 0x27u             /* 0, 7, 8 or 9: its sprites get priority 3 (bits 4-5 of ACTOR_09) */
#define K50_FACING 0x14u            /* negative: it faces left */
#define K50_DOWNED 0x45u            /* counted up each time a shot downs it facing right */
#define K42_PARENT 0x40u            /* the record (word) whose $4F it sets when he is hurt */
#define V_03F5 0x03F5u              /* nonzero: no mark on the parent */
#define RANGE_X 0x1A44u             /* the point actors are kept in range of (x, word) */

/*
 * @routine 83:91F3 ActorWaitNothing
 * WHAT IT DOES: The wait of the kinds that do nothing while they wait
 * (kinds 4-6, 9-$0B and others).
 * HOW: RTS.
 * Entry: any; called by JSR through the actor loop's wait table. Exit:
 * nothing changed.
 */
void abi_ActorWaitNothing(void)
{
}

/*
 * @routine 83:BBF1 Kind34Wait
 * WHAT IT DOES: Kind $34's wait: nothing.
 * HOW: RTS.
 * Entry: any; called by JSR through the actor loop's wait table. Exit:
 * nothing changed.
 */
void abi_Kind34Wait(void)
{
}

/*
 * @routine 83:9D46 Kind23Wait
 * WHAT IT DOES: Kind $23's wait: its animation.
 * HOW: AnimStep, RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as AnimStep.
 */
void abi_Kind23Wait(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:CE25 Kind48Wait
 * WHAT IT DOES: Kind $48's wait: its animation.
 * HOW: AnimStep, RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as AnimStep.
 */
void abi_Kind48Wait(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:D9B5 Kind56Wait
 * WHAT IT DOES: Kind $56's wait: its animation.
 * HOW: AnimStep, RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as AnimStep.
 */
void abi_Kind56Wait(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:DEB9 Kind59Wait
 * WHAT IT DOES: Kind $59's wait: its animation.
 * HOW: AnimStep, RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as AnimStep.
 */
void abi_Kind59Wait(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:E5EC Kind65Wait
 * WHAT IT DOES: Kind $65's wait: its animation.
 * HOW: AnimStep, RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as AnimStep.
 */
void abi_Kind65Wait(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:F53C Kind77Wait
 * WHAT IT DOES: The wait of kinds $77 and $78: its animation.
 * HOW: AnimStep, RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as AnimStep.
 */
void abi_Kind77Wait(void)
{
    abi_AnimStep();
}

/*
 * @routine 83:B488 Kind44Wait
 * WHAT IT DOES: Kind $44's wait: its animation, then its end when out of
 * range by box 0.
 * HOW: AnimStep; X = 0; on into ActorEndOutOfRange (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind44Wait(void)
{
    abi_AnimStep();
    ldx8_v(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:B190 Kind3EWait
 * WHAT IT DOES: Kind $3E's wait: its end when out of range by box $38.
 * HOW: X = $38; on into ActorEndOutOfRange (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind3EWait(void)
{
    ldx8_v(0x38);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:BB91 Kind33Wait
 * WHAT IT DOES: Kind $33's wait: its end when out of range by box 8.
 * HOW: X = 8; on into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind33Wait(void)
{
    ldx8_v(0x08);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:BF7E Kind3AWait
 * WHAT IT DOES: Kind $3A's wait: its end when out of range by box 8.
 * HOW: X = 8; on into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind3AWait(void)
{
    ldx8_v(0x08);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:9CE2 Kind22Wait
 * WHAT IT DOES: Kind $22's wait: Aladdin touching it is hurt; its end when
 * out of range by box $18.
 * HOW: ActorHurtsPlayerOnTouch; X = $18; on into ActorEndOutOfRange (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind22Wait(void)
{
    abi_ActorHurtsPlayerOnTouch();
    ldx8_v(0x18);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:A709 Kind2BWait
 * WHAT IT DOES: Kind $2B's wait: its animation; Aladdin touching it is
 * hurt; a shot can down it; with bit 4 of ACTOR_07 its K2B_4B is cleared;
 * unless K2B_STAYS, its end when out of range by box 8.
 * HOW: AnimStep, ActorHurtsPlayerOnTouch, ActorDownedByShot; ACTOR_07 bit
 * 4: K2B_4B 0. K2B_STAYS 0: X = 8, on into ActorEndOutOfRange; else RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange, or A =
 * K2B_STAYS (nonzero), Z clear.
 */
void abi_Kind2BWait(void)
{
    abi_AnimStep();
    abi_ActorHurtsPlayerOnTouch();
    abi_ActorDownedByShot();
    lda8(ea_dp(ACTOR_07));
    and8_v(0x10);
    if (!IF_Z)
        stz8(ea_dp(K2B_4B));
    lda8(ea_dp(K2B_STAYS));
    if (!IF_Z)
        return;
    ldx8_v(0x08);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:A818 Kind2CWait
 * WHAT IT DOES: Kind $2C's wait: its animation; while K2C_SOUNDS is set,
 * sound $40 every 6 frames.
 * HOW: AnimStep; K2C_SOUNDS nonzero: K2C_FRAMES down; at 0, SoundQueueAdd
 * $40 and K2C_FRAMES 6. RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: M 8-bit.
 */
void abi_Kind2CWait(void)
{
    abi_AnimStep();
    lda8(ea_dp(K2C_SOUNDS));
    if (IF_Z)
        return;
    MEM8(dec, ea_dp(K2C_FRAMES));
    if (!IF_Z)
        return;
    lda8_v(0x40);
    abi_SoundQueueAdd();
    lda8_v(0x06);
    sta8(ea_dp(K2C_FRAMES));
}

/*
 * @routine 83:B43B Kind43Wait
 * WHAT IT DOES: Kind $43's wait: not carrying, its animation; carrying,
 * its shot's end and the carry test; then its end when out of range by
 * box 0.
 * HOW: CARRYING 0: AnimStep. Else ActorEndsShot, X = 0, ActorTestCarries.
 * Then X = 0, on into ActorEndOutOfRange (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind43Wait(void)
{
    lda8(ea_dp(CARRYING));
    if (IF_Z) {
        abi_AnimStep();
    } else {
        abi_ActorEndsShot();
        ldx8_v(0x00);
        abi_ActorTestCarries();
    }
    ldx8_v(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:B1E8 Kind3FWait
 * WHAT IT DOES: Kind $3F's wait: not carrying, its animation; carrying,
 * the carry test with box 2; then its end when out of range by box 0.
 * HOW: CARRYING 0: AnimStep. Else X = 2, ActorTestCarries. Then X = 0, on
 * into ActorEndOutOfRange (JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind3FWait(void)
{
    lda8(ea_dp(CARRYING));
    if (IF_Z) {
        abi_AnimStep();
    } else {
        ldx8_v(0x02);
        abi_ActorTestCarries();
    }
    ldx8_v(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:B303 Kind41Wait
 * WHAT IT DOES: Kind $41's wait. Active: it turns shots back; every $20
 * frames at most, while he has health, Aladdin coming down on it (its
 * box 8 pixels higher) stands on it, with sound $12; it animates; with bit 3 or 4 of
 * ACTOR_07 it ends for good once below y $2F0 or once past K41_Y_LIMIT
 * on its K41_SIDE of K41_X_LIMIT, else when out of range by box $28.
 * Inactive: with bit 3 or 4 it ends for good once RANGE_X is $100 or
 * more past it, else when out of range by box 0.
 * HOW: K41_ACTIVE 0: ACTOR_07 & $18 zero: X = 0, ActorEndOutOfRange (JML);
 * else its x - $100 < RANGE_X: RTS, else ActorEndDropReturn. Active:
 * ActorTurnsShotBack; K41_FRAMES nonzero: down by 1; else K41_Y - 8 (old
 * kept on the stack), HEALTH: ActorBouncesOrHurtsPlayer, A = 1 (stood on):
 * SoundQueueAdd $12, K41_FRAMES $20; K41_Y back. AnimStep; ACTOR_07 & $18
 * zero: X = $28, ActorEndOutOfRange (JML). K41_Y >= $2F0: ActorEndDropReturn.
 * K41_Y >= K41_Y_LIMIT, or C of (x - K41_X_LIMIT) differing from K41_SIDE:
 * X = 0, ActorEndOutOfRange (JML); else RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
 * table. Exit: as ActorEndOutOfRange or ActorEndDropReturn, or M 8-bit.
 */
void abi_Kind41Wait(void)
{
    lda8(ea_dp(K41_ACTIVE));
    if (IF_Z) {
        lda8(ea_dp(ACTOR_07));
        bit8_v(0x18);
        if (IF_Z) {
            ldx8_v(0x00);
            abi_ActorEndOutOfRange();
            return;
        }
        op_rep(0x20);
        clc();
        lda16(ea_dp(0x0C));
        adc16_v(0xFF00);
        cmp16_v(rd16(ea_abs(RANGE_X)));
        op_sep(0x20);
        if (!IF_C)
            return;
        op_sep(0x20);
        abi_ActorEndDropReturn();
        return;
    }
    abi_ActorTurnsShotBack();
    lda8(ea_dp(K41_FRAMES));
    if (!IF_Z) {
        MEM8(dec, ea_dp(K41_FRAMES));
    } else {
        op_rep(0x20);
        lda16(ea_dp(K41_Y));
        pha16();
        clc();
        adc16_v(0xFFF8);
        sta16(ea_dp(K41_Y));
        op_sep(0x20);
        lda8(ea_abs(HEALTH));
        if (!IF_Z) {
            abi_ActorBouncesOrHurtsPlayer();
            ACC8(dec);
            if (IF_Z) {
                lda8_v(0x12);
                abi_SoundQueueAdd();
                lda8_v(0x20);
                sta8(ea_dp(K41_FRAMES));
            }
        }
        op_rep(0x20);
        pla16();
        sta16(ea_dp(K41_Y));
        op_sep(0x20);
    }
    abi_AnimStep();
    lda8(ea_dp(ACTOR_07));
    bit8_v(0x18);
    if (IF_Z) {
        ldx8_v(0x28);
        abi_ActorEndOutOfRange();
        return;
    }
    op_rep(0x20);
    lda16(ea_dp(K41_Y));
    cmp16_v(0x02F0);
    if (IF_C) {
        op_sep(0x20);
        abi_ActorEndDropReturn();
        return;
    }
    cmp16_v(rd16(ea_dp(K41_Y_LIMIT)));
    if (!IF_C) {
        lda16(ea_dp(0x0C));
        cmp16_v(rd16(ea_dp(K41_X_LIMIT)));
        op_sep(0x20);
        lda8_v(0x00);
        ACC8(rol);
        eor8_v(rd8(ea_dp(K41_SIDE)));
        if (IF_Z)
            return;
    }
    op_sep(0x20);
    ldx8_v(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:DEF3 Kind5AWait
 * WHAT IT DOES: Kind $5A's wait: its animation and picture; bit 5 of its
 * sprite priority (ACTOR_09) cleared, or bits 4-5 set (priority 3) when
 * K5A_SHAPE is 0, 7, 8 or 9.
 * HOW: AnimStep, ActorPictureSend; TRB $20 on ACTOR_09; K5A_SHAPE 0, 7, 8
 * or 9: TSB $30 on ACTOR_09. RTS.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: M 8-bit.
 */
void abi_Kind5AWait(void)
{
    uint8_t s;

    abi_AnimStep();
    abi_ActorPictureSend();
    lda8_v(0x20);
    trb8(ea_dp(ACTOR_09));
    s = rd8(ea_dp(K5A_SHAPE));
    lda8(ea_dp(K5A_SHAPE));
    if (s != 0) {
        cmp8_v(0x07);
        if (!IF_Z) {
            cmp8_v(0x08);
            if (!IF_Z) {
                cmp8_v(0x09);
                if (!IF_Z)
                    return;
            }
        }
    }
    lda8_v(0x30);
    tsb8(ea_dp(ACTOR_09));
}

/*
 * @routine 83:EC20 Kind6AWait
 * WHAT IT DOES: Kind $6A's wait: its animation; Aladdin touching it is
 * hurt; its end when out of range by box $18.
 * HOW: AnimStep, ActorHurtsPlayerOnTouch; X = $18; on into
 * ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
 * the actor loop's wait table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind6AWait(void)
{
    abi_AnimStep();
    abi_ActorHurtsPlayerOnTouch();
    ldx8_v(0x18);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:D1B0 Kind50Wait
 * WHAT IT DOES: Kind $50's wait: its animation; Aladdin touching it is
 * hurt or stomps it (a margin of $10); a shot downing it while it faces
 * right counts in K50_DOWNED; its end when out of range by box 0.
 * HOW: AnimStep, ActorStompOrHurt16, ActorDownedByShot; C clear (downed)
 * and K50_FACING not negative: K50_DOWNED up. X = 0; on into
 * ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
 * table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind50Wait(void)
{
    abi_AnimStep();
    abi_ActorStompOrHurt16();
    abi_ActorDownedByShot();
    if (!IF_C) {
        lda8(ea_dp(K50_FACING));
        if (!IF_N)
            MEM8(inc, ea_dp(K50_DOWNED));
    }
    ldx8_v(0x00);
    abi_ActorEndOutOfRange();
}

/*
 * @routine 83:C6E3 Kind42Wait
 * WHAT IT DOES: Kind $42's wait: a harmless shot downs it, a hitting one
 * ends; Aladdin standing on it plays sound $12; hurt by it, with V_03F5
 * clear, its parent record's $4F is set to 1 when 0; its animation; its
 * end when out of range by box 0.
 * HOW: ActorDownedByHarmlessShot; ActorBouncesOrHurtsPlayer: A 1
 * (stood on): SoundQueueAdd $12; A 2 (hurt), V_03F5 0: X = K42_PARENT
 * (16-bit), its $4F 0: 1. AnimStep; X = 0; on into ActorEndOutOfRange.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
 * table. Exit: as ActorEndOutOfRange.
 */
void abi_Kind42Wait(void)
{
    abi_ActorDownedByHarmlessShot();
    abi_ActorBouncesOrHurtsPlayer();
    if (!IF_Z) {
        ACC8(dec);
        if (IF_Z) {
            lda8_v(0x12);
            abi_SoundQueueAdd();
        } else {
            lda8(ea_abs(V_03F5));
            if (IF_Z) {
                op_rep(0x10);
                ldx16(ea_dp(K42_PARENT));
                lda8(dba(0x004F) + cpu.x);
                if (IF_Z) {
                    lda8_v(0x01);
                    sta8(dba(0x004F) + cpu.x);
                }
                op_sep(0x10);
            }
        }
    }
    abi_AnimStep();
    ldx8_v(0x00);
    abi_ActorEndOutOfRange();
}
