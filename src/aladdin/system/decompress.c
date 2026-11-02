/*
 * decompress.c: the graphics and map assets packed in ROM, unpacked into
 * the second bank of work RAM.
 *
 * A packed asset is groups of a flag byte and a fill byte, then a byte
 * for each set flag bit: each group unpacks to 8 bytes, bit 7 first, a set
 * bit taking the next literal byte, a clear one the fill byte. The source
 * runs on across banks: past $FFFF it goes on at $8000 of the next.
 */
#include "machine.h"
#include "ops65.h"

/* Direct page of the unpacking (D = 0). */
#define DECOMP_FLAGS 0x0C          /* the group's flag byte, shifted out bit by bit */
#define DECOMP_FILL 0x0D           /* the group's fill byte */
#define DECOMP_SRC 0x65            /* the source's long pointer (low word 0: Y is the offset) */
#define DECOMP_GROUPS 0x69         /* groups left (word) */

/*
 * @routine 81:BB49 DecompressGroup
 * WHAT IT DOES: Unpacks one 8-byte group of a packed asset to $7F:X and
 * counts it off.
 * HOW: Takes the flag and fill bytes from the source, then for each of the
 * 8 bytes shifts the flags: a set bit stores the next source byte, a clear
 * one the fill byte. Each source step past $FFFF goes on at $8000 of the
 * next bank. X moves on 8; the groups left go down by one.
 * Entry: M 8-bit, X 16-bit; D = 0; DB = $7F; X = the destination; Y = the
 * source offset in the bank at DECOMP_SRC + 2; called by JSR
 * (DecompressAsset). Exit: X = the next destination, A = X (16 bits, from
 * TXA/ADC), Y the next source offset; N and Z from the groups-left
 * decrement (Z set: none left); C from the ADC; M 8-bit, X 16-bit.
 */
void abi_DecompressGroup(void)
{
    lda8(ea_dpily(DECOMP_SRC));  /* 81BB49 lda */
    sta8(ea_dp(DECOMP_FLAGS));  /* 81BB4B sta */
    iny16();  /* 81BB4D iny */
    if (!IF_Z) goto L_81BB55;  /* 81BB4E bne */
    ldy16_v(0x8000);  /* 81BB50 ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BB53 inc */
L_81BB55:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BB55 lda */
    sta8(ea_dp(DECOMP_FILL));  /* 81BB57 sta */
    iny16();  /* 81BB59 iny */
    if (!IF_Z) goto L_81BB61;  /* 81BB5A bne */
    ldy16_v(0x8000);  /* 81BB5C ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BB5F inc */
L_81BB61:
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BB61 asl */
    if (IF_C) goto L_81BB69;  /* 81BB63 bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BB65 lda */
    goto L_81BB73;  /* 81BB67 bra */
L_81BB69:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BB69 lda */
    iny16();  /* 81BB6B iny */
    if (!IF_Z) goto L_81BB73;  /* 81BB6C bne */
    ldy16_v(0x8000);  /* 81BB6E ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BB71 inc */
L_81BB73:
    sta8(ea_absx(0x0000));  /* 81BB73 sta */
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BB76 asl */
    if (IF_C) goto L_81BB7E;  /* 81BB78 bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BB7A lda */
    goto L_81BB88;  /* 81BB7C bra */
L_81BB7E:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BB7E lda */
    iny16();  /* 81BB80 iny */
    if (!IF_Z) goto L_81BB88;  /* 81BB81 bne */
    ldy16_v(0x8000);  /* 81BB83 ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BB86 inc */
L_81BB88:
    sta8(ea_absx(0x0001));  /* 81BB88 sta */
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BB8B asl */
    if (IF_C) goto L_81BB93;  /* 81BB8D bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BB8F lda */
    goto L_81BB9D;  /* 81BB91 bra */
L_81BB93:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BB93 lda */
    iny16();  /* 81BB95 iny */
    if (!IF_Z) goto L_81BB9D;  /* 81BB96 bne */
    ldy16_v(0x8000);  /* 81BB98 ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BB9B inc */
L_81BB9D:
    sta8(ea_absx(0x0002));  /* 81BB9D sta */
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BBA0 asl */
    if (IF_C) goto L_81BBA8;  /* 81BBA2 bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BBA4 lda */
    goto L_81BBB2;  /* 81BBA6 bra */
L_81BBA8:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BBA8 lda */
    iny16();  /* 81BBAA iny */
    if (!IF_Z) goto L_81BBB2;  /* 81BBAB bne */
    ldy16_v(0x8000);  /* 81BBAD ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BBB0 inc */
L_81BBB2:
    sta8(ea_absx(0x0003));  /* 81BBB2 sta */
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BBB5 asl */
    if (IF_C) goto L_81BBBD;  /* 81BBB7 bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BBB9 lda */
    goto L_81BBC7;  /* 81BBBB bra */
L_81BBBD:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BBBD lda */
    iny16();  /* 81BBBF iny */
    if (!IF_Z) goto L_81BBC7;  /* 81BBC0 bne */
    ldy16_v(0x8000);  /* 81BBC2 ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BBC5 inc */
L_81BBC7:
    sta8(ea_absx(0x0004));  /* 81BBC7 sta */
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BBCA asl */
    if (IF_C) goto L_81BBD2;  /* 81BBCC bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BBCE lda */
    goto L_81BBDC;  /* 81BBD0 bra */
L_81BBD2:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BBD2 lda */
    iny16();  /* 81BBD4 iny */
    if (!IF_Z) goto L_81BBDC;  /* 81BBD5 bne */
    ldy16_v(0x8000);  /* 81BBD7 ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BBDA inc */
L_81BBDC:
    sta8(ea_absx(0x0005));  /* 81BBDC sta */
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BBDF asl */
    if (IF_C) goto L_81BBE7;  /* 81BBE1 bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BBE3 lda */
    goto L_81BBF1;  /* 81BBE5 bra */
L_81BBE7:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BBE7 lda */
    iny16();  /* 81BBE9 iny */
    if (!IF_Z) goto L_81BBF1;  /* 81BBEA bne */
    ldy16_v(0x8000);  /* 81BBEC ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BBEF inc */
L_81BBF1:
    sta8(ea_absx(0x0006));  /* 81BBF1 sta */
    MEM8(asl, ea_dp(DECOMP_FLAGS));  /* 81BBF4 asl */
    if (IF_C) goto L_81BBFC;  /* 81BBF6 bcs */
    lda8(ea_dp(DECOMP_FILL));  /* 81BBF8 lda */
    goto L_81BC06;  /* 81BBFA bra */
L_81BBFC:
    lda8(ea_dpily(DECOMP_SRC));  /* 81BBFC lda */
    iny16();  /* 81BBFE iny */
    if (!IF_Z) goto L_81BC06;  /* 81BBFF bne */
    ldy16_v(0x8000);  /* 81BC01 ldy */
    MEM8(inc, ea_dp(DECOMP_SRC + 2));  /* 81BC04 inc */
L_81BC06:
    sta8(ea_absx(0x0007));  /* 81BC06 sta */
    op_rep(0x20);  /* 81BC09 rep */
    clc();  /* 81BC0B clc */
    txa16();  /* 81BC0C txa */
    adc16_v(0x0008);  /* 81BC0D adc */
    tax16();  /* 81BC10 tax */
    MEM16(dec, ea_dp(DECOMP_GROUPS));  /* 81BC11 dec */
    op_sep(0x20);  /* 81BC13 sep */
    return;  /* 81BC15 rts */
}

