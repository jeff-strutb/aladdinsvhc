/*
 * kind_a2.c: actor kind $A2: a shot another actor sends out, which turns
 * back to it and is taken in, or falls away once its sender is hit.
 * Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorEndPlaced(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMoveRight(void);
void abi_ActorMoveY(void);
void abi_ActorPriorityClear(void);
void abi_ActorSpeedAddVector(void);
void abi_ActorSpeedXClear(void);
void abi_ActorSpeedYClear(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define KINDA2_TIME 0x40u  /* frames left of its way out */
#define KINDA2_SENDER 0x4Cu  /* word: the record of the actor that sent it */
#define SENDER_OUT 0x42u  /* in the sender's record: its shots out (word) */
#define SENDER_HIT 0x47u  /* in the sender's record: nonzero once it is hit */
#define KINDA2_HEADINGS 0xD501u  /* headings by facing (0 right, 3 left) plus $07, in the bank DB names */

/*
 * @routine 84:8249 KindA2Sent
 * @actor
 * WHAT IT DOES: Actor kind $A2 started new: a shot sent out by another
 * actor (its record at $4C) along one of its headings for $20 frames; then
 * it turns back and flies on until it reaches its sender, which takes it
 * in (one shot fewer out), or falls away if the sender has been hit
 * meanwhile.
 * HOW: $08 bit 1 and $09 bits $90 set, its look $0600, sound $2F,
 * animation $1A, both speeds cleared; its heading (in $14) from
 * KINDA2_HEADINGS at $07 plus 0 facing right or 3 left. Each frame
 * ActorSpeedAddVector (X = $22), ActorMoveRight and ActorMoveY; after $20
 * frames the heading's bit 3 flipped (back), and on the same each frame:
 * the sender's $47 set: animation $1B, bit 7 of $09 cleared and into the
 * fall of KindA2Hit; else within $30 below and 16 above, and $20 across,
 * of the sender: its $42 down by one, then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A2 in state $0C (through the stub at
 * $83:9416). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA2Sent(void)
{
    lda8(ea_dp(ACTOR_08));  /* 848249 lda */
    ora8_v(0x02);  /* 84824B ora */
    sta8(ea_dp(ACTOR_08));  /* 84824D sta */
    lda8(ea_dp(ACTOR_09));  /* 84824F lda */
    ora8_v(0x90);  /* 848251 ora */
    sta8(ea_dp(ACTOR_09));  /* 848253 sta */
    op_rep(0x20);  /* 848255 rep */
    lda16_v(0x0600);  /* 848257 lda */
    sta16(ea_dp(ACTOR_LOOK));  /* 84825A sta */
    op_sep(0x20);  /* 84825C sep */
    lda8_v(0x2F);  /* 84825E lda */
    abi_SoundQueueAdd();  /* 848260 jsl */
    lda8_v(0x1A);  /* 848264 lda */
    abi_AnimStart();  /* 848266 jsl */
    abi_ActorSpeedXClear();  /* 84826A jsl */
    abi_ActorSpeedYClear();  /* 84826E jsl */
    lda8(ea_dp(ACTOR_FACING));  /* 848272 lda */
    if (IF_Z) goto L_848278;  /* 848274 beq */
    lda8_v(0x03);  /* 848276 lda */
L_848278:
    clc();  /* 848278 clc */
    adc8_v(rd8(ea_dp(ACTOR_07)));  /* 848279 adc */
    tax8();  /* 84827B tax */
    lda8(ea_absx(KINDA2_HEADINGS));  /* 84827C lda */
    sta8(ea_dp(ACTOR_FACING));  /* 84827F sta */
    lda8_v(0x20);  /* 848281 lda */
    sta8(ea_dp(KINDA2_TIME));  /* 848283 sta */
L_848285:
    actor_yield(0x848285u);  /* 848285 brk */
    ldx8_v(0x22);  /* 848287 ldx */
    abi_ActorSpeedAddVector();  /* 848289 jsl */
    abi_ActorMoveRight();  /* 84828D jsl */
    abi_ActorMoveY();  /* 848291 jsl */
    MEM8(dec, ea_dp(KINDA2_TIME));  /* 848295 dec */
    if (!IF_Z) goto L_848285;  /* 848297 bne */
    lda8(ea_dp(ACTOR_FACING));  /* 848299 lda */
    eor8_v(0x08);  /* 84829B eor */
    sta8(ea_dp(ACTOR_FACING));  /* 84829D sta */
L_84829F:
    op_sep(0x30);  /* 84829F sep */
    actor_yield(0x8482A1u);  /* 8482A1 brk */
    ldx8_v(0x22);  /* 8482A3 ldx */
    abi_ActorSpeedAddVector();  /* 8482A5 jsl */
    abi_ActorMoveRight();  /* 8482A9 jsl */
    abi_ActorMoveY();  /* 8482AD jsl */
    op_rep(0x10);  /* 8482B1 rep */
    ldx16(ea_dp(KINDA2_SENDER));  /* 8482B3 ldx */
    lda8(ea_absx(SENDER_HIT));  /* 8482B5 lda */
    if (!IF_Z) goto L_8482E7;  /* 8482B8 bne */
    op_rep(0x30);  /* 8482BA rep */
    clc();  /* 8482BC clc */
    lda16(ea_absx(ACTOR_Y + 1));  /* 8482BD lda */
    adc16_v(0x0010);  /* 8482C0 adc */
    sec();  /* 8482C3 sec */
    sbc16_v(rd16(ea_dp(ACTOR_Y + 1)));  /* 8482C4 sbc */
    cmp16_v(0x0030);  /* 8482C6 cmp */
    if (IF_C) goto L_84829F;  /* 8482C9 bcs */
    clc();  /* 8482CB clc */
    lda16(ea_absx(ACTOR_X + 1));  /* 8482CC lda */
    adc16_v(0x0010);  /* 8482CF adc */
    sec();  /* 8482D2 sec */
    sbc16_v(rd16(ea_dp(ACTOR_X + 1)));  /* 8482D3 sbc */
    cmp16_v(0x0020);  /* 8482D5 cmp */
    if (IF_C) goto L_84829F;  /* 8482D8 bcs */
    lda16(ea_absx(SENDER_OUT));  /* 8482DA lda */
    ACC16(dec);  /* 8482DD dec */
    sta16(ea_absx(SENDER_OUT));  /* 8482DE sta */
    op_sep(0x30);  /* 8482E1 sep */
    abi_ActorEndPlaced(); return;  /* 8482E3 jml */
L_8482E7:
    op_sep(0x30);  /* 8482E7 sep */
    lda8_v(0x1B);  /* 8482E9 lda */
    abi_AnimStart();  /* 8482EB jsl */
    lda8_v(0x80);  /* 8482EF lda */
    trb8(ea_dp(ACTOR_09));  /* 8482F1 trb */
    goto L_84830E;  /* 8482F3 bra */
L_84830E:
    ldy8_v(0x6D);  /* 84830E ldy */
    abi_ActorMotionPreset();  /* 848310 jsl */
    abi_ActorPriorityClear();  /* 848314 jsr */
L_848317:
    actor_yield(0x848317u);  /* 848317 brk */
    abi_AnimStep();  /* 848319 jsl */
    abi_ActorFly();  /* 84831D jsl */
    lda8(ea_dp(ACTOR_09));  /* 848321 lda */
    bit8_i(0x40);  /* 848323 bit */
    if (!IF_Z) goto L_848317;  /* 848325 bne */
    abi_ActorEndPlaced(); return;  /* 848327 jml */
}


/*
 * @routine 84:82F5 KindA2Hit
 * @actor
 * WHAT IT DOES: Actor kind $A2 started in state $4C (hit): its sender has
 * one shot fewer out, and it falls away until it can no longer be hit,
 * then ends.
 * HOW: Sound $12, animation $1B; the sender's ($4C) $42 down by one
 * (8-bit); then ActorMotionPreset $6D and ActorPriorityClear; each frame
 * AnimStep and ActorFly while bit 6 of $09 is set; then ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $A2 in state $4C (through the stub at
 * $83:941F). Exit: never returns; ends in ActorEndPlaced.
 */
void abi_KindA2Hit(void)
{
    lda8_v(0x12);  /* 8482F5 lda */
    abi_SoundQueueAdd();  /* 8482F7 jsl */
    lda8_v(0x1B);  /* 8482FB lda */
    abi_AnimStart();  /* 8482FD jsl */
    op_rep(0x10);  /* 848301 rep */
    ldx16(ea_dp(KINDA2_SENDER));  /* 848303 ldx */
    lda8(ea_absx(SENDER_OUT));  /* 848305 lda */
    ACC8(dec);  /* 848308 dec */
    sta8(ea_absx(SENDER_OUT));  /* 848309 sta */
    op_sep(0x10);  /* 84830C sep */
    ldy8_v(0x6D);  /* 84830E ldy */
    abi_ActorMotionPreset();  /* 848310 jsl */
    abi_ActorPriorityClear();  /* 848314 jsr */
L_848317:
    actor_yield(0x848317u);  /* 848317 brk */
    abi_AnimStep();  /* 848319 jsl */
    abi_ActorFly();  /* 84831D jsl */
    lda8(ea_dp(ACTOR_09));  /* 848321 lda */
    bit8_i(0x40);  /* 848323 bit */
    if (!IF_Z) goto L_848317;  /* 848325 bne */
    abi_ActorEndPlaced(); return;  /* 848327 jml */
}
