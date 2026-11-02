/*
 * ground_push_marks.c: actor kind $60 (and $5F in state $4C): the mark at
 * Aladdin's feet while the ground pushes him or he slides on it, shown
 * only then. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor.h"
#include "actor_anim.h"

void abi_AnimStart(void);

#define GROUND_PUSHED 0x1429u  /* 2 while the player stands on ground that pushes, 1 while he slides on it, else 0 */
#define PLAYER_X 0x08E7u  /* the player's x pixels (word) */
#define PLAYER_Y 0x08EAu  /* the player's y pixels (word) */
#define MARK_ANIM 0x41u  /* the animation it shows */
#define MARK_ANIM_WANTED 0x40u  /* the animation it is to show */

/*
 * @routine 83:E480 GroundPushMark
 * @actor
 * WHAT IT DOES: Actor kind $60 started new (and kind $5F in state $4C): a
 * mark that follows Aladdin's feet (his x, 24 below his y) for good; shown
 * animated only while GROUND_PUSHED is set: animation 9 while the ground
 * pushes him (2), $0A while he slides (1).
 * HOW: $09 bits $90 set, its look $0600; $40 and $41 = $0A, AnimStart $0A;
 * $08 bit 0 set, bit 3 (animated) cleared. Each frame (BRK first): its
 * position from the player's; GROUND_PUSHED zero: bit 3 of $08 cleared;
 * else set, the animation wanted 9 (2) or $0A (1), started when it is not
 * the one shown ($41).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
 * the actor loop's jump for kind $60 in state $0C and kind $5F in state
 * $4C. Exit: never returns.
 */
void abi_GroundPushMark(void)
{
    lda8(ea_dp(ACTOR_09));  /* 83E480 lda */
    ora8_v(0x90);  /* 83E482 ora */
    sta8(ea_dp(ACTOR_09));  /* 83E484 sta */
    op_rep(0x20);  /* 83E486 rep */
    lda16_v(0x0600);  /* 83E488 lda */
    sta16(ea_dp(ACTOR_LOOK));  /* 83E48B sta */
    op_sep(0x20);  /* 83E48D sep */
    lda8_v(0x0A);  /* 83E48F lda */
    sta8(ea_dp(MARK_ANIM_WANTED));  /* 83E491 sta */
    sta8(ea_dp(MARK_ANIM));  /* 83E493 sta */
    abi_AnimStart();  /* 83E495 jsl */
    lda8(ea_dp(ACTOR_08));  /* 83E499 lda */
    ora8_v(0x01);  /* 83E49B ora */
    and8_v(0xF7);  /* 83E49D and */
    sta8(ea_dp(ACTOR_08));  /* 83E49F sta */
L_83E4A1:
    actor_yield(0x83E4A1u);  /* 83E4A1 brk */
    op_rep(0x20);  /* 83E4A3 rep */
    lda16(ea_abs(PLAYER_X));  /* 83E4A5 lda */
    sta16(ea_dp(ACTOR_X + 1));  /* 83E4A8 sta */
    lda16(ea_abs(PLAYER_Y));  /* 83E4AA lda */
    clc();  /* 83E4AD clc */
    adc16_v(0x0018);  /* 83E4AE adc */
    sta16(ea_dp(ACTOR_Y + 1));  /* 83E4B1 sta */
    op_sep(0x20);  /* 83E4B3 sep */
    lda8(ea_abs(GROUND_PUSHED));  /* 83E4B5 lda */
    if (!IF_Z) goto L_83E4C2;  /* 83E4B8 bne */
    lda8(ea_dp(ACTOR_08));  /* 83E4BA lda */
    and8_v(0xF7);  /* 83E4BC and */
    sta8(ea_dp(ACTOR_08));  /* 83E4BE sta */
    goto L_83E4A1;  /* 83E4C0 bra */
L_83E4C2:
    lda8(ea_dp(ACTOR_08));  /* 83E4C2 lda */
    ora8_v(0x08);  /* 83E4C4 ora */
    sta8(ea_dp(ACTOR_08));  /* 83E4C6 sta */
    lda8(ea_abs(GROUND_PUSHED));  /* 83E4C8 lda */
    ACC8(dec);  /* 83E4CB dec */
    if (!IF_Z) goto L_83E4DF;  /* 83E4CC bne */
    lda8_v(0x0A);  /* 83E4CE lda */
L_83E4D0:
    sta8(ea_dp(MARK_ANIM_WANTED));  /* 83E4D0 sta */
    sec();  /* 83E4D2 sec */
    cmp8_v(rd8(ea_dp(MARK_ANIM)));  /* 83E4D3 cmp */
    if (IF_Z) goto L_83E4A1;  /* 83E4D5 beq */
    sta8(ea_dp(MARK_ANIM));  /* 83E4D7 sta */
    abi_AnimStart();  /* 83E4D9 jsl */
    goto L_83E4A1;  /* 83E4DD bra */
L_83E4DF:
    lda8_v(0x09);  /* 83E4DF lda */
    goto L_83E4D0;  /* 83E4E1 bra */
}
