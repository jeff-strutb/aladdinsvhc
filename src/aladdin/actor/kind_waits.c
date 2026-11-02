/*
 * kind_waits.c: the routines some actor kinds run each frame they wait (by the kind
 * table at $83:8F4F, or a kind's stub): animation, shots, touches and
 * their range. Drafted by cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram_state.h"
#include "wram_video.h"
#include "wram.h"
#include "actor.h"

void abi_ActorEndOutOfRange(void);
void abi_ActorFreePlaced(void);
void abi_ActorHitsTestBoxLong(void);
void abi_AnimStep(void);
void abi_SoundQueueAdd(void);
void abi_ActorDownedByShot(void);
void abi_ActorStompOrHurt(void);
void abi_ActorTakesShot(void);
void abi_ActorStopsShot(void);
void abi_ActorZoneHasTestBox(void);
void abi_PlayerEnter8CFacing(void);
void abi_PlayerEnter8CFacingLong(void);
void abi_ActorFreeOutOfRangeLong(void);
void abi_ActorHurtsPlayerOnTouchLong(void);
void abi_ActorStopsShotLong(void);

void abi_ActorCreateChild8(void);
void abi_ActorHitsTestBoxIfPlayerFree(void);
void abi_ActorYByFrame(void);
void abi_BackdropActorShow(void);
void abi_BackdropPictureStep(void);
void abi_PlayerOnFirmGround(void);
void abi_PlayerOnFirmGroundProbe4(void);
void abi_PlayerOnFirmGroundProbe8(void);

/*
 * @routine 83:E645 ActorCount42Down
 * WHAT IT DOES: Counts the actor's word at $42 down by 1, not below 0.
 * HOW: A 16-bit subtraction; negative becomes 0.
 * Entry: M 16-bit; D = the actor's record; called by JSR (83:E60C,
 * 83:E633). Exit: M 16-bit; A = the new count; C from the subtraction.
 */
void abi_ActorCount42Down(void)
{
    sec();  /* 83E645 sec */
    lda16(ea_dp(0x42));  /* 83E646 lda */
    sbc16_v(0x0001);  /* 83E648 sbc */
    if (!IF_N) goto L_83E650;  /* 83E64B bpl */
    lda16_v(0x0000);  /* 83E64D lda */
L_83E650:
    sta16(ea_dp(0x42));  /* 83E650 sta */
    return;  /* 83E652 rts */
}


/*
 * @routine 83:A012 Kind27Wait
 * WHAT IT DOES: Kind $27's wait: animates; once its $42 is set, touching
 * the player it plays sound $4F, counts $036C up and ends; it ends at once
 * when $0367 is 0, or when out of range.
 * HOW: AnimStep; $42 clear returns. $0367 zero: ActorEndOutOfRange with
 * box $58. Else ActorHitsTestBoxLong with box $24: touched, SoundQueueAdd
 * $4F, $036C up, and into ActorEndOutOfRange past its range test (its
 * return dropped, freed); not touched, ActorEndOutOfRange with box $58.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Its $42 is set only by its untraced start for $07 bit 3 (83:9E58). Exit:
 * $42 clear: as AnimStep. Else in range: as ActorEndOutOfRange; ended:
 * does not return.
 */
void abi_Kind27Wait(void)
{
    goto L_83A012;
L_8396C6:
    op_sep(0x30);  /* 8396C6 sep */
    pla8();  /* 8396C8 pla */
    pla8();  /* 8396C9 pla */
    abi_ActorFreePlaced();  /* 8396CA jsr */
    actor_leave(0x8394BFu); return;  /* 8396CD jmp */
L_83A012:
    abi_AnimStep();  /* 83A012 jsl */
    lda8(ea_dp(0x42));  /* 83A016 lda */
    if (!IF_Z) goto L_83A01B;  /* 83A018 bne */
    return;  /* 83A01A rts */
L_83A01B:
    lda8(ea_abs(HEALTH));  /* 83A01B lda */
    if (IF_Z) goto L_83A028;  /* 83A01E beq */
    ldy8_v(0x24);  /* 83A020 ldy */
    abi_ActorHitsTestBoxLong();  /* 83A022 jsl */
    if (!IF_C) goto L_83A02D;  /* 83A026 bcc */
L_83A028:
    ldx8_v(0x58);  /* 83A028 ldx */
    abi_ActorEndOutOfRange(); return;  /* 83A02A jmp */
L_83A02D:
    lda8_v(0x4F);  /* 83A02D lda */
    abi_SoundQueueAdd();  /* 83A02F jsl */
    MEM8(inc, ea_abs(0x036C));  /* 83A033 inc */
    goto L_8396C6;  /* 83A036 jml */
}


/*
 * @routine 83:AEEC Kind31Wait
 * WHAT IT DOES: Kind $31's wait: with its $42 set it turns round at x
 * $0F00 and beyond in levels below 7, and may be downed by a shot; it
 * animates and is ended when out of range.
 * HOW: $42 set: level below 7 and x pixels $0F00 or more, facing and flip
 * toggled; ActorDownedByShot, downed: ActorStompOrHurt. Then AnimStep and
 * ActorEndOutOfRange with box $38 (by JML).
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Called by JSR: it ends by JML to ActorEndOutOfRange, which returns to
 * the loop. Exit: as ActorEndOutOfRange.
 */
void abi_Kind31Wait(void)
{
    lda8(ea_dp(0x42));  /* 83AEEC lda */
    if (IF_Z) goto L_83AF12;  /* 83AEEE beq */
    lda8(ea_abs(LEVEL));  /* 83AEF0 lda */
    cmp8_v(0x07);  /* 83AEF3 cmp */
    if (IF_C) goto L_83AF0A;  /* 83AEF5 bcs */
    op_rep(0x20);  /* 83AEF7 rep */
    lda16(ea_dp(0x0C));  /* 83AEF9 lda */
    cmp16_v(0x0F00);  /* 83AEFB cmp */
    op_sep(0x20);  /* 83AEFE sep */
    if (!IF_C) goto L_83AF0A;  /* 83AF00 bcc */
    lda8(ea_dp(0x14));  /* 83AF02 lda */
    eor8_v(0x01);  /* 83AF04 eor */
    sta8(ea_dp(0x14));  /* 83AF06 sta */
    sta8(ea_dp(0x15));  /* 83AF08 sta */
L_83AF0A:
    abi_ActorDownedByShot();  /* 83AF0A jsr */
    if (!IF_C) goto L_83AF12;  /* 83AF0D bcc */
    abi_ActorStompOrHurt();  /* 83AF0F jsr */
L_83AF12:
    abi_AnimStep();  /* 83AF12 jsl */
    ldx8_v(0x38);  /* 83AF16 ldx */
    abi_ActorEndOutOfRange(); return;  /* 83AF18 jml */
}


/*
 * @routine 83:B94A Kind30Wait
 * WHAT IT DOES: Kind $30's wait: animates; variant 7 (low three bits of
 * $07) notes the first shot that reaches it ($43); ended when out of
 * range.
 * HOW: AnimStep; variant 7 with $43 clear: ActorTakesShot, hit: $43 up.
 * Then ActorEndOutOfRange with box $18.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as ActorEndOutOfRange.
 */
void abi_Kind30Wait(void)
{
    abi_AnimStep();  /* 83B94A jsl */
    lda8(ea_dp(0x07));  /* 83B94E lda */
    and8_v(0x07);  /* 83B950 and */
    cmp8_v(0x07);  /* 83B952 cmp */
    if (!IF_Z) goto L_83B961;  /* 83B954 bne */
    lda8(ea_dp(0x43));  /* 83B956 lda */
    if (!IF_Z) goto L_83B961;  /* 83B958 bne */
    abi_ActorTakesShot();  /* 83B95A jsr */
    if (IF_C) goto L_83B961;  /* 83B95D bcs */
    MEM8(inc, ea_dp(0x43));  /* 83B95F inc */
L_83B961:
    ldx8_v(0x18);  /* 83B961 ldx */
    abi_ActorEndOutOfRange(); return;  /* 83B963 jmp */
}


/*
 * @routine 83:C94A Kind46Wait
 * WHAT IT DOES: Kind $46's wait: a shot it stops sets its $43; with $07
 * bit 1 it is ended when out of range.
 * HOW: ActorStopsShot: stopped (C clear), $43 = 1. $07 bit 1:
 * ActorEndOutOfRange with box 0; else returns.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: M 8-bit; A low = $07 AND 2; or as ActorEndOutOfRange.
 */
void abi_Kind46Wait(void)
{
    abi_ActorStopsShot();  /* 83C94A jsr */
    if (IF_C) goto L_83C953;  /* 83C94D bcs */
    lda8_v(0x01);  /* 83C94F lda */
    sta8(ea_dp(0x43));  /* 83C951 sta */
L_83C953:
    lda8(ea_dp(0x07));  /* 83C953 lda */
    and8_v(0x02);  /* 83C955 and */
    if (!IF_Z) goto L_83C95A;  /* 83C957 bne */
    return;  /* 83C959 rts */
L_83C95A:
    ldx8_v(0x00);  /* 83C95A ldx */
    abi_ActorEndOutOfRange(); return;  /* 83C95C jmp */
}




/*
 * @routine 83:D64A Kind53Wait
 * WHAT IT DOES: Kind $53's wait: stomped or hurting as touched; sets its
 * $45 once the record its $4E names has $46 set; animates.
 * HOW: ActorStompOrHurt; $45 clear: the byte at $46 of the record at $4E
 * (16-bit X) nonzero, $45 = 1. AnimStep.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: as AnimStep.
 */
void abi_Kind53Wait(void)
{
    abi_ActorStompOrHurt();  /* 83D64A jsr */
    lda8(ea_dp(0x45));  /* 83D64D lda */
    if (!IF_Z) goto L_83D660;  /* 83D64F bne */
    op_rep(0x10);  /* 83D651 rep */
    ldx16(ea_dp(0x4E));  /* 83D653 ldx */
    lda8(ea_absx(0x0046));  /* 83D655 lda */
    op_sep(0x10);  /* 83D658 sep */
    if (IF_Z) goto L_83D660;  /* 83D65A beq */
    lda8_v(0x01);  /* 83D65C lda */
    sta8(ea_dp(0x45));  /* 83D65E sta */
L_83D660:
    abi_AnimStep();  /* 83D660 jsl */
    return;  /* 83D664 rts */
}


/*
 * @routine 83:E679 Kind67Wait
 * WHAT IT DOES: Kind $67's wait: puts the player into state $8C while the
 * player is across x $04F8-$0E47 and the test box is within its zone (from
 * $1A2C) up and down.
 * HOW: The player's x pixels less $09A0 plus $04A8 below $0950; then the
 * test box's y against $C0 less $1A2C, by the test box's half-height plus
 * $4C (in $000C, doubled in $000E): inside, PlayerEnter8CFacingLong.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Exit: M 8-bit, X 8-bit; as PlayerEnter8CFacingLong, or flags from the
 * comparison.
 */
void abi_Kind67Wait(void)
{
    op_rep(0x20);  /* 83E679 rep */
    sec();  /* 83E67B sec */
    lda16(ea_abs(0x08E7));  /* 83E67C lda */
    sbc16_v(0x09A0);  /* 83E67F sbc */
    clc();  /* 83E682 clc */
    adc16_v(0x04A8);  /* 83E683 adc */
    cmp16_v(0x0950);  /* 83E686 cmp */
    if (IF_C) goto L_83E6B8;  /* 83E689 bcs */
    clc();  /* 83E68B clc */
    lda16(ea_abs(0x148A));  /* 83E68C lda */
    adc16_v(0x004C);  /* 83E68F adc */
    sta16(ea_abs(SCRATCH));  /* 83E692 sta */
    ACC16(asl);  /* 83E695 asl */
    sta16(ea_abs(0x000E));  /* 83E696 sta */
    lda16(ea_abs(0x1A2C));  /* 83E699 lda */
    eor16_v(0xFFFF);  /* 83E69C eor */
    ACC16(inc);  /* 83E69F inc */
    clc();  /* 83E6A0 clc */
    adc16_v(0x00C0);  /* 83E6A1 adc */
    sec();  /* 83E6A4 sec */
    sbc16_v(rd16(ea_abs(0x1486)));  /* 83E6A5 sbc */
    clc();  /* 83E6A8 clc */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 83E6A9 adc */
    cmp16_v(rd16(ea_abs(0x000E)));  /* 83E6AC cmp */
    if (IF_C) goto L_83E6B8;  /* 83E6AF bcs */
    op_sep(0x30);  /* 83E6B1 sep */
    abi_PlayerEnter8CFacingLong();  /* 83E6B3 jsl */
    return;  /* 83E6B7 rts */
L_83E6B8:
    op_sep(0x30);  /* 83E6B8 sep */
    return;  /* 83E6BA rts */
}





/*
 * @routine 83:EF92 Kind6DWait
 * WHAT IT DOES: Kind $6D's wait: by variant (bits 2-4 of $07) stomped or
 * hurting and downed by shots, its height set by frame (variant 4), and
 * ended when out of range; variant $1C also enters state $8C while $1B14
 * is set and ends itself 144 pixels or more from the range point.
 * HOW: Variant $14: ActorEndOutOfRange only. Others: ActorStompOrHurt,
 * ActorDownedByShot (downed: $48 up), AnimStep, variant 4 ActorYByFrame,
 * ActorEndOutOfRange with box 0. Variant $1C: $1B14 set, state bits $8C
 * set and $09 bit 7 cleared; ActorStompOrHurt, ActorDownedByShot ($48 up),
 * AnimStep; distance from RANGE_X 144 or more: into ActorEndOutOfRange
 * past its range test.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Variant $1C is reached only by cov_kind6d_variant.box. Exit: as
 * ActorEndOutOfRange; variant $1C near: M 8-bit, C clear; far: does not
 * return.
 */
void abi_Kind6DWait(void)
{
    goto L_83EF92;
L_8396C6:
    op_sep(0x30);  /* 8396C6 sep */
    pla8();  /* 8396C8 pla */
    pla8();  /* 8396C9 pla */
    abi_ActorFreePlaced();  /* 8396CA jsr */
    actor_leave(0x8394BFu); return;  /* 8396CD jmp */
L_83EF92:
    lda8(ea_dp(0x07));  /* 83EF92 lda */
    and8_v(0x1C);  /* 83EF94 and */
    cmp8_v(0x1C);  /* 83EF96 cmp */
    if (IF_Z) goto L_83EFBC;  /* 83EF98 beq */
    cmp8_v(0x14);  /* 83EF9A cmp */
    if (IF_Z) goto L_83EFB7;  /* 83EF9C beq */
    abi_ActorStompOrHurt();  /* 83EF9E jsr */
    abi_ActorDownedByShot();  /* 83EFA1 jsr */
    if (IF_C) goto L_83EFA8;  /* 83EFA4 bcs */
    MEM8(inc, ea_dp(0x48));  /* 83EFA6 inc */
L_83EFA8:
    abi_AnimStep();  /* 83EFA8 jsl */
    lda8(ea_dp(0x07));  /* 83EFAC lda */
    and8_v(0x1C);  /* 83EFAE and */
    cmp8_v(0x04);  /* 83EFB0 cmp */
    if (!IF_Z) goto L_83EFB7;  /* 83EFB2 bne */
    abi_ActorYByFrame();  /* 83EFB4 jsr */
L_83EFB7:
    ldx8_v(0x00);  /* 83EFB7 ldx */
    abi_ActorEndOutOfRange(); return;  /* 83EFB9 jmp */
L_83EFBC:
    lda8(ea_abs(0x1B14));  /* 83EFBC lda */
    if (IF_Z) goto L_83EFC9;  /* 83EFBF beq */
    lda8_v(0x8C);  /* 83EFC1 lda */
    tsb8(ea_dp(0x00));  /* 83EFC3 tsb */
    lda8_v(0x80);  /* 83EFC5 lda */
    trb8(ea_dp(0x09));  /* 83EFC7 trb */
L_83EFC9:
    abi_ActorStompOrHurt();  /* 83EFC9 jsr */
    abi_ActorDownedByShot();  /* 83EFCC jsr */
    if (IF_C) goto L_83EFD3;  /* 83EFCF bcs */
    MEM8(inc, ea_dp(0x48));  /* 83EFD1 inc */
L_83EFD3:
    abi_AnimStep();  /* 83EFD3 jsl */
    op_rep(0x20);  /* 83EFD7 rep */
    sec();  /* 83EFD9 sec */
    lda16(ea_abs(0x1A44));  /* 83EFDA lda */
    sbc16_v(rd16(ea_dp(0x0C)));  /* 83EFDD sbc */
    if (!IF_N) goto L_83EFE5;  /* 83EFDF bpl */
    eor16_v(0xFFFF);  /* 83EFE1 eor */
    ACC16(inc);  /* 83EFE4 inc */
L_83EFE5:
    cmp16_v(0x0090);  /* 83EFE5 cmp */
    op_sep(0x20);  /* 83EFE8 sep */
    if (!IF_C) goto L_83EFEF;  /* 83EFEA bcc */
    goto L_8396C6;  /* 83EFEC jmp */
L_83EFEF:
    return;  /* 83EFEF rts */
}


/*
 * @routine 83:F1D5 Kind6EWait
 * WHAT IT DOES: Kind $6E's wait: a sound ($33) every 63 frames while its
 * $49 is clear; its backdrop picture stepped and shown; touching the test
 * box (moved by its variant's offset for the test) while the player can be
 * hit, the player enters state $8C.
 * HOW: $49 clear: $48 down; at 0, back to $3F and SoundQueueAdd $33.
 * BackdropPictureStep, BackdropActorShow. Its x pixels kept at $4C and the
 * variant's offset (word at $D6AB) added; ActorHitsTestBoxIfPlayerFree;
 * met: PlayerEnter8CFacingLong. Its x put back.
 * Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
 * from the actor loop (83:94D0) each frame the actor waits, by its kind.
 * Its $09 has bit 6 clear in play, so the touch test never meets: the hurt
 * is reached only by unit_kind6e_touch.box. Exit: M 8-bit; A = its x
 * pixels.
 */
void abi_Kind6EWait(void)
{
    lda8(ea_dp(0x49));  /* 83F1D5 lda */
    if (!IF_Z) goto L_83F1E7;  /* 83F1D7 bne */
    MEM8(dec, ea_dp(0x48));  /* 83F1D9 dec */
    if (!IF_Z) goto L_83F1E7;  /* 83F1DB bne */
    lda8_v(0x3F);  /* 83F1DD lda */
    sta8(ea_dp(0x48));  /* 83F1DF sta */
    lda8_v(0x33);  /* 83F1E1 lda */
    abi_SoundQueueAdd();  /* 83F1E3 jsl */
L_83F1E7:
    abi_BackdropPictureStep();  /* 83F1E7 jsr */
    abi_BackdropActorShow();  /* 83F1EA jsr */
    lda8(ea_dp(0x07));  /* 83F1ED lda */
    ACC8(asl);  /* 83F1EF asl */
    tax8();  /* 83F1F0 tax */
    op_rep(0x20);  /* 83F1F1 rep */
    lda16(ea_dp(0x0C));  /* 83F1F3 lda */
    sta16(ea_dp(0x4C));  /* 83F1F5 sta */
    clc();  /* 83F1F7 clc */
    adc16_v(rd16(ea_absx(0xD6AB)));  /* 83F1F8 adc */
    sta16(ea_dp(0x0C));  /* 83F1FB sta */
    op_sep(0x20);  /* 83F1FD sep */
    abi_ActorHitsTestBoxIfPlayerFree();  /* 83F1FF jsr */
    if (IF_C) goto L_83F208;  /* 83F202 bcs */
    abi_PlayerEnter8CFacingLong();  /* 83F204 jsl */
L_83F208:
    op_rep(0x20);  /* 83F208 rep */
    lda16(ea_dp(0x4C));  /* 83F20A lda */
    sta16(ea_dp(0x0C));  /* 83F20C sta */
    op_sep(0x20);  /* 83F20E sep */
    return;  /* 83F210 rts */
}

