/*
 * level_actor_graphics.c: the graphics of the actor kinds a level places:
 * marked from its placement list, then unpacked into bank $7F. Drafted by
 * cdraft.py; named and described by hand.
 */
#include "machine.h"
#include "ops65.h"
#include "wram.h"
#include "actor_anim.h"
#include "wram_video.h"

void abi_AnimSetGraphicsUnpack(void);
void abi_AnimSetsFill(void);

#define PLACEMENTS 0x8A7Au  /* bank $82: by level, its placement list's offset (word); there a count, then 6-byte entries (+1 the kind, +2 bits 5-6 its variant) */
#define GFX_ENTRIES 0xAC18u  /* bank $82: per kind, its graphics entry's offset from GFX_PARTS (word) */
#define GFX_PARTS 0xAC58u  /* bank $82: per entry: length (low 14 bits; bits 14-15 the extra tile slots), ..., +5 by variant a byte */
#define KIND_GFX 0x7FE500u  /* per kind: where its graphics went (word), then three more tables to $7F:EAFF */
#define KIND_VARIANT_BYTE 0x7FEA00u  /* per kind: its variant's byte from its graphics entry, 0 until set */
#define VARIANT 0x000Eu  /* the placement's variant (word) */
#define UNPACK_NEXT 0x006Bu  /* where the next unpack goes in bank $7F (word) */

/*
 * @routine 81:B9F4 LevelActorGraphicsLoad
 * WHAT IT DOES: Loads the graphics of every actor kind the level places:
 * the kinds are marked from the level's placement list, with each kind's
 * extra tile slots and its variant's byte, then each marked kind's
 * graphics are unpacked into bank $7F from $7000 on.
 * HOW: With DB = $82: AnimSetsFill 0; $7F:E500-$7F:EAFF cleared. The
 * level's list (PLACEMENTS by LEVEL): for each of its count's entries, the
 * kind's flag in ANIM_SET_OF_TYPE = 1, its CELL_EXTRA_SLOTS = bits 6-7 of
 * byte 1 of its GFX_PARTS entry, and its KIND_VARIANT_BYTE, while 0, = the
 * entry's byte 5 + the variant (bits 5-6 of the placement's byte 2). Then
 * UNPACK_NEXT = $7000 and AnimSetGraphicsUnpack for each kind 0-255
 * marked.
 * Entry: M 8-bit, X 8-bit; DB any; called by JSL (81:86BA). Exit: M and X
 * 8-bit; X = 0; DB as at entry, N and Z from it (PLB); A, Y as the last
 * unpack left them.
 */
void abi_LevelActorGraphicsLoad(void)
{
    phb();  /* 81B9F4 phb */
    lda8_v(0x82);  /* 81B9F5 lda */
    pha8();  /* 81B9F7 pha */
    plb();  /* 81B9F8 plb */
    lda8_v(0x00);  /* 81B9F9 lda */
    abi_AnimSetsFill();  /* 81B9FB jsl */
    op_rep(0x10);  /* 81B9FF rep */
    ldx16_v(0x05FF);  /* 81BA01 ldx */
    lda8_v(0x00);  /* 81BA04 lda */
L_81BA06:
    sta8(ea_longx(KIND_GFX));  /* 81BA06 sta */
    dex16();  /* 81BA0A dex */
    if (!IF_N) goto L_81BA06;  /* 81BA0B bpl */
    lda8_v(0x00);  /* 81BA0D lda */
    xba();  /* 81BA0F xba */
    lda8(ea_abs(LEVEL));  /* 81BA10 lda */
    ACC8(asl);  /* 81BA13 asl */
    tax16();  /* 81BA14 tax */
    ldy16(ea_absx(PLACEMENTS));  /* 81BA15 ldy */
    lda8(ea_absy(PLACEMENTS));  /* 81BA18 lda */
    if (IF_Z) goto L_81BA85;  /* 81BA1B beq */
    sta8(ea_abs(SCRATCH));  /* 81BA1D sta */
L_81BA20:
    lda8_v(0x00);  /* 81BA20 lda */
    xba();  /* 81BA22 xba */
    lda8(ea_absy(PLACEMENTS + 1));  /* 81BA23 lda */
    tax16();  /* 81BA26 tax */
    lda8_v(0x01);  /* 81BA27 lda */
    sta8(ea_longx(ANIM_SET_OF_TYPE));  /* 81BA29 sta */
    lda8(ea_absy(PLACEMENTS + 2));  /* 81BA2D lda */
    ACC8(lsr);  /* 81BA30 lsr */
    ACC8(lsr);  /* 81BA31 lsr */
    ACC8(lsr);  /* 81BA32 lsr */
    ACC8(lsr);  /* 81BA33 lsr */
    ACC8(lsr);  /* 81BA34 lsr */
    and8_v(0x03);  /* 81BA35 and */
    sta8(ea_abs(VARIANT));  /* 81BA37 sta */
    stz8(ea_abs(VARIANT + 1));  /* 81BA3A stz */
    phy16();  /* 81BA3D phy */
    phb();  /* 81BA3E phb */
    lda8_v(0x82);  /* 81BA3F lda */
    pha8();  /* 81BA41 pha */
    plb();  /* 81BA42 plb */
    op_rep(0x20);  /* 81BA43 rep */
    txa16();  /* 81BA45 txa */
    and16_v(0x00FF);  /* 81BA46 and */
    ACC16(asl);  /* 81BA49 asl */
    tay16();  /* 81BA4A tay */
    lda16(ea_absy(GFX_ENTRIES));  /* 81BA4B lda */
    tay16();  /* 81BA4E tay */
    op_sep(0x20);  /* 81BA4F sep */
    lda8(ea_absy(GFX_PARTS + 1));  /* 81BA51 lda */
    ACC8(lsr);  /* 81BA54 lsr */
    ACC8(lsr);  /* 81BA55 lsr */
    ACC8(lsr);  /* 81BA56 lsr */
    ACC8(lsr);  /* 81BA57 lsr */
    ACC8(lsr);  /* 81BA58 lsr */
    ACC8(lsr);  /* 81BA59 lsr */
    sta8(ea_longx(CELL_EXTRA_SLOTS));  /* 81BA5A sta */
    lda8(ea_longx(KIND_VARIANT_BYTE));  /* 81BA5E lda */
    if (!IF_Z) goto L_81BA75;  /* 81BA62 bne */
    op_rep(0x20);  /* 81BA64 rep */
    clc();  /* 81BA66 clc */
    tya16();  /* 81BA67 tya */
    adc16_v(rd16(ea_abs(VARIANT)));  /* 81BA68 adc */
    tay16();  /* 81BA6B tay */
    op_sep(0x20);  /* 81BA6C sep */
    lda8(ea_absy(GFX_PARTS + 5));  /* 81BA6E lda */
    sta8(ea_longx(KIND_VARIANT_BYTE));  /* 81BA71 sta */
L_81BA75:
    op_rep(0x20);  /* 81BA75 rep */
    plb();  /* 81BA77 plb */
    pla16();  /* 81BA78 pla */
    clc();  /* 81BA79 clc */
    adc16_v(0x0006);  /* 81BA7A adc */
    tay16();  /* 81BA7D tay */
    op_sep(0x20);  /* 81BA7E sep */
    MEM8(dec, ea_abs(SCRATCH));  /* 81BA80 dec */
    if (!IF_Z) goto L_81BA20;  /* 81BA83 bne */
L_81BA85:
    ldx16_v(0x7000);  /* 81BA85 ldx */
    stx16(ea_abs(UNPACK_NEXT));  /* 81BA88 stx */
    op_sep(0x30);  /* 81BA8B sep */
    ldx8_v(0x00);  /* 81BA8D ldx */
L_81BA8F:
    lda8(ea_longx(ANIM_SET_OF_TYPE));  /* 81BA8F lda */
    if (IF_Z) goto L_81BA99;  /* 81BA93 beq */
    txa8();  /* 81BA95 txa */
    abi_AnimSetGraphicsUnpack();  /* 81BA96 jsr */
L_81BA99:
    inx8();  /* 81BA99 inx */
    if (!IF_Z) goto L_81BA8F;  /* 81BA9A bne */
    plb();  /* 81BA9C plb */
    return;  /* 81BA9D rtl */
}
