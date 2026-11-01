/*
 * mode7_screen.c: putting up a Mode 7 screen.
 *
 * The screen's graphics are kept in VRAM as 4-bit planar tiles; for Mode 7
 * they are read back into the staging buffer at $7F:5000, converted to
 * Mode 7's one byte a pixel (each tile's colours taking a palette row from
 * a table), and written to VRAM's high bytes; the map is read back,
 * cleared and written again (video/mode7.c).
 */
#include "machine.h"
#include "ops65.h"
void abi_Mode7ScreenSetup(void);
void abi_Mode7GraphicsConvert(void);
void abi_Mode7TilesConvert(void);
void abi_Mode7TileRowsConvert(void);
void abi_TileBlockQueue(void);
void abi_VramWordsToBuffer(void);
void abi_SineTableBuild(void);
void abi_Mode7MapToBuffer(void);
void abi_Mode7MapFromBuffer(void);
void abi_Mode7MapClear(void);
#include "hw.h"
#include "wram.h"
#include "wram_video.h"

/*
 * @routine 81:AF3B Mode7ScreenSetup
 * WHAT IT DOES: Puts up a Mode 7 screen from the graphics and map in VRAM,
 * with the interrupts off while VRAM is rewritten.
 * HOW: Builds the sine tables; NMI off and forced blank; four 2 KB parts of
 * the graphics read back and converted (Mode7GraphicsConvert); the map read
 * back, cleared and written again; then Mode 7 (BGMODE 7), its centre and
 * parameters, the screen as its shadow says, M7SEL $80, the NMI on.
 * Entry: M 8-bit, X 8-bit; D = 0; A low byte = the row flag for the
 * palette lookup; DB a bank whose $2100-$4200 are the PPU's and CPU's;
 * called by JSL. Exit: M and X 8-bit; interrupts on; A low byte = NMITIMEN
 * written; N set, Z clear.
 */
void abi_Mode7ScreenSetup(void)
{
    sta8(ea_dp(0x30));  /* 81AF3B sta */
    abi_SineTableBuild();  /* 81AF3D jsl */
    sei();  /* 81AF41 sei */
    lda8(ea_abs(NMITIMEN_SHADOW));  /* 81AF42 lda */
    and8_v(0x7F);  /* 81AF45 and */
    sta8(ea_abs(NMITIMEN));  /* 81AF47 sta */
    lda8(ea_abs(INIDISP_SHADOW));  /* 81AF4A lda */
    ora8_v(0x80);  /* 81AF4D ora */
    sta8(ea_abs(INIDISP));  /* 81AF4F sta */
    lda8_v(0x0C);  /* 81AF52 lda */
    sta8(ea_dp(0x0F));  /* 81AF54 sta */
    stz8(ea_dp(0x0E));  /* 81AF56 stz */
    lda8_v(0x30);  /* 81AF58 lda */
    sta8(ea_dp(0x0D));  /* 81AF5A sta */
    stz8(ea_dp(0x0C));  /* 81AF5C stz */
L_81AF5E:
    abi_VramWordsToBuffer();  /* 81AF5E jsr */
    abi_Mode7GraphicsConvert();  /* 81AF61 jsr */
    sec();  /* 81AF64 sec */
    lda8(ea_dp(0x0D));  /* 81AF65 lda */
    sbc8_v(0x10);  /* 81AF67 sbc */
    sta8(ea_dp(0x0D));  /* 81AF69 sta */
    sec();  /* 81AF6B sec */
    lda8(ea_dp(0x0F));  /* 81AF6C lda */
    sbc8_v(0x04);  /* 81AF6E sbc */
    sta8(ea_dp(0x0F));  /* 81AF70 sta */
    if (IF_C) goto L_81AF5E;  /* 81AF72 bcs */
    lda8_v(0x40);  /* 81AF74 lda */
    sta8(ea_dp(0x0F));  /* 81AF76 sta */
    stz8(ea_dp(0x0E));  /* 81AF78 stz */
    abi_Mode7MapToBuffer();  /* 81AF7A jsr */
    abi_Mode7MapClear();  /* 81AF7D jsr */
    stz8(ea_dp(0x0D));  /* 81AF80 stz */
    stz8(ea_dp(0x0C));  /* 81AF82 stz */
    abi_Mode7MapFromBuffer();  /* 81AF84 jsr */
    lda8_v(0x02);  /* 81AF87 lda */
    sta8(ea_abs(0x0829));  /* 81AF89 sta */
    lda8_v(0x07);  /* 81AF8C lda */
    sta8(ea_abs(0x039D));  /* 81AF8E sta */
    op_rep(0x20);  /* 81AF91 rep */
    lda16_v(0x0080);  /* 81AF93 lda */
    sta16(ea_abs(0x0395));  /* 81AF96 sta */
    lda16_v(0x0040);  /* 81AF99 lda */
    sta16(ea_abs(0x0397));  /* 81AF9C sta */
    stz16(ea_abs(0x038D));  /* 81AF9F stz */
    stz16(ea_abs(0x038F));  /* 81AFA2 stz */
    stz16(ea_abs(0x0391));  /* 81AFA5 stz */
    stz16(ea_abs(0x0393));  /* 81AFA8 stz */
    op_sep(0x30);  /* 81AFAB sep */
    lda8(ea_abs(INIDISP_SHADOW));  /* 81AFAD lda */
    sta8(ea_abs(INIDISP));  /* 81AFB0 sta */
    lda8_v(0x80);  /* 81AFB3 lda */
    sta8(ea_abs(M7SEL));  /* 81AFB5 sta */
    ora8_v(rd8(ea_abs(NMITIMEN_SHADOW)));  /* 81AFB8 ora */
    sta8(ea_abs(NMITIMEN));  /* 81AFBB sta */
    cli();  /* 81AFBE cli */
    return;  /* 81AFBF rtl */
}

/*
 * @routine 81:B059 Mode7GraphicsConvert
 * WHAT IT DOES: Converts the staged graphics to Mode 7's pixels and writes
 * them to VRAM's high bytes.
 * HOW: Mode7TilesConvert, then 4,096 bytes to VMDATAH from the VRAM word
 * address at $0C, stepping after the high byte.
 * Entry: D = 0; DB a bank whose $2115-$2119 are the PPU's; called by JSR.
 * Exit: M 8-bit, X 8-bit; X = Y = 0; Z set; A low byte = the last pixel.
 */
void abi_Mode7GraphicsConvert(void)
{
    op_rep(0x30);  /* 81B059 rep */
    abi_Mode7TilesConvert();  /* 81B05B jsr */
    op_sep(0x20);  /* 81B05E sep */
    lda8_v(0x80);  /* 81B060 lda */
    sta8(ea_abs(VMAIN));  /* 81B062 sta */
    ldx16(ea_dp(0x0C));  /* 81B065 ldx */
    stx16(ea_abs(VMADDL));  /* 81B067 stx */
    ldy16_v(0x1000);  /* 81B06A ldy */
    ldx16_v(0x0000);  /* 81B06D ldx */
L_81B070:
    lda8(ea_longx(0x7F5800u));  /* 81B070 lda */
    sta8(ea_abs(VMDATAH));  /* 81B074 sta */
    inx16();  /* 81B077 inx */
    dey16();  /* 81B078 dey */
    if (!IF_Z) goto L_81B070;  /* 81B079 bne */
    op_sep(0x10);  /* 81B07B sep */
    return;  /* 81B07D rts */
}

/*
 * @routine 81:B07E Mode7TilesConvert
 * WHAT IT DOES: Converts the 64 staged planar tiles to a byte a pixel.
 * HOW: With the data bank at $7F, Mode7TileRowsConvert over 64 tiles from
 * the staging buffer's start.
 * Entry: D = 0; called by JSR. Exit: M 8-bit, X and Y 16-bit; DB as at
 * entry, N and Z from it (PLB).
 */
void abi_Mode7TilesConvert(void)
{
    op_rep(0x30);  /* 81B07E rep */
    lda16_v(0x0040);  /* 81B080 lda */
    sta16(ea_dp(0x2C));  /* 81B083 sta */
    op_sep(0x20);  /* 81B085 sep */
    phb();  /* 81B087 phb */
    lda8_v(0x7F);  /* 81B088 lda */
    pha8();  /* 81B08A pha */
    plb();  /* 81B08B plb */
    ldx16_v(0x0000);  /* 81B08C ldx */
    ldy16_v(0x0000);  /* 81B08F ldy */
    abi_Mode7TileRowsConvert();  /* 81B092 jsr */
    plb();  /* 81B095 plb */
    return;  /* 81B096 rts */
}

/*
 * @routine 81:B097 Mode7TileRowsConvert
 * WHAT IT DOES: Converts staged 4-bit planar tiles into a byte a pixel,
 * each tile's colours in its own palette row.
 * HOW: Per tile: its palette row from the table at $B407 (indexed by its
 * place and the row flag); per row of 8 pixels, the four bit-planes
 * shifted out a bit at a time into a 4-bit colour, nonzero colours ORed
 * with the palette row, each pixel to $5800 + X.
 * Entry: M 16-bit, X 16-bit; D = 0; DB = $7F; X = the first pixel's
 * offset; Y = the first tile's offset in the staging buffer; $2C = the
 * tiles; called by JSR. Exit: M 16-bit; X, Y past the last tile; $2C =
 * 0; Z set.
 */
void abi_Mode7TileRowsConvert(void)
{
L_81B097:
    op_sep(0x20);  /* 81B097 sep */
    phx16();  /* 81B099 phx */
    stx16(ea_dp(0x32));  /* 81B09A stx */
    lda8_v(0x00);  /* 81B09C lda */
    xba();  /* 81B09E xba */
    lda8(ea_dp(0x33));  /* 81B09F lda */
    ACC8(lsr);  /* 81B0A1 lsr */
    ACC8(lsr);  /* 81B0A2 lsr */
    clc();  /* 81B0A3 clc */
    adc8_v(rd8(ea_dp(0x0F)));  /* 81B0A4 adc */
    ora8_v(rd8(ea_dp(0x30)));  /* 81B0A6 ora */
    tax16();  /* 81B0A8 tax */
    lda8(ea_longx(0x81B407u));  /* 81B0A9 lda */
    sta8(ea_dp(0x32));  /* 81B0AD sta */
    plx16();  /* 81B0AF plx */
    lda8_v(0x08);  /* 81B0B0 lda */
    sta8(ea_dp(0x2F));  /* 81B0B2 sta */
L_81B0B4:
    lda8_v(0x08);  /* 81B0B4 lda */
    sta8(ea_dp(0x2E));  /* 81B0B6 sta */
    lda8(ea_absy(0x5000));  /* 81B0B8 lda */
    sta8(ea_dp(0x10));  /* 81B0BB sta */
    lda8(ea_absy(0x5001));  /* 81B0BD lda */
    sta8(ea_dp(0x11));  /* 81B0C0 sta */
    lda8(ea_absy(0x5010));  /* 81B0C2 lda */
    sta8(ea_dp(0x12));  /* 81B0C5 sta */
    lda8(ea_absy(0x5011));  /* 81B0C7 lda */
    sta8(ea_dp(0x13));  /* 81B0CA sta */
L_81B0CC:
    lda8_v(0x00);  /* 81B0CC lda */
    MEM8(asl, ea_dp(0x13));  /* 81B0CE asl */
    ACC8(rol);  /* 81B0D0 rol */
    MEM8(asl, ea_dp(0x12));  /* 81B0D1 asl */
    ACC8(rol);  /* 81B0D3 rol */
    MEM8(asl, ea_dp(0x11));  /* 81B0D4 asl */
    ACC8(rol);  /* 81B0D6 rol */
    MEM8(asl, ea_dp(0x10));  /* 81B0D7 asl */
    ACC8(rol);  /* 81B0D9 rol */
    and8_v(0x0F);  /* 81B0DA and */
    if (IF_Z) goto L_81B0E0;  /* 81B0DC beq */
    ora8_v(rd8(ea_dp(0x32)));  /* 81B0DE ora */
L_81B0E0:
    sta8(ea_absx(0x5800));  /* 81B0E0 sta */
    inx16();  /* 81B0E3 inx */
    MEM8(dec, ea_dp(0x2E));  /* 81B0E4 dec */
    if (!IF_Z) goto L_81B0CC;  /* 81B0E6 bne */
    iny16();  /* 81B0E8 iny */
    iny16();  /* 81B0E9 iny */
    MEM8(dec, ea_dp(0x2F));  /* 81B0EA dec */
    if (!IF_Z) goto L_81B0B4;  /* 81B0EC bne */
    op_rep(0x20);  /* 81B0EE rep */
    tya16();  /* 81B0F0 tya */
    clc();  /* 81B0F1 clc */
    adc16_v(0x0010);  /* 81B0F2 adc */
    tay16();  /* 81B0F5 tay */
    MEM16(dec, ea_dp(0x2C));  /* 81B0F6 dec */
    if (!IF_Z) goto L_81B097;  /* 81B0F8 bne */
    return;  /* 81B0FA rts */
}

/*
 * @routine 81:A287 TileBlockQueue
 * WHAT IT DOES: Queues a block of tiles for the NMI to write to VRAM: four
 * tilemap rows of four tiles at VRAM address A, the tiles numbered from Y.
 * HOW: Four queue records of 10 bytes (a VRAM address, then four tilemap
 * words: tile, tile + 1, tile + 16, tile + 17), the address stepping a
 * row ($20 words) at a time; the queue's length steps by 40 and the queue
 * is marked ready.
 * Entry: M 16-bit, X 16-bit; A = the VRAM address; Y = the first tile's
 * tilemap word; DB a bank whose $0000-$1FFF is work RAM; called by JSL.
 * Exit: M and X 8-bit; X = the new length's low byte; scratch $0C = 0; N,
 * Z from the ready flag's increment.
 */
void abi_TileBlockQueue(void)
{
    ldx16(ea_abs(TILE_BLOCKS_LENGTH));  /* 81A287 ldx */
    sta16(ea_absx(TILE_BLOCKS));  /* 81A28A sta */
    ACC16(inc);  /* 81A28D inc */
    ACC16(inc);  /* 81A28E inc */
    sta16(ea_absx(0x1B4E));  /* 81A28F sta */
    clc();  /* 81A292 clc */
    adc16_v(0x003E);  /* 81A293 adc */
    sta16(ea_absx(0x1B58));  /* 81A296 sta */
    ACC16(inc);  /* 81A299 inc */
    ACC16(inc);  /* 81A29A inc */
    sta16(ea_absx(0x1B62));  /* 81A29B sta */
    tya16();  /* 81A29E tya */
    ldy16_v(0x0004);  /* 81A29F ldy */
    sty16(ea_abs(SCRATCH));  /* 81A2A2 sty */
L_81A2A5:
    sta16(ea_absx(0x1B46));  /* 81A2A5 sta */
    ACC16(inc);  /* 81A2A8 inc */
    sta16(ea_absx(0x1B48));  /* 81A2A9 sta */
    clc();  /* 81A2AC clc */
    adc16_v(0x000F);  /* 81A2AD adc */
    sta16(ea_absx(0x1B4A));  /* 81A2B0 sta */
    ACC16(inc);  /* 81A2B3 inc */
    sta16(ea_absx(0x1B4C));  /* 81A2B4 sta */
    sec();  /* 81A2B7 sec */
    sbc16_v(0x000F);  /* 81A2B8 sbc */
    tay16();  /* 81A2BB tay */
    txa16();  /* 81A2BC txa */
    clc();  /* 81A2BD clc */
    adc16_v(0x000A);  /* 81A2BE adc */
    tax16();  /* 81A2C1 tax */
    tya16();  /* 81A2C2 tya */
    MEM16(dec, ea_abs(SCRATCH));  /* 81A2C3 dec */
    if (!IF_Z) goto L_81A2A5;  /* 81A2C6 bne */
    stx16(ea_abs(TILE_BLOCKS_LENGTH));  /* 81A2C8 stx */
    op_sep(0x30);  /* 81A2CB sep */
    MEM8(inc, ea_abs(TILE_BLOCKS_PENDING));  /* 81A2CD inc */
    return;  /* 81A2D0 rtl */
}
