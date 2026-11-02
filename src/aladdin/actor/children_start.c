/*
 * children_start.c: an actor that starts two children at its spawn points, the free list put
 * in order first. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorCreateChild8(void);
void abi_ActorDownedByShot(void);
void abi_ActorEndOutOfRange(void);
void abi_ActorFreePlaced(void);
void abi_ActorHitsTestBoxIfPlayerFree(void);
void abi_ActorStompOrHurt(void);
void abi_ActorYByFrame(void);
void abi_AnimStep(void);
void abi_BackdropActorShow(void);
void abi_BackdropPictureStep(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_PlayerOnFirmGround(void);
void abi_PlayerOnFirmGroundProbe4(void);
void abi_PlayerOnFirmGroundProbe8(void);
void abi_SoundQueueAdd(void);


/*
 * @routine 83:D078 ActorVariantChildStep
 * WHAT IT DOES: For an actor with a variant in bits 2-4 of $07: makes it
 * variant 8 plus that, and starts a child (kind $30 at spawn point $3E),
 * counting $14 up for it; no variant: sets its bit ($41) in $035A and
 * counts $036C up.
 * HOW: Variant: $07 = bits 2-4 shifted down OR 8; $14 up; spawn point $3E;
 * ActorCreateChild8 with kind $30; result zero: its bit set in $035A and
 * $14 back down. None: its bit set in $035A, $036C up.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * (83:D071). Exit: M 8-bit; as ActorCreateChild8, or after the count.
 */
void abi_ActorVariantChildStep(void)
{
    lda8(ea_dp(0x07));  /* 83D078 lda */
    and8_v(0x1C);  /* 83D07A and */
    if (IF_Z) goto L_83D09A;  /* 83D07C beq */
    ACC8(lsr);  /* 83D07E lsr */
    ACC8(lsr);  /* 83D07F lsr */
    ora8_v(0x08);  /* 83D080 ora */
    sta8(ea_dp(0x07));  /* 83D082 sta */
    MEM8(inc, ea_dp(0x14));  /* 83D084 inc */
    lda8_v(0x3E);  /* 83D086 lda */
    sta8(ea_dp(0x21));  /* 83D088 sta */
    lda8_v(0x30);  /* 83D08A lda */
    abi_ActorCreateChild8();  /* 83D08C jsl */
    if (!IF_Z) goto L_83D099;  /* 83D090 bne */
    lda8(ea_dp(0x41));  /* 83D092 lda */
    tsb8(ea_abs(0x035A));  /* 83D094 tsb */
    MEM8(dec, ea_dp(0x14));  /* 83D097 dec */
L_83D099:
    return;  /* 83D099 rts */
L_83D09A:
    lda8(ea_dp(0x41));  /* 83D09A lda */
    tsb8(ea_abs(0x035A));  /* 83D09C tsb */
    MEM8(inc, ea_abs(0x036C));  /* 83D09F inc */
    return;  /* 83D0A2 rts */
}

