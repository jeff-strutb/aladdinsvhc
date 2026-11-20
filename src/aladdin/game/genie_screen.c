/*
 * genie_screen.c: the Genie screen (level $2F).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
#include "wram.h"
#include "wram_state.h"
#include "wram_video.h"

void abi_CircleWipeStep(void);
void abi_DisplayDefaults(void);
void abi_FrameStepLong(void);
void abi_HdmaTableFill(void);
void abi_LayersFillLong(void);
void abi_LevelMapBuildLong(void);
void abi_LevelRamSetupLong(void);
void abi_Mode7Rotate(void);
void abi_Mode7ScreenSetup(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_RandomNext(void);
void abi_ScreenBlank(void);
void abi_ScrollStateClear(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_SoundQueueF6With1(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TaskSlotsFree1To6(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);
void abi_UnpackSet(void);
void abi_VramCopy(void);
void abi_GenieScreen(void);
void abi_GenieHealthFill(void);
void abi_GenieGemsAdd(void);
void abi_GenieScreenEnd(void);
void abi_GenieFrame(void);
void abi_GeniePrizeSignal80(void);
void abi_GeniePrizeSignal2(void);
void abi_GenieSlotStep(void);
void abi_GenieSlotEnd(void);
void abi_GenieZoomStep(void);
void abi_GenieCountdownStep(void);
void abi_GenieZoomReset(void);
void abi_GeniePointerCheck(void);

/*
 * @routine 80:A63B GenieScreen
 * WHAT IT DOES: The Genie screen (level $2F), the bonus after a stage where
 * a figure of kind $27 was touched: the screen is built, the Genie's lamp
 * zooms in, the wheel spins until a button stops it, slows and stops on a
 * slot, and the slot's prize is given (health, a continue, or gems); then
 * GenieScreenEnd.
 * HOW: DB $80. TaskSlotsFree1To6; $033F, PLAY_MODE, $AE, $B5, $AF, $AC 0;
 * screen off and cleared (NmiDisable, ScreenBlank, ScrollStateClear,
 * OamBufferClear, DisplayDefaults, TextLayerClear); LEVEL $2F,
 * LevelRamSetupLong; the view 0, the screen's y ($00B0) $FFB0;
 * LevelMapBuildLong, LayersFillLong; UnpackSet $17 and $2E with VramCopy
 * X = $3F, $46, $4D, $54 and $23; PaletteLoad $5C, $39; NmiEnable; $0399,
 * $039A 0, $03A4 5; a frame (TaskWait 1); Mode7ScreenSetup $10;
 * CircleWipeStep $50 at $80, $80; $200 bytes $7E:5000 to $7F:F500 (MVN);
 * HdmaTableFill $FF00, $0100, $28; TaskCreate (slot $18, parameter 4,
 * $81:9E8E); $03B8 0; $0395 $80, $0397 $D0; SongPlay $29.
 * The lamp ($0399/$039A 1): GenieFrame, GenieZoomReset; $1A1D 1, $1A1C
 * $80, the speed $A5-$A7 $000100, its step $A8 $40, $A9 0; each frame
 * (GenieFrame) GenieCountdownStep until $1A1D is
 * negative (sound $3A, the count cleared), with the zoom stepped
 * (GenieZoomStep) by $AA in x or y ($0394 picks); $A9 counts the passes, to
 * 2; then two more (to 4) moving $038D down and $0393 up. Then $0399,
 * $039A $11, $0397 $80, TaskCreate twice (slots $60 and $48, $82:8000).
 * The wheel: $038B, $038C 1, $03A6 0, $03A7 $FF, $03AA $33, $03AB, $03AC
 * 0; a frame; $75 and $A5-$AA 0; $AE up; each frame until a button
 * ($037B & $C0 | $037C, bits $D0); then each frame the speed $A5 up $30
 * to $088A (FrameStepLong, Mode7Rotate by $A8, GenieSlotStep) until a
 * button with the speed at $088A; then each frame the speed down a step
 * every 4th frame ($032D) below $0130, with GeniePointerCheck,
 * GenieSlotStep, its sound $3B on a slot and Random choices, until the
 * speed runs out. $20 frames; $AC $FE; the slot from $A8 against the
 * seven bounds at $DA96; by it (the table after the JMP): a lamp ($B4
 * $2C, GeniePrizeSignal2; a health box more, to $0A, and GenieHealthFill,
 * else 3 gems), a continue ($B4 $2C, GeniePrizeSignal80, $035B up, $30
 * frames), 1 gem ($B4 $33), health filled or 1 gem ($B4 $20), 2 gems
 * ($B4 $33); then GenieScreenEnd (the health fill: from its sound).
 * Entry: M 8-bit, X 8-bit; a task's entry, by TaskExec from
 * PlayerSceneFigure (81:CC4A). Exit: never returns (TaskExec).
 */
void abi_GenieScreen(void)
{
    lda8_v(0x80);  /* 80A63B lda */
    pha8();  /* 80A63D pha */
    plb();  /* 80A63E plb */
    jsl_frame(0x80A63Fu); abi_TaskSlotsFree1To6(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A63F jsl */
    stz8(ea_abs(V_033F));  /* 80A643 stz */
    stz8(ea_abs(PLAY_MODE));  /* 80A646 stz */
    stz8(ea_abs(0x00AE));  /* 80A649 stz */
    stz8(ea_abs(0x00B5));  /* 80A64C stz */
    stz8(ea_abs(0x00AF));  /* 80A64F stz */
    stz8(ea_abs(0x00AC));  /* 80A652 stz */
    op_sep(0x20);  /* 80A655 sep */
    jsl_frame(0x80A657u); abi_NmiDisable(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A657 jsl */
    jsl_frame(0x80A65Bu); abi_ScreenBlank(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A65B jsl */
    jsl_frame(0x80A65Fu); abi_ScrollStateClear(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A65F jsl */
    jsl_frame(0x80A663u); abi_OamBufferClear(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A663 jsl */
    jsl_frame(0x80A667u); abi_DisplayDefaults(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A667 jsl */
    jsl_frame(0x80A66Bu); abi_TextLayerClear(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A66B jsl */
    lda8_v(0x2F);  /* 80A66F lda */
    sta8(ea_abs(LEVEL));  /* 80A671 sta */
    jsl_frame(0x80A674u); abi_LevelRamSetupLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A674 jsl */
    op_rep(0x20);  /* 80A678 rep */
    stz16(ea_abs(0x1773));  /* 80A67A stz */
    stz16(ea_abs(0x1777));  /* 80A67D stz */
    lda16_v(0xFFB0);  /* 80A680 lda */
    sta16(ea_abs(0x00B0));  /* 80A683 sta */
    op_sep(0x20);  /* 80A686 sep */
    jsl_frame(0x80A688u); abi_LevelMapBuildLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A688 jsl */
    jsl_frame(0x80A68Cu); abi_LayersFillLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A68C jsl */
    lda8_v(0x17);  /* 80A690 lda */
    jsl_frame(0x80A692u); abi_UnpackSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A692 jsl */
    ldx8_v(0x3F);  /* 80A696 ldx */
    jsl_frame(0x80A698u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A698 jsl */
    ldx8_v(0x46);  /* 80A69C ldx */
    jsl_frame(0x80A69Eu); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A69E jsl */
    ldx8_v(0x4D);  /* 80A6A2 ldx */
    jsl_frame(0x80A6A4u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6A4 jsl */
    ldx8_v(0x54);  /* 80A6A8 ldx */
    jsl_frame(0x80A6AAu); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6AA jsl */
    lda8_v(0x2E);  /* 80A6AE lda */
    jsl_frame(0x80A6B0u); abi_UnpackSet(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6B0 jsl */
    ldx8_v(0x23);  /* 80A6B4 ldx */
    jsl_frame(0x80A6B6u); abi_VramCopy(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6B6 jsl */
    lda8_v(0x5C);  /* 80A6BA lda */
    jsl_frame(0x80A6BCu); abi_PaletteLoad(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6BC jsl */
    lda8_v(0x39);  /* 80A6C0 lda */
    jsl_frame(0x80A6C2u); abi_PaletteLoad(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6C2 jsl */
    jsl_frame(0x80A6C6u); abi_NmiEnable(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6C6 jsl */
    stz8(ea_abs(0x0399));  /* 80A6CA stz */
    stz8(ea_abs(0x039A));  /* 80A6CD stz */
    lda8_v(0x05);  /* 80A6D0 lda */
    sta8(ea_abs(0x03A4));  /* 80A6D2 sta */
    lda8_v(0x01);  /* 80A6D5 lda */
    jsl_frame(0x80A6D7u); abi_TaskWait();  /* 80A6D7 jsl */
    lda8_v(0x10);  /* 80A6DB lda */
    jsl_frame(0x80A6DDu); abi_Mode7ScreenSetup(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6DD jsl */
    op_rep(0x30);  /* 80A6E1 rep */
    lda16_v(0x0050);  /* 80A6E3 lda */
    ldx16_v(0x0080);  /* 80A6E6 ldx */
    ldy16_v(0x0080);  /* 80A6E9 ldy */
    jsl_frame(0x80A6ECu); abi_CircleWipeStep(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A6EC jsl */
    phb();  /* 80A6F0 phb */
    ldx16_v(0x5000);  /* 80A6F1 ldx */
    ldy16_v(0xF500);  /* 80A6F4 ldy */
    lda16_v(0x01FF);  /* 80A6F7 lda */
    mvn(0x7E, 0x7F);  /* 80A6FA mvn */
    plb();  /* 80A6FD plb */
    lda16_v(0xFF00);  /* 80A6FE lda */
    ldx16_v(0x0100);  /* 80A701 ldx */
    ldy16_v(0x0028);  /* 80A704 ldy */
    jsl_frame(0x80A707u); abi_HdmaTableFill(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A707 jsl */
    op_sep(0x30);  /* 80A70B sep */
    jsl_frame(0x80A70Du); abi_TaskCreate(); cpu.unwound = 0;  /* 80A70D jsl */
    stz8(ea_abs(INIDISP_SHADOW));  /* 80A716 stz */
    op_rep(0x20);  /* 80A719 rep */
    lda16_v(0x0080);  /* 80A71B lda */
    sta16(ea_abs(0x0395));  /* 80A71E sta */
    lda16_v(0x00D0);  /* 80A721 lda */
    sta16(ea_abs(0x0397));  /* 80A724 sta */
    op_sep(0x20);  /* 80A727 sep */
    lda8_v(0x29);  /* 80A729 lda */
    jsl_frame(0x80A72Bu); abi_SongPlay(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A72B jsl */
    lda8_v(0x01);  /* 80A72F lda */
    sta8(ea_abs(0x0399));  /* 80A731 sta */
    sta8(ea_abs(0x039A));  /* 80A734 sta */
    call_frame(0xA739, abi_GenieFrame);  /* 80A737 jsr */
    call_frame(0xA73C, abi_GenieZoomReset);  /* 80A73A jsr */
    lda8_v(0x01);  /* 80A73D lda */
    sta8(ea_abs(0x1A1D));  /* 80A73F sta */
    lda8_v(0x80);  /* 80A742 lda */
    sta8(ea_abs(LAYER_OFFSET_Y));  /* 80A744 sta */
    stz8(ea_abs(0x00A5));  /* 80A747 stz */
    lda8_v(0x01);  /* 80A74A lda */
    sta8(ea_abs(0x00A6));  /* 80A74C sta */
    stz8(ea_abs(0x00A7));  /* 80A74F stz */
    lda8_v(0x40);  /* 80A752 lda */
    sta8(ea_abs(0x00A8));  /* 80A754 sta */
    stz8(ea_abs(0x00A9));  /* 80A757 stz */
L_80A75A:
    call_frame(0xA75C, abi_GenieFrame);  /* 80A75A jsr */
    call_frame(0xA75F, abi_GenieCountdownStep);  /* 80A75D jsr */
    lda8(ea_abs(0x1A1D));  /* 80A760 lda */
    if (!IF_N) goto L_80A75A;  /* 80A763 bpl */
    stz8(ea_abs(0x1A1B));  /* 80A765 stz */
    stz8(ea_abs(LAYER_OFFSET_Y));  /* 80A768 stz */
    stz8(ea_abs(0x1A1D));  /* 80A76B stz */
    lda8_v(0x3A);  /* 80A76E lda */
    jsl_frame(0x80A770u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A770 jsl */
L_80A774:
    ldx8(ea_abs(0x00A9));  /* 80A774 ldx */
    lda8(ea_absx(0xDA73));  /* 80A777 lda */
    sta8(ea_abs(0x00AA));  /* 80A77A sta */
    lda8(ea_absx(0xDA79));  /* 80A77D lda */
    sta8(ea_abs(0x00AB));  /* 80A780 sta */
L_80A783:
    call_frame(0xA785, abi_GenieFrame);  /* 80A783 jsr */
    lda8(ea_abs(0x00AB));  /* 80A786 lda */
    if (IF_Z) goto L_80A794;  /* 80A789 beq */
    ACC8(dec);  /* 80A78B dec */
    if (!IF_Z) goto L_80A791;  /* 80A78C bne */
    call_frame(0xA790, abi_GenieZoomReset);  /* 80A78E jsr */
L_80A791:
    MEM8(dec, ea_abs(0x00AB));  /* 80A791 dec */
L_80A794:
    lda8(ea_abs(0x00AA));  /* 80A794 lda */
    tay8();  /* 80A797 tay */
    ACC8(lsr);  /* 80A798 lsr */
    ACC8(lsr);  /* 80A799 lsr */
    sty8(ea_abs(SCRATCH));  /* 80A79A sty */
    sta8(ea_abs(0x000E));  /* 80A79D sta */
    ldx8(ea_abs(0x0394));  /* 80A7A0 ldx */
    if (!IF_Z) goto L_80A7AB;  /* 80A7A3 bne */
    sty8(ea_abs(0x000E));  /* 80A7A5 sty */
    sta8(ea_abs(SCRATCH));  /* 80A7A8 sta */
L_80A7AB:
    stz8(ea_abs(0x000D));  /* 80A7AB stz */
    stz8(ea_abs(0x000F));  /* 80A7AE stz */
    ldx8(ea_abs(0x00A9));  /* 80A7B1 ldx */
    lda8(ea_absx(0xDA7B));  /* 80A7B4 lda */
    sta8(ea_abs(0x0010));  /* 80A7B7 sta */
    call_frame(0xA7BC, abi_GenieZoomStep);  /* 80A7BA jsr */
    if (!IF_N) goto L_80A783;  /* 80A7BD bpl */
    stz8(ea_abs(0x00AA));  /* 80A7BF stz */
    lda8(ea_absx(0xDA7F));  /* 80A7C2 lda */
    sta8(ea_abs(0x00A5));  /* 80A7C5 sta */
    lda8(ea_absx(0xDA83));  /* 80A7C8 lda */
    sta8(ea_abs(0x00A6));  /* 80A7CB sta */
    lda8(ea_absx(0xDA87));  /* 80A7CE lda */
    sta8(ea_abs(0x00A7));  /* 80A7D1 sta */
    lda8(ea_absx(0xDA8B));  /* 80A7D4 lda */
    sta8(ea_abs(0x00A8));  /* 80A7D7 sta */
L_80A7DA:
    call_frame(0xA7DC, abi_GenieFrame);  /* 80A7DA jsr */
    lda8(ea_abs(0x00AA));  /* 80A7DD lda */
    tay8();  /* 80A7E0 tay */
    ACC8(asl);  /* 80A7E1 asl */
    ACC8(asl);  /* 80A7E2 asl */
    sty8(ea_abs(SCRATCH));  /* 80A7E3 sty */
    sta8(ea_abs(0x000E));  /* 80A7E6 sta */
    ldx8(ea_abs(0x0394));  /* 80A7E9 ldx */
    if (!IF_Z) goto L_80A813;  /* 80A7EC bne */
    sty8(ea_abs(0x000E));  /* 80A7EE sty */
    sta8(ea_abs(SCRATCH));  /* 80A7F1 sta */
    stz8(ea_abs(0x000D));  /* 80A7F4 stz */
    stz8(ea_abs(0x000F));  /* 80A7F7 stz */
    call_frame(0xA7FC, abi_GenieCountdownStep);  /* 80A7FA jsr */
    lda8(ea_abs(0x1A1D));  /* 80A7FD lda */
    if (!IF_N) goto L_80A813;  /* 80A800 bpl */
    lda8_v(0x3A);  /* 80A802 lda */
    jsl_frame(0x80A804u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A804 jsl */
    stz8(ea_abs(0x1A1B));  /* 80A808 stz */
    stz8(ea_abs(LAYER_OFFSET_Y));  /* 80A80B stz */
    stz8(ea_abs(0x1A1D));  /* 80A80E stz */
    goto L_80A844;  /* 80A811 bra */
L_80A813:
    ldx8(ea_abs(0x00A9));  /* 80A813 ldx */
    lda8(ea_abs(0x00AA));  /* 80A816 lda */
    cmp8_v(rd8(ea_absx(0xDA77)));  /* 80A819 cmp */
    if (IF_C) goto L_80A7DA;  /* 80A81C bcs */
    op_rep(0x21);  /* 80A81E rep */
    lda16(ea_abs(0x038D));  /* 80A820 lda */
    adc16_v(rd16(ea_abs(SCRATCH)));  /* 80A823 adc */
    sta16(ea_abs(0x038D));  /* 80A826 sta */
    lda16(ea_abs(0x0393));  /* 80A829 lda */
    sec();  /* 80A82C sec */
    sbc16_v(rd16(ea_abs(0x000E)));  /* 80A82D sbc */
    sta16(ea_abs(0x0393));  /* 80A830 sta */
    op_sep(0x20);  /* 80A833 sep */
    ldx8(ea_abs(0x00A9));  /* 80A835 ldx */
    lda8(ea_abs(0x00AA));  /* 80A838 lda */
    clc();  /* 80A83B clc */
    adc8_v(rd8(ea_absx(0xDA7D)));  /* 80A83C adc */
    sta8(ea_abs(0x00AA));  /* 80A83F sta */
    goto L_80A7DA;  /* 80A842 bra */
L_80A844:
    MEM8(inc, ea_abs(0x00A9));  /* 80A844 inc */
    lda8(ea_abs(0x00A9));  /* 80A847 lda */
    cmp8_v(0x02);  /* 80A84A cmp */
    if (IF_Z) goto L_80A851;  /* 80A84C beq */
    goto L_80A774;  /* 80A84E jmp */
L_80A851:
    ldx8(ea_abs(0x00A9));  /* 80A851 ldx */
    lda8(ea_absx(0xDA7F));  /* 80A854 lda */
    sta8(ea_abs(0x00A5));  /* 80A857 sta */
    lda8(ea_absx(0xDA83));  /* 80A85A lda */
    sta8(ea_abs(0x00A6));  /* 80A85D sta */
    lda8(ea_absx(0xDA87));  /* 80A860 lda */
    sta8(ea_abs(0x00A7));  /* 80A863 sta */
    lda8(ea_absx(0xDA8B));  /* 80A866 lda */
    sta8(ea_abs(0x00A8));  /* 80A869 sta */
L_80A86C:
    call_frame(0xA86E, abi_GenieFrame);  /* 80A86C jsr */
    lda8(ea_abs(0x038D));  /* 80A86F lda */
    if (IF_Z) goto L_80A877;  /* 80A872 beq */
    MEM8(dec, ea_abs(0x038D));  /* 80A874 dec */
L_80A877:
    lda8(ea_abs(0x0393));  /* 80A877 lda */
    if (IF_Z) goto L_80A88D;  /* 80A87A beq */
    MEM8(inc, ea_abs(0x0393));  /* 80A87C inc */
    MEM8(inc, ea_abs(0x0393));  /* 80A87F inc */
    lda8(ea_abs(0x0393));  /* 80A882 lda */
    if (IF_N) goto L_80A88D;  /* 80A885 bmi */
    stz8(ea_abs(0x0393));  /* 80A887 stz */
    MEM8(inc, ea_abs(0x0394));  /* 80A88A inc */
L_80A88D:
    call_frame(0xA88F, abi_GenieCountdownStep);  /* 80A88D jsr */
    lda8(ea_abs(0x1A1D));  /* 80A890 lda */
    if (!IF_N) goto L_80A86C;  /* 80A893 bpl */
    lda8_v(0x3A);  /* 80A895 lda */
    jsl_frame(0x80A897u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A897 jsl */
    stz8(ea_abs(0x1A1B));  /* 80A89B stz */
    stz8(ea_abs(LAYER_OFFSET_Y));  /* 80A89E stz */
    stz8(ea_abs(0x1A1D));  /* 80A8A1 stz */
    MEM8(inc, ea_abs(0x00A9));  /* 80A8A4 inc */
    lda8(ea_abs(0x00A9));  /* 80A8A7 lda */
    cmp8_v(0x04);  /* 80A8AA cmp */
    if (!IF_Z) goto L_80A851;  /* 80A8AC bne */
    lda8_v(0x11);  /* 80A8AE lda */
    sta8(ea_abs(0x0399));  /* 80A8B0 sta */
    sta8(ea_abs(0x039A));  /* 80A8B3 sta */
    lda8_v(0x80);  /* 80A8B6 lda */
    sta8(ea_abs(0x0397));  /* 80A8B8 sta */
    jsl_frame(0x80A8BBu); abi_TaskCreate(); cpu.unwound = 0;  /* 80A8BB jsl */
    jsl_frame(0x80A8C4u); abi_TaskCreate(); cpu.unwound = 0;  /* 80A8C4 jsl */
    lda8_v(0x01);  /* 80A8CD lda */
    sta8(ea_abs(TMW_SHADOW));  /* 80A8CF sta */
    sta8(ea_abs(TSW_SHADOW));  /* 80A8D2 sta */
    stz8(ea_abs(0x03A6));  /* 80A8D5 stz */
    lda8_v(0xFF);  /* 80A8D8 lda */
    sta8(ea_abs(0x03A7));  /* 80A8DA sta */
    lda8_v(0x33);  /* 80A8DD lda */
    sta8(ea_abs(0x03AA));  /* 80A8DF sta */
    stz8(ea_abs(0x03AB));  /* 80A8E2 stz */
    stz8(ea_abs(WOBJSEL_SHADOW));  /* 80A8E5 stz */
    lda8_v(0x01);  /* 80A8E8 lda */
    jsl_frame(0x80A8EAu); abi_TaskWait();  /* 80A8EA jsl */
    stz8(ea_abs(TASK_PARAM));  /* 80A8EE stz */
    stz8(ea_abs(0x00A7));  /* 80A8F1 stz */
    stz8(ea_abs(0x00A8));  /* 80A8F4 stz */
    stz8(ea_abs(0x00A5));  /* 80A8F7 stz */
    stz8(ea_abs(0x00A6));  /* 80A8FA stz */
    stz8(ea_abs(0x00A9));  /* 80A8FD stz */
    stz8(ea_abs(0x00AA));  /* 80A900 stz */
    MEM8(inc, ea_abs(0x00AE));  /* 80A903 inc */
L_80A906:
    call_frame(0xA908, abi_GenieFrame);  /* 80A906 jsr */
    lda8(ea_abs(JOY_PRESSED));  /* 80A909 lda */
    and8_v(0xC0);  /* 80A90C and */
    ora8_v(rd8(ea_abs(0x037C)));  /* 80A90E ora */
    bit8_i(0xD0);  /* 80A911 bit */
    if (IF_Z) goto L_80A906;  /* 80A913 beq */
    stz8(ea_abs(0x00AE));  /* 80A915 stz */
L_80A918:
    lda8_v(0x01);  /* 80A918 lda */
    jsl_frame(0x80A91Au); abi_TaskWait();  /* 80A91A jsl */
    op_rep(0x20);  /* 80A91E rep */
    lda16(ea_abs(0x00A5));  /* 80A920 lda */
    cmp16_v(0x088A);  /* 80A923 cmp */
    op_sep(0x20);  /* 80A926 sep */
    if (!IF_C) goto L_80A93B;  /* 80A928 bcc */
    lda8_v(0x01);  /* 80A92A lda */
    sta8(ea_abs(0x00AE));  /* 80A92C sta */
    lda8(ea_abs(JOY_PRESSED));  /* 80A92F lda */
    and8_v(0xC0);  /* 80A932 and */
    ora8_v(rd8(ea_abs(0x037C)));  /* 80A934 ora */
    bit8_i(0xD0);  /* 80A937 bit */
    if (!IF_Z) goto L_80A971;  /* 80A939 bne */
L_80A93B:
    jsl_frame(0x80A93Bu); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A93B jsl */
    op_rep(0x21);  /* 80A93F rep */
    lda16(ea_abs(0x00A5));  /* 80A941 lda */
    cmp16_v(0x088A);  /* 80A944 cmp */
    if (!IF_C) goto L_80A951;  /* 80A947 bcc */
    lda16_v(0x088A);  /* 80A949 lda */
    sta16(ea_abs(0x00A5));  /* 80A94C sta */
    goto L_80A95A;  /* 80A94F bra */
L_80A951:
    lda16(ea_abs(0x00A5));  /* 80A951 lda */
    adc16_v(0x0030);  /* 80A954 adc */
    sta16(ea_abs(0x00A5));  /* 80A957 sta */
L_80A95A:
    lda16(ea_abs(0x00A5));  /* 80A95A lda */
    adc16_v(rd16(ea_abs(0x00A7)));  /* 80A95D adc */
    sta16(ea_abs(0x00A7));  /* 80A960 sta */
    op_sep(0x20);  /* 80A963 sep */
    lda8(ea_abs(0x00A8));  /* 80A965 lda */
    jsl_frame(0x80A968u); abi_Mode7Rotate(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A968 jsl */
    call_frame(0xA96E, abi_GenieSlotStep);  /* 80A96C jsr */
    goto L_80A918;  /* 80A96F bra */
L_80A971:
    lda8_v(0xFF);  /* 80A971 lda */
    sta8(ea_abs(0x00AC));  /* 80A973 sta */
L_80A976:
    stz8(ea_abs(0x00AE));  /* 80A976 stz */
    call_frame(0xA97B, abi_GenieFrame);  /* 80A979 jsr */
    lda8(0x00032Du);  /* 80A97C lda */
    and8_v(0x03);  /* 80A980 and */
    if (!IF_Z) goto L_80A998;  /* 80A982 bne */
    op_rep(0x20);  /* 80A984 rep */
    lda16(ea_abs(0x00A5));  /* 80A986 lda */
    cmp16_v(0x0130);  /* 80A989 cmp */
    if (IF_C) goto L_80A996;  /* 80A98C bcs */
    MEM16(dec, ea_abs(0x00A5));  /* 80A98E dec */
    if (!IF_N) goto L_80A996;  /* 80A991 bpl */
    goto L_80AA2E;  /* 80A993 jmp */
L_80A996:
    op_sep(0x20);  /* 80A996 sep */
L_80A998:
    stz8(ea_abs(0x00AD));  /* 80A998 stz */
    call_frame(0xA99D, abi_GeniePointerCheck);  /* 80A99B jsr */
    call_frame(0xA9A0, abi_GenieSlotStep);  /* 80A99E jsr */
    if (!IF_C) goto L_80A9D8;  /* 80A9A1 bcc */
    op_rep(0x20);  /* 80A9A3 rep */
    lda16(ea_abs(0x00A5));  /* 80A9A5 lda */
    cmp16_v(0x0110);  /* 80A9A8 cmp */
    if (IF_C) goto L_80A9C6;  /* 80A9AB bcs */
    lda16(ea_abs(0x00A5));  /* 80A9AD lda */
    cmp16_v(0x0070);  /* 80A9B0 cmp */
    if (!IF_C) goto L_80A9FD;  /* 80A9B3 bcc */
    cmp16_v(0x0080);  /* 80A9B5 cmp */
    if (!IF_C) goto L_80A9EE;  /* 80A9B8 bcc */
    lda16(ea_abs(0x00A5));  /* 80A9BA lda */
    sec();  /* 80A9BD sec */
    sbc16_v(0x0006);  /* 80A9BE sbc */
    sta16(ea_abs(0x00A5));  /* 80A9C1 sta */
    goto L_80A9CC;  /* 80A9C4 bra */
L_80A9C6:
    sbc16_v(0x002E);  /* 80A9C6 sbc */
    sta16(ea_abs(0x00A5));  /* 80A9C9 sta */
L_80A9CC:
    if (IF_N) goto L_80AA2E;  /* 80A9CC bmi */
L_80A9CE:
    op_sep(0x20);  /* 80A9CE sep */
    lda8_v(0x3B);  /* 80A9D0 lda */
    jsl_frame(0x80A9D2u); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A9D2 jsl */
    op_rep(0x20);  /* 80A9D6 rep */
L_80A9D8:
    op_rep(0x21);  /* 80A9D8 rep */
    lda16(ea_abs(0x00A5));  /* 80A9DA lda */
    adc16_v(rd16(ea_abs(0x00A7)));  /* 80A9DD adc */
    sta16(ea_abs(0x00A7));  /* 80A9E0 sta */
    op_sep(0x20);  /* 80A9E3 sep */
    lda8(ea_abs(0x00A8));  /* 80A9E5 lda */
    jsl_frame(0x80A9E8u); abi_Mode7Rotate(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A9E8 jsl */
    goto L_80A976;  /* 80A9EC bra */
L_80A9EE:
    op_sep(0x20);  /* 80A9EE sep */
    lda8(ea_abs(0x00AA));  /* 80A9F0 lda */
    if (!IF_Z) goto L_80A9FD;  /* 80A9F3 bne */
    jsl_frame(0x80A9F5u); abi_RandomNext(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80A9F5 jsl */
    and8_v(0x03);  /* 80A9F9 and */
    if (!IF_Z) goto L_80A9CE;  /* 80A9FB bne */
L_80A9FD:
    op_sep(0x20);  /* 80A9FD sep */
    lda8_v(0x01);  /* 80A9FF lda */
    sta8(ea_abs(0x00AD));  /* 80AA01 sta */
    sta8(ea_abs(0x00AF));  /* 80AA04 sta */
L_80AA07:
    call_frame(0xAA09, abi_GenieFrame);  /* 80AA07 jsr */
    op_rep(0x20);  /* 80AA0A rep */
    MEM16(dec, ea_abs(0x00A5));  /* 80AA0C dec */
    if (IF_N) goto L_80AA2E;  /* 80AA0F bmi */
    lda16(ea_abs(0x00A7));  /* 80AA11 lda */
    sec();  /* 80AA14 sec */
    sbc16_v(rd16(ea_abs(0x00A5)));  /* 80AA15 sbc */
    sta16(ea_abs(0x00A7));  /* 80AA18 sta */
    op_sep(0x20);  /* 80AA1B sep */
    call_frame(0xAA1F, abi_GeniePointerCheck);  /* 80AA1D jsr */
    call_frame(0xAA22, abi_GenieSlotEnd);  /* 80AA20 jsr */
    if (!IF_C) goto L_80AA3F;  /* 80AA23 bcc */
    lda8(ea_abs(0x00A8));  /* 80AA25 lda */
    jsl_frame(0x80AA28u); abi_Mode7Rotate(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AA28 jsl */
    goto L_80AA07;  /* 80AA2C bra */
L_80AA2E:
    op_sep(0x20);  /* 80AA2E sep */
    lda8_v(0x20);  /* 80AA30 lda */
    sta8(ea_abs(0x00B2));  /* 80AA32 sta */
L_80AA35:
    call_frame(0xAA37, abi_GenieFrame);  /* 80AA35 jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80AA38 dec */
    if (!IF_Z) goto L_80AA35;  /* 80AA3B bne */
    goto L_80AA4E;  /* 80AA3D bra */
L_80AA3F:
    lda8_v(0x01);  /* 80AA3F lda */
    sta8(ea_abs(0x00AA));  /* 80AA41 sta */
    MEM8(dec, ea_abs(0x00A9));  /* 80AA44 dec */
    if (!IF_N) goto L_80A9D8;  /* 80AA47 bpl */
    stz8(ea_abs(0x00A9));  /* 80AA49 stz */
    goto L_80A9D8;  /* 80AA4C bra */
L_80AA4E:
    lda8_v(0xFE);  /* 80AA4E lda */
    sta8(ea_abs(0x00AC));  /* 80AA50 sta */
    lda8(ea_abs(0x00A8));  /* 80AA53 lda */
    ldy8_v(0x00);  /* 80AA56 ldy */
L_80AA58:
    cmp8_v(rd8(ea_absy(0xDA96)));  /* 80AA58 cmp */
    if (!IF_C) goto L_80AA64;  /* 80AA5B bcc */
    iny8();  /* 80AA5D iny */
    cpy8_v(0x07);  /* 80AA5E cpy */
    if (!IF_Z) goto L_80AA58;  /* 80AA60 bne */
    ldy8_v(0x00);  /* 80AA62 ldy */
L_80AA64:
    tya8();  /* 80AA64 tya */
    ACC8(asl);  /* 80AA65 asl */
    tax8();  /* 80AA66 tax */
    switch (cpu.x) { case 0: goto L_80AA78; case 2: goto L_80AA98; case 4: goto L_80AABA; case 6: goto L_80AAC9; case 8: goto L_80AA98; case 10: goto L_80AADE; case 12: goto L_80AA98; default: jump_routine(0x800000u | rd16(0x800000u | (uint16_t)(0xAA6A + cpu.x))); return; }  /* 80AA67 jmp */
L_80AA78:
    lda8_v(0x2C);  /* 80AA78 lda */
    sta8(ea_abs(0x00B4));  /* 80AA7A sta */
    call_frame(0xAA7F, abi_GeniePrizeSignal2);  /* 80AA7D jsr */
    lda8(ea_abs(HEALTH_MARKS));  /* 80AA80 lda */
    ACC8(inc);  /* 80AA83 inc */
    cmp8_v(0x0B);  /* 80AA84 cmp */
    if (!IF_C) goto L_80AA8F;  /* 80AA86 bcc */
    lda8_v(0x03);  /* 80AA88 lda */
    call_frame(0xAA8C, abi_GenieGemsAdd);  /* 80AA8A jsr */
    goto L_80AA95;  /* 80AA8D bra */
L_80AA8F:
    sta8(ea_abs(HEALTH_MARKS));  /* 80AA8F sta */
    call_frame(0xAA94, abi_GenieHealthFill);  /* 80AA92 jsr */
L_80AA95:
    abi_GenieScreenEnd(); return;  /* 80AA95 jmp */
L_80AA98:
    lda8_v(0x2C);  /* 80AA98 lda */
    sta8(ea_abs(0x00B4));  /* 80AA9A sta */
    call_frame(0xAA9F, abi_GeniePrizeSignal80);  /* 80AA9D jsr */
    MEM8(inc, ea_abs(CONTINUES));  /* 80AAA0 inc */
    lda8_v(0x30);  /* 80AAA3 lda */
    sta8(ea_abs(0x00B2));  /* 80AAA5 sta */
L_80AAA8:
    call_frame(0xAAAA, abi_GenieFrame);  /* 80AAA8 jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80AAAB dec */
    if (!IF_Z) goto L_80AAA8;  /* 80AAAE bne */
    abi_GenieScreenEnd(); return;  /* 80AAB0 jmp */
L_80AAB3:
    lda8_v(0x01);  /* 80AAB3 lda */
    call_frame(0xAAB7, abi_GenieGemsAdd);  /* 80AAB5 jsr */
    abi_GenieScreenEnd(); return;  /* 80AAB8 bra */
L_80AABA:
    lda8_v(0x33);  /* 80AABA lda */
    sta8(ea_abs(0x00B4));  /* 80AABC sta */
    call_frame(0xAAC1, abi_GeniePrizeSignal2);  /* 80AABF jsr */
    lda8_v(0x01);  /* 80AAC2 lda */
    call_frame(0xAAC6, abi_GenieGemsAdd);  /* 80AAC4 jsr */
    abi_GenieScreenEnd(); return;  /* 80AAC7 bra */
L_80AAC9:
    lda8_v(0x20);  /* 80AAC9 lda */
    sta8(ea_abs(0x00B4));  /* 80AACB sta */
    call_frame(0xAAD0, abi_GeniePrizeSignal2);  /* 80AACE jsr */
    lda8(ea_abs(HEALTH_MARKS));  /* 80AAD1 lda */
    cmp8_v(rd8(ea_abs(HEALTH)));  /* 80AAD4 cmp */
    if (IF_Z) goto L_80AAB3;  /* 80AAD7 beq */
    call_frame(0xAADB, abi_GenieHealthFill);  /* 80AAD9 jsr */
    goto L_80AB4D;  /* 80AADC bra */
L_80AADE:
    lda8_v(0x33);  /* 80AADE lda */
    sta8(ea_abs(0x00B4));  /* 80AAE0 sta */
    call_frame(0xAAE5, abi_GeniePrizeSignal2);  /* 80AAE3 jsr */
    lda8_v(0x02);  /* 80AAE6 lda */
    call_frame(0xAAEA, abi_GenieGemsAdd);  /* 80AAE8 jsr */
    abi_GenieScreenEnd(); return;  /* 80AAEB bra */
L_80AB4D:
    lda8_v(0xF0);  /* 80AB4D lda */
    jsl_frame(0x80AB4Fu); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB4F jsl */
    call_frame(0xAB55, abi_GenieFrame);  /* 80AB53 jsr */
    call_frame(0xAB58, abi_GenieFrame);  /* 80AB56 jsr */
    lda8(ea_abs(0x00B4));  /* 80AB59 lda */
    jsl_frame(0x80AB5Cu); abi_SongPlay(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB5C jsl */
    lda8_v(0xA0);  /* 80AB60 lda */
    sta8(ea_abs(0x00A9));  /* 80AB62 sta */
L_80AB65:
    call_frame(0xAB67, abi_GenieFrame);  /* 80AB65 jsr */
    MEM8(dec, ea_abs(0x00A9));  /* 80AB68 dec */
    if (!IF_Z) goto L_80AB65;  /* 80AB6B bne */
    jsl_frame(0x80AB6Du); abi_TaskCreate(); cpu.unwound = 0;  /* 80AB6D jsl */
    jsl_frame(0x80AB76u); abi_SoundQueueF6With1(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB76 jsl */
L_80AB7A:
    lda8_v(0x01);  /* 80AB7A lda */
    jsl_frame(0x80AB7Cu); abi_TaskWait();  /* 80AB7C jsl */
    lda8(ea_abs(0x0087));  /* 80AB80 lda */
    if (!IF_Z) goto L_80AB7A;  /* 80AB83 bne */
    jsl_frame(0x80AB85u); abi_TaskSlotsFree1To6(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB85 jsl */
    jsl_frame(0x80AB89u); abi_TaskExec(); return;  /* 80AB89 jsl */
}

/*
 * @routine 80:AAED GenieHealthFill
 * WHAT IT DOES: Fills Aladdin's health to its top, a box every 10 frames,
 * then waits 64 frames.
 * HOW: 10 frames (GenieFrame, $B2 counting), then while $0367 is below
 * $0365: $0367 up and again; then 64 frames.
 * Entry: M 8-bit, X 8-bit; called by JSR from GenieScreen. Exit: M 8-bit,
 * X 8-bit; $B2 0.
 */
void abi_GenieHealthFill(void)
{
L_80AAED:
    lda8_v(0x0A);  /* 80AAED lda */
    sta8(ea_abs(0x00B2));  /* 80AAEF sta */
L_80AAF2:
    call_frame(0xAAF4, abi_GenieFrame);  /* 80AAF2 jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80AAF5 dec */
    if (!IF_Z) goto L_80AAF2;  /* 80AAF8 bne */
    lda8(ea_abs(HEALTH));  /* 80AAFA lda */
    cmp8_v(rd8(ea_abs(HEALTH_MARKS)));  /* 80AAFD cmp */
    if (IF_C) goto L_80AB07;  /* 80AB00 bcs */
    MEM8(inc, ea_abs(HEALTH));  /* 80AB02 inc */
    goto L_80AAED;  /* 80AB05 bra */
L_80AB07:
    lda8_v(0x40);  /* 80AB07 lda */
    sta8(ea_abs(0x00B2));  /* 80AB09 sta */
L_80AB0C:
    call_frame(0xAB0E, abi_GenieFrame);  /* 80AB0C jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80AB0F dec */
    if (!IF_Z) goto L_80AB0C;  /* 80AB12 bne */
    return;  /* 80AB14 rts */
}

/*
 * @routine 80:AB15 GenieGemsAdd
 * WHAT IT DOES: Gives A gems, one every 16 frames, up to 100.
 * HOW: $B3 = A; each time 16 frames (GenieFrame), then $0364 at $64: done;
 * else $0364 up, $B3 down, again until 0.
 * Entry: M 8-bit, X 8-bit; A = the gems; called by JSR from GenieScreen.
 * Exit: M 8-bit, X 8-bit.
 */
void abi_GenieGemsAdd(void)
{
    sta8(ea_abs(0x00B3));  /* 80AB15 sta */
L_80AB18:
    lda8_v(0x10);  /* 80AB18 lda */
    sta8(ea_abs(0x00B2));  /* 80AB1A sta */
L_80AB1D:
    call_frame(0xAB1F, abi_GenieFrame);  /* 80AB1D jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80AB20 dec */
    if (!IF_Z) goto L_80AB1D;  /* 80AB23 bne */
    lda8(ea_abs(0x0364));  /* 80AB25 lda */
    cmp8_v(0x64);  /* 80AB28 cmp */
    if (IF_Z) goto L_80AB34;  /* 80AB2A beq */
    MEM8(inc, ea_abs(0x0364));  /* 80AB2C inc */
    MEM8(dec, ea_abs(0x00B3));  /* 80AB2F dec */
    if (!IF_Z) goto L_80AB18;  /* 80AB32 bne */
L_80AB34:
    return;  /* 80AB34 rts */
}

/*
 * @routine 80:AB35 GenieScreenEnd
 * WHAT IT DOES: The Genie screen's end: a pause, the prize's cue, its song,
 * then the next stage is set going.
 * HOW: $58 frames; $B5 1 for a frame, then 0; (from here GenieScreen's
 * health fill joins) SoundQueueAdd $F0; two frames; SongPlay $B4; $A0
 * frames; TaskCreate (slot $18, parameter 4, $81:9EB9); $81:F9D2; each
 * frame (TaskWait 1) until $0087 is 0; TaskSlotsFree1To6; TaskExec
 * $81:8181.
 * Entry: M 8-bit, X 8-bit; reached by a jump from GenieScreen's prizes.
 * Exit: never returns (TaskExec).
 */
void abi_GenieScreenEnd(void)
{
    lda8_v(0x58);  /* 80AB35 lda */
    sta8(ea_abs(0x00B2));  /* 80AB37 sta */
L_80AB3A:
    call_frame(0xAB3C, abi_GenieFrame);  /* 80AB3A jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80AB3D dec */
    if (!IF_Z) goto L_80AB3A;  /* 80AB40 bne */
    lda8_v(0x01);  /* 80AB42 lda */
    sta8(ea_abs(0x00B5));  /* 80AB44 sta */
    call_frame(0xAB49, abi_GenieFrame);  /* 80AB47 jsr */
    stz8(ea_abs(0x00B5));  /* 80AB4A stz */
    lda8_v(0xF0);  /* 80AB4D lda */
    jsl_frame(0x80AB4Fu); abi_SoundQueueAdd(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB4F jsl */
    call_frame(0xAB55, abi_GenieFrame);  /* 80AB53 jsr */
    call_frame(0xAB58, abi_GenieFrame);  /* 80AB56 jsr */
    lda8(ea_abs(0x00B4));  /* 80AB59 lda */
    jsl_frame(0x80AB5Cu); abi_SongPlay(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB5C jsl */
    lda8_v(0xA0);  /* 80AB60 lda */
    sta8(ea_abs(0x00A9));  /* 80AB62 sta */
L_80AB65:
    call_frame(0xAB67, abi_GenieFrame);  /* 80AB65 jsr */
    MEM8(dec, ea_abs(0x00A9));  /* 80AB68 dec */
    if (!IF_Z) goto L_80AB65;  /* 80AB6B bne */
    jsl_frame(0x80AB6Du); abi_TaskCreate(); cpu.unwound = 0;  /* 80AB6D jsl */
    jsl_frame(0x80AB76u); abi_SoundQueueF6With1(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB76 jsl */
L_80AB7A:
    lda8_v(0x01);  /* 80AB7A lda */
    jsl_frame(0x80AB7Cu); abi_TaskWait();  /* 80AB7C jsl */
    lda8(ea_abs(0x0087));  /* 80AB80 lda */
    if (!IF_Z) goto L_80AB7A;  /* 80AB83 bne */
    jsl_frame(0x80AB85u); abi_TaskSlotsFree1To6(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB85 jsl */
    jsl_frame(0x80AB89u); abi_TaskExec(); return;  /* 80AB89 jsl */
}

/*
 * @routine 80:AB90 GenieFrame
 * WHAT IT DOES: One frame of the Genie screen: the task waits a frame, then
 * one frame of play.
 * HOW: TaskWait 1; FrameStepLong.
 * Entry: M 8-bit, X 8-bit; called by JSR. Exit: as FrameStepLong.
 */
void abi_GenieFrame(void)
{
    lda8_v(0x01);  /* 80AB90 lda */
    jsl_frame(0x80AB92u); abi_TaskWait();  /* 80AB92 jsl */
    jsl_frame(0x80AB96u); abi_FrameStepLong(); cpu.s = (uint16_t)(cpu.s + 3);  /* 80AB96 jsl */
    return;  /* 80AB9A rts */
}

/*
 * @routine 80:AB9B GeniePrizeSignal80
 * WHAT IT DOES: The screen's step ($00B5) $80 for a frame (the figures'
 * second cue), then $EA frames.
 * HOW: $B5 $80; GenieFrame; on at GeniePrizeSignal2's end: $B5 0, $EA
 * frames.
 * Entry: M 8-bit, X 8-bit; called by JSR from GenieScreen. Exit: M 8-bit,
 * X 8-bit; $B2 0.
 */
void abi_GeniePrizeSignal80(void)
{
    lda8_v(0x80);  /* 80AB9B lda */
    sta8(ea_abs(0x00B5));  /* 80AB9D sta */
    call_frame(0xABA2, abi_GenieFrame);  /* 80ABA0 jsr */
    goto L_80ABAD;  /* 80ABA3 bra */
L_80ABAD:
    stz8(ea_abs(0x00B5));  /* 80ABAD stz */
    lda8_v(0xEA);  /* 80ABB0 lda */
    sta8(ea_abs(0x00B2));  /* 80ABB2 sta */
L_80ABB5:
    call_frame(0xABB7, abi_GenieFrame);  /* 80ABB5 jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80ABB8 dec */
    if (!IF_Z) goto L_80ABB5;  /* 80ABBB bne */
    return;  /* 80ABBD rts */
}

/*
 * @routine 80:ABA5 GeniePrizeSignal2
 * WHAT IT DOES: The screen's step ($00B5) 2 for a frame (the prize's figure
 * cue), then $EA frames.
 * HOW: $B5 2; GenieFrame; $B5 0; $B2 $EA, GenieFrame each until 0.
 * Entry: M 8-bit, X 8-bit; called by JSR from GenieScreen. Exit: M 8-bit,
 * X 8-bit; $B2 0.
 */
void abi_GeniePrizeSignal2(void)
{
    lda8_v(0x02);  /* 80ABA5 lda */
    sta8(ea_abs(0x00B5));  /* 80ABA7 sta */
    call_frame(0xABAC, abi_GenieFrame);  /* 80ABAA jsr */
    stz8(ea_abs(0x00B5));  /* 80ABAD stz */
    lda8_v(0xEA);  /* 80ABB0 lda */
    sta8(ea_abs(0x00B2));  /* 80ABB2 sta */
L_80ABB5:
    call_frame(0xABB7, abi_GenieFrame);  /* 80ABB5 jsr */
    MEM8(dec, ea_abs(0x00B2));  /* 80ABB8 dec */
    if (!IF_Z) goto L_80ABB5;  /* 80ABBB bne */
    return;  /* 80ABBD rts */
}

/*
 * @routine 80:ABBE GenieSlotStep
 * WHAT IT DOES: Tells whether the wheel's angle ($A8) has passed the next
 * slot's bound: C set, and the slot count ($A9) up (wrapping at 7).
 * HOW: X = $A9; 0: only with $A8 not negative go on; 7: $A9 and X 0; then
 * $A8 against $DA64,X (bank $80): at or past it, $A9 up and C set; else C
 * clear.
 * Entry: M 8-bit, X 8-bit; DB $80; called by JSR from GenieScreen. Exit:
 * C as above; X = the old count (0 after a wrap).
 */
void abi_GenieSlotStep(void)
{
    ldx8(ea_abs(0x00A9));  /* 80ABBE ldx */
    if (IF_Z) goto L_80ABDD;  /* 80ABC1 beq */
    cpx8_v(0x07);  /* 80ABC3 cpx */
    if (!IF_Z) goto L_80ABCE;  /* 80ABC5 bne */
    stz8(ea_abs(0x00A9));  /* 80ABC7 stz */
    ldx8_v(0x00);  /* 80ABCA ldx */
    goto L_80ABDD;  /* 80ABCC bra */
L_80ABCE:
    lda8(ea_abs(0x00A8));  /* 80ABCE lda */
    cmp8_v(rd8(ea_absx(0xDA64)));  /* 80ABD1 cmp */
    if (!IF_C) goto L_80ABDB;  /* 80ABD4 bcc */
    MEM8(inc, ea_abs(0x00A9));  /* 80ABD6 inc */
    sec();  /* 80ABD9 sec */
    return;  /* 80ABDA rts */
L_80ABDB:
    clc();  /* 80ABDB clc */
    return;  /* 80ABDC rts */
L_80ABDD:
    lda8(ea_abs(0x00A8));  /* 80ABDD lda */
    if (IF_N) goto L_80ABDB;  /* 80ABE0 bmi */
    goto L_80ABCE;  /* 80ABE2 bra */
}

/*
 * @routine 80:ABE4 GenieSlotEnd
 * WHAT IT DOES: Compares the wheel's angle ($A8) with the end of the slot
 * the count ($A9) names: C set at or past it.
 * HOW: X = $A9; 1 with $A8 negative: C set; else CMP $DA6B,X.
 * Entry: M 8-bit, X 8-bit; DB $80; called by JSR from GenieScreen. Exit:
 * C and Z from the compare (or C set); X = $A9.
 */
void abi_GenieSlotEnd(void)
{
    ldx8(ea_abs(0x00A9));  /* 80ABE4 ldx */
    cpx8_v(0x01);  /* 80ABE7 cpx */
    if (IF_Z) goto L_80ABF2;  /* 80ABE9 beq */
L_80ABEB:
    lda8(ea_abs(0x00A8));  /* 80ABEB lda */
    cmp8_v(rd8(ea_absx(0xDA6B)));  /* 80ABEE cmp */
    return;  /* 80ABF1 rts */
L_80ABF2:
    lda8(ea_abs(0x00A8));  /* 80ABF2 lda */
    if (IF_N) goto L_80ABEB;  /* 80ABF5 bmi */
    sec();  /* 80ABF7 sec */
    return;  /* 80ABF8 rts */
}

/*
 * @routine 80:ABF9 GenieZoomStep
 * WHAT IT DOES: A step of the lamp's zoom: the mode-7 scale words move by
 * $000C and $000E, and $AA by $0010.
 * HOW: 16-bit, carry clear: $0393 += $000C, $038D -= $000E; 8-bit with C
 * set: $AA -= $0010.
 * Entry: M 8-bit; called by JSR from GenieScreen. Exit: M 8-bit; A = the
 * new $AA; N, Z, C from its subtraction.
 */
void abi_GenieZoomStep(void)
{
    op_rep(0x21);  /* 80ABF9 rep */
    lda16(ea_abs(SCRATCH));  /* 80ABFB lda */
    adc16_v(rd16(ea_abs(0x0393)));  /* 80ABFE adc */
    sta16(ea_abs(0x0393));  /* 80AC01 sta */
    lda16(ea_abs(0x038D));  /* 80AC04 lda */
    sec();  /* 80AC07 sec */
    sbc16_v(rd16(ea_abs(0x000E)));  /* 80AC08 sbc */
    sta16(ea_abs(0x038D));  /* 80AC0B sta */
    op_sep(0x21);  /* 80AC0E sep */
    lda8(ea_abs(0x00AA));  /* 80AC10 lda */
    sbc8_v(rd8(ea_abs(0x0010)));  /* 80AC13 sbc */
    sta8(ea_abs(0x00AA));  /* 80AC16 sta */
    return;  /* 80AC19 rts */
}

/*
 * @routine 80:AC1A GenieCountdownStep
 * WHAT IT DOES: The 24-bit count at $1A1B less the speed ($A5-$A7), and the
 * speed up by $A8.
 * HOW: SEC; byte by byte $1A1B-$1A1D -= $A5-$A7; then $A5-$A7 += $A8
 * (the carry on through).
 * Entry: M 8-bit; called by JSR from GenieScreen. Exit: M 8-bit; A = the
 * new $A7.
 */
void abi_GenieCountdownStep(void)
{
    lda8(ea_abs(0x1A1B));  /* 80AC1A lda */
    sec();  /* 80AC1D sec */
    sbc8_v(rd8(ea_abs(0x00A5)));  /* 80AC1E sbc */
    sta8(ea_abs(0x1A1B));  /* 80AC21 sta */
    lda8(ea_abs(LAYER_OFFSET_Y));  /* 80AC24 lda */
    sbc8_v(rd8(ea_abs(0x00A6)));  /* 80AC27 sbc */
    sta8(ea_abs(LAYER_OFFSET_Y));  /* 80AC2A sta */
    lda8(ea_abs(0x1A1D));  /* 80AC2D lda */
    sbc8_v(rd8(ea_abs(0x00A7)));  /* 80AC30 sbc */
    sta8(ea_abs(0x1A1D));  /* 80AC33 sta */
    lda8(ea_abs(0x00A8));  /* 80AC36 lda */
    clc();  /* 80AC39 clc */
    adc8_v(rd8(ea_abs(0x00A5)));  /* 80AC3A adc */
    sta8(ea_abs(0x00A5));  /* 80AC3D sta */
    lda8_v(0x00);  /* 80AC40 lda */
    adc8_v(rd8(ea_abs(0x00A6)));  /* 80AC42 adc */
    sta8(ea_abs(0x00A6));  /* 80AC45 sta */
    lda8_v(0x00);  /* 80AC48 lda */
    adc8_v(rd8(ea_abs(0x00A7)));  /* 80AC4A adc */
    sta8(ea_abs(0x00A7));  /* 80AC4D sta */
    return;  /* 80AC50 rts */
}

/*
 * @routine 80:AC51 GenieZoomReset
 * WHAT IT DOES: The lamp's mode-7 scale back to 1 and its offsets 0.
 * HOW: 16-bit: $038D, $0393 $0100; $038F, $0391 0.
 * Entry: M 8-bit; called by JSR. Exit: M 8-bit; A = $0100.
 */
void abi_GenieZoomReset(void)
{
    op_rep(0x20);  /* 80AC51 rep */
    lda16_v(0x0100);  /* 80AC53 lda */
    sta16(ea_abs(0x038D));  /* 80AC56 sta */
    sta16(ea_abs(0x0393));  /* 80AC59 sta */
    stz16(ea_abs(0x038F));  /* 80AC5C stz */
    stz16(ea_abs(0x0391));  /* 80AC5F stz */
    op_sep(0x20);  /* 80AC62 sep */
    return;  /* 80AC64 rts */
}

/*
 * @routine 80:AC65 GeniePointerCheck
 * WHAT IT DOES: Tells the pointer whether the wheel's angle ($A8) is inside
 * one of the seven slots' windows: $AC up when it is; else, if it was set,
 * $AC $FF.
 * HOW: X from 6 down to 0: $A8 at or past $DA64,X and below $DA8F,X:
 * $AC up, done. None: $AC nonzero, $AC $FF.
 * Entry: M 8-bit, X 8-bit; DB $80; called by JSR from GenieScreen. Exit:
 * M 8-bit, X 8-bit.
 */
void abi_GeniePointerCheck(void)
{
    ldx8_v(0x06);  /* 80AC65 ldx */
L_80AC67:
    lda8(ea_abs(0x00A8));  /* 80AC67 lda */
    cmp8_v(rd8(ea_absx(0xDA64)));  /* 80AC6A cmp */
    if (!IF_C) goto L_80AC79;  /* 80AC6D bcc */
    cmp8_v(rd8(ea_absx(0xDA8F)));  /* 80AC6F cmp */
    if (IF_C) goto L_80AC79;  /* 80AC72 bcs */
    MEM8(inc, ea_abs(0x00AC));  /* 80AC74 inc */
    goto L_80AC86;  /* 80AC77 bra */
L_80AC79:
    dex8();  /* 80AC79 dex */
    if (!IF_N) goto L_80AC67;  /* 80AC7A bpl */
    lda8(ea_abs(0x00AC));  /* 80AC7C lda */
    if (IF_Z) goto L_80AC86;  /* 80AC7F beq */
    lda8_v(0xFF);  /* 80AC81 lda */
    sta8(ea_abs(0x00AC));  /* 80AC83 sta */
L_80AC86:
    return;  /* 80AC86 rts */
}

