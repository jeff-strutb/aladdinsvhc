/*
 * kind52.c: actor kind $52's start, the third level end's guard of act 1
 * stage 0. Drafted by cdraft.py; named and described by hand. Its wait is
 * Kind52Wait (kind_waits.c).
 */
#include "machine.h"
#include "ops65.h"
#include "actor.h"

void abi_ActorDirectionToPlayer(void);
void abi_ActorEndOutOfRange(void);
void abi_ActorMoveByVector(void);
void abi_ActorNearPlayer(void);
void abi_ActorSideOfPlayer(void);
void abi_ActorZoneHasTestBox(void);
void abi_AnimStart(void);
void abi_AnimStep(void);
void abi_PlayerEnter8CFacing(void);
void abi_ActorSpawnSameLook(void);
void abi_RandomFromList(void);
void abi_RandomNext(void);
void abi_SoundQueueAdd(void);

#define KIND52_SHOOTABLE 0x48u     /* nonzero: Kind52Wait tests shots on it */
#define KIND52_FRAMES 0x40u        /* frames left of what it is doing */
#define KIND52_REST 0x41u          /* frames left before it may strike again */
#define KIND52_SPARE 0x42u         /* counted down each frame; set only by the unreached block */
#define KIND52_LIFE 0x44u          /* variant: frames left of turning toward the player */
#define KIND52_TURN_IN 0x43u       /* variant: frames left to its next turn */
#define KIND52_DONE_TURNING 0x46u  /* variant: nonzero once its turning time is over */
#define KIND_COUNT 0x7EFF00u        /* by hit box, how many such actors are live */
#define PLAYER_X_PIXEL (PLAYER + ACTOR_X + 1)

/*
 * @routine 83:D359 Kind52Start
 * @actor
 * WHAT IT DOES: Actor kind $52 started. $07 zero: it waits until the
 * player comes near, faces him and rises (animation 4, then 1), and can
 * be shot from then on; then, each frame, facing him: within $38 pixels
 * across it strikes (animation 2 for $6C frames, sound $20) unless it is
 * resting after a strike; further away it waits, alternating animation 0
 * ($10 or $20 frames) and 1 ($20 or $2C frames), until he comes within
 * box $24. $07 nonzero: it flies toward the player by direction, turning
 * one step toward him every $14 frames for a random time, and hurts him
 * when its zone meets the test box; it ends out of range.
 * HOW: $48 = 0, bit 1 of $08 set. $07 zero: $09 |= $90, $21 = $07 = $4C,
 * animation 5; each frame until ActorNearPlayer box $34; ActorSideOfPlayer
 * into $14 and its opposite into $15; animation 4, $17 frames; $48 = 1, 9
 * frames; animation 1, $18 frames. The loop: each frame ActorSideOfPlayer
 * again, the distance |x - player's x| picks 0 (below $38), 1 (below $78)
 * or 2, $41 and $42 counted down to 0, then JMP ($D3F7,X). Way 0: $41
 * nonzero, back to the loop; else animation 2 for $6C frames with sound
 * $20 when $42 frames are left, then animation 0 and $41 =
 * RandomFromList $0A. Ways 1 and 2: animation 0 for $10 frames
 * (RandomNext negative) or $20, then animation 1 for $20 (negative) or
 * $2C, and again; back to the loop on any frame ActorNearPlayer box $24
 * says he is inside. The variant: animation 7, bits 0-1 of $08 cleared,
 * $14 times 8 (a direction), $44 = RandomFromList $10, $43 = $14, $46 =
 * 0; each frame: until $44 runs out, every $14 frames $14 one step (of
 * 16) toward ActorDirectionToPlayer unless already on it;
 * ActorMoveByVector $12, AnimStep, ActorEndOutOfRange 0;
 * ActorZoneHasTestBox $34 met: PlayerEnter8CFacing.
 * The block at $83:D42E-D46B (a COP wait, ActorSpawnSameLook with its hit
 * box and a count up at $7E:FF00 by hit box, setting $42) is reached from
 * nowhere: the table at $D3F7 holds $D3FD, $D46C, $D46C, and no branch,
 * jump or pointer names it. Its C stands where the block does, under a
 * label nothing jumps to.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for state $0C. Exit: never returns; the variant
 * ends in ActorEndOutOfRange.
 */
void abi_Kind52Start(void)
{
    stz8(ea_dp(KIND52_SHOOTABLE));  /* 83D359 stz */
    lda8_v(0x02);  /* 83D35B lda */
    tsb8(ea_dp(ACTOR_08));  /* 83D35D tsb */
    lda8(ea_dp(ACTOR_07));  /* 83D35F lda */
    if (IF_Z) goto L_83D366;  /* 83D361 beq */
    goto L_83D4B2;  /* 83D363 jmp */
L_83D366:
    lda8_v(0x90);  /* 83D366 lda */
    tsb8(ea_dp(ACTOR_09));  /* 83D368 tsb */
    lda8_v(0x4C);  /* 83D36A lda */
    sta8(ea_dp(ACTOR_SPAWN_AT));  /* 83D36C sta */
    sta8(ea_dp(ACTOR_07));  /* 83D36E sta */
    lda8_v(0x05);  /* 83D370 lda */
    abi_AnimStart();  /* 83D372 jsl */
L_83D376:
    actor_yield(0x83D376u);  /* 83D376 brk */
    ldy8_v(0x34);  /* 83D378 ldy */
    abi_ActorNearPlayer();  /* 83D37A jsl */
    if (IF_C) goto L_83D376;  /* 83D37E bcs */
    abi_ActorSideOfPlayer();  /* 83D380 jsl */
    sta8(ea_dp(ACTOR_FACING));  /* 83D384 sta */
    eor8_v(0x01);  /* 83D386 eor */
    sta8(ea_dp(ACTOR_FLIP));  /* 83D388 sta */
    lda8_v(0x04);  /* 83D38A lda */
    abi_AnimStart();  /* 83D38C jsl */
    lda8_v(0x17);  /* 83D390 lda */
    sta8(ea_dp(KIND52_FRAMES));  /* 83D392 sta */
L_83D394:
    actor_yield(0x83D394u);  /* 83D394 brk */
    MEM8(dec, ea_dp(KIND52_FRAMES));  /* 83D396 dec */
    if (!IF_Z) goto L_83D394;  /* 83D398 bne */
    lda8_v(0x01);  /* 83D39A lda */
    sta8(ea_dp(KIND52_SHOOTABLE));  /* 83D39C sta */
    lda8_v(0x09);  /* 83D39E lda */
    sta8(ea_dp(KIND52_FRAMES));  /* 83D3A0 sta */
L_83D3A2:
    actor_yield(0x83D3A2u);  /* 83D3A2 brk */
    MEM8(dec, ea_dp(KIND52_FRAMES));  /* 83D3A4 dec */
    if (!IF_Z) goto L_83D3A2;  /* 83D3A6 bne */
    lda8_v(0x01);  /* 83D3A8 lda */
    abi_AnimStart();  /* 83D3AA jsl */
    lda8_v(0x18);  /* 83D3AE lda */
    sta8(ea_dp(KIND52_FRAMES));  /* 83D3B0 sta */
L_83D3B2:
    actor_yield(0x83D3B2u);  /* 83D3B2 brk */
    MEM8(dec, ea_dp(KIND52_FRAMES));  /* 83D3B4 dec */
    if (!IF_Z) goto L_83D3B2;  /* 83D3B6 bne */
L_83D3B8:
    actor_yield(0x83D3B8u);  /* 83D3B8 brk */
    abi_ActorSideOfPlayer();  /* 83D3BA jsl */
    sta8(ea_dp(ACTOR_FACING));  /* 83D3BE sta */
    eor8_v(0x01);  /* 83D3C0 eor */
    sta8(ea_dp(ACTOR_FLIP));  /* 83D3C2 sta */
    op_rep(0x20);  /* 83D3C4 rep */
    lda16(ea_dp(ACTOR_X + 1));  /* 83D3C6 lda */
    sec();  /* 83D3C8 sec */
    sbc16_v(rd16(ea_abs(PLAYER_X_PIXEL)));  /* 83D3C9 sbc */
    if (!IF_N) goto L_83D3D2;  /* 83D3CC bpl */
    eor16_v(0xFFFF);  /* 83D3CE eor */
    ACC16(inc);  /* 83D3D1 inc */
L_83D3D2:
    cmp16_v(0x0038);  /* 83D3D2 cmp */
    if (IF_C) goto L_83D3DB;  /* 83D3D5 bcs */
    ldx8_v(0x00);  /* 83D3D7 ldx */
    goto L_83D3E6;  /* 83D3D9 bra */
L_83D3DB:
    cmp16_v(0x0078);  /* 83D3DB cmp */
    if (IF_C) goto L_83D3E4;  /* 83D3DE bcs */
    ldx8_v(0x02);  /* 83D3E0 ldx */
    goto L_83D3E6;  /* 83D3E2 bra */
L_83D3E4:
    ldx8_v(0x04);  /* 83D3E4 ldx */
L_83D3E6:
    op_sep(0x20);  /* 83D3E6 sep */
    lda8(ea_dp(KIND52_REST));  /* 83D3E8 lda */
    if (IF_Z) goto L_83D3EE;  /* 83D3EA beq */
    MEM8(dec, ea_dp(KIND52_REST));  /* 83D3EC dec */
L_83D3EE:
    lda8(ea_dp(KIND52_SPARE));  /* 83D3EE lda */
    if (IF_Z) goto L_83D3F4;  /* 83D3F0 beq */
    MEM8(dec, ea_dp(KIND52_SPARE));  /* 83D3F2 dec */
L_83D3F4:
    /* 83D3F4 jmp ($D3F7,x): $D3FD, $D46C, $D46C */
    if (cpu.x != 0)
        goto L_83D46C;
    lda8(ea_dp(KIND52_REST));  /* 83D3FD lda */
    if (!IF_Z) goto L_83D3B8;  /* 83D3FF bne, 83D42B jmp */
    lda8_v(0x02);  /* 83D401 lda */
    abi_AnimStart();  /* 83D403 jsl */
    lda8_v(0x6C);  /* 83D407 lda */
    sta8(ea_dp(KIND52_FRAMES));  /* 83D409 sta */
L_83D40B:
    actor_yield(0x83D40Bu);  /* 83D40B brk */
    lda8(ea_dp(KIND52_FRAMES));  /* 83D40D lda */
    cmp8_v(0x42);  /* 83D40F cmp */
    if (!IF_Z) goto L_83D419;  /* 83D411 bne */
    lda8_v(0x20);  /* 83D413 lda */
    abi_SoundQueueAdd();  /* 83D415 jsl */
L_83D419:
    MEM8(dec, ea_dp(KIND52_FRAMES));  /* 83D419 dec */
    if (!IF_Z) goto L_83D40B;  /* 83D41B bne */
    lda8_v(0x00);  /* 83D41D lda */
    abi_AnimStart();  /* 83D41F jsl */
    ldx8_v(0x0A);  /* 83D423 ldx */
    abi_RandomFromList();  /* 83D425 jsl */
    sta8(ea_dp(KIND52_REST));  /* 83D429 sta */
    goto L_83D3B8;  /* 83D42B jmp */
    /* Reached from nowhere (see the header). */
L_83D42E: __attribute__((unused));
    lda8(ea_dp(KIND52_SPARE));  /* 83D42E lda */
    if (!IF_Z) goto L_83D3B8;  /* 83D430 bne, 83D469 jmp */
    lda8_v(0x06);  /* 83D432 lda */
    abi_AnimStart();  /* 83D434 jsl */
    lda8_v(0x18);  /* 83D438 lda */
    actor_wait(0x83D43Au);  /* 83D43A cop */
    lda8(ea_dp(ACTOR_FLIP));  /* 83D43C lda */
    eor8_v(0x01);  /* 83D43E eor */
    sta8(ea_dp(ACTOR_FLIP));  /* 83D440 sta */
    lda8(ea_dp(ACTOR_06));  /* 83D442 lda */
    abi_ActorSpawnSameLook();  /* 83D444 jsl */
    if (IF_N) goto L_83D455;  /* 83D448 bmi */
    ldx8(ea_dp(ACTOR_06));  /* 83D44A ldx */
    lda8(ea_longx(KIND_COUNT));  /* 83D44C lda */
    ACC8(inc);  /* 83D450 inc */
    sta8(ea_longx(KIND_COUNT));  /* 83D451 sta */
L_83D455:
    lda8(ea_dp(ACTOR_FLIP));  /* 83D455 lda */
    eor8_v(0x01);  /* 83D457 eor */
    sta8(ea_dp(ACTOR_FLIP));  /* 83D459 sta */
    lda8_v(0x00);  /* 83D45B lda */
    abi_AnimStart();  /* 83D45D jsl */
    ldx8_v(0x0E);  /* 83D461 ldx */
    abi_RandomFromList();  /* 83D463 jsl */
    sta8(ea_dp(KIND52_SPARE));  /* 83D467 sta */
    goto L_83D3B8;  /* 83D469 jmp */
L_83D46C:
    ldx8_v(0x10);  /* 83D46C ldx */
    abi_RandomNext();  /* 83D46E jsl */
    if (IF_N) goto L_83D476;  /* 83D472 bmi */
    ldx8_v(0x20);  /* 83D474 ldx */
L_83D476:
    stx8(ea_dp(KIND52_FRAMES));  /* 83D476 stx */
    lda8_v(0x00);  /* 83D478 lda */
    abi_AnimStart();  /* 83D47A jsl */
L_83D47E:
    actor_yield(0x83D47Eu);  /* 83D47E brk */
    MEM8(dec, ea_dp(KIND52_FRAMES));  /* 83D480 dec */
    if (IF_Z) goto L_83D48F;  /* 83D482 beq */
    ldy8_v(0x24);  /* 83D484 ldy */
    abi_ActorNearPlayer();  /* 83D486 jsl */
    if (IF_C) goto L_83D47E;  /* 83D48A bcs */
    goto L_83D3B8;  /* 83D48C jmp */
L_83D48F:
    ldx8_v(0x20);  /* 83D48F ldx */
    abi_RandomNext();  /* 83D491 jsl */
    if (IF_N) goto L_83D499;  /* 83D495 bmi */
    ldx8_v(0x2C);  /* 83D497 ldx */
L_83D499:
    stx8(ea_dp(KIND52_FRAMES));  /* 83D499 stx */
    lda8_v(0x01);  /* 83D49B lda */
    abi_AnimStart();  /* 83D49D jsl */
L_83D4A1:
    actor_yield(0x83D4A1u);  /* 83D4A1 brk */
    MEM8(dec, ea_dp(KIND52_FRAMES));  /* 83D4A3 dec */
    if (IF_Z) goto L_83D46C;  /* 83D4A5 beq */
    ldy8_v(0x24);  /* 83D4A7 ldy */
    abi_ActorNearPlayer();  /* 83D4A9 jsl */
    if (IF_C) goto L_83D4A1;  /* 83D4AD bcs */
    goto L_83D3B8;  /* 83D4AF jmp */
L_83D4B2:
    lda8_v(0x07);  /* 83D4B2 lda */
    abi_AnimStart();  /* 83D4B4 jsl */
    lda8_v(0x03);  /* 83D4B8 lda */
    trb8(ea_dp(ACTOR_08));  /* 83D4BA trb */
    MEM8(asl, ea_dp(ACTOR_FACING));  /* 83D4BC asl */
    MEM8(asl, ea_dp(ACTOR_FACING));  /* 83D4BE asl */
    MEM8(asl, ea_dp(ACTOR_FACING));  /* 83D4C0 asl */
    ldx8_v(0x10);  /* 83D4C2 ldx */
    abi_RandomFromList();  /* 83D4C4 jsl */
    sta8(ea_dp(KIND52_LIFE));  /* 83D4C8 sta */
    lda8_v(0x14);  /* 83D4CA lda */
    sta8(ea_dp(KIND52_TURN_IN));  /* 83D4CC sta */
    stz8(ea_dp(KIND52_DONE_TURNING));  /* 83D4CE stz */
L_83D4D0:
    actor_yield(0x83D4D0u);  /* 83D4D0 brk */
    lda8(ea_dp(KIND52_DONE_TURNING));  /* 83D4D2 lda */
    if (!IF_Z) goto L_83D4FD;  /* 83D4D4 bne */
    MEM8(dec, ea_dp(KIND52_LIFE));  /* 83D4D6 dec */
    if (!IF_Z) goto L_83D4DC;  /* 83D4D8 bne */
    MEM8(inc, ea_dp(KIND52_DONE_TURNING));  /* 83D4DA inc */
L_83D4DC:
    MEM8(dec, ea_dp(KIND52_TURN_IN));  /* 83D4DC dec */
    if (!IF_Z) goto L_83D4FD;  /* 83D4DE bne */
    lda8_v(0x14);  /* 83D4E0 lda */
    sta8(ea_dp(KIND52_TURN_IN));  /* 83D4E2 sta */
    abi_ActorDirectionToPlayer();  /* 83D4E4 jsl */
    sec();  /* 83D4E8 sec */
    sbc8_v(rd8(ea_dp(ACTOR_FACING)));  /* 83D4E9 sbc */
    and8_v(0x0F);  /* 83D4EB and */
    if (IF_Z) goto L_83D4FD;  /* 83D4ED beq */
    cmp8_v(0x08);  /* 83D4EF cmp */
    ldx8(ea_dp(ACTOR_FACING));  /* 83D4F1 ldx */
    inx8();  /* 83D4F3 inx */
    if (!IF_C) goto L_83D4F8;  /* 83D4F4 bcc */
    dex8();  /* 83D4F6 dex */
    dex8();  /* 83D4F7 dex */
L_83D4F8:
    txa8();  /* 83D4F8 txa */
    and8_v(0x0F);  /* 83D4F9 and */
    sta8(ea_dp(ACTOR_FACING));  /* 83D4FB sta */
L_83D4FD:
    ldx8_v(0x12);  /* 83D4FD ldx */
    abi_ActorMoveByVector();  /* 83D4FF jsl */
    abi_AnimStep();  /* 83D503 jsl */
    ldx8_v(0x00);  /* 83D507 ldx */
    push16(0xD50B); abi_ActorEndOutOfRange(); cpu.s = (uint16_t)(cpu.s + 2);  /* 83D509 jsr */
    ldy8_v(0x34);  /* 83D50C ldy */
    abi_ActorZoneHasTestBox();  /* 83D50E jsr */
    if (IF_C) goto L_83D4D0;  /* 83D511 bcs */
    abi_PlayerEnter8CFacing();  /* 83D513 jsr */
    goto L_83D4D0;  /* 83D516 bra */
}
