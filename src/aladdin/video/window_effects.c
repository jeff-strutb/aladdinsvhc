/*
 * window_effects.c: the windows some screens draw shapes with by HDMA:
 * a channel started with its table at the table area's base, the window
 * and colour math shadows set for a frame, a shape narrowed round its
 * middle, a half turn of sines, and a window box round a point in the
 * level.
 */
#include "machine.h"
#include "ops65.h"
void abi_HalfSineBuild(void);
void abi_HdmaStartAtBase(void);
void abi_TaskWait(void);
void abi_WindowBoxAt(void);
void abi_WindowColoursWait(void);
void abi_WindowLinesFromShape(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"


/*
 * @routine 82:8226 WindowColoursWait
 * WHAT IT DOES: Sets the window and colour math shadows the window effects
 * use (BG1 and BG2 masked on the main screen, the fixed colour added to
 * the backdrop), then waits a frame.
 * HOW: TMW 3, TSW 0, WOBJSEL $30, CGWSEL $20, CGADSUB $20 and the fixed
 * colour (red 6, green 3, blue 22) into their shadows; TaskWait 1.
 * Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR
 * from a task. Exit: as TaskWait, when the task resumes.
 */
void abi_WindowColoursWait(void)
{
    lda8_v(0x03);  /* 828226 lda */
    sta8(ea_abs(TMW_SHADOW));  /* 828228 sta */
    stz8(ea_abs(TSW_SHADOW));  /* 82822B stz */
    lda8_v(0x30);  /* 82822E lda */
    sta8(ea_abs(WOBJSEL_SHADOW));  /* 828230 sta */
    lda8_v(0x20);  /* 828233 lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 828235 sta */
    lda8_v(0x20);  /* 828238 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 82823A sta */
    lda8_v(0x06);  /* 82823D lda */
    sta8(ea_abs(COLDATA_RED));  /* 82823F sta */
    lda8_v(0x03);  /* 828242 lda */
    sta8(ea_abs(COLDATA_GREEN));  /* 828244 sta */
    lda8_v(0x16);  /* 828247 lda */
    sta8(ea_abs(COLDATA_BLUE));  /* 828249 sta */
    lda8_v(0x01);  /* 82824C lda */
    jsl_frame(0x82824Eu); abi_TaskWait();  /* 82824E jsl */
    return;  /* 828252 rts */
}

/*
 * @routine 82:8253 WindowLinesFromShape
 * WHAT IT DOES: Writes the window's left and right edges for up to $0033
 * lines, from the bottom up: the shape in the table at $82:BBAE narrowed
 * to at most $0032 pixels either side of each line's middle, moved right
 * by $0030.
 * HOW: From the shape's last pair (the table's first word is its offset)
 * and the edges at X, back two bytes a line: the middle of the shape's
 * left and right; the left edge the larger of the shape's left and the
 * middle less the half width, the right edge the smaller of its right and
 * the middle plus it, each plus the shift, to $7E:F800 + X and + X + 1.
 * Ends when X goes negative, the shape's offset reaches 0 or the line
 * count runs out.
 * Entry: X 16-bit; X = the offset of the bottom line's edges; DB = $82;
 * called by JSR. Exit: M 8-bit; $002C, $002D = the last shape pair used,
 * $002E its middle; X, Y past it.
 */
void abi_WindowLinesFromShape(void)
{
    ldy16(ea_abs(0xBBAE));  /* 828253 ldy */
L_828256:
    op_sep(0x20);  /* 828256 sep */
    lda8(ea_absy(0xBBAE));  /* 828258 lda */
    sta8(ea_abs(0x002C));  /* 82825B sta */
    lda8(ea_absy(0xBBAF));  /* 82825E lda */
    sta8(ea_abs(0x002D));  /* 828261 sta */
    sec();  /* 828264 sec */
    sbc8_v(rd8(ea_abs(0x002C)));  /* 828265 sbc */
    ACC8(lsr);  /* 828268 lsr */
    clc();  /* 828269 clc */
    adc8_v(rd8(ea_abs(0x002C)));  /* 82826A adc */
    sta8(ea_abs(0x002E));  /* 82826D sta */
    sec();  /* 828270 sec */
    sbc8_v(rd8(ea_abs(0x0032)));  /* 828271 sbc */
    cmp8_v(rd8(ea_abs(0x002C)));  /* 828274 cmp */
    if (IF_C) goto L_82827C;  /* 828277 bcs */
    lda8(ea_abs(0x002C));  /* 828279 lda */
L_82827C:
    clc();  /* 82827C clc */
    adc8_v(rd8(ea_abs(0x0030)));  /* 82827D adc */
    sta8(ea_longx(BAND_POS));  /* 828280 sta */
    clc();  /* 828284 clc */
    lda8(ea_abs(0x002E));  /* 828285 lda */
    adc8_v(rd8(ea_abs(0x0032)));  /* 828288 adc */
    cmp8_v(rd8(ea_abs(0x002D)));  /* 82828B cmp */
    if (!IF_C) goto L_828293;  /* 82828E bcc */
    lda8(ea_abs(0x002D));  /* 828290 lda */
L_828293:
    clc();  /* 828293 clc */
    adc8_v(rd8(ea_abs(0x0030)));  /* 828294 adc */
    sta8(ea_longx(0x7EF801u));  /* 828297 sta */
    dex16();  /* 82829B dex */
    dex16();  /* 82829C dex */
    if (IF_N) goto L_8282A8;  /* 82829D bmi */
    dey16();  /* 82829F dey */
    dey16();  /* 8282A0 dey */
    if (IF_Z) goto L_8282A8;  /* 8282A1 beq */
    MEM8(dec, ea_abs(0x0033));  /* 8282A3 dec */
    if (!IF_Z) goto L_828256;  /* 8282A6 bne */
L_8282A8:
    op_sep(0x20);  /* 8282A8 sep */
    return;  /* 8282AA rts */
}

/*
 * @routine 82:82AB HalfSineBuild
 * WHAT IT DOES: Builds at $7F:5600 a half turn of sines as bytes (64
 * rising, then the same falling) and at $7F:5680 their complements.
 * HOW: With DB = $82, for each of 64 entries the high byte of the next
 * sine word at $82:87D3 goes to entry x and to entry 63 - x of the second
 * quarter ($7F:5640), and EORed with $FF to the same entries $80 on.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: DB = $82; X = $40; Y =
 * $80; C set, Z set.
 */
void abi_HalfSineBuild(void)
{
    lda8_v(0x82);  /* 8282AB lda */
    pha8();  /* 8282AD pha */
    plb();  /* 8282AE plb */
    ldx8_v(0x00);  /* 8282AF ldx */
    ldy8_v(0x00);  /* 8282B1 ldy */
L_8282B3:
    lda8(ea_absy(0x87D4));  /* 8282B3 lda */
    sta8(ea_longx(0x7F5600u));  /* 8282B6 sta */
    eor8_v(0xFF);  /* 8282BA eor */
    sta8(ea_longx(0x7F5680u));  /* 8282BC sta */
    phx8();  /* 8282C0 phx */
    txa8();  /* 8282C1 txa */
    eor8_v(0x3F);  /* 8282C2 eor */
    tax8();  /* 8282C4 tax */
    lda8(ea_absy(0x87D4));  /* 8282C5 lda */
    sta8(ea_longx(0x7F5640u));  /* 8282C8 sta */
    eor8_v(0xFF);  /* 8282CC eor */
    sta8(ea_longx(0x7F56C0u));  /* 8282CE sta */
    plx8();  /* 8282D2 plx */
    iny8();  /* 8282D3 iny */
    iny8();  /* 8282D4 iny */
    inx8();  /* 8282D5 inx */
    cpx8_v(0x40);  /* 8282D6 cpx */
    if (!IF_C) goto L_8282B3;  /* 8282D8 bcc */
    return;  /* 8282DA rts */
}

/*
 * @routine 82:8417 WindowBoxAt
 * WHAT IT DOES: Adds to the HDMA table being built at $7F:5600 the lines
 * of a window 128 pixels wide and up to 96 lines tall whose top left is
 * the level point in $000C, $000E; C set, nothing added, when the box is
 * off the screen.
 * HOW: The point less the play field's scroll ($002C across, $002E down);
 * off when across is outside -128 to 255 or down outside -96 to 223.
 * Entries of 3 bytes from X, a line count then the window's left and
 * right: lines above the box (down above 0) as entries of no window (left
 * $FF, right 0), two when more than 127; then the box's lines (96, fewer
 * at the screen's bottom, or down + 97 when it starts above the screen)
 * with the window from across to across + 128, clipped to 0 and 255.
 * Entry: M 16-bit, X 8-bit; X = the next entry's offset; DB a bank whose
 * $0000-$1FFF is work RAM; called by JSR. Exit: added: C clear, X past
 * the entries; off: C set (at $82:8415).
 */
void abi_WindowBoxAt(void)
{
    goto L_828417;
L_828415:
    sec();  /* 828415 sec */
    return;  /* 828416 rts */
L_828417:
    sec();  /* 828417 sec */
    lda16(ea_abs(SCRATCH));  /* 828418 lda */
    sbc16_v(rd16(ea_abs(0x1773)));  /* 82841B sbc */
    sta16(ea_abs(0x002C));  /* 82841E sta */
    cmp16_v(0x0100);  /* 828421 cmp */
    if (!IF_C) goto L_82842F;  /* 828424 bcc */
    clc();  /* 828426 clc */
    adc16_v(0x0080);  /* 828427 adc */
    cmp16_v(0x0100);  /* 82842A cmp */
    if (IF_C) goto L_828415;  /* 82842D bcs */
L_82842F:
    sec();  /* 82842F sec */
    lda16(ea_abs(0x000E));  /* 828430 lda */
    sbc16_v(rd16(ea_abs(0x1777)));  /* 828433 sbc */
    sta16(ea_abs(0x002E));  /* 828436 sta */
    cmp16_v(0x00E0);  /* 828439 cmp */
    if (!IF_C) goto L_828447;  /* 82843C bcc */
    clc();  /* 82843E clc */
    adc16_v(0x0060);  /* 82843F adc */
    cmp16_v(0x00E0);  /* 828442 cmp */
    if (IF_C) goto L_828415;  /* 828445 bcs */
L_828447:
    lda16(ea_abs(0x002E));  /* 828447 lda */
    if (IF_N) goto L_828486;  /* 82844A bmi */
    if (IF_Z) goto L_828486;  /* 82844C beq */
    cmp16_v(0x0080);  /* 82844E cmp */
    if (!IF_C) goto L_828467;  /* 828451 bcc */
    sbc16_v(0x007F);  /* 828453 sbc */
    sta16(ea_longx(0x7F5600u));  /* 828456 sta */
    lda16_v(0x00FF);  /* 82845A lda */
    sta16(ea_longx(0x7F5601u));  /* 82845D sta */
    inx8();  /* 828461 inx */
    inx8();  /* 828462 inx */
    inx8();  /* 828463 inx */
    lda16_v(0x007F);  /* 828464 lda */
L_828467:
    sta16(ea_longx(0x7F5600u));  /* 828467 sta */
    lda16_v(0x00FF);  /* 82846B lda */
    sta16(ea_longx(0x7F5601u));  /* 82846E sta */
    inx8();  /* 828472 inx */
    inx8();  /* 828473 inx */
    inx8();  /* 828474 inx */
    sec();  /* 828475 sec */
    lda16_v(0x00E0);  /* 828476 lda */
    sbc16_v(rd16(ea_abs(0x002E)));  /* 828479 sbc */
    cmp16_v(0x0060);  /* 82847C cmp */
    if (!IF_C) goto L_82848D;  /* 82847F bcc */
    lda16_v(0x0060);  /* 828481 lda */
    goto L_82848D;  /* 828484 bra */
L_828486:
    clc();  /* 828486 clc */
    lda16(ea_abs(0x002E));  /* 828487 lda */
    adc16_v(0x0061);  /* 82848A adc */
L_82848D:
    sta16(ea_longx(0x7F5600u));  /* 82848D sta */
    lda16(ea_abs(0x002C));  /* 828491 lda */
    if (!IF_N) goto L_8284A0;  /* 828494 bpl */
    clc();  /* 828496 clc */
    adc16_v(0x0080);  /* 828497 adc */
    xba();  /* 82849A xba */
    and16_v(0xFF00);  /* 82849B and */
    goto L_8284C2;  /* 82849E bra */
L_8284A0:
    sec();  /* 8284A0 sec */
    lda16_v(0x0100);  /* 8284A1 lda */
    sbc16_v(rd16(ea_abs(0x002C)));  /* 8284A4 sbc */
    cmp16_v(0x0081);  /* 8284A7 cmp */
    if (!IF_C) goto L_8284BC;  /* 8284AA bcc */
    op_sep(0x20);  /* 8284AC sep */
    clc();  /* 8284AE clc */
    lda8(ea_abs(0x002C));  /* 8284AF lda */
    adc8_v(0x80);  /* 8284B2 adc */
    xba();  /* 8284B4 xba */
    lda8(ea_abs(0x002C));  /* 8284B5 lda */
    op_rep(0x20);  /* 8284B8 rep */
    goto L_8284C2;  /* 8284BA bra */
L_8284BC:
    lda16(ea_abs(0x002C));  /* 8284BC lda */
    ora16_v(0xFF00);  /* 8284BF ora */
L_8284C2:
    sta16(ea_longx(0x7F5601u));  /* 8284C2 sta */
    inx8();  /* 8284C6 inx */
    inx8();  /* 8284C7 inx */
    inx8();  /* 8284C8 inx */
    clc();  /* 8284C9 clc */
    return;  /* 8284CA rts */
}



