/*
 * text_bands.c: a screen of text shown band by band (the story text in
 * bank $82): a page of lines written to the text tilemap, and the bands'
 * window bytes handed to HDMA channel 5 a frame at a time.
 */
#include "machine.h"
#include "ops65.h"
void abi_TextBandsCommit(void);
void abi_TextPageDraw(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 82:B270 TextBandsCommit
 * WHAT IT DOES: Hands the 16 window bytes of the text bands ($1C8E) to
 * HDMA channel 5, through whichever of its two tables is not in use.
 * HOW: By bit 0 of $F9: clear, the bytes go to $1C9E and channel 5's
 * table address to $CEEF; set, to $1CAE and $CF3B (the addresses into
 * $0352, channel 5's in hdma_table_at). $F9 steps up.
 * Entry: M 8-bit, X 8-bit; D the page holding $F9; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: X = $FF; Y = $0F or $1F;
 * A low byte = the last byte copied.
 */
void abi_TextBandsCommit(void)
{
    ldx8_v(0x0F);  /* 82B270 ldx */
    ldy8_v(0x1F);  /* 82B272 ldy */
    lda8_v(0xEF);  /* 82B274 lda */
    sta8(ea_abs(0x0352));  /* 82B276 sta */
    lda8_v(0xCE);  /* 82B279 lda */
    sta8(ea_abs(0x0353));  /* 82B27B sta */
    lda8(ea_dp(0xF9));  /* 82B27E lda */
    ACC8(lsr);  /* 82B280 lsr */
    if (!IF_C) goto L_82B28F;  /* 82B281 bcc */
    ldy8_v(0x2F);  /* 82B283 ldy */
    lda8_v(0x3B);  /* 82B285 lda */
    sta8(ea_abs(0x0352));  /* 82B287 sta */
    lda8_v(0xCF);  /* 82B28A lda */
    sta8(ea_abs(0x0353));  /* 82B28C sta */
L_82B28F:
    lda8(ea_absx(TEXT_BANDS));  /* 82B28F lda */
    sta8(ea_absy(TEXT_BANDS));  /* 82B292 sta */
    dey8();  /* 82B295 dey */
    dex8();  /* 82B296 dex */
    if (!IF_N) goto L_82B28F;  /* 82B297 bpl */
    MEM8(inc, ea_dp(0xF9));  /* 82B299 inc */
    return;  /* 82B29B rts */
}

/*
 * @routine 82:B190 TextPageDraw
 * WHAT IT DOES: Writes a page of text into the text tilemap, each line at
 * the next place from the list at $F5 with the line's attribute byte,
 * then has the NMI upload the tilemap.
 * HOW: The page's first byte is its line count ($000C). Per line: the
 * list's next word (stepping $F5) is its tilemap offset; then the line's
 * attribute byte ($000D) and length ($000E), then its characters, each
 * the low byte of a tilemap word whose high byte is the attribute.
 * text_tilemap_upload steps up.
 * Entry: M 8-bit, X 16-bit; Y = the page's offset; D = 0; DB = the bank of
 * the page and the list; called by JSR. Exit: X, Y past the last line;
 * $000C, $000E = 0.
 */
void abi_TextPageDraw(void)
{
    lda8(ea_absy(0x0000));  /* 82B190 lda */
    sta8(ea_abs(SCRATCH));  /* 82B193 sta */
    iny16();  /* 82B196 iny */
L_82B197:
    phy16();  /* 82B197 phy */
    ldx16(ea_dp(0xF5));  /* 82B198 ldx */
    ldy16(ea_absx(0x0000));  /* 82B19A ldy */
    inx16();  /* 82B19D inx */
    inx16();  /* 82B19E inx */
    stx16(ea_dp(0xF5));  /* 82B19F stx */
    tyx16();  /* 82B1A1 tyx */
    ply16();  /* 82B1A2 ply */
    lda8(ea_absy(0x0000));  /* 82B1A3 lda */
    sta8(ea_abs(0x000D));  /* 82B1A6 sta */
    lda8(ea_absy(0x0001));  /* 82B1A9 lda */
    sta8(ea_abs(0x000E));  /* 82B1AC sta */
    iny16();  /* 82B1AF iny */
    iny16();  /* 82B1B0 iny */
L_82B1B1:
    lda8(ea_absy(0x0000));  /* 82B1B1 lda */
    sta8(ea_longx(TEXT_TILEMAP));  /* 82B1B4 sta */
    lda8(ea_abs(0x000D));  /* 82B1B8 lda */
    sta8(ea_longx(0x7ED801u));  /* 82B1BB sta */
    inx16();  /* 82B1BF inx */
    inx16();  /* 82B1C0 inx */
    iny16();  /* 82B1C1 iny */
    MEM8(dec, ea_dp(0x0E));  /* 82B1C2 dec */
    if (!IF_Z) goto L_82B1B1;  /* 82B1C4 bne */
    MEM8(dec, ea_dp(0x0C));  /* 82B1C6 dec */
    if (!IF_Z) goto L_82B197;  /* 82B1C8 bne */
    MEM8(inc, ea_abs(TEXT_TILEMAP_UPLOAD));  /* 82B1CA inc */
    return;  /* 82B1CD rts */
}
