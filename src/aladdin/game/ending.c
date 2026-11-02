/*
 * ending.c: the ending: its zoom scene's task, the stages task that shows
 * the levels played back, and the text pages with their extras.
 */
#include "machine.h"
#include "ops65.h"
void abi_ActorCreate(void);
void abi_ActorRecordsInit(void);
void abi_DemoStartLong(void);
void abi_EndingStagesTask(void);
void abi_EndingTextTask(void);
void abi_KeptOptionsSeal(void);
void abi_Mode7Rotate(void);
void abi_NewGameStateLong(void);
void abi_OamBufferClear(void);
void abi_PaletteLoad(void);
void abi_PlayStartCountsLong(void);
void abi_SceneSpinZoomTask(void);
void abi_SongPlay(void);
void abi_StageCountdownSet(void);
void abi_TaskCreate(void);
void abi_TaskWait(void);
void abi_TextBandsCommit(void);
void abi_TextBandsConceal(void);
void abi_TextBandsReveal(void);
void abi_TextLayerClear(void);
void abi_TextPageDraw(void);
void abi_TextPageExtra(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 80:B1D8 SceneSpinZoomTask
 * WHAT IT DOES: Spins and zooms a Mode 7 scene: the angle steps on by a
 * speed that grows every 16 frames and the zoom grows every 24, the zoom
 * taken as its negative past $7F.
 * HOW: D = the task's slot page. The centre ($80, $50); Mode 7 scaled
 * (nmi_ppu_mode 1); angle 0, speed 4, zoom 8, counts 16 and 16. Each
 * frame: the counts step (speed up every 16, zoom up every 24); the zoom
 * (negated when negative) into the four scales; the matrix shadows
 * shifted left 4 (C = -B); TaskWait 1; Mode7Rotate by the angle, then the
 * angle plus the speed.
 * Entry: M 8-bit, X 8-bit; Y = the task's slot; a task's entry. Exit:
 * never returns.
 */
void abi_SceneSpinZoomTask(void)
{
    op_rep(0x20);  /* 80B1D8 rep */
    clc();  /* 80B1DA clc */
    tya16();  /* 80B1DB tya */
    adc16_v(0x006F);  /* 80B1DC adc */
    tcd();  /* 80B1DF tcd */
    lda16_v(0x0080);  /* 80B1E0 lda */
    sta16(ea_abs(0x0395));  /* 80B1E3 sta */
    lda16_v(0x0050);  /* 80B1E6 lda */
    sta16(ea_abs(0x0397));  /* 80B1E9 sta */
    op_sep(0x20);  /* 80B1EC sep */
    lda8_v(0x01);  /* 80B1EE lda */
    sta8(ea_abs(0x0829));  /* 80B1F0 sta */
    stz8(ea_dp(0x06));  /* 80B1F3 stz */
    lda8_v(0x04);  /* 80B1F5 lda */
    sta8(ea_dp(0x07));  /* 80B1F7 sta */
    lda8_v(0x08);  /* 80B1F9 lda */
    sta8(ea_dp(0x08));  /* 80B1FB sta */
    lda8_v(0x10);  /* 80B1FD lda */
    sta8(ea_dp(0x09));  /* 80B1FF sta */
    lda8_v(0x10);  /* 80B201 lda */
    sta8(ea_dp(0x0A));  /* 80B203 sta */
L_80B205:
    MEM8(dec, ea_dp(0x09));  /* 80B205 dec */
    if (!IF_Z) goto L_80B20F;  /* 80B207 bne */
    lda8_v(0x10);  /* 80B209 lda */
    sta8(ea_dp(0x09));  /* 80B20B sta */
    MEM8(inc, ea_dp(0x07));  /* 80B20D inc */
L_80B20F:
    MEM8(dec, ea_dp(0x0A));  /* 80B20F dec */
    if (!IF_Z) goto L_80B219;  /* 80B211 bne */
    lda8_v(0x18);  /* 80B213 lda */
    sta8(ea_dp(0x0A));  /* 80B215 sta */
    MEM8(inc, ea_dp(0x08));  /* 80B217 inc */
L_80B219:
    lda8(ea_dp(0x08));  /* 80B219 lda */
    if (!IF_N) goto L_80B220;  /* 80B21B bpl */
    eor8_v(0xFF);  /* 80B21D eor */
    ACC8(inc);  /* 80B21F inc */
L_80B220:
    sta8(ea_abs(0x0827));  /* 80B220 sta */
    sta8(ea_abs(0x0828));  /* 80B223 sta */
    sta8(ea_abs(0x0825));  /* 80B226 sta */
    sta8(ea_abs(0x0826));  /* 80B229 sta */
    op_rep(0x20);  /* 80B22C rep */
    lda16(ea_abs(0x038D));  /* 80B22E lda */
    ACC16(asl);  /* 80B231 asl */
    ACC16(asl);  /* 80B232 asl */
    ACC16(asl);  /* 80B233 asl */
    ACC16(asl);  /* 80B234 asl */
    sta16(ea_abs(0x038D));  /* 80B235 sta */
    sta16(ea_abs(0x0393));  /* 80B238 sta */
    lda16(ea_abs(0x038F));  /* 80B23B lda */
    ACC16(asl);  /* 80B23E asl */
    ACC16(asl);  /* 80B23F asl */
    ACC16(asl);  /* 80B240 asl */
    ACC16(asl);  /* 80B241 asl */
    sta16(ea_abs(0x038F));  /* 80B242 sta */
    eor16_v(0xFFFF);  /* 80B245 eor */
    ACC16(inc);  /* 80B248 inc */
    sta16(ea_abs(0x0391));  /* 80B249 sta */
    op_sep(0x20);  /* 80B24C sep */
    lda8_v(0x01);  /* 80B24E lda */
    jsl_frame(0x80B250u); abi_TaskWait();  /* 80B250 jsl */
    lda8(ea_dp(0x06));  /* 80B254 lda */
    abi_Mode7Rotate();  /* 80B256 jsl */
    clc();  /* 80B25A clc */
    lda8(ea_dp(0x06));  /* 80B25B lda */
    adc8_v(rd8(ea_dp(0x07)));  /* 80B25D adc */
    sta8(ea_dp(0x06));  /* 80B25F sta */
    goto L_80B205;  /* 80B261 bra */
}



