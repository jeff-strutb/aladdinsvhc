/*
 * actor_anim_tile.c: a bank $A5 actor's animation chosen by the tile it
 * stands on and its speed.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorAnimByTile(void);
void abi_ActorAnimChange(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine A5:E9D2 ActorAnimByTile
 * WHAT IT DOES: Picks the actor's animation: $16 on a tile with behaviour
 * bit 5, else by its speed: 0 standing, 1 or 2 moving by the speed's sign
 * once it is far from $1C0, else 1 unless already 2 or 4.
 * HOW: Bit 5 of $0012: ActorAnimChange $16 (A5:EA13). Speed 0:
 * ActorAnimChange 0. Else the speed less $1C0 at $002C: outside -16 to
 * 15, 1 when its high byte is negative, else 2; inside, nothing when the
 * animation ($2D) is 4 or 2, else 1.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSR. Exit:
 * as ActorAnimChange, or M 8-bit.
 */
void abi_ActorAnimByTile(void)
{
    lda8(ea_abs(V_0012));  /* A5E9D2 lda */
    bit8_i(0x20);  /* A5E9D5 bit */
    if (!IF_Z) goto L_A5EA13;  /* A5E9D7 bne */
    op_rep(0x20);  /* A5E9D9 rep */
    lda16(ea_dp(0x19));  /* A5E9DB lda */
    if (IF_Z) goto L_A5EA0D;  /* A5E9DD beq */
    sec();  /* A5E9DF sec */
    sbc16_v(0x01C0);  /* A5E9E0 sbc */
    sta16(ea_abs(0x002C));  /* A5E9E3 sta */
    clc();  /* A5E9E6 clc */
    adc16_v(0x0010);  /* A5E9E7 adc */
    cmp16_v(0x0020);  /* A5E9EA cmp */
    if (!IF_C) goto L_A5E9FC;  /* A5E9ED bcc */
    op_sep(0x20);  /* A5E9EF sep */
    lda8_v(0x01);  /* A5E9F1 lda */
    ldx8(ea_abs(0x002D));  /* A5E9F3 ldx */
    if (IF_N) { abi_ActorAnimChange(); return; }  /* A5E9F6 bmi */
    lda8_v(0x02);  /* A5E9F8 lda */
    abi_ActorAnimChange(); return;  /* A5E9FA bra */
L_A5E9FC:
    op_sep(0x20);  /* A5E9FC sep */
    ldx8_v(0x04);  /* A5E9FE ldx */
    cpx8_v(rd8(ea_dp(0x2D)));  /* A5EA00 cpx */
    if (IF_Z) goto L_A5EA0C;  /* A5EA02 beq */
    lda8_v(0x01);  /* A5EA04 lda */
    ldx8_v(0x02);  /* A5EA06 ldx */
    cpx8_v(rd8(ea_dp(0x2D)));  /* A5EA08 cpx */
    if (!IF_Z) { abi_ActorAnimChange(); return; }  /* A5EA0A bne */
L_A5EA0C:
    return;  /* A5EA0C rts */
L_A5EA0D:
    op_sep(0x20);  /* A5EA0D sep */
    lda8_v(0x00);  /* A5EA0F lda */
    abi_ActorAnimChange(); return;  /* A5EA11 bra */
L_A5EA13:
    lda8_v(0x16);  /* A5EA13 lda */
    abi_ActorAnimChange(); return;
}
