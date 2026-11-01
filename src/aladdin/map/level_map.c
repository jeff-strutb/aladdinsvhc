/*
 * level_map.c: building a level's map in work RAM.
 *
 * A level's map is made of screens of 32x16 tiles (LEVEL_TILES, $200
 * bytes a screen), built from 8x8 blocks of 2x2 tiles, and a screen map
 * that places them (SCREEN_MAP, 32 screens across). The level's map
 * header, at $80:E3DC (a word per level, pointing to it in that table),
 * gives the screens to build, the banks of their block and tile data, and
 * the layout.
 */
#include "machine.h"
#include "ops65.h"
void abi_LevelMapBuildLong(void);
void abi_LevelMapBuild(void);
void abi_ScreenMapLayout(void);
void abi_BlockSourceAdd(void);
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:BEEB LevelMapBuildLong
 * WHAT IT DOES: LevelMapBuild for callers in other banks.
 * HOW: LevelMapBuild.
 * Entry: as LevelMapBuild; called by JSL. Exit: as LevelMapBuild.
 */
void abi_LevelMapBuildLong(void)
{
    abi_LevelMapBuild();  /* 81BEEB jsr */
    return;  /* 81BEEE rtl */
}

/*
 * @routine 81:BEEF LevelMapBuild
 * WHAT IT DOES: Builds the level's map: its screens' tiles and the screen
 * map that places them.
 * HOW: Clears the screen map; from the level's map header takes the
 * screen count, the banks of the block lists, block tiles and metatiles,
 * and the main and sub screen layers; for each screen adds its block
 * source (BlockSourceAdd), copies its 64-block list and writes each
 * block's 2x2 tiles into the screen's 32x16 tiles; then lays out the
 * screen map from the level's layouts (ScreenMapLayout), a second layout
 * too when the level has one.
 * Entry: M 8-bit; D = 0; DB any; called by JSR. Exit: P and DB as at
 * entry (pulled); X, Y and A as the layouts left them (16-bit X, Y).
 */
void abi_LevelMapBuild(void)
{
    php();  /* 81BEEF php */
    phb();  /* 81BEF0 phb */
    op_rep(0x30);  /* 81BEF1 rep */
    ldx16_v(0x07FE);  /* 81BEF3 ldx */
    lda16_v(0x0000);  /* 81BEF6 lda */
L_81BEF9:
    sta16(ea_longx(ANIM_SET_OF_TYPE));  /* 81BEF9 sta */
    dex16();  /* 81BEFD dex */
    dex16();  /* 81BEFE dex */
    if (!IF_N) goto L_81BEF9;  /* 81BEFF bpl */
    stz16(ea_dp(0x0C));  /* 81BF01 stz */
    stz16(ea_dp(0x16));  /* 81BF03 stz */
    lda16(ea_abs(LEVEL));  /* 81BF05 lda */
    ACC16(asl);  /* 81BF08 asl */
    tax16();  /* 81BF09 tax */
    phx16();  /* 81BF0A phx */
    lda16(ea_longx(0x80E3DCu));  /* 81BF0B lda */
    tax16();  /* 81BF0F tax */
    op_sep(0x20);  /* 81BF10 sep */
    lda8(ea_longx(0x80E3DCu));  /* 81BF12 lda */
    sta8(ea_dp(0x0E));  /* 81BF16 sta */
    lda8(ea_longx(0x80E3DDu));  /* 81BF18 lda */
    sta8(ea_abs(0x0056));  /* 81BF1C sta */
    lda8_v(0x80);  /* 81BF1F lda */
    sta8(ea_abs(0x0055));  /* 81BF21 sta */
    lda8(ea_longx(0x80E3DEu));  /* 81BF24 lda */
    sta8(ea_abs(0x0059));  /* 81BF28 sta */
    lda8_v(0x80);  /* 81BF2B lda */
    sta8(ea_abs(0x0058));  /* 81BF2D sta */
    lda8(ea_longx(0x80E3DFu));  /* 81BF30 lda */
    sta8(ea_abs(LEVEL_METATILE_BANK));  /* 81BF34 sta */
    lda8_v(0x80);  /* 81BF37 lda */
    sta8(ea_abs(0x005B));  /* 81BF39 sta */
    lda8(ea_longx(0x80E3E0u));  /* 81BF3C lda */
    sta8(ea_abs(0x0399));  /* 81BF40 sta */
    sta8(ea_abs(0x039A));  /* 81BF43 sta */
L_81BF46:
    lda8(ea_longx(0x80E3E1u));  /* 81BF46 lda */
    inx16();  /* 81BF4A inx */
    phx16();  /* 81BF4B phx */
    xba();  /* 81BF4C xba */
    lda8_v(0x00);  /* 81BF4D lda */
    op_rep(0x20);  /* 81BF4F rep */
    ACC16(lsr);  /* 81BF51 lsr */
    tay16();  /* 81BF52 tay */
    abi_BlockSourceAdd();  /* 81BF53 jsr */
    op_sep(0x20);  /* 81BF56 sep */
    lda8(ea_abs(0x0056));  /* 81BF58 lda */
    pha8();  /* 81BF5B pha */
    plb();  /* 81BF5C plb */
    ldx16_v(0x0000);  /* 81BF5D ldx */
    lda8_v(0x84);  /* 81BF60 lda */
    sta8(ea_dp(0x10));  /* 81BF62 sta */
L_81BF64:
    lda8(ea_absy(0x8000));  /* 81BF64 lda */
    sta8(ea_absx(0x1A94));  /* 81BF67 sta */
    iny16();  /* 81BF6A iny */
    inx16();  /* 81BF6B inx */
    MEM8(dec, ea_dp(0x10));  /* 81BF6C dec */
    if (!IF_Z) goto L_81BF64;  /* 81BF6E bne */
    lda8(ea_abs(0x0059));  /* 81BF70 lda */
    pha8();  /* 81BF73 pha */
    plb();  /* 81BF74 plb */
    op_rep(0x20);  /* 81BF75 rep */
    ldx16(ea_dp(0x0C));  /* 81BF77 ldx */
    ldy16_v(0x0000);  /* 81BF79 ldy */
    lda16_v(0x0008);  /* 81BF7C lda */
    sta16(ea_dp(0x12));  /* 81BF7F sta */
L_81BF81:
    lda16_v(0x0008);  /* 81BF81 lda */
    sta16(ea_dp(0x14));  /* 81BF84 sta */
L_81BF86:
    lda16(ea_absy(0x1A94));  /* 81BF86 lda */
    iny16();  /* 81BF89 iny */
    iny16();  /* 81BF8A iny */
    phy16();  /* 81BF8B phy */
    tay16();  /* 81BF8C tay */
    lda16(ea_absy(0x8000));  /* 81BF8D lda */
    sta16(ea_longx(0x7F0000u));  /* 81BF90 sta */
    lda16(ea_absy(0x8002));  /* 81BF94 lda */
    sta16(ea_longx(0x7F0002u));  /* 81BF97 sta */
    lda16(ea_absy(0x8004));  /* 81BF9B lda */
    sta16(ea_longx(0x7F0020u));  /* 81BF9E sta */
    lda16(ea_absy(0x8006));  /* 81BFA2 lda */
    sta16(ea_longx(0x7F0022u));  /* 81BFA5 sta */
    inx16();  /* 81BFA9 inx */
    inx16();  /* 81BFAA inx */
    inx16();  /* 81BFAB inx */
    inx16();  /* 81BFAC inx */
    ply16();  /* 81BFAD ply */
    MEM16(dec, ea_dp(0x14));  /* 81BFAE dec */
    if (!IF_Z) goto L_81BF86;  /* 81BFB0 bne */
    clc();  /* 81BFB2 clc */
    txa16();  /* 81BFB3 txa */
    adc16_v(0x0020);  /* 81BFB4 adc */
    tax16();  /* 81BFB7 tax */
    MEM16(dec, ea_dp(0x12));  /* 81BFB8 dec */
    if (!IF_Z) goto L_81BF81;  /* 81BFBA bne */
    stx16(ea_dp(0x0C));  /* 81BFBC stx */
    plx16();  /* 81BFBE plx */
    op_sep(0x20);  /* 81BFBF sep */
    MEM8(dec, ea_dp(0x0E));  /* 81BFC1 dec */
    if (!IF_Z) goto L_81BF46;  /* 81BFC3 bne */
    lda8_v(0x7F);  /* 81BFC5 lda */
    pha8();  /* 81BFC7 pha */
    plb();  /* 81BFC8 plb */
    op_rep(0x20);  /* 81BFC9 rep */
    plx16();  /* 81BFCB plx */
    lda16(ea_longx(0x80E460u));  /* 81BFCC lda */
    tax16();  /* 81BFD0 tax */
    phx16();  /* 81BFD1 phx */
    lda16(ea_longx(0x80E460u));  /* 81BFD2 lda */
    tax16();  /* 81BFD6 tax */
    ldy16_v(0x0042);  /* 81BFD7 ldy */
    abi_ScreenMapLayout();  /* 81BFDA jsr */
    plx16();  /* 81BFDD plx */
    lda16(ea_longx(0x80E462u));  /* 81BFDE lda */
    if (IF_Z) goto L_81BFEB;  /* 81BFE2 beq */
    tax16();  /* 81BFE4 tax */
    ldy16_v(0x0442);  /* 81BFE5 ldy */
    abi_ScreenMapLayout();  /* 81BFE8 jsr */
L_81BFEB:
    plb();  /* 81BFEB plb */
    plp();  /* 81BFEC plp */
    return;  /* 81BFED rts */
}

/*
 * @routine 81:BFEE ScreenMapLayout
 * WHAT IT DOES: Writes a layout into the screen map: which screen goes in
 * each place.
 * HOW: The layout's width and height (each less one), then its screen
 * numbers row by row; each number * 2 goes to the high byte of the place's
 * word (Y on, a row 64 bytes on).
 * Entry: M 16-bit, X 16-bit; X = the layout's offset in the layouts; Y =
 * the first place's byte offset; DB = $7F; D = 0; called by JSR. Exit: P
 * as at entry (pulled); X past the layout; Y = the last row's start + $40.
 */
void abi_ScreenMapLayout(void)
{
    php();  /* 81BFEE php */
    op_sep(0x20);  /* 81BFEF sep */
    sty16(ea_dp(0x10));  /* 81BFF1 sty */
    lda8(ea_longx(0x80E460u));  /* 81BFF3 lda */
    sta8(ea_dp(0x0C));  /* 81BFF7 sta */
    lda8(ea_longx(0x80E461u));  /* 81BFF9 lda */
    sta8(ea_dp(0x0D));  /* 81BFFD sta */
L_81BFFF:
    lda8(ea_dp(0x0C));  /* 81BFFF lda */
    sta8(ea_dp(0x0E));  /* 81C001 sta */
L_81C003:
    lda8(ea_longx(0x80E462u));  /* 81C003 lda */
    inx16();  /* 81C007 inx */
    ACC8(asl);  /* 81C008 asl */
    sta8(ea_absy(0xF001));  /* 81C009 sta */
    iny16();  /* 81C00C iny */
    iny16();  /* 81C00D iny */
    MEM8(dec, ea_dp(0x0E));  /* 81C00E dec */
    if (!IF_N) goto L_81C003;  /* 81C010 bpl */
    op_rep(0x20);  /* 81C012 rep */
    clc();  /* 81C014 clc */
    lda16(ea_dp(0x10));  /* 81C015 lda */
    adc16_v(0x0040);  /* 81C017 adc */
    sta16(ea_dp(0x10));  /* 81C01A sta */
    tay16();  /* 81C01C tay */
    op_sep(0x20);  /* 81C01D sep */
    MEM8(dec, ea_dp(0x0D));  /* 81C01F dec */
    if (!IF_N) goto L_81BFFF;  /* 81C021 bpl */
    plp();  /* 81C023 plp */
    return;  /* 81C024 rts */
}
