/*
 * small_actor_helpers.c: small actor routines: kind $89, which sets
 * Aladdin's sprite priority by where he is; a waiting actor that sends out
 * a kind $8E child every 8 frames; and a child of kind $3D started at a
 * spawn point and kept in its parent's list. Drafted by cdraft.py; named
 * and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_ActorCreateChild(void);
void abi_ActorCreateChild8(void);
void abi_ActorPictureSend(void);
void abi_AnimStep(void);

#define PLAYER_09 0x08E4u  /* the player's byte at $09 (bits 4-5: his sprite priority) */
#define PRIORITY_BY_SIDE 0xD7C8u  /* his priority bits: left of x $0934, then right of it (in the bank DB names) */
#define V_032D 0x032Du  /* the high byte of the word at $032C */
#define CHILD_COUNT 0x44u  /* children started so far; the list of their records is at $40 */

/*
 * @routine 84:8F9C Kind89PlayerPriority
 * @actor
 * WHAT IT DOES: Actor kind $89: each frame sets Aladdin's sprite priority
 * (bits 4-5 of his $09) from PRIORITY_BY_SIDE by whether he is left of x
 * $0934 or not, for good.
 * HOW: Each frame (BRK first): X = 0, or 1 when his x pixels are $0934 or
 * more; PLAYER_09 and $CF, OR PRIORITY_BY_SIDE at X.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
 * the actor loop's jump for kind $89 in state $0C (through the stub at
 * $83:9326). Exit: never returns.
 */
void abi_Kind89PlayerPriority(void)
{
L_848F9C:
    actor_yield(0x848F9Cu);  /* 848F9C brk */
    ldx8_v(0x00);  /* 848F9E ldx */
    op_rep(0x20);  /* 848FA0 rep */
    lda16(ea_abs(PLAYER + ACTOR_X + 1));  /* 848FA2 lda */
    cmp16_v(0x0934);  /* 848FA5 cmp */
    op_sep(0x20);  /* 848FA8 sep */
    if (!IF_C) goto L_848FAD;  /* 848FAA bcc */
    inx8();  /* 848FAC inx */
L_848FAD:
    lda8(ea_abs(PLAYER_09));  /* 848FAD lda */
    and8_v(0xCF);  /* 848FB0 and */
    ora8_v(rd8(ea_absx(PRIORITY_BY_SIDE)));  /* 848FB2 ora */
    sta8(ea_abs(PLAYER_09));  /* 848FB5 sta */
    goto L_848F9C;  /* 848FB8 bra */
}


/*
 * @routine 84:9B4D SenderWait8E
 * WHAT IT DOES: What a sending actor runs each frame it waits: its
 * animation on and its picture sent; when the low three bits of V_032D are
 * 0, a child of kind $8E ($07 = 1) at its spawn point $80.
 * HOW: AnimStep, ActorPictureSend; V_032D and 7 zero: $21 = $80, $07 = 1,
 * ActorCreateChild8 with kind $8E.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL from the
 * actor loop's stub at $83:9348. Exit: as ActorCreateChild8 or
 * ActorPictureSend.
 */
void abi_SenderWait8E(void)
{
    abi_AnimStep();  /* 849B4D jsl */
    abi_ActorPictureSend();  /* 849B51 jsl */
    lda8(ea_abs(V_032D));  /* 849B55 lda */
    and8_v(0x07);  /* 849B58 and */
    if (!IF_Z) goto L_849B6A;  /* 849B5A bne */
    lda8_v(0x80);  /* 849B5C lda */
    sta8(ea_dp(ACTOR_SPAWN_AT));  /* 849B5E sta */
    lda8_v(0x01);  /* 849B60 lda */
    sta8(ea_dp(ACTOR_07));  /* 849B62 sta */
    lda8_v(0x8E);  /* 849B64 lda */
    abi_ActorCreateChild8();  /* 849B66 jsl */
L_849B6A:
    return;  /* 849B6A rtl */
}


/*
 * @routine 83:C4EC ChildStartListed
 * WHAT IT DOES: Starts a child of kind $3D at spawn point X with $07 = Y,
 * and keeps its record in the actor's list of children ($40 + 2 * $44),
 * counting $44 up.
 * HOW: $21 = X, $07 = Y; ActorCreateChild $3D; the record (X) to Y; at $40
 * + $44 * 2 (direct page, X-indexed) the record (16-bit); M and X 8-bit;
 * $44 up.
 * Entry: M 8-bit, X 8-bit; X = the spawn point; Y = the child's $07; D =
 * the actor's record; DB = $83; called by JSR (83:C32E, 83:C358, 83:C35F).
 * Exit: M and X 8-bit; X = $44 * 2 before the count (low byte); Y = the
 * record's low byte.
 */
void abi_ChildStartListed(void)
{
    stx8(ea_dp(ACTOR_SPAWN_AT));  /* 83C4EC stx */
    sty8(ea_dp(ACTOR_07));  /* 83C4EE sty */
    lda8_v(0x3D);  /* 83C4F0 lda */
    abi_ActorCreateChild();  /* 83C4F2 jsl */
    txy16();  /* 83C4F6 txy */
    op_rep(0x20);  /* 83C4F7 rep */
    lda16(ea_dp(CHILD_COUNT));  /* 83C4F9 lda */
    and16_v(0x00FF);  /* 83C4FB and */
    ACC16(asl);  /* 83C4FE asl */
    tax16();  /* 83C4FF tax */
    sty16(ea_dpx(ACTOR_40));  /* 83C500 sty */
    op_sep(0x30);  /* 83C502 sep */
    MEM8(inc, ea_dp(CHILD_COUNT));  /* 83C504 inc */
    return;  /* 83C506 rts */
}
