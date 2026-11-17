/*
 * kind2f.c: actor kind $2F, which lies still until it can be hit, rises,
 * then walks the ground and hops where it ends; and its downed state. Its
 * wait is ActorWaitHitsRange0 ($83:B630).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFall(void);
void abi_ActorFallSpeedCap(void);
void abi_ActorFly(void);
void abi_ActorMapAhead5Down12(void);
void abi_ActorMotionPreset(void);
void abi_ActorMotionPresetY(void);
void abi_ActorMoveX(void);
void abi_ActorOutOfGround(void);
void abi_ActorOutOfWallProbe4(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorSnapToGround(void);
void abi_ActorSpeedXSet(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);

#define KIND2F_FRAMES 0x40u    /* frames left of a step (a word for its walk) */
#define KIND2F_HITS 0x42u      /* shots that hit it without downing it (its wait counts them) */
#define KIND2F_16 0x16u        /* word: $D16E */
#define PLAYER_X_AT (PLAYER + ACTOR_X + 1)
#define COPY_TEST 0x701000u    /* save RAM, which the retail board lacks: a write there reads back on a copier */

/*
 * @routine 83:B4BE Kind2FStart
 * @actor
 * WHAT IT DOES: Actor kind $2F: it lies still until it can be hit (bit 6
 * of $09), rises with sound $16 and falls back to the ground, then walks
 * along it for up to 256 frames, turning at walls, and hops when the
 * ground ahead ends, again and again. When the walk runs out it stands
 * and waits $B4 frames before rising again (no script has that). On a
 * cartridge whose save RAM reads back (a copier), it puts the player at
 * x 0 as it starts.
 * HOW: The byte at COPY_TEST,X counted up and read back equal: the
 * player's x pixels = 0. ACTOR_08 = 2, ACTOR_09 = $90, $16 = $D16E;
 * facing and flip = bits 0-1 of $07 shifted right once, or (zero)
 * ActorSideOfPlayer; KIND2F_HITS = 0; AnimStart 0. Each frame (BRK) until
 * ACTOR_09 bit 6; KIND2F_FRAMES 8 frames, AnimStart 1, $10 frames;
 * ActorMotionPresetY with Y = $1E; SoundQueueAdd $16; each frame ActorFall,
 * ActorFallSpeedCap, ActorOutOfGround until it is in the ground. Landed:
 * AnimStart 6 for 5 frames, AnimStart 2 for $10; AnimStart 3 (4 with bit
 * 2 of $07); KIND2F_FRAMES = $100 (word); ActorSpeedXSet with Y = 6 (9).
 * Each frame ActorMoveX, ActorSnapToGround; ActorMapAhead5Down12 zero:
 * the hop (AnimStart 2 for 8 frames, AnimStart 5, ActorMotionPreset with
 * Y = $1D, each frame ActorFly, ActorFallSpeedCap, ActorOutOfWallProbe4,
 * ActorOutOfGround until in the ground, then landed again). A wall
 * (ActorOutOfWallProbe4, C set): facing and flip turned (no script).
 * KIND2F_FRAMES down: at 0 (no script) AnimStart 2 for $10 frames,
 * AnimStart 0, KIND2F_FRAMES = $B4 and back to the 8-frame wait before
 * the rise (BRK only between the 2's frames, as the code has it).
 * Entry: M 8-bit, X 8-bit; X = the kind's table offset; D = the actor's
 * record; DB = $80; reached by the actor loop's jump for kind $2F in its
 * start state (the table at $83:8CAB). Exit: never returns.
 */
void abi_Kind2FStart(void)
{
    lda8(ea_longx(COPY_TEST));  /* 83B4BE lda */
    ACC8(inc);  /* 83B4C2 inc */
    sta8(ea_longx(COPY_TEST));  /* 83B4C3 sta */
    cmp8_v(rd8(ea_longx(COPY_TEST)));  /* 83B4C7 cmp */
    if (!IF_Z) goto L_83B4D3;  /* 83B4CB bne */
    stz8(ea_abs(PLAYER_X_AT));  /* 83B4CD stz */
    stz8(ea_abs(PLAYER_X_AT + 1));  /* 83B4D0 stz */
L_83B4D3:
    lda8_v(0x02);  /* 83B4D3 lda */
    sta8(ea_dp(ACTOR_08));  /* 83B4D5 sta */
    lda8_v(0x90);  /* 83B4D7 lda */
    sta8(ea_dp(ACTOR_09));  /* 83B4D9 sta */
    op_rep(0x20);  /* 83B4DB rep */
    lda16_v(0xD16E);  /* 83B4DD lda */
    sta16(ea_dp(KIND2F_16));  /* 83B4E0 sta */
    op_sep(0x20);  /* 83B4E2 sep */
    lda8(ea_dp(ACTOR_07));  /* 83B4E4 lda */
    and8_v(0x03);  /* 83B4E6 and */
    if (IF_Z) goto L_83B4ED;  /* 83B4E8 beq */
    ACC8(lsr);  /* 83B4EA lsr */
    goto L_83B4F1;  /* 83B4EB bra */
L_83B4ED:
    abi_ActorSideOfPlayer();  /* 83B4ED jsl */
L_83B4F1:
    sta8(ea_dp(ACTOR_FACING));  /* 83B4F1 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83B4F3 sta */
    stz8(ea_dp(KIND2F_HITS));  /* 83B4F5 stz */
    lda8_v(0x00);  /* 83B4F7 lda */
    abi_AnimStart();  /* 83B4F9 jsl */
L_83B4FD:
    actor_yield(0x83B4FDu);  /* 83B4FD brk */
    lda8(ea_dp(ACTOR_09));  /* 83B4FF lda */
    and8_v(0x40);  /* 83B501 and */
    if (IF_Z) goto L_83B4FD;  /* 83B503 beq */
    lda8_v(0x08);  /* 83B505 lda */
L_83B507:
    sta8(ea_dp(KIND2F_FRAMES));  /* 83B507 sta */
L_83B509:
    actor_yield(0x83B509u);  /* 83B509 brk */
    MEM8(dec, ea_dp(KIND2F_FRAMES));  /* 83B50B dec */
    if (!IF_Z) goto L_83B509;  /* 83B50D bne */
    lda8_v(0x10);  /* 83B50F lda */
    sta8(ea_dp(KIND2F_FRAMES));  /* 83B511 sta */
    lda8_v(0x01);  /* 83B513 lda */
    abi_AnimStart();  /* 83B515 jsl */
L_83B519:
    actor_yield(0x83B519u);  /* 83B519 brk */
    MEM8(dec, ea_dp(KIND2F_FRAMES));  /* 83B51B dec */
    if (!IF_Z) goto L_83B519;  /* 83B51D bne */
    ldy8_v(0x1E);  /* 83B51F ldy */
    abi_ActorMotionPresetY();  /* 83B521 jsl */
    lda8_v(0x16);  /* 83B525 lda */
    abi_SoundQueueAdd();  /* 83B527 jsl */
L_83B52B:
    actor_yield(0x83B52Bu);  /* 83B52B brk */
    abi_ActorFall();  /* 83B52D jsl */
    abi_ActorFallSpeedCap();  /* 83B531 jsr */
    abi_ActorOutOfGround();  /* 83B534 jsl */
    if (IF_Z) goto L_83B52B;  /* 83B538 beq */
L_83B53A:
    lda8_v(0x05);  /* 83B53A lda */
    sta8(ea_dp(KIND2F_FRAMES));  /* 83B53C sta */
    lda8_v(0x06);  /* 83B53E lda */
    abi_AnimStart();  /* 83B540 jsl */
L_83B544:
    actor_yield(0x83B544u);  /* 83B544 brk */
    MEM8(dec, ea_dp(KIND2F_FRAMES));  /* 83B546 dec */
    if (!IF_Z) goto L_83B544;  /* 83B548 bne */
    lda8_v(0x02);  /* 83B54A lda */
    abi_AnimStart();  /* 83B54C jsl */
    lda8_v(0x10);  /* 83B550 lda */
    sta8(ea_dp(KIND2F_FRAMES));  /* 83B552 sta */
L_83B554:
    actor_yield(0x83B554u);  /* 83B554 brk */
    MEM8(dec, ea_dp(KIND2F_FRAMES));  /* 83B556 dec */
    if (!IF_Z) goto L_83B554;  /* 83B558 bne */
    ldx8_v(0x03);  /* 83B55A ldx */
    lda8(ea_dp(ACTOR_07));  /* 83B55C lda */
    and8_v(0x04);  /* 83B55E and */
    if (IF_Z) goto L_83B564;  /* 83B560 beq */
    ldx8_v(0x04);  /* 83B562 ldx */
L_83B564:
    txa8();  /* 83B564 txa */
    abi_AnimStart();  /* 83B565 jsl */
    op_rep(0x20);  /* 83B569 rep */
    lda16_v(0x0100);  /* 83B56B lda */
    sta16(ea_dp(KIND2F_FRAMES));  /* 83B56E sta */
    op_sep(0x20);  /* 83B570 sep */
    ldy8_v(0x06);  /* 83B572 ldy */
    lda8(ea_dp(ACTOR_07));  /* 83B574 lda */
    and8_v(0x04);  /* 83B576 and */
    if (IF_Z) goto L_83B57C;  /* 83B578 beq */
    ldy8_v(0x09);  /* 83B57A ldy */
L_83B57C:
    abi_ActorSpeedXSet();  /* 83B57C jsl */
L_83B580:
    actor_yield(0x83B580u);  /* 83B580 brk */
    abi_ActorMoveX();  /* 83B582 jsl */
    abi_ActorSnapToGround();  /* 83B586 jsl */
    abi_ActorMapAhead5Down12();  /* 83B58A jsr */
    if (IF_Z) goto L_83B5C4;  /* 83B58D beq */
    abi_ActorOutOfWallProbe4();  /* 83B58F jsl */
    if (!IF_C) goto L_83B59D;  /* 83B593 bcc */
    lda8(ea_dp(ACTOR_FACING));  /* 83B595 lda */
    eor8_v(0x01);  /* 83B597 eor */
    sta8(ea_dp(ACTOR_FACING));  /* 83B599 sta */
    sta8(ea_dp(ACTOR_FLIP));  /* 83B59B sta */
L_83B59D:
    op_rep(0x20);  /* 83B59D rep */
    MEM16(dec, ea_dp(KIND2F_FRAMES));  /* 83B59F dec */
    op_sep(0x20);  /* 83B5A1 sep */
    if (IF_Z) goto L_83B5A7;  /* 83B5A3 beq */
    goto L_83B580;  /* 83B5A5 bra */
L_83B5A7:
    lda8_v(0x10);  /* 83B5A7 lda */
    sta8(ea_dp(KIND2F_FRAMES));  /* 83B5A9 sta */
    lda8_v(0x02);  /* 83B5AB lda */
    abi_AnimStart();  /* 83B5AD jsl */
L_83B5B1:
    MEM8(dec, ea_dp(KIND2F_FRAMES));  /* 83B5B1 dec */
    if (IF_Z) goto L_83B5B9;  /* 83B5B3 beq */
    actor_yield(0x83B5B5u);  /* 83B5B5 brk */
    goto L_83B5B1;  /* 83B5B7 bra */
L_83B5B9:
    lda8_v(0x00);  /* 83B5B9 lda */
    abi_AnimStart();  /* 83B5BB jsl */
    lda8_v(0xB4);  /* 83B5BF lda */
    goto L_83B507;  /* 83B5C1 jmp */
L_83B5C4:
    lda8_v(0x02);  /* 83B5C4 lda */
    abi_AnimStart();  /* 83B5C6 jsl */
    lda8_v(0x08);  /* 83B5CA lda */
    sta8(ea_dp(KIND2F_FRAMES));  /* 83B5CC sta */
L_83B5CE:
    actor_yield(0x83B5CEu);  /* 83B5CE brk */
    MEM8(dec, ea_dp(KIND2F_FRAMES));  /* 83B5D0 dec */
    if (!IF_Z) goto L_83B5CE;  /* 83B5D2 bne */
    lda8_v(0x05);  /* 83B5D4 lda */
    abi_AnimStart();  /* 83B5D6 jsl */
    ldy8_v(0x1D);  /* 83B5DA ldy */
    abi_ActorMotionPreset();  /* 83B5DC jsl */
L_83B5E0:
    actor_yield(0x83B5E0u);  /* 83B5E0 brk */
    abi_ActorFly();  /* 83B5E2 jsl */
    abi_ActorFallSpeedCap();  /* 83B5E6 jsr */
    abi_ActorOutOfWallProbe4();  /* 83B5E9 jsl */
    abi_ActorOutOfGround();  /* 83B5ED jsl */
    if (IF_Z) goto L_83B5E0;  /* 83B5F1 beq */
    goto L_83B53A;  /* 83B5F3 jmp */
}

/*
 * @routine 83:B643 Kind2FSecond
 * @actor
 * WHAT IT DOES: Actor kind $2F in its second state (downed): it is knocked
 * up with sound $0F (no sound when shots have hit it and it faces right),
 * plays its downed animation for 10 frames, then flies off until it is
 * out of range.
 * HOW: ActorMotionPreset with Y = $1F; AnimStart 7; KIND2F_FRAMES = $0A.
 * KIND2F_HITS nonzero and facing not negative: no sound (no script);
 * else SoundQueueAdd $0F. Facing negative: its bit 7 cleared (no script).
 * 10 frames (BRK) of AnimStep; then each frame AnimStep, ActorFly and
 * ActorEndOutOfRange with X = 0 (by JSR from its own level: out of range
 * it ends there).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $2F in a state with bit 7 set (the table
 * at $83:8DFD). Exit: never returns.
 */
void abi_Kind2FSecond(void)
{
    ldy8_v(0x1F);  /* 83B643 ldy */
    abi_ActorMotionPreset();  /* 83B645 jsl */
    lda8_v(0x07);  /* 83B649 lda */
    abi_AnimStart();  /* 83B64B jsl */
    lda8_v(0x0A);  /* 83B64F lda */
    sta8(ea_dp(KIND2F_FRAMES));  /* 83B651 sta */
    lda8(ea_dp(KIND2F_HITS));  /* 83B653 lda */
    if (IF_Z) goto L_83B65B;  /* 83B655 beq */
    lda8(ea_dp(ACTOR_FACING));  /* 83B657 lda */
    if (!IF_N) goto L_83B661;  /* 83B659 bpl */
L_83B65B:
    lda8_v(0x0F);  /* 83B65B lda */
    abi_SoundQueueAdd();  /* 83B65D jsl */
L_83B661:
    lda8(ea_dp(ACTOR_FACING));  /* 83B661 lda */
    if (!IF_N) goto L_83B669;  /* 83B663 bpl */
    and8_v(0x7F);  /* 83B665 and */
    sta8(ea_dp(ACTOR_FACING));  /* 83B667 sta */
L_83B669:
    actor_yield(0x83B669u);  /* 83B669 brk */
    abi_AnimStep();  /* 83B66B jsl */
    MEM8(dec, ea_dp(KIND2F_FRAMES));  /* 83B66F dec */
    if (!IF_Z) goto L_83B669;  /* 83B671 bne */
L_83B673:
    actor_yield(0x83B673u);  /* 83B673 brk */
    abi_AnimStep();  /* 83B675 jsl */
    abi_ActorFly();  /* 83B679 jsl */
    ldx8_v(0x00);  /* 83B67D ldx */
    push16(0xB681);
    abi_ActorEndOutOfRange();
    cpu.s = (uint16_t)(cpu.s + 2);  /* 83B67F jsr */
    goto L_83B673;  /* 83B682 bra */
}

