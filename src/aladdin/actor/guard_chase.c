/*
 * guard_chase.c: the bank $A5 guard's chase: keeping pace with the
 * player, and jumping up or down to a ledge when he is well above or
 * below.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorLedgeAbove(void);
void abi_ActorMotionPreset(void);
void abi_ActorProbeUpTowardPlayer(void);
void abi_GuardChase(void);
void abi_GuardLedgeJump(void);
void abi_PointInGroundLong(void);
void abi_RandomFromList(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine A5:F284 ActorLedgeAbove
 * WHAT IT DOES: Looks for the top of the ground at a point offset from
 * the actor, going up a tile at a time: C clear when a ground tile with
 * an empty tile above it is found within X probes.
 * HOW: The point's y = the actor's + A, its x = the actor's + Y (- Y
 * facing left). PointInGroundLong (a ground tile has a kind in the
 * behaviour's low 3 bits): while empty, a tile up and a probe less; once
 * in ground, a tile up at a time until a probe finds none (C clear). C
 * set when the probes run out.
 * Entry: M 16-bit, X 8-bit; A = the y offset; X = the probes; Y = the x
 * offset; D = the actor; DB = $81; called by JSR. Exit: M 16-bit, X
 * 8-bit; C as above; $000C, $000E the last point; $002C the probes left.
 */
void abi_ActorLedgeAbove(void)
{
    op_rep(0x20);  /* A5F284 rep */
    clc();  /* A5F286 clc */
    adc16_v(rd16(ea_dp(0x0F)));  /* A5F287 adc */
    sta16(ea_abs(0x000E));  /* A5F289 sta */
    txa16();  /* A5F28C txa */
    sta16(ea_abs(0x002C));  /* A5F28D sta */
    tya16();  /* A5F290 tya */
    ldy8(ea_dp(0x14));  /* A5F291 ldy */
    if (IF_Z) goto L_A5F299;  /* A5F293 beq */
    eor16_v(0xFFFF);  /* A5F295 eor */
    ACC16(inc);  /* A5F298 inc */
L_A5F299:
    clc();  /* A5F299 clc */
    adc16_v(rd16(ea_dp(0x0C)));  /* A5F29A adc */
    sta16(ea_abs(SCRATCH));  /* A5F29C sta */
L_A5F29F:
    abi_PointInGroundLong();  /* A5F29F jsl */
    op_sep(0x30);  /* A5F2A3 sep */
    lda8(ea_abs(V_0012));  /* A5F2A5 lda */
    and8_v(0x07);  /* A5F2A8 and */
    if (!IF_Z) goto L_A5F2BF;  /* A5F2AA bne */
    op_rep(0x20);  /* A5F2AC rep */
    MEM16(dec, ea_abs(0x002C));  /* A5F2AE dec */
    if (IF_Z) goto L_A5F2DF;  /* A5F2B1 beq */
    clc();  /* A5F2B3 clc */
    lda16(ea_abs(0x000E));  /* A5F2B4 lda */
    adc16_v(0xFFF0);  /* A5F2B7 adc */
    sta16(ea_abs(0x000E));  /* A5F2BA sta */
    goto L_A5F29F;  /* A5F2BD bra */
L_A5F2BF:
    op_rep(0x20);  /* A5F2BF rep */
    clc();  /* A5F2C1 clc */
    lda16(ea_abs(0x000E));  /* A5F2C2 lda */
    adc16_v(0xFFF0);  /* A5F2C5 adc */
    sta16(ea_abs(0x000E));  /* A5F2C8 sta */
    abi_PointInGroundLong();  /* A5F2CB jsl */
    op_sep(0x30);  /* A5F2CF sep */
    lda8(ea_abs(V_0012));  /* A5F2D1 lda */
    and8_v(0x07);  /* A5F2D4 and */
    if (IF_Z) goto L_A5F2E1;  /* A5F2D6 beq */
    MEM8(dec, ea_abs(0x002C));  /* A5F2D8 dec */
    if (!IF_Z) goto L_A5F2BF;  /* A5F2DB bne */
    op_rep(0x20);  /* A5F2DD rep */
L_A5F2DF:
    sec();  /* A5F2DF sec */
    return;  /* A5F2E0 rts */
L_A5F2E1:
    op_rep(0x20);  /* A5F2E1 rep */
    clc();  /* A5F2E3 clc */
    return;  /* A5F2E4 rts */
}

/*
 * @routine A5:F19D GuardLedgeJump
 * WHAT IT DOES: When the player is well above or below the guard, picks a
 * ledge toward him and starts the guard's jump to it, leaving its caller;
 * C set when it does not jump (or returns at once when he is near its
 * height).
 * HOW: The player's y + 16 less the guard's. Above by more than 128, or
 * by 65-128 and a random pick (RandomFromList, list $4A) allows:
 * ActorProbeUpTowardPlayer (offset -48, 5 probes, 24 across); found with
 * more than 2 probes left or on a tile with behaviour bit 3: the guard's
 * code word ($03) = $E6A0, the caller's return dropped, facing the
 * player, ActorMotionPreset by the probes left (table $D054), on a bit-3
 * tile its speed + $80. Below with $44 set ($44 then cleared): by less
 * than 64 only on a random pick (list $4C); then ActorLedgeAbove (96
 * below, 5 probes, 32 ahead); found: code word $E6A0, return dropped,
 * ActorMotionPreset $83.
 * Entry: M 16-bit, X 8-bit; D = the guard; DB = $81; called by JSR from
 * GuardChase. Exit: jumping: to GuardChase's caller, M 8-bit; else M
 * 16-bit and C set, or C as the compare left it.
 */
void abi_GuardLedgeJump(void)
{
    clc();  /* A5F19D clc */
    lda16(ea_abs(0x08EA));  /* A5F19E lda */
    adc16_v(0x0010);  /* A5F1A1 adc */
    sec();  /* A5F1A4 sec */
    sbc16_v(rd16(ea_dp(0x0F)));  /* A5F1A5 sbc */
    if (!IF_N) goto L_A5F1B5;  /* A5F1A7 bpl */
    cmp16_v(0xFFC0);  /* A5F1A9 cmp */
    if (IF_C) goto L_A5F1C4;  /* A5F1AC bcs */
    cmp16_v(0xFF80);  /* A5F1AE cmp */
    if (IF_C) goto L_A5F1C5;  /* A5F1B1 bcs */
    goto L_A5F1CF;  /* A5F1B3 bra */
L_A5F1B5:
    ldx8(ea_dp(0x44));  /* A5F1B5 ldx */
    if (IF_Z) goto L_A5F1C4;  /* A5F1B7 beq */
    ldx8_v(0x00);  /* A5F1B9 ldx */
    stx8(ea_dp(0x44));  /* A5F1BB stx */
    cmp16_v(0x0040);  /* A5F1BD cmp */
    if (!IF_C) goto L_A5F21B;  /* A5F1C0 bcc */
    goto L_A5F225;  /* A5F1C2 bra */
L_A5F1C4:
    return;  /* A5F1C4 rts */
L_A5F1C5:
    ldx8_v(0x4A);  /* A5F1C5 ldx */
    abi_RandomFromList();  /* A5F1C7 jsl */
    op_rep(0x20);  /* A5F1CB rep */
    if (IF_Z) goto L_A5F219;  /* A5F1CD beq */
L_A5F1CF:
    lda16_v(0xFFD0);  /* A5F1CF lda */
    ldx8_v(0x05);  /* A5F1D2 ldx */
    ldy8_v(0x18);  /* A5F1D4 ldy */
    abi_ActorProbeUpTowardPlayer();  /* A5F1D6 jsr */
    if (IF_C) goto L_A5F240;  /* A5F1D9 bcs */
    ldx8_v(0x02);  /* A5F1DB ldx */
    cpx8_v(rd8(ea_abs(0x002C)));  /* A5F1DD cpx */
    if (!IF_C) goto L_A5F1E7;  /* A5F1E0 bcc */
    bit16_i(0x0008);  /* A5F1E2 bit */
    if (IF_Z) goto L_A5F240;  /* A5F1E5 beq */
L_A5F1E7:
    lda16_v(0xE6A0);  /* A5F1E7 lda */
    sta16(ea_dp(0x03));  /* A5F1EA sta */
    pla16();  /* A5F1EC pla */
    lda16(ea_dp(0x0C));  /* A5F1ED lda */
    cmp16_v(rd16(ea_abs(0x08E7)));  /* A5F1EF cmp */
    op_sep(0x20);  /* A5F1F2 sep */
    lda8_v(0x00);  /* A5F1F4 lda */
    ACC8(rol);  /* A5F1F6 rol */
    sta8(ea_dp(0x14));  /* A5F1F7 sta */
    sta8(ea_dp(0x15));  /* A5F1F9 sta */
    ldx8(ea_abs(0x002C));  /* A5F1FB ldx */
    ldy8(ea_absx(0xD054));  /* A5F1FE ldy */
    abi_ActorMotionPreset();  /* A5F201 jsl */
    lda8(ea_abs(V_0012));  /* A5F205 lda */
    and8_v(0x08);  /* A5F208 and */
    if (IF_Z) goto L_A5F218;  /* A5F20A beq */
    op_rep(0x20);  /* A5F20C rep */
    clc();  /* A5F20E clc */
    lda16(ea_dp(0x19));  /* A5F20F lda */
    adc16_v(0x0080);  /* A5F211 adc */
    sta16(ea_dp(0x19));  /* A5F214 sta */
    op_sep(0x20);  /* A5F216 sep */
L_A5F218:
    return;  /* A5F218 rts */
L_A5F219:
    sec();  /* A5F219 sec */
    return;  /* A5F21A rts */
L_A5F21B:
    ldx8_v(0x4C);  /* A5F21B ldx */
    abi_RandomFromList();  /* A5F21D jsl */
    op_rep(0x20);  /* A5F221 rep */
    if (IF_Z) goto L_A5F240;  /* A5F223 beq */
L_A5F225:
    lda16_v(0x0060);  /* A5F225 lda */
    ldx8_v(0x05);  /* A5F228 ldx */
    ldy8_v(0x20);  /* A5F22A ldy */
    call_routine(0xA5F284u, 0);  /* A5F22C jsr */
    if (IF_C) goto L_A5F240;  /* A5F22F bcs */
    lda16_v(0xE6A0);  /* A5F231 lda */
    sta16(ea_dp(0x03));  /* A5F234 sta */
    pla16();  /* A5F236 pla */
    op_sep(0x20);  /* A5F237 sep */
    ldy8_v(0x83);  /* A5F239 ldy */
    abi_ActorMotionPreset();  /* A5F23B jsl */
    return;  /* A5F23F rts */
L_A5F240:
    sec();  /* A5F240 sec */
    return;  /* A5F241 rts */
}

/*
 * @routine A5:F0F4 GuardChase
 * WHAT IT DOES: The guard's chase step: GuardLedgeJump, then its speed
 * and facing set by how far the player is ahead of it.
 * HOW: GuardLedgeJump; then the player's x less the guard's (dx). Within
 * 48 either way ($146E cleared if set, then as within 96): a moving guard
 * slows by $30 (to 0); a stopped one gets code word $E5B6 and $146B = 1
 * (once) and nothing more. Within 96: above a player speed of $C0 it
 * closes on his speed by 8 a frame (0 if that goes negative), else takes
 * $C0. Within 368: it speeds up by 8, to at most $300. Farther, with $49
 * clear: code word $E794, $146B = $40, and back. Each new speed: code
 * word $E5E8 and $146B = 2 (once), the speed's high byte cleared, the
 * facing (and flip) toward the player.
 * Entry: M 16-bit, X 8-bit; D = the guard; DB = $81; called by JSR. Exit:
 * as GuardLedgeJump when it jumps; else M 8-bit when the facing was set,
 * M 16-bit otherwise.
 */
void abi_GuardChase(void)
{
    call_routine(0xA5F19Du, 0);  /* A5F0F4 jsr */
    sec();  /* A5F0F7 sec */
    lda16(ea_abs(0x08E7));  /* A5F0F8 lda */
    sbc16_v(rd16(ea_dp(0x0C)));  /* A5F0FB sbc */
    clc();  /* A5F0FD clc */
    adc16_v(0x0030);  /* A5F0FE adc */
    cmp16_v(0x0060);  /* A5F101 cmp */
    if (!IF_C) goto L_A5F172;  /* A5F104 bcc */
    clc();  /* A5F106 clc */
    adc16_v(0x0030);  /* A5F107 adc */
    cmp16_v(0x00C0);  /* A5F10A cmp */
    if (!IF_C) goto L_A5F158;  /* A5F10D bcc */
    clc();  /* A5F10F clc */
    adc16_v(0x0110);  /* A5F110 adc */
    cmp16_v(0x02E0);  /* A5F113 cmp */
    if (!IF_C) goto L_A5F127;  /* A5F116 bcc */
    ldx8(ea_dp(0x49));  /* A5F118 ldx */
    if (!IF_Z) goto L_A5F127;  /* A5F11A bne */
    ldx8_v(0x40);  /* A5F11C ldx */
    stx8(ea_abs(0x146B));  /* A5F11E stx */
    lda16_v(0xE794);  /* A5F121 lda */
    sta16(ea_dp(0x03));  /* A5F124 sta */
    return;  /* A5F126 rts */
L_A5F127:
    lda16_v(0x0300);  /* A5F127 lda */
    cmp16_v(rd16(ea_dp(0x19)));  /* A5F12A cmp */
    if (!IF_C) goto L_A5F134;  /* A5F12C bcc */
L_A5F12E:
    clc();  /* A5F12E clc */
    lda16_v(0x0008);  /* A5F12F lda */
    adc16_v(rd16(ea_dp(0x19)));  /* A5F132 adc */
L_A5F134:
    sta16(ea_dp(0x19));  /* A5F134 sta */
    ldx8_v(0x02);  /* A5F136 ldx */
    cpx8_v(rd8(ea_abs(0x146B)));  /* A5F138 cpx */
    if (IF_Z) goto L_A5F145;  /* A5F13B beq */
    stx8(ea_abs(0x146B));  /* A5F13D stx */
    lda16_v(0xE5E8);  /* A5F140 lda */
    sta16(ea_dp(0x03));  /* A5F143 sta */
L_A5F145:
    ldx8_v(0x00);  /* A5F145 ldx */
    stx8(ea_dp(0x1B));  /* A5F147 stx */
    lda16(ea_dp(0x0C));  /* A5F149 lda */
    cmp16_v(rd16(ea_abs(0x08E7)));  /* A5F14B cmp */
    op_sep(0x20);  /* A5F14E sep */
    lda8_v(0x00);  /* A5F150 lda */
    ACC8(rol);  /* A5F152 rol */
    sta8(ea_dp(0x14));  /* A5F153 sta */
    sta8(ea_dp(0x15));  /* A5F155 sta */
    return;  /* A5F157 rts */
L_A5F158:
    lda16_v(0x00C0);  /* A5F158 lda */
    cmp16_v(rd16(ea_abs(0x08F4)));  /* A5F15B cmp */
    if (IF_C) goto L_A5F134;  /* A5F15E bcs */
    lda16(ea_dp(0x19));  /* A5F160 lda */
    cmp16_v(rd16(ea_abs(0x08F4)));  /* A5F162 cmp */
    if (!IF_C) goto L_A5F12E;  /* A5F165 bcc */
    sec();  /* A5F167 sec */
    sbc16_v(0x0008);  /* A5F168 sbc */
    if (!IF_N) goto L_A5F134;  /* A5F16B bpl */
    lda16_v(0x0000);  /* A5F16D lda */
    goto L_A5F134;  /* A5F170 bra */
L_A5F172:
    ldx8(ea_abs(0x146E));  /* A5F172 ldx */
    if (IF_Z) goto L_A5F17E;  /* A5F175 beq */
    ldx8_v(0x00);  /* A5F177 ldx */
    stx8(ea_abs(0x146E));  /* A5F179 stx */
    goto L_A5F158;  /* A5F17C bra */
L_A5F17E:
    lda16(ea_dp(0x19));  /* A5F17E lda */
    if (IF_Z) goto L_A5F18D;  /* A5F180 beq */
    sec();  /* A5F182 sec */
    sbc16_v(0x0030);  /* A5F183 sbc */
    if (!IF_N) goto L_A5F134;  /* A5F186 bpl */
    lda16_v(0x0000);  /* A5F188 lda */
    goto L_A5F134;  /* A5F18B bra */
L_A5F18D:
    ldx8_v(0x01);  /* A5F18D ldx */
    cpx8_v(rd8(ea_abs(0x146B)));  /* A5F18F cpx */
    if (IF_Z) goto L_A5F19C;  /* A5F192 beq */
    stx8(ea_abs(0x146B));  /* A5F194 stx */
    lda16_v(0xE5B6);  /* A5F197 lda */
    sta16(ea_dp(0x03));  /* A5F19A sta */
L_A5F19C:
    return;  /* A5F19C rts */
}
