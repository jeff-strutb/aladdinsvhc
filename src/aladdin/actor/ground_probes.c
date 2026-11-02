/*
 * ground_probes.c: tests of the ground round an actor of bank $A5: a
 * wall in front, a ledge ahead with room above it, ground above a point
 * toward the player.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorLedgeAhead(void);
void abi_ActorOutOfWall8(void);
void abi_ActorOutOfWallLong(void);
void abi_ActorProbeUpTowardPlayer(void);
void abi_PointInGroundLong(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine A5:EB17 ActorOutOfWall8
 * WHAT IT DOES: Moves the actor out of a wall at its probe 8 in front:
 * C set when it was moved.
 * HOW: ActorOutOfWallLong with A = 8.
 * Entry: M 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
 * ActorOutOfWall.
 */
void abi_ActorOutOfWall8(void)
{
    lda8_v(0x08);  /* A5EB17 lda */
    abi_ActorOutOfWallLong();  /* A5EB19 jsl */
    return;  /* A5EB1D rts */
}

/*
 * @routine A5:EA2C ActorLedgeAhead
 * WHAT IT DOES: Looks for ground 48 pixels ahead of the actor with room
 * above it, from 32 pixels below its y up: C clear when found within four
 * tiles.
 * HOW: The point 48 pixels in front by the facing ($30, or -$30 facing
 * left) and 32 below; up to 4 probes (PointInGroundLong, a kind in the
 * behaviour's low 3 bits being ground), a tile up each time: first while
 * no ground, then, once in ground, a tile up at a time until a probe finds
 * none (C clear). C set when the probes run out.
 * Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSR. Exit:
 * M 8-bit, X 8-bit; C as above; the point at the last probe.
 */
void abi_ActorLedgeAhead(void)
{
    op_rep(0x20);  /* A5EA2C rep */
    lda16_v(0x0030);  /* A5EA2E lda */
    ldx8(ea_dp(0x14));  /* A5EA31 ldx */
    if (IF_Z) goto L_A5EA38;  /* A5EA33 beq */
    lda16_v(0xFFD0);  /* A5EA35 lda */
L_A5EA38:
    clc();  /* A5EA38 clc */
    adc16_v(rd16(ea_dp(0x0C)));  /* A5EA39 adc */
    sta16(ea_abs(SCRATCH));  /* A5EA3B sta */
    clc();  /* A5EA3E clc */
    lda16_v(0x0020);  /* A5EA3F lda */
    adc16_v(rd16(ea_dp(0x0F)));  /* A5EA42 adc */
    sta16(ea_abs(0x000E));  /* A5EA44 sta */
    lda16_v(0x0004);  /* A5EA47 lda */
    sta16(ea_abs(0x002C));  /* A5EA4A sta */
L_A5EA4D:
    php();  /* A5EA4D php */
    abi_PointInGroundLong();  /* A5EA4E jsl */
    plp();  /* A5EA52 plp */
    op_sep(0x30);  /* A5EA53 sep */
    lda8(ea_abs(V_0012));  /* A5EA55 lda */
    and8_v(0x07);  /* A5EA58 and */
    if (!IF_Z) goto L_A5EA6F;  /* A5EA5A bne */
    MEM8(dec, ea_abs(0x002C));  /* A5EA5C dec */
    if (IF_Z) goto L_A5EA8F;  /* A5EA5F beq */
    op_rep(0x20);  /* A5EA61 rep */
    clc();  /* A5EA63 clc */
    lda16(ea_abs(0x000E));  /* A5EA64 lda */
    adc16_v(0xFFF0);  /* A5EA67 adc */
    sta16(ea_abs(0x000E));  /* A5EA6A sta */
    goto L_A5EA4D;  /* A5EA6D bra */
L_A5EA6F:
    op_rep(0x20);  /* A5EA6F rep */
    clc();  /* A5EA71 clc */
    lda16(ea_abs(0x000E));  /* A5EA72 lda */
    adc16_v(0xFFF0);  /* A5EA75 adc */
    sta16(ea_abs(0x000E));  /* A5EA78 sta */
    php();  /* A5EA7B php */
    abi_PointInGroundLong();  /* A5EA7C jsl */
    plp();  /* A5EA80 plp */
    op_sep(0x30);  /* A5EA81 sep */
    lda8(ea_abs(V_0012));  /* A5EA83 lda */
    and8_v(0x07);  /* A5EA86 and */
    if (IF_Z) goto L_A5EA91;  /* A5EA88 beq */
    MEM8(dec, ea_abs(0x002C));  /* A5EA8A dec */
    if (!IF_Z) goto L_A5EA6F;  /* A5EA8D bne */
L_A5EA8F:
    sec();  /* A5EA8F sec */
    return;  /* A5EA90 rts */
L_A5EA91:
    clc();  /* A5EA91 clc */
    return;  /* A5EA92 rts */
}

/*
 * @routine A5:F242 ActorProbeUpTowardPlayer
 * WHAT IT DOES: Probes up from a point offset from the actor toward the
 * player, a tile at a time for X probes, for a tile whose behaviour has
 * bit 3 or 6: C clear when one is found.
 * HOW: The point's y = the actor's + A; its x = the actor's + Y when the
 * player is not left of the actor, else - Y; X probes (PointInGroundLong,
 * the behaviour byte tested with $48), the point a tile up after each.
 * Entry: M 16-bit, X 8-bit; A = the y offset; X = the probes; Y = the x
 * offset; D = the actor; DB = $81; called by JSR. Exit: M 16-bit, X
 * 8-bit; C as above.
 */
void abi_ActorProbeUpTowardPlayer(void)
{
    op_rep(0x20);  /* A5F242 rep */
    clc();  /* A5F244 clc */
    adc16_v(rd16(ea_dp(0x0F)));  /* A5F245 adc */
    sta16(ea_abs(0x000E));  /* A5F247 sta */
    txa16();  /* A5F24A txa */
    sta16(ea_abs(0x002C));  /* A5F24B sta */
    lda16(ea_abs(0x08E7));  /* A5F24E lda */
    cmp16_v(rd16(ea_dp(0x0C)));  /* A5F251 cmp */
    tya16();  /* A5F253 tya */
    if (IF_C) goto L_A5F25A;  /* A5F254 bcs */
    eor16_v(0xFFFF);  /* A5F256 eor */
    ACC16(inc);  /* A5F259 inc */
L_A5F25A:
    clc();  /* A5F25A clc */
    adc16_v(rd16(ea_dp(0x0C)));  /* A5F25B adc */
    sta16(ea_abs(SCRATCH));  /* A5F25D sta */
L_A5F260:
    abi_PointInGroundLong();  /* A5F260 jsl */
    op_sep(0x30);  /* A5F264 sep */
    lda8(ea_abs(V_0012));  /* A5F266 lda */
    bit8_i(0x48);  /* A5F269 bit */
    if (!IF_Z) goto L_A5F280;  /* A5F26B bne */
    op_rep(0x20);  /* A5F26D rep */
    clc();  /* A5F26F clc */
    lda16(ea_abs(0x000E));  /* A5F270 lda */
    adc16_v(0xFFF0);  /* A5F273 adc */
    sta16(ea_abs(0x000E));  /* A5F276 sta */
    MEM16(dec, ea_abs(0x002C));  /* A5F279 dec */
    if (!IF_Z) goto L_A5F260;  /* A5F27C bne */
    sec();  /* A5F27E sec */
    return;  /* A5F27F rts */
L_A5F280:
    op_rep(0x20);  /* A5F280 rep */
    clc();  /* A5F282 clc */
    return;  /* A5F283 rts */
}
