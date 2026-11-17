/*
 * kind2a.c: actor kind $2A: its start, which paces (or, no script, leaps
 * at Aladdin), and its second state, downed: knocked away, a short
 * flight up and a fall. Its wait is Kind2AWait (actor_waits_small.c).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorAccelerateY(void);
void abi_ActorFly(void);
void abi_ActorMotionPreset(void);
void abi_ActorMotionReset(void);
void abi_ActorStepY(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_ActorAccelerateX(void);
void abi_ActorDecelerateX(void);
void abi_ActorDecelerateY(void);
void abi_ActorFall(void);
void abi_ActorMove(void);
void abi_ActorMoveX(void);
void abi_ActorNearPlayer(void);
void abi_ActorPlayerBelow(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorStepYBack(void);

#define KIND2A_DOWNS 0x47u     /* counted each time it is downed */
#define KIND2A_HITS 0x48u      /* counted when downed by a shot (Kind2AWait) */
#define KIND2A_RIGHT 0x4Cu     /* counted when downed facing right (Kind2AWait) */
#define KIND2A_FRAMES 0x43u    /* frames left of its flight */
#define PLAYER_FLIP_AT (PLAYER + ACTOR_FLIP)
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)
#define KIND2A_BOB 0x49u       /* nonzero: it bobs with its frames (Kind2AWait) */
#define KIND2A_UP 0x4Bu        /* Kind2AWait's bob direction */
#define KIND2A_FRAME 0x4Au     /* the frame it last bobbed on (Kind2AWait) */
#define KIND2A_SPEED 0x45u     /* its pacing speed (word) */
#define KIND2A_LEG 0x44u       /* frames of each pace after the first */
#define KIND2A_BELOW 0x41u     /* the leaper: how far below it he must be (word) */
#define KIND2A_LEAP 0x40u      /* the leaper: frames left of its leap */
#define KIND2A_SPEEDS 0xD0A4u  /* in bank $80: pacing speeds by variant (words) */
#define KIND2A_LEGS 0xD0B0u    /* in bank $80: frames of the first pace by variant */
#define KIND2A_REACH 0xD0A2u   /* in bank $80: the leaper's KIND2A_BELOW by $07 */
#define PLAYER_Y_AT (PLAYER + ACTOR_Y + 1)
#define COPY_TEST 0x701331u    /* save RAM, which the retail board lacks: a write there reads back on a copier */

/*
 * @routine 83:A1E5 Kind2AStart
 * @actor
 * WHAT IT DOES: Actor kind $2A: with bit 4 of $07 it paces across at the
 * speed of its variant, slowing to a stop, turning and speeding up again,
 * bobbing with its frames. Without it (no script has that) it waits until
 * Aladdin is below and near, rises toward him, then leaps across at him,
 * or (by $07) drops on him and leaps from there, again and again.
 * HOW: KIND2A_RIGHT cleared, ACTOR_09 bits $90 set, AnimStart 0,
 * ActorMotionPreset with Y = 6, KIND2A_DOWNS, KIND2A_HITS, KIND2A_BOB and
 * KIND2A_UP cleared. Bit 4: by the low digit of $07 its speed (KIND2A_SPEEDS)
 * in its speed x and KIND2A_SPEED, KIND2A_FRAMES = KIND2A_LEGS by it and
 * KIND2A_LEG twice that; AnimStart 2, facing right, KIND2A_BOB up,
 * KIND2A_FRAME = $FF, accel x $14. Then for ever: KIND2A_FRAMES frames of
 * ActorMoveX; ActorMoveX and ActorDecelerateX each frame until its speed
 * x is 0; COP 8; turned; COP 8; ActorMoveX and ActorAccelerateX until the
 * speed reaches KIND2A_SPEED; KIND2A_FRAMES = KIND2A_LEG. Without bit 4:
 * KIND2A_BELOW = KIND2A_REACH by $07, $07 halved, facing him; each frame
 * until ActorPlayerBelow says he is below and ActorNearPlayer with Y =
 * $0C; AnimStart 1, COP 5. $07 now zero: each frame ActorStepY (and
 * ActorAccelerateY while its speed y is below $400) until its y plus
 * KIND2A_BELOW reaches his; else ActorFall each frame until its y plus
 * $18 reaches his, accel y $80, then ActorStepY and ActorDecelerateY each
 * frame until it stops, AnimStart 2, COP $20. The leap: AnimStart 2,
 * accel y $18 (or as left), accel x $20, KIND2A_LEAP = $38, facing him;
 * each frame ActorAccelerateX below $240, ActorDecelerateY, ActorMove
 * until KIND2A_LEAP runs out; accel y 6, then each frame
 * ActorAccelerateY, ActorMoveX, ActorStepYBack for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $2A in its start state (the table at
 * $83:8CAB). Exit: never returns.
 */
void abi_Kind2AStart(void)
{
    stz8(ea_dp(KIND2A_RIGHT));  /* 83A1E5 stz */
    lda8_v(0x90);  /* 83A1E7 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83A1E9 tsb */
    lda8_v(0x00);  /* 83A1EB lda */
    abi_AnimStart();  /* 83A1ED jsl */
    ldy8_v(0x06);  /* 83A1F1 ldy */
    abi_ActorMotionPreset();  /* 83A1F3 jsl */
    stz8(ea_dp(KIND2A_DOWNS));  /* 83A1F7 stz */
    stz8(ea_dp(KIND2A_HITS));  /* 83A1F9 stz */
    stz8(ea_dp(KIND2A_BOB));  /* 83A1FB stz */
    stz8(ea_dp(KIND2A_UP));  /* 83A1FD stz */
    lda8(ea_dp(ACTOR_07));  /* 83A1FF lda */
    bit8_i(0x10);  /* 83A201 bit */
    if (IF_Z) goto L_83A208;  /* 83A203 beq */
    goto L_83A2FA;  /* 83A205 jmp */
L_83A208:
    ldx8(ea_dp(ACTOR_07));  /* 83A208 ldx */
    lda8(ea_absx(KIND2A_REACH));  /* 83A20A lda */
    sta8(ea_dp(KIND2A_BELOW));  /* 83A20D sta */
    stz8(ea_dp(KIND2A_BELOW + 1));  /* 83A20F stz */
    txa8();  /* 83A211 txa */
    ACC8(lsr);  /* 83A212 lsr */
    sta8(ea_dp(ACTOR_07));  /* 83A213 sta */
    abi_ActorSideOfPlayer();  /* 83A215 jsl */
    sta8(ea_dp(ACTOR_FACING));  /* 83A219 sta */
    eor8_v(0x01);  /* 83A21B eor */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A21D sta */
L_83A21F:
    abi_ActorPlayerBelow();  /* 83A21F jsl */
    if (IF_Z) goto L_83A240;  /* 83A223 beq */
    ldy8_v(0x0C);  /* 83A225 ldy */
    abi_ActorNearPlayer();  /* 83A227 jsl */
    if (IF_C) goto L_83A240;  /* 83A22B bcs */
    lda8_v(0x01);  /* 83A22D lda */
    abi_AnimStart();  /* 83A22F jsl */
    lda8_v(0x05);  /* 83A233 lda */
    actor_wait(0x83A235u);  /* 83A235 cop */
    lda8(ea_dp(ACTOR_07));  /* 83A237 lda */
    if (IF_Z) goto L_83A244;  /* 83A239 beq; 83A23E bra */
    goto L_83A2B9;  /* 83A23B jmp */
L_83A240:
    actor_yield(0x83A240u);  /* 83A240 brk */
    goto L_83A21F;  /* 83A242 bra */
L_83A244:
    abi_ActorStepY();  /* 83A244 jsl */
    op_rep(0x20);  /* 83A248 rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83A24A lda */
    adc16_v(rd16(ea_dp(KIND2A_BELOW)));  /* 83A24C adc */
    cmp16_v(rd16(ea_abs(PLAYER_Y_AT)));  /* 83A24E cmp */
    if (!IF_C) goto L_83A273;  /* 83A251 bcc */
    op_sep(0x20);  /* 83A253 sep */
    lda8_v(0x02);  /* 83A255 lda */
    abi_AnimStart();  /* 83A257 jsl */
    lda8_v(0x18);  /* 83A25B lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* 83A25D sta */
L_83A25F:
    lda8_v(0x20);  /* 83A25F lda */
    sta8(ea_dp(ACTOR_ACCEL_X));  /* 83A261 sta */
    lda8_v(0x38);  /* 83A263 lda */
    sta8(ea_dp(KIND2A_LEAP));  /* 83A265 sta */
    abi_ActorSideOfPlayer();  /* 83A267 jsl */
    sta8(ea_dp(ACTOR_FACING));  /* 83A26B sta */
    eor8_v(0x01);  /* 83A26D eor */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A26F sta */
    goto L_83A2A5;  /* 83A271 bra */
L_83A273:
    lda16(ea_dp(ACTOR_SPEED_Y));  /* 83A273 lda */
    cmp16_v(0x0400);  /* 83A275 cmp */
    op_sep(0x20);  /* 83A278 sep */
    if (IF_C) goto L_83A280;  /* 83A27A bcs */
    abi_ActorAccelerateY();  /* 83A27C jsl */
L_83A280:
    actor_yield(0x83A280u);  /* 83A280 brk */
    goto L_83A244;  /* 83A282 bra */
L_83A284:
    op_rep(0x20);  /* 83A284 rep */
    lda16(ea_dp(ACTOR_SPEED_X));  /* 83A286 lda */
    cmp16_v(0x0240);  /* 83A288 cmp */
    op_sep(0x20);  /* 83A28B sep */
    if (IF_C) goto L_83A293;  /* 83A28D bcs */
    abi_ActorAccelerateX();  /* 83A28F jsl */
L_83A293:
    abi_ActorDecelerateY();  /* 83A293 jsl */
    abi_ActorMove();  /* 83A297 jsl */
    MEM8(dec, ea_dp(KIND2A_LEAP));  /* 83A29B dec */
    if (!IF_Z) goto L_83A2A5;  /* 83A29D bne */
    lda8_v(0x06);  /* 83A29F lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* 83A2A1 sta */
    goto L_83A2A9;  /* 83A2A3 bra */
L_83A2A5:
    actor_yield(0x83A2A5u);  /* 83A2A5 brk */
    goto L_83A284;  /* 83A2A7 bra */
L_83A2A9:
    abi_ActorAccelerateY();  /* 83A2A9 jsl */
    abi_ActorMoveX();  /* 83A2AD jsl */
    abi_ActorStepYBack();  /* 83A2B1 jsl */
    actor_yield(0x83A2B5u);  /* 83A2B5 brk */
    goto L_83A2A9;  /* 83A2B7 bra */
L_83A2B9:
    lda8_v(0x18);  /* 83A2B9 lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* 83A2BB sta */
L_83A2BD:
    abi_ActorFall();  /* 83A2BD jsl */
    op_rep(0x20);  /* 83A2C1 rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83A2C3 lda */
    adc16_v(0x0018);  /* 83A2C5 adc */
    cmp16_v(rd16(ea_abs(PLAYER_Y_AT)));  /* 83A2C8 cmp */
    op_sep(0x20);  /* 83A2CB sep */
    if (!IF_C) goto L_83A2D5;  /* 83A2CD bcc */
    lda8_v(0x80);  /* 83A2CF lda */
    sta8(ea_dp(ACTOR_ACCEL_Y));  /* 83A2D1 sta */
    goto L_83A2D9;  /* 83A2D3 bra */
L_83A2D5:
    actor_yield(0x83A2D5u);  /* 83A2D5 brk */
    goto L_83A2BD;  /* 83A2D7 bra */
L_83A2D9:
    abi_ActorStepY();  /* 83A2D9 jsl */
    abi_ActorDecelerateY();  /* 83A2DD jsl */
    lda8(ea_dp(ACTOR_SPEED_Y));  /* 83A2E1 lda */
    if (!IF_Z) goto L_83A2F6;  /* 83A2E3 bne */
    lda8(ea_dp(ACTOR_SPEED_Y + 1));  /* 83A2E5 lda */
    if (!IF_Z) goto L_83A2F6;  /* 83A2E7 bne */
    lda8_v(0x02);  /* 83A2E9 lda */
    abi_AnimStart();  /* 83A2EB jsl */
    lda8_v(0x20);  /* 83A2EF lda */
    actor_wait(0x83A2F1u);  /* 83A2F1 cop */
    goto L_83A25F;  /* 83A2F3 jmp */
L_83A2F6:
    actor_yield(0x83A2F6u);  /* 83A2F6 brk */
    goto L_83A2D9;  /* 83A2F8 bra */
L_83A2FA:
    and8_v(0x0F);  /* 83A2FA and */
    tay8();  /* 83A2FC tay */
    ACC8(asl);  /* 83A2FD asl */
    tax8();  /* 83A2FE tax */
    lda8(ea_absx(KIND2A_SPEEDS));  /* 83A2FF lda */
    sta8(ea_dp(ACTOR_SPEED_X));  /* 83A302 sta */
    sta8(ea_dp(KIND2A_SPEED));  /* 83A304 sta */
    lda8(ea_absx(KIND2A_SPEEDS + 1));  /* 83A306 lda */
    sta8(ea_dp(ACTOR_SPEED_X + 1));  /* 83A309 sta */
    sta8(ea_dp(KIND2A_SPEED + 1));  /* 83A30B sta */
    stz8(ea_dp(ACTOR_SPEED_X + 2));  /* 83A30D stz */
    lda8(ea_absy(KIND2A_LEGS));  /* 83A30F lda */
    sta8(ea_dp(KIND2A_FRAMES));  /* 83A312 sta */
    ACC8(asl);  /* 83A314 asl */
    sta8(ea_dp(KIND2A_LEG));  /* 83A315 sta */
    lda8_v(0x02);  /* 83A317 lda */
    abi_AnimStart();  /* 83A319 jsl */
    stz8(ea_dp(ACTOR_FACING));  /* 83A31D stz */
    lda8_v(0x01);  /* 83A31F lda */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A321 sta */
    MEM8(inc, ea_dp(KIND2A_BOB));  /* 83A323 inc */
    lda8_v(0xFF);  /* 83A325 lda */
    sta8(ea_dp(KIND2A_FRAME));  /* 83A327 sta */
    lda8_v(0x14);  /* 83A329 lda */
    sta8(ea_dp(ACTOR_ACCEL_X));  /* 83A32B sta */
L_83A32D:
    actor_yield(0x83A32Du);  /* 83A32D brk */
    abi_ActorMoveX();  /* 83A32F jsl */
    MEM8(dec, ea_dp(KIND2A_FRAMES));  /* 83A333 dec */
    if (!IF_Z) goto L_83A32D;  /* 83A335 bne */
L_83A337:
    actor_yield(0x83A337u);  /* 83A337 brk */
    abi_ActorMoveX();  /* 83A339 jsl */
    abi_ActorDecelerateX();  /* 83A33D jsl */
    lda8(ea_dp(ACTOR_SPEED_X));  /* 83A341 lda */
    if (!IF_Z) goto L_83A337;  /* 83A343 bne */
    lda8(ea_dp(ACTOR_SPEED_X + 1));  /* 83A345 lda */
    if (!IF_Z) goto L_83A337;  /* 83A347 bne */
    lda8_v(0x08);  /* 83A349 lda */
    actor_wait(0x83A34Bu);  /* 83A34B cop */
    lda8(ea_dp(ACTOR_FACING));  /* 83A34D lda */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A34F sta */
    eor8_v(0x01);  /* 83A351 eor */
    sta8(ea_dp(ACTOR_FACING));  /* 83A353 sta */
    lda8_v(0x08);  /* 83A355 lda */
    actor_wait(0x83A357u);  /* 83A357 cop */
L_83A359:
    actor_yield(0x83A359u);  /* 83A359 brk */
    abi_ActorMoveX();  /* 83A35B jsl */
    abi_ActorAccelerateX();  /* 83A35F jsl */
    op_rep(0x20);  /* 83A363 rep */
    lda16(ea_dp(ACTOR_SPEED_X));  /* 83A365 lda */
    cmp16_v(rd16(ea_dp(KIND2A_SPEED)));  /* 83A367 cmp */
    op_sep(0x20);  /* 83A369 sep */
    if (!IF_C) goto L_83A359;  /* 83A36B bcc */
    lda8(ea_dp(KIND2A_SPEED));  /* 83A36D lda */
    sta8(ea_dp(ACTOR_SPEED_X));  /* 83A36F sta */
    lda8(ea_dp(KIND2A_SPEED + 1));  /* 83A371 lda */
    sta8(ea_dp(ACTOR_SPEED_X + 1));  /* 83A373 sta */
    lda8(ea_dp(KIND2A_LEG));  /* 83A375 lda */
    sta8(ea_dp(KIND2A_FRAMES));  /* 83A377 sta */
    goto L_83A32D;  /* 83A379 bra */
}


/*
 * @routine 83:A37B Kind2ASecond
 * @actor
 * WHAT IT DOES: Actor kind $2A in its second state (downed): it is knocked
 * away from Aladdin, flies up a little, then falls for good (its wait
 * ends it out of range). On a cartridge whose save RAM can be written
 * (a copy, not the retail board) each frame of the fall also pushes the
 * player 16 pixels right.
 * HOW: AnimStart 4; ActorMotionPreset with Y = $28; ACTOR_09 bit 7 set;
 * KIND2A_DOWNS up. Facing negative (no script has it): facing and flip =
 * its bit 0, speed x high byte up, on at the fly. Else KIND2A_RIGHT zero:
 * SoundQueueAdd $0F, y plus 9. KIND2A_HITS zero: facing away from the
 * player's flip byte, COP 9. ActorMotionPreset with Y = $28; 16 frames
 * (BRK) of ActorFly; AnimStart 3; ActorMotionReset with A = $10; then each
 * frame (BRK) AnimStep, ActorAccelerateY unless the speed y high byte is
 * 3, ActorStepY, and the byte at COPY_TEST counted up: read back equal
 * (save RAM), the player's x plus 16.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $2A in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind2ASecond(void)
{
    lda8_v(0x04);  /* 83A37B lda */
    abi_AnimStart();  /* 83A37D jsl */
    ldy8_v(0x28);  /* 83A381 ldy */
    abi_ActorMotionPreset();  /* 83A383 jsl */
    lda8_v(0x80);  /* 83A387 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83A389 tsb */
    MEM8(inc, ea_dp(KIND2A_DOWNS));  /* 83A38B inc */
    lda8(ea_dp(ACTOR_FACING));  /* 83A38D lda */
    if (!IF_N) goto L_83A39B;  /* 83A38F bpl */
    and8_v(0x01);  /* 83A391 and */
    sta8(ea_dp(ACTOR_FACING));  /* 83A393 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A395 sta */
    MEM8(inc, ea_dp(ACTOR_SPEED_X + 1));  /* 83A397 inc */
    goto L_83A3C7;  /* 83A399 bra */
L_83A39B:
    lda8(ea_dp(KIND2A_RIGHT));  /* 83A39B lda */
    if (!IF_Z) goto L_83A3B0;  /* 83A39D bne */
    lda8_v(0x0F);  /* 83A39F lda */
    abi_SoundQueueAdd();  /* 83A3A1 jsl */
    op_rep(0x21);  /* 83A3A5 rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 83A3A7 lda */
    adc16_v(0x0009);  /* 83A3A9 adc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83A3AC sta */
    op_sep(0x20);  /* 83A3AE sep */
L_83A3B0:
    lda8(ea_dp(KIND2A_HITS));  /* 83A3B0 lda */
    if (!IF_Z) goto L_83A3C1;  /* 83A3B2 bne */
    lda8(ea_abs(PLAYER_FLIP_AT));  /* 83A3B4 lda */
    sta8(ea_dp(ACTOR_FLIP));  /* 83A3B7 sta */
    eor8_v(0x01);  /* 83A3B9 eor */
    sta8(ea_dp(ACTOR_FACING));  /* 83A3BB sta */
    lda8_v(0x09);  /* 83A3BD lda */
    actor_wait(0x83A3BFu);  /* 83A3BF cop */
L_83A3C1:
    ldy8_v(0x28);  /* 83A3C1 ldy */
    abi_ActorMotionPreset();  /* 83A3C3 jsl */
L_83A3C7:
    lda8_v(0x10);  /* 83A3C7 lda */
    sta8(ea_dp(KIND2A_FRAMES));  /* 83A3C9 sta */
L_83A3CB:
    actor_yield(0x83A3CBu);  /* 83A3CB brk */
    abi_ActorFly();  /* 83A3CD jsl */
    MEM8(dec, ea_dp(KIND2A_FRAMES));  /* 83A3D1 dec */
    if (!IF_Z) goto L_83A3CB;  /* 83A3D3 bne */
    lda8_v(0x03);  /* 83A3D5 lda */
    abi_AnimStart();  /* 83A3D7 jsl */
    lda8_v(0x10);  /* 83A3DB lda */
    abi_ActorMotionReset();  /* 83A3DD jsl */
L_83A3E1:
    actor_yield(0x83A3E1u);  /* 83A3E1 brk */
    abi_AnimStep();  /* 83A3E3 jsl */
    lda8(ea_dp(ACTOR_SPEED_Y + 1));  /* 83A3E7 lda */
    cmp8_v(0x03);  /* 83A3E9 cmp */
    if (IF_Z) goto L_83A3F1;  /* 83A3EB beq */
    abi_ActorAccelerateY();  /* 83A3ED jsl */
L_83A3F1:
    abi_ActorStepY();  /* 83A3F1 jsl */
    lda8(ea_long(COPY_TEST));  /* 83A3F5 lda */
    ACC8(inc);  /* 83A3F9 inc */
    sta8(ea_long(COPY_TEST));  /* 83A3FA sta */
    cmp8_v(rd8(ea_long(COPY_TEST)));  /* 83A3FE cmp */
    if (!IF_Z) goto L_83A411;  /* 83A402 bne */
    op_rep(0x21);  /* 83A404 rep */
    lda16(ea_abs(PLAYER_X_AT));  /* 83A406 lda */
    adc16_v(0x0010);  /* 83A409 adc */
    sta16(ea_abs(PLAYER_X_AT));  /* 83A40C sta */
    op_sep(0x20);  /* 83A40F sep */
L_83A411:
    goto L_83A3E1;  /* 83A411 bra */
}
