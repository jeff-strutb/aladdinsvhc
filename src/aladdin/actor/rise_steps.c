/*
 * rise_steps.c: actor kinds $88 (a thing that flies up a course and back
 * down each time it is sent) and $96 (one that shows itself after its
 * waits). Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorFly(void);
void abi_ActorFlyAgainst(void);
void abi_ActorMotionPreset(void);
void abi_ActorYHistoryPush(void);
void abi_AnimStart(void);
void abi_SoundQueueAdd(void);

#define V_1B1A 0x1B1Au  /* nonzero: send the riser */
#define V_1B1B 0x1B1Bu  /* the course to send it on */
#define RISE_UP 0xD7BEu  /* by course, its motion preset up (in the bank DB names) */
#define RISE_DOWN 0xD7C1u  /* by course, its motion preset down */
#define RISE_FLOOR 0x40u  /* word: the y pixels it rests at ($0100) */
#define RISE_COURSE 0x42u  /* the course it was sent on */
#define STEP_WAITS 0xD8FCu  /* by $07, the frames its first wait lasts (in the bank DB names) */
#define STEP_WAIT 0x40u  /* the frames of its second wait */

/*
 * @routine 84:8E6A RiserActor
 * @actor
 * WHAT IT DOES: Actor kind $88: rests at y 256 until it is sent (V_1B1A),
 * then flies up its course (V_1B1B) until it starts down, and comes back
 * down to y 256; its heights are kept in the y history (ActorYHistoryPush)
 * all the while. Sent again as long as V_1B1A stays set.
 * HOW: $09 = $80; resting: y and $40 = $0100, BRK, until V_1B1A; the
 * course to $42, ActorMotionPreset by RISE_UP; each frame ActorFlyAgainst
 * and ActorYHistoryPush while y is at or below $40; then ActorMotionPreset
 * by RISE_DOWN, each frame ActorFly and ActorYHistoryPush until y is back
 * at $40 or lower; then V_1B1A again.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $88 in state $0C (through the stub at
 * $83:9319). Exit: never returns.
 */
void abi_RiserActor(void)
{
    lda8_v(0x80);  /* 848E6A lda */
    sta8(ea_dp(ACTOR_09));  /* 848E6C sta */
L_848E6E:
    op_rep(0x20);  /* 848E6E rep */
    lda16_v(0x0100);  /* 848E70 lda */
    sta16(ea_dp(ACTOR_Y + 1));  /* 848E73 sta */
    sta16(ea_dp(ACTOR_40));  /* 848E75 sta */
    op_sep(0x20);  /* 848E77 sep */
    actor_yield(0x848E79u);  /* 848E79 brk */
L_848E7B:
    lda8(ea_abs(V_1B1A));  /* 848E7B lda */
    if (IF_Z) goto L_848E6E;  /* 848E7E beq */
    ldx8(ea_abs(V_1B1B));  /* 848E80 ldx */
    stx8(ea_dp(ACTOR_42));  /* 848E83 stx */
    ldy8(ea_absx(RISE_UP));  /* 848E85 ldy */
    abi_ActorMotionPreset();  /* 848E88 jsl */
L_848E8C:
    actor_yield(0x848E8Cu);  /* 848E8C brk */
    abi_ActorFlyAgainst();  /* 848E8E jsl */
    abi_ActorYHistoryPush();  /* 848E92 jsr */
    op_rep(0x20);  /* 848E95 rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 848E97 lda */
    cmp16_v(rd16(ea_dp(ACTOR_40)));  /* 848E99 cmp */
    op_sep(0x20);  /* 848E9B sep */
    if (IF_C) goto L_848E8C;  /* 848E9D bcs */
    ldx8(ea_dp(ACTOR_42));  /* 848E9F ldx */
    ldy8(ea_absx(RISE_DOWN));  /* 848EA1 ldy */
    abi_ActorMotionPreset();  /* 848EA4 jsl */
L_848EA8:
    actor_yield(0x848EA8u);  /* 848EA8 brk */
    abi_ActorFly();  /* 848EAA jsl */
    abi_ActorYHistoryPush();  /* 848EAE jsr */
    op_rep(0x20);  /* 848EB1 rep */
    lda16(ea_dp(ACTOR_Y + 1));  /* 848EB3 lda */
    cmp16_v(rd16(ea_dp(ACTOR_40)));  /* 848EB5 cmp */
    op_sep(0x20);  /* 848EB7 sep */
    if (!IF_C) goto L_848EA8;  /* 848EB9 bcc */
    goto L_848E7B;  /* 848EBB bra */
}


/*
 * @routine 84:9EF5 SteppedShowActor
 * @actor
 * WHAT IT DOES: Actor kind $96. With $07 bit 4 clear: waits the frames
 * STEP_WAITS gives for $07, then as long again, then shows itself (sound
 * $44, animation twice $07) and stays. Bit 4 set: shown at once (animation
 * 1) at its priority bits set, and stays.
 * HOW: Bits 1-3 of $2C cleared. Clear: $09 bit 4 and $2C bits 2-3 set;
 * STEP_WAITS by $07 to $40, COP with it, COP with $40; $08 bits 0-1 and
 * $09 bit 7 set; SoundQueueAdd $44; AnimStart $07 * 2; COP for good. Set:
 * $2C bits 1-3, $08 bit 1, $09 bits $B0 set; AnimStart 1; BRK for good.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $96 in state $0C (through the stub at
 * $83:93AC). Exit: never returns.
 */
void abi_SteppedShowActor(void)
{
    lda8_v(0x0E);  /* 849EF5 lda */
    trb8(ea_dp(ACTOR_2C));  /* 849EF7 trb */
    lda8(ea_dp(ACTOR_07));  /* 849EF9 lda */
    bit8_i(0x10);  /* 849EFB bit */
    if (IF_Z) goto L_849F02;  /* 849EFD beq */
    goto L_849F32;  /* 849EFF jmp */
L_849F02:
    lda8_v(0x10);  /* 849F02 lda */
    tsb8(ea_dp(ACTOR_09));  /* 849F04 tsb */
    lda8_v(0x0C);  /* 849F06 lda */
    tsb8(ea_dp(ACTOR_2C));  /* 849F08 tsb */
    ldx8(ea_dp(ACTOR_07));  /* 849F0A ldx */
    lda8(ea_absx(STEP_WAITS));  /* 849F0C lda */
    sta8(ea_dp(ACTOR_40));  /* 849F0F sta */
    actor_wait(0x849F11u);  /* 849F11 cop */
    lda8(ea_dp(ACTOR_40));  /* 849F13 lda */
    actor_wait(0x849F15u);  /* 849F15 cop */
    lda8(ea_dp(ACTOR_08));  /* 849F17 lda */
    ora8_v(0x03);  /* 849F19 ora */
    sta8(ea_dp(ACTOR_08));  /* 849F1B sta */
    lda8_v(0x80);  /* 849F1D lda */
    tsb8(ea_dp(ACTOR_09));  /* 849F1F tsb */
    lda8_v(0x44);  /* 849F21 lda */
    abi_SoundQueueAdd();  /* 849F23 jsl */
    lda8(ea_dp(ACTOR_07));  /* 849F27 lda */
    ACC8(asl);  /* 849F29 asl */
    abi_AnimStart();  /* 849F2A jsl */
L_849F2E:
    actor_wait(0x849F2Eu);  /* 849F2E cop */
    goto L_849F2E;  /* 849F30 bra */
L_849F32:
    lda8_v(0x0E);  /* 849F32 lda */
    tsb8(ea_dp(ACTOR_2C));  /* 849F34 tsb */
    lda8(ea_dp(ACTOR_08));  /* 849F36 lda */
    ora8_v(0x02);  /* 849F38 ora */
    sta8(ea_dp(ACTOR_08));  /* 849F3A sta */
    lda8_v(0xB0);  /* 849F3C lda */
    tsb8(ea_dp(ACTOR_09));  /* 849F3E tsb */
    lda8_v(0x01);  /* 849F40 lda */
    abi_AnimStart();  /* 849F42 jsl */
L_849F46:
    actor_yield(0x849F46u);  /* 849F46 brk */
    goto L_849F46;  /* 849F48 bra */
}
