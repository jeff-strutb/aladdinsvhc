/*
 * palette_steps.c: one palette set at a time: from the ROM's table of
 * palettes, to white, or back to its kept colours.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorPaletteFromRom(void);
void abi_PaletteFromRom(void);
void abi_PaletteWhite(void);
void abi_PaletteRestoreReversed(void);
void abi_PaletteOffset(void);
#include "wram.h"
#include "actor.h"

/* Palettes in bank $84 for actors: 16 colours (32 bytes) each. */
#define ACTOR_PALETTES 0xB000u

/*
 * @routine 81:9903 ActorPaletteFromRom
 * WHAT IT DOES: Sets the actor's sprite palette to palette Y of the bank
 * $84 table, and has the NMI upload the palettes.
 * HOW: The palette is bits 1-3 of the actor's sprite attributes' high
 * byte; with DB = $84, 16 words from $84:B000 + Y * 32 go to the palette
 * buffer at sprite palette (8 +) that one; then palette_upload steps up.
 * Entry: M 8-bit, X 8-bit; D = the actor; Y low byte = the ROM palette;
 * called by JSL. Exit: M and X 8-bit; DB as at entry, N and Z from it
 * (PLB); $0C = 0; X, Y past the copy (low bytes).
 */
void abi_ActorPaletteFromRom(void)
{
    lda8(ea_dp(ACTOR_LOOK + 1));  /* 819903 lda */
    ACC8(lsr);  /* 819905 lsr */
    and8_v(0x07);  /* 819906 and */
    tax8();  /* 819908 tax */
    phb();  /* 819909 phb */
    lda8_v(0x84);  /* 81990A lda */
    pha8();  /* 81990C pha */
    plb();  /* 81990D plb */
    op_rep(0x30);  /* 81990E rep */
    tya16();  /* 819910 tya */
    and16_v(0x00FF);  /* 819911 and */
    ACC16(asl);  /* 819914 asl */
    ACC16(asl);  /* 819915 asl */
    ACC16(asl);  /* 819916 asl */
    ACC16(asl);  /* 819917 asl */
    ACC16(asl);  /* 819918 asl */
    tay16();  /* 819919 tay */
    txa16();  /* 81991A txa */
    and16_v(0x00FF);  /* 81991B and */
    ACC16(asl);  /* 81991E asl */
    ACC16(asl);  /* 81991F asl */
    ACC16(asl);  /* 819920 asl */
    ACC16(asl);  /* 819921 asl */
    ACC16(asl);  /* 819922 asl */
    tax16();  /* 819923 tax */
    lda16_v(0x0010);  /* 819924 lda */
    sta16(ea_abs(SCRATCH));  /* 819927 sta */
L_81992A:
    lda16(ea_absy(ACTOR_PALETTES));  /* 81992A lda */
    sta16(ea_longx(PALETTE_BUFFER + 0x100));  /* 81992D sta */
    inx16();  /* 819931 inx */
    inx16();  /* 819932 inx */
    iny16();  /* 819933 iny */
    iny16();  /* 819934 iny */
    MEM16(dec, ea_abs(SCRATCH));  /* 819935 dec */
    if (!IF_Z) goto L_81992A;  /* 819938 bne */
    op_sep(0x30);  /* 81993A sep */
    MEM8(inc, ea_abs(PALETTE_UPLOAD));  /* 81993C inc */
    plb();  /* 81993F plb */
    return;  /* 819940 rtl */
}

/*
 * @routine 81:9941 PaletteFromRom
 * WHAT IT DOES: Sets palette X (counted from palette 4) to palette A of
 * the ROM's table, and has the NMI upload the palettes.
 * HOW: With DB = $87: 16 words from $87:FF40 + A * 32 to the palette
 * buffer at palette 4 + X; then palette_upload steps up.
 * Entry: A low byte = the ROM palette; X low byte = the palette; called by
 * JSL. Exit: M 8-bit, X 8-bit; DB as at entry; $0C = 0; X, Y past the
 * copy.
 */
void abi_PaletteFromRom(void)
{
    phb();  /* 819941 phb */
    ldy8_v(0x87);  /* 819942 ldy */
    phy8();  /* 819944 phy */
    plb();  /* 819945 plb */
    op_rep(0x30);  /* 819946 rep */
    and16_v(0x00FF);  /* 819948 and */
    ACC16(asl);  /* 81994B asl */
    ACC16(asl);  /* 81994C asl */
    ACC16(asl);  /* 81994D asl */
    ACC16(asl);  /* 81994E asl */
    ACC16(asl);  /* 81994F asl */
    tay16();  /* 819950 tay */
    txa16();  /* 819951 txa */
    and16_v(0x00FF);  /* 819952 and */
    ACC16(asl);  /* 819955 asl */
    ACC16(asl);  /* 819956 asl */
    ACC16(asl);  /* 819957 asl */
    ACC16(asl);  /* 819958 asl */
    ACC16(asl);  /* 819959 asl */
    tax16();  /* 81995A tax */
    lda16_v(0x0010);  /* 81995B lda */
    sta16(ea_abs(SCRATCH));  /* 81995E sta */
L_819961:
    lda16(ea_absy(0xFF40));  /* 819961 lda */
    sta16(ea_longx(0x7EFD80u));  /* 819964 sta */
    inx16();  /* 819968 inx */
    inx16();  /* 819969 inx */
    iny16();  /* 81996A iny */
    iny16();  /* 81996B iny */
    MEM16(dec, ea_abs(SCRATCH));  /* 81996C dec */
    if (!IF_Z) goto L_819961;  /* 81996F bne */
    op_sep(0x30);  /* 819971 sep */
    MEM8(inc, ea_abs(PALETTE_UPLOAD));  /* 819973 inc */
    plb();  /* 819976 plb */
    return;  /* 819977 rtl */
}

/*
 * @routine 81:9CD7 PaletteWhite
 * WHAT IT DOES: Sets palette A to white and has the NMI upload the
 * palettes.
 * HOW: PaletteOffset, then 16 words of $FFFF into the palette buffer there;
 * palette_upload steps up.
 * Entry: A = the palette (low 4 bits); DB = $00-$3F or $80-$BF; called by
 * JSL. Exit: M 8-bit, X 8-bit; A low byte = $FF; Y = $FF.
 */
void abi_PaletteWhite(void)
{
    op_rep(0x30);  /* 819CD7 rep */
    abi_PaletteOffset();  /* 819CD9 jsr */
    ldy16_v(0x000F);  /* 819CDC ldy */
    lda16_v(0xFFFF);  /* 819CDF lda */
L_819CE2:
    sta16(ea_longx(PALETTE_BUFFER));  /* 819CE2 sta */
    inx16();  /* 819CE6 inx */
    inx16();  /* 819CE7 inx */
    dey16();  /* 819CE8 dey */
    if (!IF_N) goto L_819CE2;  /* 819CE9 bpl */
    op_sep(0x30);  /* 819CEB sep */
    MEM8(inc, ea_abs(PALETTE_UPLOAD));  /* 819CED inc */
    return;  /* 819CF0 rtl */
}

/*
 * @routine 81:9CF1 PaletteRestoreReversed
 * WHAT IT DOES: Sets palette A back to the colours PaletteSaveReversed
 * kept, and has the NMI upload the palettes.
 * HOW: PaletteOffset; the kept colours (the first 16 words of
 * palette_saved, last first) go into the palette buffer there;
 * palette_upload steps up.
 * Entry: A = the palette (low 4 bits); DB = $00-$3F or $80-$BF; called by
 * JSL. Exit: M 8-bit, X 8-bit; Y = $FE.
 */
void abi_PaletteRestoreReversed(void)
{
    op_rep(0x30);  /* 819CF1 rep */
    abi_PaletteOffset();  /* 819CF3 jsr */
    ldy16_v(0x001E);  /* 819CF6 ldy */
L_819CF9:
    lda16(ea_absy(PALETTE_SAVED));  /* 819CF9 lda */
    sta16(ea_longx(PALETTE_BUFFER));  /* 819CFC sta */
    inx16();  /* 819D00 inx */
    inx16();  /* 819D01 inx */
    dey16();  /* 819D02 dey */
    dey16();  /* 819D03 dey */
    if (!IF_N) goto L_819CF9;  /* 819D04 bpl */
    op_sep(0x30);  /* 819D06 sep */
    MEM8(inc, ea_abs(PALETTE_UPLOAD));  /* 819D08 inc */
    return;  /* 819D0B rtl */
}
