/*
 * window_effects.c: the windows some screens draw shapes with by HDMA:
 * a channel started with its table at the table area's base, the window
 * and colour math shadows set for a frame, a shape narrowed round its
 * middle, a half turn of sines, and a window box round a point in the
 * level.
 */
#include "machine.h"
#include "ops65.h"
void abi_HdmaChannelFree(void);
void abi_HdmaEffect10(void);
void abi_HdmaEffect11(void);
void abi_HdmaEffect12(void);
void abi_HdmaEffect2(void);
void abi_HdmaEffect4(void);
void abi_HdmaEffect8(void);
void abi_HdmaEffect9(void);
void abi_HdmaStart(void);
void abi_TaskExit(void);
void abi_WindowEffectTask(void);
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

/*
 * @routine 82:871B HdmaChannelFree
 * WHAT IT DOES: Finds a free HDMA channel among 4-7, from 7 down: C clear
 * and X = the channel, or C set when all four are in use.
 * HOW: The high nibble of HDMAEN's shadow ORed with $03B5; all set: C
 * set. Else shifted left until a clear bit, X stepping down from 7.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
 * called by JSR. Exit: X = the channel (or 3 or 4 when none); A shifted.
 */
void abi_HdmaChannelFree(void)
{
    ldx8_v(0x07);  /* 82871B ldx */
    lda8(ea_abs(HDMAEN_SHADOW));  /* 82871D lda */
    ora8_v(rd8(ea_abs(0x03B5)));  /* 828720 ora */
    and8_v(0xF0);  /* 828723 and */
    cmp8_v(0xF0);  /* 828725 cmp */
    if (IF_Z) goto L_828731;  /* 828727 beq */
L_828729:
    ACC8(asl);  /* 828729 asl */
    if (!IF_C) goto L_828733;  /* 82872A bcc */
    dex8();  /* 82872C dex */
    cpx8_v(0x04);  /* 82872D cpx */
    if (IF_C) goto L_828729;  /* 82872F bcs */
L_828731:
    sec();  /* 828731 sec */
    return;  /* 828732 rts */
L_828733:
    clc();  /* 828733 clc */
    return;  /* 828734 rts */
}

/*
 * @routine 82:873B HdmaStart
 * WHAT IT DOES: Starts HDMA table set X on register set Y in a free
 * channel, its table built at $7E:F400 plus $0032: C clear, $002C the
 * channel's HDMAEN bit; C set when no channel is free.
 * HOW: $0030 cleared; HdmaChannelFree. The channel's bit ($82:BCA1) at
 * $002C, its register base * 16 at $002D; the table address ($F400 +
 * $0032) into hdma_table_at for it; DMAP and BBAD from the register set
 * ($82:BCA9), bank $7E. The set's entries ($82:BCBB: count, then a word)
 * copied to the table, $0030 added to each word, until a count of 0.
 * Entry: M 8-bit, X 8-bit; X = the table set; Y = the register set; $0032
 * = the table's place; DB = $82; called by JSR. Exit: M 8-bit, X 8-bit; C
 * as above.
 */
void abi_HdmaStart(void)
{
    stz8(ea_abs(0x0030));  /* 82873B stz */
    stz8(ea_abs(0x0031));  /* 82873E stz */
    stx8(ea_abs(0x002E));  /* 828741 stx */
    call_routine(0x82871Bu, 0);  /* 828744 jsr */
    if (!IF_C) goto L_82874A;  /* 828747 bcc */
    return;  /* 828749 rts */
L_82874A:
    txa8();  /* 82874A txa */
    ACC8(asl);  /* 82874B asl */
    ACC8(asl);  /* 82874C asl */
    ACC8(asl);  /* 82874D asl */
    ACC8(asl);  /* 82874E asl */
    sta8(ea_abs(0x002D));  /* 82874F sta */
    lda8(ea_absx(0xBCA1));  /* 828752 lda */
    sta8(ea_abs(0x002C));  /* 828755 sta */
    txa8();  /* 828758 txa */
    sec();  /* 828759 sec */
    sbc8_v(0x04);  /* 82875A sbc */
    ACC8(asl);  /* 82875C asl */
    tax8();  /* 82875D tax */
    clc();  /* 82875E clc */
    lda8(ea_abs(0x0032));  /* 82875F lda */
    adc8_v(0x00);  /* 828762 adc */
    sta8(ea_abs(0x0032));  /* 828764 sta */
    sta8(ea_absx(HDMA_TABLE_AT));  /* 828767 sta */
    lda8(ea_abs(0x0033));  /* 82876A lda */
    adc8_v(0xF4);  /* 82876D adc */
    sta8(ea_abs(0x0033));  /* 82876F sta */
    sta8(ea_absx(0x0351));  /* 828772 sta */
    ldx8(ea_abs(0x002D));  /* 828775 ldx */
    lda8(ea_absy(0xBCA9));  /* 828778 lda */
    sta8(ea_absx(DMAP0));  /* 82877B sta */
    lda8(ea_absy(0xBCAA));  /* 82877E lda */
    sta8(ea_absx(BBAD0));  /* 828781 sta */
    lda8_v(0x7E);  /* 828784 lda */
    sta8(ea_absx(A1B0));  /* 828786 sta */
    sta8(ea_absx(DASB0));  /* 828789 sta */
    ldx8(ea_abs(0x002E));  /* 82878C ldx */
    op_rep(0x10);  /* 82878F rep */
    ldy16(ea_absx(0xBCBB));  /* 828791 ldy */
    ldx16(ea_abs(0x0032));  /* 828794 ldx */
L_828797:
    lda8(ea_absy(0xBCBB));  /* 828797 lda */
    sta8(ea_longx(0x7E0000u));  /* 82879A sta */
    if (IF_Z) goto L_8287BD;  /* 82879E beq */
    clc();  /* 8287A0 clc */
    lda8(ea_absy(0xBCBC));  /* 8287A1 lda */
    adc8_v(rd8(ea_abs(0x0030)));  /* 8287A4 adc */
    sta8(ea_longx(0x7E0001u));  /* 8287A7 sta */
    lda8(ea_absy(0xBCBD));  /* 8287AB lda */
    adc8_v(rd8(ea_abs(0x0031)));  /* 8287AE adc */
    sta8(ea_longx(0x7E0002u));  /* 8287B1 sta */
    inx16();  /* 8287B5 inx */
    inx16();  /* 8287B6 inx */
    inx16();  /* 8287B7 inx */
    iny16();  /* 8287B8 iny */
    iny16();  /* 8287B9 iny */
    iny16();  /* 8287BA iny */
    goto L_828797;  /* 8287BB bra */
L_8287BD:
    op_sep(0x10);  /* 8287BD sep */
    clc();  /* 8287BF clc */
    return;  /* 8287C0 rts */
}

/*
 * @routine 82:8000 WindowEffectTask
 * WHAT IT DOES: Runs window effect $06 of its slot (its parameter) from
 * the table at $82:8017, retrying each frame while the effect cannot
 * start.
 * HOW: D = the slot's page ($6F + Y); JSR through the table by the
 * parameter; when it returns (no HDMA channel was free), TaskWait 1 and
 * again.
 * Entry: M 8-bit, X 8-bit; Y = the task's slot; a task's entry. Exit:
 * never returns (the effects end the task).
 */
void abi_WindowEffectTask(void)
{
    op_rep(0x20);  /* 828000 rep */
    clc();  /* 828002 clc */
    tya16();  /* 828003 tya */
    adc16_v(0x006F);  /* 828004 adc */
    tcd();  /* 828007 tcd */
    op_sep(0x20);  /* 828008 sep */
L_82800A:
    ldx8(ea_dp(0x06));  /* 82800A ldx */
    call_routine(0x820000u | rd16(0x820000u | (uint16_t)(0x8017 + cpu.x)), 0);  /* 82800C jsr */
    lda8_v(0x01);  /* 82800F lda */
    jsl_frame(0x828011u); abi_TaskWait();  /* 828011 jsl */
    goto L_82800A;  /* 828015 bra */
}

/*
 * @routine 82:80E2 HdmaEffect2
 * WHAT IT DOES: Window effect 2: HDMA table set 4 on register set 6 from
 * the table area's base, then the task ends.
 * HOW: HdmaStartAtBase; no channel: back (to retry); else the channel on
 * in HDMAEN's shadow and TaskExit.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect2(void)
{
    ldx8_v(0x04);  /* 8280E2 ldx */
    ldy8_v(0x06);  /* 8280E4 ldy */
    call_routine(0x828735u, 0);  /* 8280E6 jsr */
    if (!IF_C) goto L_8280EC;  /* 8280E9 bcc */
    return;  /* 8280EB rts */
L_8280EC:
    lda8(ea_abs(0x002C));  /* 8280EC lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 8280EF tsb */
    abi_TaskExit(); return;  /* 8280F2 jml */
}

/*
 * @routine 82:82DB HdmaEffect4
 * WHAT IT DOES: Window effect 4: HDMA table set 6 on register set 8 with
 * its table $20 into the area, the value at $7E:F480 7, and the task left
 * waiting for good.
 * HOW: $0032 = $20; HdmaStart; no channel: back. $7E:F480 = 7, $7E:F481 =
 * 0; the channel on; TaskWait 2 forever.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect4(void)
{
    lda8_v(0x20);  /* 8282DB lda */
    sta8(ea_abs(0x0032));  /* 8282DD sta */
    stz8(ea_abs(0x0033));  /* 8282E0 stz */
    ldx8_v(0x06);  /* 8282E3 ldx */
    ldy8_v(0x08);  /* 8282E5 ldy */
    call_routine(0x82873Bu, 0);  /* 8282E7 jsr */
    if (!IF_C) goto L_8282ED;  /* 8282EA bcc */
    return;  /* 8282EC rts */
L_8282ED:
    lda8_v(0x07);  /* 8282ED lda */
    sta8(0x7EF480u);  /* 8282EF sta */
    lda8_v(0x00);  /* 8282F3 lda */
    sta8(0x7EF481u);  /* 8282F5 sta */
    lda8(ea_abs(0x002C));  /* 8282F9 lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 8282FC tsb */
L_8282FF:
    lda8_v(0x02);  /* 8282FF lda */
    jsl_frame(0x828301u); abi_TaskWait();  /* 828301 jsl */
    goto L_8282FF;  /* 828305 bra */
}

/*
 * @routine 82:8388 HdmaEffect8
 * WHAT IT DOES: Window effect 8: windows round two places in the level
 * ($0E40, $00F8 and $0D60, $01D8), rebuilt each frame and swapped between
 * two tables.
 * HOW: HdmaStartAtBase with set $0E on register set $0E; $00BD up; the
 * channel on; its table address slot kept at $06 and the table at $07
 * ($F400). Each frame: TaskWait 1; the table address set; WindowBoxAt for
 * each place; an entry of 1 line of no window and the end; WH0's shadow
 * from the first entry; the 16 bytes moved from $7F:5600 to the table,
 * and the table swapped ($10 apart).
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect8(void)
{
    ldx8_v(0x0E);  /* 828388 ldx */
    ldy8_v(0x0E);  /* 82838A ldy */
    call_routine(0x828735u, 0);  /* 82838C jsr */
    if (!IF_C) goto L_828392;  /* 82838F bcc */
    return;  /* 828391 rts */
L_828392:
    MEM8(inc, ea_abs(0x00BD));  /* 828392 inc */
    lda8(ea_abs(0x002C));  /* 828395 lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 828398 tsb */
    lda8(ea_abs(0x002D));  /* 82839B lda */
    ACC8(lsr);  /* 82839E lsr */
    ACC8(lsr);  /* 82839F lsr */
    ACC8(lsr);  /* 8283A0 lsr */
    ACC8(lsr);  /* 8283A1 lsr */
    clc();  /* 8283A2 clc */
    adc8_v(0xFC);  /* 8283A3 adc */
    ACC8(asl);  /* 8283A5 asl */
    sta8(ea_dp(0x06));  /* 8283A6 sta */
    tax8();  /* 8283A8 tax */
    op_rep(0x20);  /* 8283A9 rep */
    lda16_v(0xF400);  /* 8283AB lda */
    sta16(ea_dp(0x07));  /* 8283AE sta */
L_8283B0:
    op_sep(0x30);  /* 8283B0 sep */
    lda8_v(0x01);  /* 8283B2 lda */
    jsl_frame(0x8283B4u); abi_TaskWait();  /* 8283B4 jsl */
    op_rep(0x20);  /* 8283B8 rep */
    ldx8(ea_dp(0x06));  /* 8283BA ldx */
    lda16(ea_dp(0x07));  /* 8283BC lda */
    sta16(ea_absx(HDMA_TABLE_AT));  /* 8283BE sta */
    ldx8_v(0x00);  /* 8283C1 ldx */
    lda16_v(0x0E40);  /* 8283C3 lda */
    sta16(ea_abs(SCRATCH));  /* 8283C6 sta */
    lda16_v(0x00F8);  /* 8283C9 lda */
    sta16(ea_abs(0x000E));  /* 8283CC sta */
    abi_WindowBoxAt();  /* 8283CF jsr */
    lda16_v(0x0D60);  /* 8283D2 lda */
    sta16(ea_abs(SCRATCH));  /* 8283D5 sta */
    lda16_v(0x01D8);  /* 8283D8 lda */
    sta16(ea_abs(0x000E));  /* 8283DB sta */
    abi_WindowBoxAt();  /* 8283DE jsr */
    lda16_v(0x0001);  /* 8283E1 lda */
    sta16(ea_longx(0x7F5600u));  /* 8283E4 sta */
    lda16_v(0x00FF);  /* 8283E8 lda */
    sta16(ea_longx(0x7F5601u));  /* 8283EB sta */
    lda16_v(0x0000);  /* 8283EF lda */
    sta16(ea_longx(0x7F5603u));  /* 8283F2 sta */
    lda16(0x7F5601u);  /* 8283F6 lda */
    sta16(ea_abs(0x03A6));  /* 8283FA sta */
    op_rep(0x10);  /* 8283FD rep */
    phb();  /* 8283FF phb */
    ldx16_v(0x5600);  /* 828400 ldx */
    ldy16(ea_dp(0x07));  /* 828403 ldy */
    lda16_v(0x000F);  /* 828405 lda */
    mvn(0x7E, 0x7F);  /* 828408 mvn */
    plb();  /* 82840B plb */
    lda16(ea_dp(0x07));  /* 82840C lda */
    eor16_v(0x0010);  /* 82840E eor */
    sta16(ea_dp(0x07));  /* 828411 sta */
    goto L_8283B0;  /* 828413 bra */
}

/*
 * @routine 82:84CB HdmaEffect10
 * WHAT IT DOES: Window effect 10: a window and colour math round the
 * place $0E78, $0144 in the level while it is on the screen; when $00C1
 * is set, the window closes in from both sides and the task ends.
 * HOW: HdmaStart with set $10 on register set $0C at place $100; the
 * channel on (its bit kept at $06); the masks cleared, WOBJSEL $C0; the
 * table's 32 bytes at $7E:F500 kept at $F520. Each frame: the place less
 * the scroll and the layer's shift: on the screen, the window's left and
 * right (right clamped to 255) and its first line into the table and
 * WH0/WH1's shadows, the fixed colour (8, 8, 16) added (CGADSUB $33), a
 * WRAM move queued to restore the table; off, the colour math off. With
 * $00C1 set, WH0 up and WH1 down a step a frame until they cross, then
 * the colour math off, the channel off and TaskExit.
 * Entry: M 8-bit, X 8-bit; D the task's page; DB = $82; called by JSR
 * from WindowEffectTask. Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect10(void)
{
    lda8_v(0x01);  /* 8284CB lda */
    sta8(ea_abs(0x0033));  /* 8284CD sta */
    stz8(ea_abs(0x0032));  /* 8284D0 stz */
    ldx8_v(0x10);  /* 8284D3 ldx */
    ldy8_v(0x0C);  /* 8284D5 ldy */
    call_routine(0x82873Bu, 0);  /* 8284D7 jsr */
    if (!IF_C) goto L_8284DD;  /* 8284DA bcc */
    return;  /* 8284DC rts */
L_8284DD:
    lda8(ea_abs(0x002C));  /* 8284DD lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 8284E0 tsb */
    sta8(ea_dp(0x06));  /* 8284E3 sta */
    lda8_v(0xFF);  /* 8284E5 lda */
    sta8(0x7EF510u);  /* 8284E7 sta */
    ACC8(inc);  /* 8284EB inc */
    sta8(0x7EF511u);  /* 8284EC sta */
    stz8(ea_abs(TMW_SHADOW));  /* 8284F0 stz */
    stz8(ea_abs(TSW_SHADOW));  /* 8284F3 stz */
    stz8(ea_abs(0x03AA));  /* 8284F6 stz */
    stz8(ea_abs(0x03AB));  /* 8284F9 stz */
    lda8_v(0xC0);  /* 8284FC lda */
    sta8(ea_abs(WOBJSEL_SHADOW));  /* 8284FE sta */
    phb();  /* 828501 phb */
    op_rep(0x10);  /* 828502 rep */
    ldx16_v(0x001F);  /* 828504 ldx */
    txa8();  /* 828507 txa */
    ldx16_v(0xF500);  /* 828508 ldx */
    ldy16_v(0xF520);  /* 82850B ldy */
    mvn(0x7E, 0x7E);  /* 82850E mvn */
    op_sep(0x10);  /* 828511 sep */
    plb();  /* 828513 plb */
L_828514:
    op_rep(0x20);  /* 828514 rep */
    sec();  /* 828516 sec */
    lda16_v(0x0144);  /* 828517 lda */
    sbc16_v(rd16(ea_abs(0x1777)));  /* 82851A sbc */
    sec();  /* 82851D sec */
    sbc16_v(rd16(ea_abs(LAYER_OFFSET_Y)));  /* 82851E sbc */
    cmp16_v(0x00E0);  /* 828521 cmp */
    if (IF_C) goto L_8285A1;  /* 828524 bcs */
    sta16(ea_abs(0x002E));  /* 828526 sta */
    sec();  /* 828529 sec */
    lda16_v(0x0E78);  /* 82852A lda */
    sbc16_v(rd16(ea_abs(0x1773)));  /* 82852D sbc */
    sec();  /* 828530 sec */
    sbc16_v(rd16(ea_abs(BG1_X_SHIFT)));  /* 828531 sbc */
    cmp16_v(0x0100);  /* 828534 cmp */
    if (IF_C) goto L_8285A1;  /* 828537 bcs */
    sta16(ea_abs(0x002C));  /* 828539 sta */
    op_sep(0x20);  /* 82853C sep */
    clc();  /* 82853E clc */
    adc8_v(0x10);  /* 82853F adc */
    if (!IF_C) goto L_828545;  /* 828541 bcc */
    lda8_v(0xFF);  /* 828543 lda */
L_828545:
    sta8(0x7EF533u);  /* 828545 sta */
    sta8(ea_abs(0x03A9));  /* 828549 sta */
    lda8(ea_abs(0x002C));  /* 82854C lda */
    sta8(0x7EF532u);  /* 82854F sta */
    sta8(ea_abs(0x03A8));  /* 828553 sta */
    lda8(ea_abs(0x002E));  /* 828556 lda */
    ACC8(lsr);  /* 828559 lsr */
    adc8_v(0x00);  /* 82855A adc */
    sta8(0x7EF520u);  /* 82855C sta */
    sta8(0x7EF523u);  /* 828560 sta */
    lda8_v(0x08);  /* 828564 lda */
    sta8(ea_abs(COLDATA_RED));  /* 828566 sta */
    sta8(ea_abs(COLDATA_GREEN));  /* 828569 sta */
    ACC8(asl);  /* 82856C asl */
    sta8(ea_abs(COLDATA_BLUE));  /* 82856D sta */
    lda8_v(0x20);  /* 828570 lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 828572 sta */
    lda8_v(0x33);  /* 828575 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 828577 sta */
    op_rep(0x20);  /* 82857A rep */
    ldy8(ea_abs(0x1705));  /* 82857C ldy */
    lda16_v(0xF520);  /* 82857F lda */
    sta16(ea_absy(0x1707));  /* 828582 sta */
    lda16_v(0xF500);  /* 828585 lda */
    sta16(ea_absy(0x1709));  /* 828588 sta */
    lda16_v(0x001F);  /* 82858B lda */
    sta16(ea_absy(0x170B));  /* 82858E sta */
    op_sep(0x20);  /* 828591 sep */
    clc();  /* 828593 clc */
    tya8();  /* 828594 tya */
    adc8_v(0x06);  /* 828595 adc */
    sta8(ea_abs(0x1705));  /* 828597 sta */
    lda8(ea_abs(0x00C1));  /* 82859A lda */
    if (!IF_Z) goto L_8285CE;  /* 82859D bne */
    goto L_8285C5;  /* 82859F bra */
L_8285A1:
    op_sep(0x20);  /* 8285A1 sep */
    lda8_v(0x00);  /* 8285A3 lda */
    sta8(0x7EF500u);  /* 8285A5 sta */
    stz8(ea_abs(COLDATA_RED));  /* 8285A9 stz */
    stz8(ea_abs(COLDATA_GREEN));  /* 8285AC stz */
    stz8(ea_abs(COLDATA_BLUE));  /* 8285AF stz */
    lda8_v(0xFF);  /* 8285B2 lda */
    sta8(ea_abs(0x03A8));  /* 8285B4 sta */
    ACC8(inc);  /* 8285B7 inc */
    sta8(ea_abs(0x03A8));  /* 8285B8 sta */
    lda8_v(0x02);  /* 8285BB lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 8285BD sta */
    lda8_v(0x53);  /* 8285C0 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 8285C2 sta */
L_8285C5:
    lda8_v(0x01);  /* 8285C5 lda */
    jsl_frame(0x8285C7u); abi_TaskWait();  /* 8285C7 jsl */
    goto L_828514;  /* 8285CB jmp */
L_8285CE:
    lda8_v(0x01);  /* 8285CE lda */
    jsl_frame(0x8285D0u); abi_TaskWait();  /* 8285D0 jsl */
    lda8(ea_abs(0x03A8));  /* 8285D4 lda */
    ACC8(inc);  /* 8285D7 inc */
    sta8(ea_abs(0x03A8));  /* 8285D8 sta */
    sta8(0x7EF512u);  /* 8285DB sta */
    lda8(ea_abs(0x03A9));  /* 8285DF lda */
    ACC8(dec);  /* 8285E2 dec */
    sta8(ea_abs(0x03A9));  /* 8285E3 sta */
    sta8(0x7EF513u);  /* 8285E6 sta */
    cmp8_v(rd8(ea_abs(0x03A8)));  /* 8285EA cmp */
    if (IF_C) goto L_8285CE;  /* 8285ED bcs */
    stz8(ea_abs(COLDATA_RED));  /* 8285EF stz */
    stz8(ea_abs(COLDATA_GREEN));  /* 8285F2 stz */
    stz8(ea_abs(COLDATA_BLUE));  /* 8285F5 stz */
    lda8_v(0x02);  /* 8285F8 lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 8285FA sta */
    lda8_v(0x53);  /* 8285FD lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 8285FF sta */
    lda8(ea_dp(0x06));  /* 828602 lda */
    trb8(ea_abs(HDMAEN_SHADOW));  /* 828604 trb */
    abi_TaskExit(); return;  /* 828607 jml */
}

/*
 * @routine 82:860B HdmaEffect11
 * WHAT IT DOES: Window effect 11: a window that narrows over the lines
 * above $00F8 in the level, a step a frame, then the task ends.
 * HOW: HdmaStart with set $12 on register set $0C at place $80; the
 * channel on; WH2 and the table's window $00FF/$FF00; the line count
 * ($00F8 less the scroll y, + $30). Each frame: TaskWait 1, the left edge
 * up, the right edge down, the count down; at 0 TaskExit.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect11(void)
{
    lda8_v(0x80);  /* 82860B lda */
    sta8(ea_abs(0x0032));  /* 82860D sta */
    stz8(ea_abs(0x0033));  /* 828610 stz */
    ldx8_v(0x12);  /* 828613 ldx */
    ldy8_v(0x0C);  /* 828615 ldy */
    call_routine(0x82873Bu, 0);  /* 828617 jsr */
    if (!IF_C) goto L_82861D;  /* 82861A bcc */
    return;  /* 82861C rts */
L_82861D:
    lda8(ea_abs(0x002C));  /* 82861D lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 828620 tsb */
    op_rep(0x20);  /* 828623 rep */
    lda16_v(0x00FF);  /* 828625 lda */
    sta16(0x7EF490u);  /* 828628 sta */
    sta16(ea_abs(0x03A8));  /* 82862C sta */
    lda16_v(0xFF00);  /* 82862F lda */
    sta16(0x7EF492u);  /* 828632 sta */
    sec();  /* 828636 sec */
    lda16_v(0x00F8);  /* 828637 lda */
    sbc16_v(rd16(ea_abs(0x1777)));  /* 82863A sbc */
    op_sep(0x20);  /* 82863D sep */
    clc();  /* 82863F clc */
    adc8_v(0x30);  /* 828640 adc */
    sta8(0x7EF483u);  /* 828642 sta */
    sta8(0x7EF486u);  /* 828646 sta */
L_82864A:
    lda8_v(0x01);  /* 82864A lda */
    jsl_frame(0x82864Cu); abi_TaskWait();  /* 82864C jsl */
    lda8(0x7EF480u);  /* 828650 lda */
    ACC8(inc);  /* 828654 inc */
    sta8(0x7EF480u);  /* 828655 sta */
    lda8(0x7EF492u);  /* 828659 lda */
    ACC8(inc);  /* 82865D inc */
    sta8(0x7EF492u);  /* 82865E sta */
    eor8_v(0xFF);  /* 828662 eor */
    sta8(0x7EF493u);  /* 828664 sta */
    lda8(0x7EF483u);  /* 828668 lda */
    ACC8(dec);  /* 82866C dec */
    sta8(0x7EF483u);  /* 82866D sta */
    sta8(0x7EF486u);  /* 828671 sta */
    if (!IF_Z) goto L_82864A;  /* 828675 bne */
    abi_TaskExit(); return;  /* 828677 jml */
}

/*
 * @routine 82:867B HdmaEffect9
 * WHAT IT DOES: Window effect 9: a window below the line $0200 in the
 * level less the scroll (at most 240 lines down), kept each frame.
 * HOW: HdmaStartAtBase with set $16 on register set $10; the table's
 * first count $10; the channel on. Each frame: TaskWait 1; $0200 less the
 * scroll y: past the screen or 0 lines, the first count 0; else half of
 * it (at most 240) as the first two counts and $10 after.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect9(void)
{
    ldx8_v(0x16);  /* 82867B ldx */
    ldy8_v(0x10);  /* 82867D ldy */
    call_routine(0x828735u, 0);  /* 82867F jsr */
    if (!IF_C) goto L_828685;  /* 828682 bcc */
    return;  /* 828684 rts */
L_828685:
    lda8_v(0x10);  /* 828685 lda */
    sta8(0x7EF500u);  /* 828687 sta */
    lda8_v(0x00);  /* 82868B lda */
    sta8(0x7EF501u);  /* 82868D sta */
    lda8(ea_abs(0x002C));  /* 828691 lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 828694 tsb */
L_828697:
    lda8_v(0x01);  /* 828697 lda */
    jsl_frame(0x828699u); abi_TaskWait();  /* 828699 jsl */
    op_rep(0x20);  /* 82869D rep */
    sec();  /* 82869F sec */
    lda16_v(0x0200);  /* 8286A0 lda */
    sbc16_v(rd16(ea_abs(0x1777)));  /* 8286A3 sbc */
    if (!IF_C) goto L_8286C5;  /* 8286A6 bcc */
    cmp16_v(0x00F0);  /* 8286A8 cmp */
    if (!IF_C) goto L_8286B0;  /* 8286AB bcc */
    lda16_v(0x00F0);  /* 8286AD lda */
L_8286B0:
    ACC16(lsr);  /* 8286B0 lsr */
    if (IF_Z) goto L_8286C5;  /* 8286B1 beq */
    op_sep(0x20);  /* 8286B3 sep */
    sta8(BAND_HDMA);  /* 8286B5 sta */
    sta8(0x7EF403u);  /* 8286B9 sta */
    lda8_v(0x10);  /* 8286BD lda */
    sta8(0x7EF500u);  /* 8286BF sta */
    goto L_828697;  /* 8286C3 bra */
L_8286C5:
    op_sep(0x20);  /* 8286C5 sep */
    lda8_v(0x00);  /* 8286C7 lda */
    sta8(0x7EF500u);  /* 8286C9 sta */
    goto L_828697;  /* 8286CD bra */
}

/*
 * @routine 82:86CF HdmaEffect12
 * WHAT IT DOES: Window effect 12: HDMA table set $14 on register set 0
 * from the base, with the layers' window masks and the windows set, then
 * the task ends.
 * HOW: HdmaStartAtBase; the table's first values from TM's shadow; the
 * channel on; TMW and TSW $10, WH0 $38, WH1 $C7, WH2 and WH3 0, WOBJSEL
 * 3; TaskExit.
 * Entry: M 8-bit, X 8-bit; DB = $82; called by JSR from WindowEffectTask.
 * Exit: no channel: C set; else never returns.
 */
void abi_HdmaEffect12(void)
{
    ldx8_v(0x14);  /* 8286CF ldx */
    ldy8_v(0x00);  /* 8286D1 ldy */
    call_routine(0x828735u, 0);  /* 8286D3 jsr */
    if (!IF_C) goto L_8286D9;  /* 8286D6 bcc */
    return;  /* 8286D8 rts */
L_8286D9:
    lda8(ea_abs(0x0399));  /* 8286D9 lda */
    sta8(0x7EF480u);  /* 8286DC sta */
    and8_v(0x0F);  /* 8286E0 and */
    sta8(0x7EF481u);  /* 8286E2 sta */
    lda8(ea_abs(0x002C));  /* 8286E6 lda */
    tsb8(ea_abs(HDMAEN_SHADOW));  /* 8286E9 tsb */
    lda8_v(0x10);  /* 8286EC lda */
    sta8(ea_abs(TMW_SHADOW));  /* 8286EE sta */
    sta8(ea_abs(TSW_SHADOW));  /* 8286F1 sta */
    lda8_v(0x38);  /* 8286F4 lda */
    sta8(ea_abs(0x03A6));  /* 8286F6 sta */
    lda8_v(0xC7);  /* 8286F9 lda */
    sta8(ea_abs(0x03A7));  /* 8286FB sta */
    stz8(ea_abs(0x03AA));  /* 8286FE stz */
    stz8(ea_abs(0x03AB));  /* 828701 stz */
    lda8_v(0x03);  /* 828704 lda */
    sta8(ea_abs(WOBJSEL_SHADOW));  /* 828706 sta */
    abi_TaskExit(); return;  /* 828709 jml */
}
