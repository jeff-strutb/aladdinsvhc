/*
 * kind36.c: actor kind $36, a figure of the menu and story screens that
 * stands, follows a cursor, or walks, by its $07.
 */
#include "machine.h"
#include "native.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAccelerateX(void);
void abi_ActorCreateChild8(void);
void abi_ActorEndPlaced(void);
void abi_ActorMoveX(void);
void abi_ActorMoveY(void);
void abi_ActorSpeedXSet(void);
void abi_ActorSpeedXSetAccel(void);
void abi_ActorSpeedYSet(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define K36_FRAMES 0x40u          /* frames left of its walk */
#define K36_00A5 0x00A5u          /* part 2: the index into K36_X_BY_00A5 */
#define K36_00AC 0x00ACu          /* parts 4-$0A: nonzero, it speeds off */
#define K36_X_BY_00A5 0xD1CAu     /* part 2: its x pixels' low byte, by K36_00A5 (bank DB) */
#define K36_PARTS 0xAFA5u         /* bank $83: the parts' code, by $07 with bit 4 clear (words) */
#define K36_MOVES 0xB02Fu         /* bank $83: the walks' code, by $07 bits 0-2 (words) */

/* Every JSL below puts its return address on the stack while the callee
   runs (jsl_frame), as the original's does, and steps S back after. */

/*
 * @routine 83:AF8E Kind36Start
 * @actor
 * WHAT IT DOES: Actor kind $36, a figure of the menu and story screens
 * that plays one of nine parts by its $07. With bit 4 clear, $07 itself
 * picks: 0, it stands (pose 5); 2, it stands (pose 5) with its x pixels'
 * low byte set each frame from K36_X_BY_00A5 by K36_00A5; 4 and 6 (facing
 * left, right), 8 (facing left, pose 2) and $0A (pose 1), it stands until
 * K36_00AC is set, then speeds off sideways for 13 frames with sound $42
 * and stays. With bit 4 set, it walks for $16 frames after $18 frames of
 * waiting, by bits 0-2: 0 (turned round with bit 3) and stays; 1 with
 * pose 1, then sound $42, a child (ActorCreateChild8 with A = $85) and
 * poses 3 and 4 for 16 frames each, then moves vertically for good; 2
 * turned round with pose 2, then ends.
 * HOW: ACTOR_09 bits $90 set; ACTOR_FACING and ACTOR_FLIP 0. Bit 4 clear:
 * JMP (K36_PARTS,X) by $07. Parts 4-$0A: AnimStart with the part's pose;
 * each frame (BRK) until K36_00AC nonzero; speed x 0 (three bytes);
 * ActorSpeedXSetAccel with A = $40, Y = $60; K36_FRAMES = $0D frames of
 * ActorAccelerateX and ActorMoveX; SoundQueueAdd $42; BRK for good. Bit 4
 * set: K36_FRAMES = $16; ActorSpeedXSet with Y = $2D; JMP (K36_MOVES,X)
 * by bits 0-2 times 2; each walks: AnimStart, COP $18, then K36_FRAMES
 * frames (BRK first) of ActorMoveX. Move 0: ACTOR_08 bits 0-1 set, with
 * $07 bit 3 facing and flip up by one, AnimStart 0; after the walk, BRK
 * for good. Move 1: AnimStart 1; after the walk SoundQueueAdd $42,
 * ACTOR_SPAWN_AT 0, ActorCreateChild8 with A = $85, AnimStart 3, COP $10,
 * ActorSpeedYSet with Y = $57, AnimStart 4, COP $10, then each frame
 * ActorMoveY. Move 2: AnimStart 2, facing up by one; after the walk, JML
 * ActorEndPlaced.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $36 in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind36Start(void)
{
    lda8(ea_dp(ACTOR_09));  /* 83AF8E lda */
    ora8_v(0x90);  /* 83AF90 ora */
    sta8(ea_dp(ACTOR_09));  /* 83AF92 sta */
    stz8(ea_dp(ACTOR_FACING));  /* 83AF94 stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 83AF96 stz */
    lda8(ea_dp(ACTOR_07));  /* 83AF98 lda */
    bit8_i(0x10);  /* 83AF9A bit */
    if (IF_Z) goto L_83AFA1;  /* 83AF9C beq */
    goto L_83B01C;  /* 83AF9E jmp */
L_83AFA1:
    tax8();  /* 83AFA1 tax */
    switch (cpu.x) {  /* 83AFA2 jmp (K36_PARTS,x) */
    case 0: goto L_83AFB1;
    case 2: goto L_83AFBB;
    case 4: goto L_83AFCD;
    case 6: goto L_83AFD5;
    case 8: goto L_83B010;
    case 10: goto L_83B018;
    default:  /* past the table: its next bytes */
        jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(K36_PARTS + cpu.x)));
        return;
    }
L_83AFB1:
    lda8_v(0x05);  /* 83AFB1 lda */
    jsl_frame(0x83AFB3u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83AFB3 jsl */
L_83AFB7:
    actor_yield(0x83AFB7u);  /* 83AFB7 brk */
    goto L_83AFB7;  /* 83AFB9 bra */
L_83AFBB:
    lda8_v(0x05);  /* 83AFBB lda */
    jsl_frame(0x83AFBDu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83AFBD jsl */
L_83AFC1:
    actor_yield(0x83AFC1u);  /* 83AFC1 brk */
    ldx8(ea_abs(K36_00A5));  /* 83AFC3 ldx */
    lda8(ea_absx(K36_X_BY_00A5));  /* 83AFC6 lda */
    sta8(ea_dp(ACTOR_X + 1));  /* 83AFC9 sta */
    goto L_83AFC1;  /* 83AFCB bra */
L_83AFCD:
    lda8_v(0x01);  /* 83AFCD lda */
    sta8(ea_dp(ACTOR_FACING));  /* 83AFCF sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83AFD1 sta */
    goto L_83AFD9;  /* 83AFD3 bra */
L_83AFD5:
    stz8(ea_dp(ACTOR_FACING));  /* 83AFD5 stz */
    stz8(ea_dp(ACTOR_FLIP));  /* 83AFD7 stz */
L_83AFD9:
    lda8_v(0x00);  /* 83AFD9 lda */
L_83AFDB:
    jsl_frame(0x83AFDBu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83AFDB jsl */
L_83AFDF:
    actor_yield(0x83AFDFu);  /* 83AFDF brk */
    lda8(ea_abs(K36_00AC));  /* 83AFE1 lda */
    if (IF_Z) goto L_83AFDF;  /* 83AFE4 beq */
    stz8(ea_dp(ACTOR_SPEED_X));  /* 83AFE6 stz */
    stz8(ea_dp(ACTOR_SPEED_X + 1));  /* 83AFE8 stz */
    stz8(ea_dp(ACTOR_SPEED_X + 2));  /* 83AFEA stz */
    lda8_v(0x40);  /* 83AFEC lda */
    ldy8_v(0x60);  /* 83AFEE ldy */
    jsl_frame(0x83AFF0u); abi_ActorSpeedXSetAccel(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83AFF0 jsl */
    lda8_v(0x0D);  /* 83AFF4 lda */
    sta8(ea_dp(K36_FRAMES));  /* 83AFF6 sta */
L_83AFF8:
    actor_yield(0x83AFF8u);  /* 83AFF8 brk */
    jsl_frame(0x83AFFAu); abi_ActorAccelerateX(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83AFFA jsl */
    jsl_frame(0x83AFFEu); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83AFFE jsl */
    MEM8(dec, ea_dp(K36_FRAMES));  /* 83B002 dec */
    if (!IF_Z) goto L_83AFF8;  /* 83B004 bne */
    lda8_v(0x42);  /* 83B006 lda */
    jsl_frame(0x83B008u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B008 jsl */
L_83B00C:
    actor_yield(0x83B00Cu);  /* 83B00C brk */
    goto L_83B00C;  /* 83B00E bra */
L_83B010:
    lda8_v(0x01);  /* 83B010 lda */
    sta8(ea_dp(ACTOR_FACING));  /* 83B012 sta */
    lda8_v(0x02);  /* 83B014 lda */
    goto L_83AFDB;  /* 83B016 bra */
L_83B018:
    lda8_v(0x01);  /* 83B018 lda */
    goto L_83AFDB;  /* 83B01A bra */
L_83B01C:
    lda8_v(0x16);  /* 83B01C lda */
    sta8(ea_dp(K36_FRAMES));  /* 83B01E sta */
    ldy8_v(0x2D);  /* 83B020 ldy */
    jsl_frame(0x83B022u); abi_ActorSpeedXSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B022 jsl */
    lda8(ea_dp(ACTOR_07));  /* 83B026 lda */
    and8_v(0x07);  /* 83B028 and */
    ACC8(asl);  /* 83B02A asl */
    tax8();  /* 83B02B tax */
    switch (cpu.x) {  /* 83B02C jmp (K36_MOVES,x) */
    case 0: goto L_83B035;
    case 2: goto L_83B05D;
    case 4: goto L_83B0A1;
    default:  /* past the table: its next bytes */
        jump_routine(0x830000u | rd16(0x830000u | (uint16_t)(K36_MOVES + cpu.x)));
        return;
    }
L_83B035:
    lda8(ea_dp(ACTOR_08));  /* 83B035 lda */
    ora8_v(0x03);  /* 83B037 ora */
    sta8(ea_dp(ACTOR_08));  /* 83B039 sta */
    lda8(ea_dp(ACTOR_07));  /* 83B03B lda */
    bit8_i(0x08);  /* 83B03D bit */
    if (IF_Z) goto L_83B045;  /* 83B03F beq */
    MEM8(inc, ea_dp(ACTOR_FACING));  /* 83B041 inc */
    MEM8(inc, ea_dp(ACTOR_FLIP));  /* 83B043 inc */
L_83B045:
    lda8_v(0x00);  /* 83B045 lda */
    jsl_frame(0x83B047u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B047 jsl */
    lda8_v(0x18);  /* 83B04B lda */
    actor_wait(0x83B04Du);  /* 83B04D cop */
L_83B04F:
    actor_yield(0x83B04Fu);  /* 83B04F brk */
    jsl_frame(0x83B051u); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B051 jsl */
    MEM8(dec, ea_dp(K36_FRAMES));  /* 83B055 dec */
    if (!IF_Z) goto L_83B04F;  /* 83B057 bne */
L_83B059:
    actor_yield(0x83B059u);  /* 83B059 brk */
    goto L_83B059;  /* 83B05B bra */
L_83B05D:
    lda8_v(0x01);  /* 83B05D lda */
    jsl_frame(0x83B05Fu); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B05F jsl */
    lda8_v(0x18);  /* 83B063 lda */
    actor_wait(0x83B065u);  /* 83B065 cop */
L_83B067:
    actor_yield(0x83B067u);  /* 83B067 brk */
    jsl_frame(0x83B069u); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B069 jsl */
    MEM8(dec, ea_dp(K36_FRAMES));  /* 83B06D dec */
    if (!IF_Z) goto L_83B067;  /* 83B06F bne */
    lda8_v(0x42);  /* 83B071 lda */
    jsl_frame(0x83B073u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B073 jsl */
    stz8(ea_dp(ACTOR_SPAWN_AT));  /* 83B077 stz */
    lda8_v(0x85);  /* 83B079 lda */
    jsl_frame(0x83B07Bu); abi_ActorCreateChild8(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B07B jsl */
    lda8_v(0x03);  /* 83B07F lda */
    jsl_frame(0x83B081u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B081 jsl */
    lda8_v(0x10);  /* 83B085 lda */
    actor_wait(0x83B087u);  /* 83B087 cop */
    ldy8_v(0x57);  /* 83B089 ldy */
    jsl_frame(0x83B08Bu); abi_ActorSpeedYSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B08B jsl */
    lda8_v(0x04);  /* 83B08F lda */
    jsl_frame(0x83B091u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B091 jsl */
    lda8_v(0x10);  /* 83B095 lda */
    actor_wait(0x83B097u);  /* 83B097 cop */
L_83B099:
    actor_yield(0x83B099u);  /* 83B099 brk */
    jsl_frame(0x83B09Bu); abi_ActorMoveY(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B09B jsl */
    goto L_83B099;  /* 83B09F bra */
L_83B0A1:
    lda8_v(0x02);  /* 83B0A1 lda */
    jsl_frame(0x83B0A3u); abi_AnimStart(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B0A3 jsl */
    MEM8(inc, ea_dp(ACTOR_FACING));  /* 83B0A7 inc */
    lda8_v(0x18);  /* 83B0A9 lda */
    actor_wait(0x83B0ABu);  /* 83B0AB cop */
L_83B0AD:
    actor_yield(0x83B0ADu);  /* 83B0AD brk */
    jsl_frame(0x83B0AFu); abi_ActorMoveX(); cpu.s = (uint16_t)(cpu.s + 3);  /* 83B0AF jsl */
    MEM8(dec, ea_dp(K36_FRAMES));  /* 83B0B3 dec */
    if (!IF_Z) goto L_83B0AD;  /* 83B0B5 bne */
    abi_ActorEndPlaced();  /* 83B0B7 jml */
    return;
}
