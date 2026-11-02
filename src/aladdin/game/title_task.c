/*
 * title_task.c: the title: its start (the opening story first) and its
 * task: the Mode 7 title, the demo when left alone, the menu and the
 * level select.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorCreate(void);
void abi_ActorsFreeAll(void);
void abi_DemoStart(void);
void abi_DisplayDefaults(void);
void abi_GameRamClear(void);
void abi_LayersFillLong(void);
void abi_LevelCountersClear(void);
void abi_LevelMapBuildLong(void);
void abi_LevelNumberText(void);
void abi_LevelSelect(void);
void abi_Mode7ScreenSetup(void);
void abi_NewGameState(void);
void abi_NmiDisable(void);
void abi_NmiEnable(void);
void abi_OamBufferClear(void);
void abi_PadConfirmPressed(void);
void abi_PaletteCycleStartLong(void);
void abi_PaletteLoad(void);
void abi_PaletteSave(void);
void abi_PlayStartCounts(void);
void abi_ScrollStateClear(void);
void abi_SongPlay(void);
void abi_SoundQueueAdd(void);
void abi_SoundQueueF6(void);
void abi_SoundQueueF6With1(void);
void abi_TaskCreate(void);
void abi_TaskExec(void);
void abi_TaskSlotsFree1To6(void);
void abi_TaskWait(void);
void abi_TextLayerClear(void);
void abi_TitleStartTask(void);
void abi_TitleTask(void);
void abi_VramCopy(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 80:9C64 TitleStartTask
 * WHAT IT DOES: Starts the title's task: frees the other tasks, stage 0,
 * play mode 0, and runs the opening story, which ends by starting
 * TitleTask; the jump to TitleTask's middle after it ($80:9C76) is never
 * reached, since OpeningStory never returns.
 * HOW: TaskSlotsFree1To6; $033F and the play mode cleared; OpeningStory;
 * then a JMP to $80:9C9D (hand-written: no script reaches it).
 * Entry: M 8-bit, X 8-bit; a task's entry. Exit: never returns.
 */
void abi_TitleStartTask(void)
{
    lda8_v(0x80);  /* 809C64 lda */
    pha8();  /* 809C66 pha */
    plb();  /* 809C67 plb */
    abi_TaskSlotsFree1To6();  /* 809C68 jsl */
    stz8(ea_abs(V_033F));  /* 809C6C stz */
    stz8(ea_abs(PLAY_MODE));  /* 809C6F stz */
    call_routine(0x80AC87u, 1);  /* 809C72 jsl */
    jump_routine(0x809C9Du); return;  /* 809C76 jmp (no script reaches it: OpeningStory never returns) */
}

/*
 * @routine 80:9C91 TitleTask
 * WHAT IT DOES: The title screen: puts up the Mode 7 title, waits
 * (playing the demo or the story after long idles), and runs the menu:
 * game start, password, options; with $0009 set, the level select.
 * HOW: The screen built (level map, layers, graphics, palette,
 * Mode7ScreenSetup) and faded in. After a demo ($0345 set) it comes back
 * at $80:9E7F and waits for a press (PadConfirmPressed) or the zoom
 * task's end; else the menu runs until a press, the level select
 * (LevelSelect, with $0009 set) or the countdown at $0343, which starts
 * the demo ($81:8031) or the opening story. A choice ends the task by
 * TaskExec: $83:8979, PasswordScreen or $81:8095.
 * Entry: M 8-bit, X 8-bit; a task's entry. Exit: never returns.
 */
void abi_TitleTask(void)
{
    abi_TaskSlotsFree1To6();  /* 809C91 jsl */
    abi_LevelCountersClear();  /* 809C95 jsl */
    abi_GameRamClear();  /* 809C99 jsl */
    stz8(ea_abs(PLAY_MODE));  /* 809C9D stz */
    stz8(ea_abs(0x0360));  /* 809CA0 stz */
    abi_NmiDisable();  /* 809CA3 jsl */
    call_routine(0x8196B1u, 1);  /* 809CA7 jsl */
    abi_ScrollStateClear();  /* 809CAB jsl */
    abi_OamBufferClear();  /* 809CAF jsl */
    abi_DisplayDefaults();  /* 809CB3 jsl */
    abi_TextLayerClear();  /* 809CB7 jsl */
    lda8_v(0x40);  /* 809CBB lda */
    sta8(ea_abs(LEVEL));  /* 809CBD sta */
    call_routine(0x81825Bu, 1);  /* 809CC0 jsl */
    lda8_v(0xFF);  /* 809CC4 lda */
    sta8(ea_abs(V_1A08));  /* 809CC6 sta */
    op_rep(0x20);  /* 809CC9 rep */
    stz16(ea_abs(0x1773));  /* 809CCB stz */
    stz16(ea_abs(0x1777));  /* 809CCE stz */
    op_sep(0x20);  /* 809CD1 sep */
    abi_LevelMapBuildLong();  /* 809CD3 jsl */
    abi_LayersFillLong();  /* 809CD7 jsl */
    stz8(ea_abs(0x0399));  /* 809CDB stz */
    stz8(ea_abs(0x039A));  /* 809CDE stz */
    lda8_v(0x01);  /* 809CE1 lda */
    call_routine(0x81BAF1u, 1);  /* 809CE3 jsl */
    ldx8_v(0x00);  /* 809CE7 ldx */
    abi_VramCopy();  /* 809CE9 jsl */
    lda8_v(0x04);  /* 809CED lda */
    call_routine(0x81BAF1u, 1);  /* 809CEF jsl */
    ldx8_v(0x15);  /* 809CF3 ldx */
    abi_VramCopy();  /* 809CF5 jsl */
    ldx8_v(0x31);  /* 809CF9 ldx */
    abi_VramCopy();  /* 809CFB jsl */
    lda8_v(0x2F);  /* 809CFF lda */
    call_routine(0x81BAF1u, 1);  /* 809D01 jsl */
    ldx8_v(0x23);  /* 809D05 ldx */
    abi_VramCopy();  /* 809D07 jsl */
    lda8_v(0x08);  /* 809D0B lda */
    abi_PaletteLoad();  /* 809D0D jsl */
    abi_NmiEnable();  /* 809D11 jsl */
    lda8_v(0x01);  /* 809D15 lda */
    jsl_frame(0x809D17u); abi_TaskWait();  /* 809D17 jsl */
    lda8_v(0x00);  /* 809D1B lda */
    abi_Mode7ScreenSetup();  /* 809D1D jsl */
    lda8_v(0x00);  /* 809D21 lda */
    sta8(ea_abs(INIDISP_SHADOW));  /* 809D23 sta */
    call_routine(0x81830Du, 1);  /* 809D26 jsl */
    lda8_v(0x20);  /* 809D2A lda */
    jsl_frame(0x809D2Cu); abi_TaskWait();  /* 809D2C jsl */
    jsl_frame(0x809D30u); abi_TaskCreate(); cpu.unwound = 0;  /* 809D30 jsl */
    lda8_v(0x5C);  /* 809D39 lda */
    sta8(ea_abs(0x03A0));  /* 809D3B sta */
    lda8_v(0x55);  /* 809D3E lda */
    sta8(ea_abs(0x03A4));  /* 809D40 sta */
    lda8_v(0x11);  /* 809D43 lda */
    sta8(ea_abs(0x0399));  /* 809D45 sta */
    sta8(ea_abs(0x039A));  /* 809D48 sta */
    lda8_v(0x01);  /* 809D4B lda */
    jsl_frame(0x809D4Du); abi_TaskWait();  /* 809D4D jsl */
    lda8(ea_abs(0x0345));  /* 809D51 lda */
    if (IF_Z) goto L_809D59;  /* 809D54 beq */
    goto L_809E7F;  /* 809D56 jmp */
L_809D59:
    op_rep(0x20);  /* 809D59 rep */
    lda16_v(0x0200);  /* 809D5B lda */
    sta16(ea_abs(V_0343));  /* 809D5E sta */
    op_sep(0x20);  /* 809D61 sep */
    abi_ActorsFreeAll();  /* 809D63 jsl */
    stz8(ea_abs(0x0399));  /* 809D67 stz */
    lda8_v(0x82);  /* 809D6A lda */
    abi_ActorCreate();  /* 809D6C jsl */
    if (IF_C) goto L_809DC5;  /* 809D70 bcs */
    lda8_v(0x0C);  /* 809D72 lda */
    sta8(ea_absx(0x0000));  /* 809D74 sta */
    op_rep(0x20);  /* 809D77 rep */
    lda16_v(0x0900);  /* 809D79 lda */
    sta16(ea_absx(0x002B));  /* 809D7C sta */
    lda16_v(0x0080);  /* 809D7F lda */
    sta16(ea_absx(SCRATCH));  /* 809D82 sta */
    lda16_v(0x0080);  /* 809D85 lda */
    sta16(ea_absx(0x000F));  /* 809D88 sta */
    lda16_v(0x0080);  /* 809D8B lda */
    sta16(ea_abs(0x0395));  /* 809D8E sta */
    lda16_v(0x00B0);  /* 809D91 lda */
    sta16(ea_abs(0x0397));  /* 809D94 sta */
    op_sep(0x20);  /* 809D97 sep */
    abi_NmiDisable();  /* 809D99 jsl */
    call_routine(0x8196B1u, 1);  /* 809D9D jsl */
    lda8_v(0x4C);  /* 809DA1 lda */
    abi_PaletteLoad();  /* 809DA3 jsl */
    lda8_v(0x22);  /* 809DA7 lda */
    call_routine(0x81BAF1u, 1);  /* 809DA9 jsl */
    ldx8_v(0xFC);  /* 809DAD ldx */
    abi_VramCopy();  /* 809DAF jsl */
    abi_NmiEnable();  /* 809DB3 jsl */
    lda8(ea_abs(INIDISP_SHADOW));  /* 809DB7 lda */
    and8_v(0x0F);  /* 809DBA and */
    sta8(ea_abs(INIDISP_SHADOW));  /* 809DBC sta */
    lda8_v(0x14);  /* 809DBF lda */
    abi_SongPlay();  /* 809DC1 jsl */
L_809DC5:
    lda8_v(0x01);  /* 809DC5 lda */
    jsl_frame(0x809DC7u); abi_TaskWait();  /* 809DC7 jsl */
    lda8(ea_abs(0x0087));  /* 809DCB lda */
    if (!IF_Z) goto L_809DC5;  /* 809DCE bne */
    stz8(ea_abs(CGWSEL_SHADOW));  /* 809DD0 stz */
    lda8_v(0x31);  /* 809DD3 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 809DD5 sta */
    stz8(ea_abs(COLDATA_RED));  /* 809DD8 stz */
    stz8(ea_abs(COLDATA_GREEN));  /* 809DDB stz */
    stz8(ea_abs(COLDATA_BLUE));  /* 809DDE stz */
L_809DE1:
    lda8_v(0x01);  /* 809DE1 lda */
    jsl_frame(0x809DE3u); abi_TaskWait();  /* 809DE3 jsl */
    lda8(ea_abs(COLDATA_RED));  /* 809DE7 lda */
    ACC8(inc);  /* 809DEA inc */
    sta8(ea_abs(COLDATA_RED));  /* 809DEB sta */
    sta8(ea_abs(COLDATA_GREEN));  /* 809DEE sta */
    sta8(ea_abs(COLDATA_BLUE));  /* 809DF1 sta */
    cmp8_v(0x1F);  /* 809DF4 cmp */
    if (!IF_C) goto L_809DE1;  /* 809DF6 bcc */
    lda8_v(0x0C);  /* 809DF8 lda */
    ldy8_v(0x01);  /* 809DFA ldy */
    abi_PaletteSave();  /* 809DFC jsl */
    lda8_v(0xE1);  /* 809E00 lda */
    sta8(ea_abs(0x0076));  /* 809E02 sta */
    ldy8_v(0x01);  /* 809E05 ldy */
    call_routine(0x819DCDu, 1);  /* 809E07 jsl */
    lda8_v(0x11);  /* 809E0B lda */
    sta8(ea_abs(0x0399));  /* 809E0D sta */
    op_rep(0x20);  /* 809E10 rep */
    lda16_v(0x00D0);  /* 809E12 lda */
    sta16(ea_abs(0x038D));  /* 809E15 sta */
    sta16(ea_abs(0x0393));  /* 809E18 sta */
    op_sep(0x20);  /* 809E1B sep */
L_809E1D:
    lda8_v(0x01);  /* 809E1D lda */
    jsl_frame(0x809E1Fu); abi_TaskWait();  /* 809E1F jsl */
    call_routine(0x81830Du, 1);  /* 809E23 jsl */
    MEM8(inc, ea_abs(0x038D));  /* 809E27 inc */
    MEM8(inc, ea_abs(0x0393));  /* 809E2A inc */
    lda8(ea_abs(COLDATA_RED));  /* 809E2D lda */
    ACC8(dec);  /* 809E30 dec */
    sta8(ea_abs(COLDATA_RED));  /* 809E31 sta */
    sta8(ea_abs(COLDATA_GREEN));  /* 809E34 sta */
    sta8(ea_abs(COLDATA_BLUE));  /* 809E37 sta */
    if (!IF_Z) goto L_809E1D;  /* 809E3A bne */
    lda8_v(0x10);  /* 809E3C lda */
    sta8(ea_abs(0x039A));  /* 809E3E sta */
    lda8_v(0x02);  /* 809E41 lda */
    sta8(ea_abs(CGWSEL_SHADOW));  /* 809E43 sta */
    lda8_v(0x21);  /* 809E46 lda */
    sta8(ea_abs(CGADSUB_SHADOW));  /* 809E48 sta */
L_809E4B:
    lda8_v(0x01);  /* 809E4B lda */
    jsl_frame(0x809E4Du); abi_TaskWait();  /* 809E4D jsl */
    call_routine(0x81830Du, 1);  /* 809E51 jsl */
    lda8(ea_abs(0x0076));  /* 809E55 lda */
    ldy8_v(0x01);  /* 809E58 ldy */
    call_routine(0x819DCDu, 1);  /* 809E5A jsl */
    MEM8(inc, ea_abs(0x0076));  /* 809E5E inc */
    if (IF_Z) goto L_809E74;  /* 809E61 beq */
    lda8(ea_abs(0x038D));  /* 809E63 lda */
    if (IF_Z) goto L_809E4B;  /* 809E66 beq */
    op_rep(0x20);  /* 809E68 rep */
    MEM16(inc, ea_abs(0x038D));  /* 809E6A inc */
    MEM16(inc, ea_abs(0x0393));  /* 809E6D inc */
    op_sep(0x20);  /* 809E70 sep */
    goto L_809E4B;  /* 809E72 bra */
L_809E74:
    ldx8_v(0x00);  /* 809E74 ldx */
    lda8_v(0x22);  /* 809E76 lda */
    abi_PaletteCycleStartLong();  /* 809E78 jsl */
    goto L_809F08;  /* 809E7C jmp */
L_809E7F:
    op_rep(0x20);  /* 809E7F rep */
    lda16_v(0x1300);  /* 809E81 lda */
    sta16(ea_abs(V_0343));  /* 809E84 sta */
    op_sep(0x20);  /* 809E87 sep */
    lda8_v(0x12);  /* 809E89 lda */
    abi_SongPlay();  /* 809E8B jsl */
L_809E8F:
    abi_PadConfirmPressed();  /* 809E8F jsr */
    if (!IF_Z) goto L_809EC8;  /* 809E92 bne */
    lda8_v(0x01);  /* 809E94 lda */
    jsl_frame(0x809E96u); abi_TaskWait();  /* 809E96 jsl */
    call_routine(0x81830Du, 1);  /* 809E9A jsl */
    lda8(ea_abs(0x146B));  /* 809E9E lda */
    if (IF_Z) goto L_809E8F;  /* 809EA1 beq */
    jsl_frame(0x809EA3u); abi_TaskCreate(); cpu.unwound = 0;  /* 809EA3 jsl */
L_809EAC:
    abi_PadConfirmPressed();  /* 809EAC jsr */
    if (!IF_Z) goto L_809EC8;  /* 809EAF bne */
    lda8_v(0x01);  /* 809EB1 lda */
    jsl_frame(0x809EB3u); abi_TaskWait();  /* 809EB3 jsl */
    call_routine(0x81830Du, 1);  /* 809EB7 jsl */
    lda8(ea_abs(0x146B));  /* 809EBB lda */
    if (!IF_N) goto L_809EAC;  /* 809EBE bpl */
    lda8_v(0x13);  /* 809EC0 lda */
    abi_SongPlay();  /* 809EC2 jsl */
    goto L_809F08;  /* 809EC6 bra */
L_809EC8:
    jsl_frame(0x809EC8u); abi_TaskCreate(); cpu.unwound = 0;  /* 809EC8 jsl */
L_809ED1:
    lda8_v(0x01);  /* 809ED1 lda */
    jsl_frame(0x809ED3u); abi_TaskWait();  /* 809ED3 jsl */
    call_routine(0x81830Du, 1);  /* 809ED7 jsl */
    lda8(ea_abs(0x0087));  /* 809EDB lda */
    if (!IF_Z) goto L_809ED1;  /* 809EDE bne */
    stz8(ea_abs(0x00B7));  /* 809EE0 stz */
    stz8(ea_abs(0x00FF));  /* 809EE3 stz */
    stz8(ea_abs(HDMAEN_SHADOW));  /* 809EE6 stz */
    stz8(ea_abs(HDMAEN));  /* 809EE9 stz */
    jsl_frame(0x809EECu); abi_TaskCreate(); cpu.unwound = 0;  /* 809EEC jsl */
    jsl_frame(0x809EF5u); abi_TaskCreate(); cpu.unwound = 0;  /* 809EF5 jsl */
    abi_ActorsFreeAll();  /* 809EFE jsl */
    lda8_v(0x13);  /* 809F02 lda */
    abi_SongPlay();  /* 809F04 jsl */
L_809F08:
    lda8_v(0x01);  /* 809F08 lda */
    jsl_frame(0x809F0Au); abi_TaskWait();  /* 809F0A jsl */
    lda8_v(0x4A);  /* 809F0E lda */
    abi_ActorCreate();  /* 809F10 jsl */
    if (IF_C) goto L_809F2B;  /* 809F14 bcs */
    lda8_v(0x0C);  /* 809F16 lda */
    sta8(ea_absx(0x0000));  /* 809F18 sta */
    op_rep(0x20);  /* 809F1B rep */
    lda16_v(0x007C);  /* 809F1D lda */
    sta16(ea_absx(SCRATCH));  /* 809F20 sta */
    lda16_v(0x00C8);  /* 809F23 lda */
    sta16(ea_absx(0x000F));  /* 809F26 sta */
    op_sep(0x20);  /* 809F29 sep */
L_809F2B:
    lda8_v(0x01);  /* 809F2B lda */
    sta8(ea_abs(0x038E));  /* 809F2D sta */
    stz8(ea_abs(0x038D));  /* 809F30 stz */
    sta8(ea_abs(0x0394));  /* 809F33 sta */
    stz8(ea_abs(0x0393));  /* 809F36 stz */
    jsl_frame(0x809F39u); abi_TaskCreate(); cpu.unwound = 0;  /* 809F39 jsl */
    call_routine(0x81EE04u, 1);  /* 809F42 jsl */
    lda8_v(0xA6);  /* 809F46 lda */
    call_routine(0x81972Du, 1);  /* 809F48 jsl */
    lda8_v(0x0E);  /* 809F4C lda */
    call_routine(0x81972Du, 1);  /* 809F4E jsl */
    lda8_v(0x10);  /* 809F52 lda */
    call_routine(0x81972Du, 1);  /* 809F54 jsl */
    lda8_v(0x12);  /* 809F58 lda */
    call_routine(0x81972Du, 1);  /* 809F5A jsl */
    stz8(ea_abs(0x0847));  /* 809F5E stz */
    stz8(ea_abs(0x0848));  /* 809F61 stz */
    stz8(ea_abs(0x0849));  /* 809F64 stz */
    stz8(ea_abs(0x084A));  /* 809F67 stz */
    stz8(ea_abs(0x084B));  /* 809F6A stz */
    stz8(ea_abs(LEVEL_035C));  /* 809F6D stz */
    stz8(ea_abs(LEVEL));  /* 809F70 stz */
    stz8(ea_abs(0x035D));  /* 809F73 stz */
    lda8(ea_abs(CHEAT_0009));  /* 809F76 lda */
    if (IF_Z) goto L_809F7E;  /* 809F79 beq */
    abi_LevelNumberText();  /* 809F7B jsr */
L_809F7E:
    lda8_v(0x03);  /* 809F7E lda */
    sta8(ea_abs(HEALTH_MARKS));  /* 809F80 sta */
    lda8_v(0x03);  /* 809F83 lda */
    sta8(ea_abs(0x0362));  /* 809F85 sta */
    lda8_v(0x10);  /* 809F88 lda */
    sta8(ea_abs(0x0369));  /* 809F8A sta */
    stz8(ea_abs(0x036C));  /* 809F8D stz */
    stz8(ea_abs(0x036E));  /* 809F90 stz */
    stz8(ea_abs(0x036B));  /* 809F93 stz */
    stz8(ea_abs(0x142E));  /* 809F96 stz */
    stz8(ea_abs(0x0347));  /* 809F99 stz */
    stz8(ea_abs(0x0370));  /* 809F9C stz */
    stz8(ea_abs(0x0358));  /* 809F9F stz */
    stz8(ea_abs(0x0373));  /* 809FA2 stz */
    stz8(ea_abs(0x0374));  /* 809FA5 stz */
    stz8(ea_abs(0x0375));  /* 809FA8 stz */
    stz8(ea_abs(0x0376));  /* 809FAB stz */
    stz8(ea_abs(0x0359));  /* 809FAE stz */
    stz8(ea_abs(0x035A));  /* 809FB1 stz */
    lda8_v(0x03);  /* 809FB4 lda */
    sta8(ea_abs(CONTINUES));  /* 809FB6 sta */
L_809FB9:
    lda8_v(0x01);  /* 809FB9 lda */
    jsl_frame(0x809FBBu); abi_TaskWait();  /* 809FBB jsl */
    call_routine(0x81830Du, 1);  /* 809FBF jsl */
    lda8(ea_abs(CHEAT_0009));  /* 809FC3 lda */
    if (IF_Z) goto L_809FCD;  /* 809FC6 beq */
    abi_LevelSelect();  /* 809FC8 jsr */
    if (!IF_C) goto L_809FD1;  /* 809FCB bcc */
L_809FCD:
    call_routine(0x81EDA2u, 1);  /* 809FCD jsl */
L_809FD1:
    op_rep(0x20);  /* 809FD1 rep */
    MEM16(dec, ea_abs(V_0343));  /* 809FD3 dec */
    op_sep(0x20);  /* 809FD6 sep */
    if (!IF_Z) goto L_809FF3;  /* 809FD8 bne */
    lda8_v(0x04);  /* 809FDA lda */
    abi_SoundQueueF6();  /* 809FDC jsl */
    lda8(ea_abs(0x0345));  /* 809FE0 lda */
    if (IF_Z) goto L_809FEC;  /* 809FE3 beq */
    stz8(ea_abs(0x0345));  /* 809FE5 stz */
    jump_routine(0x80AC87u); return;  /* 809FE8 jml */
L_809FEC:
    MEM8(inc, ea_abs(0x0345));  /* 809FEC inc */
    goto L_818031;  /* 809FEF jml */
L_809FF3:
    abi_PadConfirmPressed();  /* 809FF3 jsr */
    if (IF_Z) goto L_809FB9;  /* 809FF6 beq */
    lda8_v(0x01);  /* 809FF8 lda */
    jsl_frame(0x809FFAu); abi_TaskWait();  /* 809FFA jsl */
    lda8_v(0x4F);  /* 809FFE lda */
    abi_SoundQueueAdd();  /* 80A000 jsl */
    lda8_v(0x01);  /* 80A004 lda */
    jsl_frame(0x80A006u); abi_TaskWait();  /* 80A006 jsl */
    jsl_frame(0x80A00Au); abi_TaskCreate(); cpu.unwound = 0;  /* 80A00A jsl */
    abi_SoundQueueF6With1();  /* 80A013 jsl */
L_80A017:
    lda8_v(0x01);  /* 80A017 lda */
    jsl_frame(0x80A019u); abi_TaskWait();  /* 80A019 jsl */
    lda8(ea_abs(0x0087));  /* 80A01D lda */
    if (!IF_Z) goto L_80A017;  /* 80A020 bne */
    stz8(ea_abs(HDMAEN_SHADOW));  /* 80A022 stz */
    stz8(ea_abs(HDMAEN));  /* 80A025 stz */
    abi_TaskSlotsFree1To6();  /* 80A028 jsl */
    lda8(ea_abs(0x0849));  /* 80A02C lda */
    cmp8_v(0x02);  /* 80A02F cmp */
    if (!IF_Z) goto L_80A03A;  /* 80A031 bne */
    jsl_frame(0x80A033u); abi_TaskExec(); return;  /* 80A033 jsl */
L_80A03A:
    lda8(ea_abs(0x0849));  /* 80A03A lda */
    cmp8_v(0x01);  /* 80A03D cmp */
    if (!IF_Z) goto L_80A048;  /* 80A03F bne */
    jsl_frame(0x80A041u); abi_TaskExec(); return;  /* 80A041 jsl */
L_80A048:
    jsl_frame(0x80A048u); abi_TaskExec(); return;  /* 80A048 jsl */
L_818031:
    jsl_frame(0x818031u); abi_TaskCreate(); cpu.unwound = 0;  /* 818031 jsl */
L_81803A:
    lda8_v(0x01);  /* 81803A lda */
    jsl_frame(0x81803Cu); abi_TaskWait();  /* 81803C jsl */
    lda8(ea_abs(0x0087));  /* 818040 lda */
    if (!IF_Z) goto L_81803A;  /* 818043 bne */
    abi_DemoStart();  /* 818045 jsr */
    MEM8(inc, ea_abs(V_033F));  /* 818048 inc */
    lda8(ea_abs(V_033F));  /* 81804B lda */
    cmp8_v(0x02);  /* 81804E cmp */
    if (!IF_Z) goto L_818055;  /* 818050 bne */
    stz8(ea_abs(V_033F));  /* 818052 stz */
L_818055:
    op_rep(0x20);  /* 818055 rep */
    lda16_v(0x04EC);  /* 818057 lda */
    sta16(ea_abs(0x0341));  /* 81805A sta */
    op_sep(0x20);  /* 81805D sep */
    goto L_8180D3;  /* 81805F jmp */
L_8180D3:
    abi_NewGameState();  /* 8180D3 jsr */
    abi_PlayStartCounts();  /* 8180D6 jsr */
    jump_routine(0x818191u); return;  /* 8180D9 jmp */
}
