/*
 * sound_driver.c: the SPC700 sound driver: its start and main loop,
 * the S-CPU's commands, the sequencer and the voices' pitch and volume.
 */
#include "spc.h"
#include "spc_ops.h"

void abi_SoundDriverStart(void);
void abi_DspSetup(void);
void abi_PortCommandPoll(void);
void abi_PortCommandPollEnd(void);
void abi_SoundCommand(void);
void abi_VoiceStateClear(void);
void abi_VoiceDpClear(void);
void abi_SoundCommandTable(void);
void abi_SoundStopAll(void);
void abi_SoundCmdF1(void);
void abi_SequencerReset(void);
void abi_PageClear50(void);
void abi_SoundCmdFA(void);
void abi_SoundCmdFB(void);
void abi_SongStartKeep(void);
void abi_SoundCmdFC(void);
void abi_SoundCmdFD(void);
void abi_SoundCmdF3(void);
void abi_SoundCmdF4(void);
void abi_VoicesReleaseFree(void);
void abi_SoundCmdF5(void);
void abi_SoundCmdF6(void);
void abi_FadeStart(void);
void abi_FadeClear(void);
void abi_FadeSet(void);
void abi_SoundCmdF7(void);
void abi_SoundCmdF8(void);
void abi_SoundUpload(void);
void abi_VoiceTick(void);
void abi_TrackByte(void);
void abi_TrackByteEnd(void);
void abi_SeqCmd00(void);
void abi_SeqCmd01(void);
void abi_SeqCmd02(void);
void abi_SeqCmd03(void);
void abi_VoiceFlagsToggle(void);
void abi_VoiceFlagsStore(void);
void abi_SeqCmd05(void);
void abi_SeqCmd06(void);
void abi_SeqCmd07(void);
void abi_SeqCmd08(void);
void abi_VoiceInstrument(void);
void abi_VoiceInstrumentFrom(void);
void abi_VoiceInstrumentEnd(void);
void abi_SeqCmd09(void);
void abi_SeqCmd0A(void);
void abi_SeqCmd0B(void);
void abi_SeqCmd0C(void);
void abi_SeqCmd0D(void);
void abi_SeqCmd0E(void);
void abi_SeqCmd0F(void);
void abi_SeqCmd10(void);
void abi_SeqCmd11(void);
void abi_SeqLoop(void);
void abi_SeqCmd16(void);
void abi_SeqLoopSkip(void);
void abi_VoiceFlagsSet(void);
void abi_SeqCmd17(void);
void abi_VoiceStop(void);
void abi_VoiceSfxMark(void);
void abi_SeqCmd18(void);
void abi_SeqCmd19(void);
void abi_SeqCmd1A(void);
void abi_SeqCmd1B(void);
void abi_EchoSetup(void);
void abi_EchoSetupEnd(void);
void abi_EchoOff(void);
void abi_EchoFirSet(void);
void abi_SeqCmd1C(void);
void abi_SeqCmd1D(void);
void abi_VoiceSlide(void);
void abi_VoiceVolumeSet(void);
void abi_CurveAt16(void);
void abi_CurveAtY(void);
void abi_CurveAt8(void);
void abi_CurveShift(void);
void abi_VoicePitchUpdate(void);
void abi_VoicePitchSet(void);
void abi_VoicePitchFrom(void);
void abi_VoicePitchVibrato(void);
void abi_VoicePitchOut(void);
void abi_DspWrite(void);
void abi_DspWriteVoice(void);
void abi_VoiceRelease(void);
void abi_VoiceAdsr1Set(void);

/* (d)+Y: the word at direct page d and d+1, read low then high, plus Y. */
static uint16_t ind_y(uint8_t d)
{
    uint16_t p = spc_rd(spc_dp(d));
    p |= (uint16_t)(spc_rd(spc_dp((uint8_t)(d + 1))) << 8);
    return (uint16_t)(p + spc.y);
}

/* The sequencer command at handler address h (the table at $0831). */
static void command_run(uint16_t h)
{
    switch (h) {
    case 0x0871: abi_SeqCmd00(); return;
    case 0x0875: abi_SeqCmd01(); return;
    case 0x0879: abi_SeqCmd02(); return;
    case 0x087F: abi_SeqCmd03(); return;
    case 0x0971: abi_VoiceFlagsSet(); return;
    case 0x0886: abi_SeqCmd05(); return;
    case 0x08A1: abi_SeqCmd06(); return;
    case 0x08A5: abi_SeqCmd07(); return;
    case 0x08BA: abi_SeqCmd08(); return;
    case 0x08FC: abi_SeqCmd09(); return;
    case 0x0907: abi_SeqCmd0A(); return;
    case 0x090D: abi_SeqCmd0B(); return;
    case 0x0911: abi_SeqCmd0C(); return;
    case 0x0915: abi_SeqCmd0D(); return;
    case 0x0919: abi_SeqCmd0E(); return;
    case 0x091D: abi_SeqCmd0F(); return;
    case 0x0921: abi_SeqCmd10(); return;
    case 0x0925: abi_SeqCmd11(); return;
    case 0x095B: abi_SeqCmd16(); return;
    case 0x097C: abi_SeqCmd17(); return;
    case 0x09B7: abi_SeqCmd18(); return;
    case 0x09BE: abi_SeqCmd19(); return;
    case 0x09C8: abi_SeqCmd1A(); return;
    case 0x09EE: abi_SeqCmd1B(); return;
    case 0x0AA3: abi_SeqCmd1C(); return;
    case 0x0ABD: abi_SeqCmd1D(); return;
    case 0x0830: abi_TrackByteEnd(); return;
    }
}

/*
 * @routine 85:80BD SoundDriverStart
 * @spc 02C0
 * WHAT IT DOES: The sound driver's start and main loop: clears its RAM,
 * sets up the DSP and timer 0, then forever takes the S-CPU's commands
 * and, on each timer tick, steps the music and sound-effect voices.
 * HOW: Clears direct pages 0 and 1 and $0200-$02BF; DspSetup; timer 0 at
 * $40, the timers on; then each loop polls the ports (PortCommandPoll) and
 * counts timer 0 into $B1; on a tick, with the return address $02F5
 * pushed, either the music half (every other tick, $B0 bit 7: each voice
 * with a track, VoiceTick, then the fade) or the effects half (the voices
 * $B8 marks), then the key-on and key-off writes; their last DSP write
 * returns to $02F5.
 * Entry: jumped to by the boot ROM after the upload. Exit: never returns.
 */
void abi_SoundDriverStart(void)
{
    spc.p = 0;  /* 02C0 clrp  */
    spc.x = spc_nz(0xCF);  /* 02C1 mov x,#$CF */
    spc.sp = spc.x;  /* 02C3 mov sp,x */
    spc.a = spc_nz(0x00);  /* 02C4 mov a,#$00 */
    spc.x = spc_nz(spc.a);  /* 02C6 mov x,a */
L_02C7:
    spc_wr(spc_dp(spc.x), spc.a); spc.x++;  /* 02C7 mov (x+),a */
    spc_cmp(spc.x, 0xE0);  /* 02C8 cmp x,#$E0 */
    if (!spc.z) goto L_02C7;  /* 02CA bne spc_02C7_0280C4 */
    spc.p = 1;  /* 02CC setp  */
    spc.x = spc_nz(spc.a);  /* 02CD mov x,a */
L_02CE:
    spc_wr(spc_dp(spc.x), spc.a); spc.x++;  /* 02CE mov (x+),a */
    spc_cmp(spc.x, 0xB0);  /* 02CF cmp x,#$B0 */
    if (!spc.z) goto L_02CE;  /* 02D1 bne spc_02CE_0280CB */
    spc.p = 0;  /* 02D3 clrp  */
    spc.x = spc_nz(spc.a);  /* 02D4 mov x,a */
L_02D5:
    spc_st((uint16_t)(0x0200 + spc.x), spc.a);  /* 02D5 mov.w $0200+x,a */
    spc.x = spc_inc(spc.x);  /* 02D8 inc x */
    spc_cmp(spc.x, 0xC0);  /* 02D9 cmp x,#$C0 */
    if (!spc.z) goto L_02D5;  /* 02DB bne spc_02D5_0280D2 */
    spc_call(abi_DspSetup, 0x02E0);  /* 02DD call $0425 */
    spc.a = spc_nz(0x40);  /* 02E0 mov a,#$40 */
    spc_st(0x00FAU, spc.a);  /* 02E2 mov.w $00FA,a */
    spc.a = spc_nz(0xF1);  /* 02E5 mov a,#$F1 */
    spc_st(0x00F1U, spc.a);  /* 02E7 mov.w $00F1,a */
    spc.a = spc_nz(0x01);  /* 02EA mov a,#$01 */
    spc_st(spc_dp(0xB2), spc.a);  /* 02EC mov.b $B2,a */
    spc_st(0x00F6U, spc.a);  /* 02EE mov.w $00F6,a */
    spc.a = spc_dec(spc.a);  /* 02F1 dec a */
    spc_st(0x00F7U, spc.a);  /* 02F2 mov.w $00F7,a */
L_02F5:
    spc_call(abi_PortCommandPoll, 0x02F8);  /* 02F5 call $043E */
    spc.a = spc_nz(spc_rd(0x00FDU));  /* 02F8 mov.w a,$00FD */
    spc.c = 0;  /* 02FB clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0xB1)));  /* 02FC adc.b a,$B1 */
    spc_st(spc_dp(0xB1), spc.a);  /* 02FE mov.b $B1,a */
    if (spc.z) goto L_02F5;  /* 0300 beq spc_02F5_0280F2 */
    spc_rmw(spc_dp(0xB1), spc_dec);  /* 0302 dec.b $B1 */
    spc.y = spc_nz(0x02);  /* 0304 mov y,#$02 */
    spc.a = spc_nz(0xF5);  /* 0306 mov a,#$F5 */
    spc_push(spc.y);  /* 0308 push y */
    spc_push(spc.a);  /* 0309 push a */
    spc.c = 0;  /* 030A clrc  */
    { uint8_t d = spc_rd(spc_dp(0xB0)); spc_wr(spc_dp(0xB0), spc_adc(d, 0x80)); }  /* 030B adc $B0,#$80 */
    if (!spc.n) goto L_0313;  /* 030E bpl spc_0313_028110 */
    goto L_0399;  /* 0310 jmp $0399 */
L_0313:
    { uint8_t s = spc_rd(spc_dp(0xCD)); uint8_t d = spc_rd(spc_dp(0xCC)); spc_cmp(d, s); }  /* 0313 cmp $CC,$CD */
    if (spc.z) goto L_032D;  /* 0316 beq spc_032D_02812A */
    if ((spc_rd(spc_dp(0xCD)) >> 7 & 1)) goto L_0329;  /* 0318 bbs $CD.7,spc_0329_028126 */
    spc.y = spc_nz(0x6C);  /* 031B mov y,#$6C */
    spc.a = spc_nz(0x00);  /* 031D mov a,#$00 */
    spc_call(abi_DspWrite, 0x0322);  /* 031F call $0CBB */
    spc.y = spc_nz(0x4D);  /* 0322 mov y,#$4D */
    spc.a = spc_nz(0xFF);  /* 0324 mov a,#$FF */
    spc_call(abi_DspWrite, 0x0329);  /* 0326 call $0CBB */
L_0329:
    spc_rmw(spc_dp(0xCD), spc_inc);  /* 0329 inc.b $CD */
    goto L_0348;  /* 032B bra spc_0348_028145 */
L_032D:
    spc.y = spc_nz(0x4D);  /* 032D mov y,#$4D */
    spc.a = spc_nz(spc_rd(spc_dp(0xB8)));  /* 032F mov.b a,$B8 */
    spc.a = spc_eor(spc.a, 0xFF);  /* 0331 eor a,#$FF */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0xBC)));  /* 0333 and.b a,$BC */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xB7)));  /* 0335 or.b a,$B7 */
    spc_call(abi_DspWrite, 0x033A);  /* 0337 call $0CBB */
    spc.y = spc_nz(0x2C);  /* 033A mov y,#$2C */
    spc.a = spc_nz(spc_rd(spc_dp(0xCE)));  /* 033C mov.b a,$CE */
    spc_call(abi_DspWrite, 0x0341);  /* 033E call $0CBB */
    spc.y = spc_nz(0x3C);  /* 0341 mov y,#$3C */
    spc.a = spc_nz(spc_rd(spc_dp(0xCF)));  /* 0343 mov.b a,$CF */
    spc_call(abi_DspWrite, 0x0348);  /* 0345 call $0CBB */
L_0348:
    spc.x = spc_nz(0x07);  /* 0348 mov x,#$07 */
    spc.y = spc_nz(0x57);  /* 034A mov y,#$57 */
    spc.a = spc_nz(spc_rd(spc_dp(0xBA)));  /* 034C mov.b a,$BA */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xB5)));  /* 034E or.b a,$B5 */
    spc_push(spc.a);  /* 0350 push a */
L_0351:
    spc_push(spc.y);  /* 0351 push y */
    if (!(spc_rd(spc_dp(0xB5)) >> 7 & 1)) goto L_035F;  /* 0352 bbc $B5.7,spc_035F_02815C */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0238 + spc.y)));  /* 0355 mov.w a,$0238+y */
    spc_st(spc_dp(0xA3), spc.a);  /* 0358 mov.b $A3,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0240 + spc.y)));  /* 035A mov.w a,$0240+y */
    goto L_0375;  /* 035D bra spc_0375_028172 */
L_035F:
    if (!(spc_rd(spc_dp(0xBD)) >> 7 & 1)) goto L_036A;  /* 035F bbc $BD.7,spc_036A_028167 */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0250 + spc.x)));  /* 0362 mov.w a,$0250+x */
    spc_call(abi_VoiceInstrument, 0x0368);  /* 0365 call $08C4 */
    goto L_0381;  /* 0368 bra spc_0381_02817E */
L_036A:
    if (!(spc_rd(spc_dp(0xBA)) >> 7 & 1)) goto L_0381;  /* 036A bbc $BA.7,spc_0381_02817E */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0238 + spc.x)));  /* 036D mov.w a,$0238+x */
    spc_st(spc_dp(0xA3), spc.a);  /* 0370 mov.b $A3,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0240 + spc.x)));  /* 0372 mov.w a,$0240+x */
L_0375:
    spc.y = spc_nz(0x07);  /* 0375 mov y,#$07 */
    spc_call(abi_DspWriteVoice, 0x037A);  /* 0377 call $0CB0 */
    spc.y = spc_nz(0x05);  /* 037A mov y,#$05 */
    spc.a = spc_nz(spc_rd(spc_dp(0xA3)));  /* 037C mov.b a,$A3 */
    spc_call(abi_DspWriteVoice, 0x0381);  /* 037E call $0CB0 */
L_0381:
    spc_rmw(spc_dp(0xB5), spc_asl);  /* 0381 asl.b $B5 */
    spc_rmw(spc_dp(0xBD), spc_asl);  /* 0383 asl.b $BD */
    spc_rmw(spc_dp(0xBA), spc_asl);  /* 0385 asl.b $BA */
    spc.y = spc_pull();  /* 0387 pop y */
    spc.y = spc_dec(spc.y);  /* 0388 dec y */
    spc.x = spc_dec(spc.x);  /* 0389 dec x */
    if (!spc.n) goto L_0351;  /* 038A bpl spc_0351_02814E */
    spc.y = spc_nz(0x5C);  /* 038C mov y,#$5C */
    spc.a = spc_nz(0x00);  /* 038E mov a,#$00 */
    spc_call(abi_DspWrite, 0x0393);  /* 0390 call $0CBB */
    spc.a = spc_pull();  /* 0393 pop a */
    spc.y = spc_nz(0x4C);  /* 0394 mov y,#$4C */
    { abi_DspWrite(); goto L_02F5; }  /* 0396 jmp $0CBB */
L_0399:
    spc_bit_set(0xB0, 7, 0);  /* 0399 clr1 $B0.7 */
    if ((spc_rd(spc_dp(0xB0)) >> 6 & 1)) goto L_03D7;  /* 039B bbs $B0.6,spc_03D7_0281D4 */
    spc_st(spc_dp(0xB3), 0x80);  /* 039E mov $B3,#$80 */
    { uint8_t t = spc_rd(spc_dp(0xB8)); spc_wr(spc_dp(0xB9), t); }  /* 03A1 mov $B9,$B8 */
    spc.x = spc_nz(0x07);  /* 03A4 mov x,#$07 */
    spc_movw_ld(0xBE);  /* 03A6 movw ya,$BE */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xC0)) | spc_rd(spc_dp(0xC1)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 03A8 addw ya,$C0 */
    spc_movw_st(0xA9);  /* 03AA movw $A9,ya */
    spc.y = spc_nz(0x00);  /* 03AC mov y,#$00 */
    spc.a = spc_nz(spc_rd(spc_dp(0xC2)));  /* 03AE mov.b a,$C2 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA9)) | spc_rd(spc_dp(0xAA)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 03B0 addw ya,$A9 */
    spc_st(spc_dp(0xA8), spc.y);  /* 03B2 mov.b $A8,y */
    spc_st(spc_dp(0xC2), spc.a);  /* 03B4 mov.b $C2,a */
L_03B6:
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x08 + spc.x))));  /* 03B6 mov.b a,$08+x */
    spc.a = spc_or(spc.a, spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 03B8 or.b a,$00+x */
    if (spc.z) goto L_03BF;  /* 03BA beq spc_03BF_0281BC */
    spc_call(abi_VoiceTick, 0x03BF);  /* 03BC call $06B1 */
L_03BF:
    spc.x = spc_dec(spc.x);  /* 03BF dec x */
    spc_rmw(spc_dp(0xB9), spc_asl);  /* 03C0 asl.b $B9 */
    spc_rmw(spc_dp(0xB3), spc_lsr);  /* 03C2 lsr.b $B3 */
    if (!spc.z) goto L_03B6;  /* 03C4 bne spc_03B6_0281B3 */
    spc_movw_ld(0xC7);  /* 03C6 movw ya,$C7 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xC9)) | spc_rd(spc_dp(0xCA)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 03C8 addw ya,$C9 */
    if (!spc.c) goto L_03D5;  /* 03CA bcc spc_03D5_0281D2 */
    spc_st(spc_dp(0xC7), 0x00);  /* 03CC mov $C7,#$00 */
    spc_st(spc_dp(0xC8), 0x00);  /* 03CF mov $C8,#$00 */
    spc.y = spc_nz(0xFF);  /* 03D2 mov y,#$FF */
    spc.a = spc_nz(spc.y);  /* 03D4 mov a,y */
L_03D5:
    spc_movw_st(0xC9);  /* 03D5 movw $C9,ya */
L_03D7:
    spc_bit_set(0xB0, 7, 1);  /* 03D7 set1 $B0.7 */
    spc_st(spc_dp(0xB3), 0x80);  /* 03D9 mov $B3,#$80 */
    spc_st(spc_dp(0xB9), 0x00);  /* 03DC mov $B9,#$00 */
    spc.x = spc_nz(0x57);  /* 03DF mov x,#$57 */
L_03E1:
    spc.a = spc_nz(spc_rd(spc_dp(0xB8)));  /* 03E1 mov.b a,$B8 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0xB3)));  /* 03E3 and.b a,$B3 */
    if (spc.z) goto L_0400;  /* 03E5 beq spc_0400_0281FD */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0260 + spc.x)));  /* 03E7 mov.w a,$0260+x */
    spc_st(spc_dp(0xAA), spc.a);  /* 03EA mov.b $AA,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0258 + spc.x)));  /* 03EC mov.w a,$0258+x */
    spc_st(spc_dp(0xA9), spc.a);  /* 03EF mov.b $A9,a */
    spc.y = spc_nz(0x00);  /* 03F1 mov y,#$00 */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0268 + spc.x)));  /* 03F3 mov.w a,$0268+x */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA9)) | spc_rd(spc_dp(0xAA)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 03F6 addw ya,$A9 */
    spc_st(spc_dp(0xA8), spc.y);  /* 03F8 mov.b $A8,y */
    spc_st((uint16_t)(0x0268 + spc.x), spc.a);  /* 03FA mov.w $0268+x,a */
    spc_call(abi_VoiceTick, 0x0400);  /* 03FD call $06B1 */
L_0400:
    spc.x = spc_dec(spc.x);  /* 0400 dec x */
    spc_rmw(spc_dp(0xB3), spc_lsr);  /* 0401 lsr.b $B3 */
    if (!spc.z) goto L_03E1;  /* 0403 bne spc_03E1_0281DE */
    spc.a = spc_nz(spc_rd(spc_dp(0xB6)));  /* 0405 mov.b a,$B6 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xBB)));  /* 0407 or.b a,$BB */
    spc_st(spc_dp(0xB6), 0x00);  /* 0409 mov $B6,#$00 */
    spc_st(spc_dp(0xBB), 0x00);  /* 040C mov $BB,#$00 */
    spc.y = spc_nz(0x5C);  /* 040F mov y,#$5C */
    { abi_DspWrite(); goto L_02F5; }  /* 0411 jmp $0CBB */
}

/*
 * @routine 85:8222 DspSetup
 * @spc 0425
 * WHAT IT DOES: Sets the DSP's main volume and echo settings at start.
 * HOW: The DSP registers and values of the tables at $0438 (3 pairs) to
 * $F2/$F3, then the echo setup (its code from $09FA).
 * Entry: called by CALL. Exit: as the echo setup.
 */
void abi_DspSetup(void)
{
    spc.y = spc_nz(0x03);  /* 0425 mov y,#$03 */
L_0427:
    spc.a = spc_nz(spc_rd((uint16_t)(0x0437 + spc.y)));  /* 0427 mov.w a,$0437+y */
    spc_st(0x00F2U, spc.a);  /* 042A mov.w $00F2,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x043A + spc.y)));  /* 042D mov.w a,$043A+y */
    spc_st(0x00F3U, spc.a);  /* 0430 mov.w $00F3,a */
    if ((spc.y = (uint8_t)(spc.y - 1)) != 0) goto L_0427;  /* 0433 dbnz y,spc_0427_028224 */
    { abi_EchoSetup(); return; }  /* 0435 jmp $09FA */
}

/*
 * @routine 85:823B PortCommandPoll
 * @spc 043E
 * WHAT IT DOES: Takes the S-CPU's next command from port 0, if any, and
 * acknowledges it.
 * HOW: Reads port 0 until two reads agree; nonzero: counts $B2, runs it
 * (SoundCommand), clears the ports' inputs ($F1 = $31) and writes $B2 and
 * the command back to ports 2 and 3.
 * Entry: called by CALL. Exit: A = the command (or 0); Y = the command.
 */
void abi_PortCommandPoll(void)
{
L_043E:
    spc.a = spc_nz(spc_rd(0x00F4U));  /* 043E mov.w a,$00F4 */
    spc_cmp(spc.a, spc_rd(0x00F4U));  /* 0441 cmp.w a,$00F4 */
    if (!spc.z) goto L_043E;  /* 0444 bne spc_043E_02823B */
    spc.y = spc_nz(spc.a);  /* 0446 mov y,a */
    if (spc.z) { abi_PortCommandPollEnd(); return; }  /* 0447 beq spc_045D_02825A */
    spc_push(spc.a);  /* 0449 push a */
    spc_rmw(spc_dp(0xB2), spc_inc);  /* 044A inc.b $B2 */
    spc_call(abi_SoundCommand, 0x044F);  /* 044C call $045E */
    spc.a = spc_nz(0x31);  /* 044F mov a,#$31 */
    spc_st(0x00F1U, spc.a);  /* 0451 mov.w $00F1,a */
    spc.a = spc_nz(spc_rd(spc_dp(0xB2)));  /* 0454 mov.b a,$B2 */
    spc_st(0x00F6U, spc.a);  /* 0456 mov.w $00F6,a */
    spc.a = spc_pull();  /* 0459 pop a */
    spc_st(0x00F7U, spc.a);  /* 045A mov.w $00F7,a */
    abi_PortCommandPollEnd();  /* on into 045D */
}

void abi_PortCommandPollEnd(void)
{
    spc_ret(); return;  /* 045D ret  */
}

/*
 * @routine 85:825B SoundCommand
 * @spc 045E
 * WHAT IT DOES: Runs one S-CPU command: below $F0 starts song or sound
 * effect A, $F0 and up is a driver command.
 * HOW: $F0 and up: the command table at $0511 by the low nibble (JMP
 * (abs+X)). Else A, less the song count at $1DB0 until below it, indexes
 * the song table at $1DB2; a song's header gives its priority and each
 * voice's track; voices it may take (priority at or above theirs) are
 * cleared and given their track.
 * Entry: A = the command; called by CALL. Exit: as the command's code
 * leaves it.
 */
void abi_SoundCommand(void)
{
    spc_cmp(spc.a, 0xF0);  /* 045E cmp a,#$F0 */
    if (!spc.c) goto L_0465;  /* 0460 bcc spc_0465_028262 */
    { abi_SoundCommandTable(); return; }  /* 0462 jmp $050A */
L_0465:
    spc_cmp(spc.a, spc_rd(0x1DB0U));  /* 0465 cmp.w a,$1DB0 */
    if (!spc.c) goto L_0470;  /* 0468 bcc spc_0470_02826D */
    spc.c = 1;  /* 046A setc , not run by the traces */
    spc.a = spc_sbc(spc.a, spc_rd(0x1DB0U));  /* 046B sbc.w a,$1DB0, not run by the traces */
    goto L_0465;  /* 046E bra $0465, not run by the traces */
L_0470:
    spc.a = spc_asl(spc.a);  /* 0470 asl a */
    spc.x = spc_nz(spc.a);  /* 0471 mov x,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x1DB2 + spc.x)));  /* 0472 mov.w a,$1DB2+x */
    spc_st(spc_dp(0xA1), spc.a);  /* 0475 mov.b $A1,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x1DB3 + spc.x)));  /* 0477 mov.w a,$1DB3+x */
    spc_st(spc_dp(0xA0), spc.a);  /* 047A mov.b $A0,a */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xA1)));  /* 047C or.b a,$A1 */
    if (spc.z) { abi_PortCommandPollEnd(); return; }  /* 047E beq spc_045D_02825A */
    spc.y = spc_nz(0x00);  /* 0480 mov y,#$00 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 0482 mov a,($A0)+y */
    if (!spc.z) goto L_0489;  /* 0484 bne spc_0489_028286 */
    { abi_SongStartKeep(); return; }  /* 0486 jmp $0570, not run by the traces */
L_0489:
    spc_st(spc_dp(0xA4), spc.a);  /* 0489 mov.b $A4,a */
    spc_st(spc_dp(0xB3), 0x80);  /* 048B mov $B3,#$80 */
    spc.x = spc_nz(0x57);  /* 048E mov x,#$57 */
L_0490:
    spc.y = spc_nz(0x00);  /* 0490 mov y,#$00 */
    spc_incw(0xA0, 1);  /* 0492 incw $A0 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 0494 mov a,($A0)+y */
    spc_st(spc_dp(0xA3), spc.a);  /* 0496 mov.b $A3,a */
    spc_incw(0xA0, 1);  /* 0498 incw $A0 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 049A mov a,($A0)+y */
    spc_st(spc_dp(0xA2), spc.a);  /* 049C mov.b $A2,a */
    spc.a = spc_nz(spc_rd(ind_y(0xA2)));  /* 049E mov a,($A2)+y */
    spc_cmp(spc.a, 0x17);  /* 04A0 cmp a,#$17 */
    if (spc.z) goto L_04DE;  /* 04A2 beq spc_04DE_0282DB */
    spc.a = spc_nz(spc_rd(spc_dp(0xA4)));  /* 04A4 mov.b a,$A4 */
    spc_cmp(spc.a, spc_rd((uint16_t)(0x0250 + spc.x)));  /* 04A6 cmp.w a,$0250+x */
    if (!spc.c) goto L_04DE;  /* 04A9 bcc spc_04DE_0282DB */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xB8)); spc_wr(spc_dp(0xB8), spc_or(d, s)); }  /* 04AB or $B8,$B3 */
    spc.a = spc_nz(spc_rd(spc_dp(0xB3)));  /* 04AE mov.b a,$B3 */
    spc.a = spc_eor(spc.a, 0xFF);  /* 04B0 eor a,#$FF */
    spc_st(spc_dp(0xA5), spc.a);  /* 04B2 mov.b $A5,a */
    { uint8_t s = spc_rd(spc_dp(0xA5)); uint8_t d = spc_rd(spc_dp(0xB7)); spc_wr(spc_dp(0xB7), spc_and(d, s)); }  /* 04B4 and $B7,$A5 */
    { uint8_t s = spc_rd(spc_dp(0xA5)); uint8_t d = spc_rd(spc_dp(0xBA)); spc_wr(spc_dp(0xBA), spc_and(d, s)); }  /* 04B7 and $BA,$A5 */
    spc_call(abi_VoiceStateClear, 0x04BD);  /* 04BA call $04E4 */
    spc_call(abi_VoiceRelease, 0x04C0);  /* 04BD call $0414 */
    spc.a = spc_nz(spc_rd(spc_dp(0xA4)));  /* 04C0 mov.b a,$A4 */
    spc_st((uint16_t)(0x0250 + spc.x), spc.a);  /* 04C2 mov.w $0250+x,a */
    spc_movw_ld(0xA2);  /* 04C5 movw ya,$A2 */
    spc_st(spc_dp((uint8_t)(0x08 + spc.x)), spc.y);  /* 04C7 mov.b $08+x,y */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 04C9 mov.b $00+x,a */
    spc.a = spc_nz(0x10);  /* 04CB mov a,#$10 */
    spc_st((uint16_t)(0x0100 + spc.x), spc.a);  /* 04CD mov.w $0100+x,a */
    spc.a = spc_nz(0xB8);  /* 04D0 mov a,#$B8 */
    spc_st((uint16_t)(0x0240 + spc.x), spc.a);  /* 04D2 mov.w $0240+x,a */
    spc.a = spc_nz(spc_rd(0x00F5U));  /* 04D5 mov.w a,$00F5 */
    spc.c = 0;  /* 04D8 clrc  */
    spc.a = spc_adc(spc.a, 0x80);  /* 04D9 adc a,#$80 */
    spc_st((uint16_t)(0x0248 + spc.x), spc.a);  /* 04DB mov.w $0248+x,a */
L_04DE:
    spc.x = spc_dec(spc.x);  /* 04DE dec x */
    spc_rmw(spc_dp(0xB3), spc_lsr);  /* 04DF lsr.b $B3 */
    if (!spc.z) goto L_0490;  /* 04E1 bne spc_0490_02828D */
    spc_ret(); return;  /* 04E3 ret  */
}

/*
 * @routine 85:82E1 VoiceStateClear
 * @spc 04E4
 * WHAT IT DOES: Clears voice X's sequencer state.
 * HOW: Every eighth byte of $0200-$02BF from $0208 plus X; then, in page 1
 * and page 0 (VoiceDpClear), every eighth byte from X.
 * Entry: X = the voice; called by CALL. Exit: A = $A0 or more, flags from
 * the compare; X unchanged; P clear.
 */
void abi_VoiceStateClear(void)
{
    spc.a = spc_nz(spc.x);  /* 04E4 mov a,x */
    spc.c = 0;  /* 04E5 clrc  */
    spc.a = spc_adc(spc.a, 0x08);  /* 04E6 adc a,#$08 */
L_04E8:
    spc.y = spc_nz(spc.a);  /* 04E8 mov y,a */
    spc.a = spc_nz(0x00);  /* 04E9 mov a,#$00 */
    spc_st((uint16_t)(0x0200 + spc.y), spc.a);  /* 04EB mov.w $0200+y,a */
    spc.a = spc_nz(spc.y);  /* 04EE mov a,y */
    spc.c = 0;  /* 04EF clrc  */
    spc.a = spc_adc(spc.a, 0x08);  /* 04F0 adc a,#$08 */
    spc_cmp(spc.a, 0xC0);  /* 04F2 cmp a,#$C0 */
    if (!spc.c) goto L_04E8;  /* 04F4 bcc spc_04E8_0282E5 */
    spc.p = 1;  /* 04F6 setp  */
    spc_call(abi_VoiceDpClear, 0x04FA);  /* 04F7 call $04FB */
    spc.p = 0;  /* 04FA clrp  */
    abi_VoiceDpClear();  /* on into 04FB */
}

/*
 * @routine 85:82F8 VoiceDpClear
 * @spc 04FB
 * WHAT IT DOES: Clears every eighth byte of the direct page from X below
 * $A0: voice X's direct-page state.
 * HOW: A loop of MOV (X),A stepping X by 8; X kept on the stack.
 * Entry: X = the voice; called by CALL. Exit: A = the last X stepped; X
 * unchanged.
 */
void abi_VoiceDpClear(void)
{
    spc_push(spc.x);  /* 04FB push x */
L_04FC:
    spc.a = spc_nz(0x00);  /* 04FC mov a,#$00 */
    spc_st(spc_dp(spc.x), spc.a);  /* 04FE mov (x),a */
    spc.a = spc_nz(spc.x);  /* 04FF mov a,x */
    spc.c = 0;  /* 0500 clrc  */
    spc.a = spc_adc(spc.a, 0x08);  /* 0501 adc a,#$08 */
    spc.x = spc_nz(spc.a);  /* 0503 mov x,a */
    spc_cmp(spc.x, 0xA0);  /* 0504 cmp x,#$A0 */
    if (!spc.c) goto L_04FC;  /* 0506 bcc spc_04FC_0282F9 */
    spc.x = spc_pull();  /* 0508 pop x */
    spc_ret(); return;  /* 0509 ret  */
}

void abi_SoundCommandTable(void)
{
    spc.a = spc_and(spc.a, 0x0F);  /* 050A and a,#$0F */
    spc.a = spc_asl(spc.a);  /* 050C asl a */
    spc.x = spc_nz(spc.a);  /* 050D mov x,a */
    { uint16_t t = spc_rd((uint16_t)(0x0511 + spc.x)); t |= (uint16_t)(spc_rd((uint16_t)(0x0512 + spc.x)) << 8); switch (t) { case 0x052D: abi_SoundStopAll(); return; case 0x0530: abi_SoundCmdF1(); return; case 0x0549: abi_SequencerReset(); return; case 0x05DC: abi_SoundCmdF3(); return; case 0x05E0: abi_SoundCmdF4(); return; case 0x0616: abi_SoundCmdF5(); return; case 0x061A: abi_SoundCmdF6(); return; case 0x0650: abi_SoundCmdF7(); return; case 0x0653: abi_SoundCmdF8(); return; case 0x0656: abi_SoundUpload(); return; case 0x0567: abi_SoundCmdFA(); return; case 0x056A: abi_SoundCmdFB(); return; case 0x05B1: abi_SoundCmdFC(); return; case 0x05D6: abi_SoundCmdFD(); return; } }  /* 050E jmp ($0511+x) */
}

/*
 * @routine 85:832A SoundStopAll
 * @spc 052D
 * WHAT IT DOES: Command $F0: stops the music and every sound effect.
 * HOW: SequencerReset; then each voice $B8 marks is stopped (VoiceStop);
 * $B5 and $B7 cleared.
 * Entry: reached from the command table or by CALL. Exit: X = the last
 * voice; A = 0.
 */
void abi_SoundStopAll(void)
{
    spc_call(abi_SequencerReset, 0x0530);  /* 052D call $0549 */
    abi_SoundCmdF1();  /* on into 0530 */
}

void abi_SoundCmdF1(void)
{
    spc_st(spc_dp(0xB3), 0x80);  /* 0530 mov $B3,#$80 */
    spc.x = spc_nz(0x57);  /* 0533 mov x,#$57 */
L_0535:
    if (!(spc_rd(spc_dp(0xB8)) >> 7 & 1)) goto L_053B;  /* 0535 bbc $B8.7,spc_053B_028338 */
    spc_call(abi_VoiceStop, 0x053B);  /* 0538 call $099C */
L_053B:
    spc.x = spc_dec(spc.x);  /* 053B dec x */
    spc_rmw(spc_dp(0xB8), spc_asl);  /* 053C asl.b $B8 */
    spc_rmw(spc_dp(0xB3), spc_lsr);  /* 053E lsr.b $B3 */
    if (!spc.z) goto L_0535;  /* 0540 bne spc_0535_028332 */
    spc_st(spc_dp(0xB5), 0x00);  /* 0542 mov $B5,#$00 */
    spc_st(spc_dp(0xB7), 0x00);  /* 0545 mov $B7,#$00 */
    spc_ret(); return;  /* 0548 ret  */
}

/*
 * @routine 85:8346 SequencerReset
 * @spc 0549
 * WHAT IT DOES: Command $F2: the music's state cleared and its voices
 * released.
 * HOW: VoicesReleaseFree; $BC and $0208-$0257 cleared; page 1's first $50
 * bytes and page 0's (PageClear50).
 * Entry: reached from the command table or by CALL. Exit: A = 0; X = $50;
 * P clear.
 */
void abi_SequencerReset(void)
{
    spc_call(abi_VoicesReleaseFree, 0x054C);  /* 0549 call $05F9 */
    spc.a = spc_nz(0x00);  /* 054C mov a,#$00 */
    spc_st(spc_dp(0xBC), spc.a);  /* 054E mov.b $BC,a */
    spc.x = spc_nz(0x08);  /* 0550 mov x,#$08 */
L_0552:
    spc_st((uint16_t)(0x0200 + spc.x), spc.a);  /* 0552 mov.w $0200+x,a */
    spc.x = spc_inc(spc.x);  /* 0555 inc x */
    spc_cmp(spc.x, 0x58);  /* 0556 cmp x,#$58 */
    if (!spc.z) goto L_0552;  /* 0558 bne spc_0552_02834F */
    spc.p = 1;  /* 055A setp  */
    spc_call(abi_PageClear50, 0x055E);  /* 055B call $055F */
    spc.p = 0;  /* 055E clrp  */
    abi_PageClear50();  /* on into 055F */
}

/*
 * @routine 85:835C PageClear50
 * @spc 055F
 * WHAT IT DOES: Clears the first $50 bytes of the direct page.
 * HOW: MOV (X+),A from X = 0.
 * Entry: A = 0; called by CALL. Exit: X = $50; flags from the compare.
 */
void abi_PageClear50(void)
{
    spc.x = spc_nz(0x00);  /* 055F mov x,#$00 */
L_0561:
    spc_wr(spc_dp(spc.x), spc.a); spc.x++;  /* 0561 mov (x+),a */
    spc_cmp(spc.x, 0x50);  /* 0562 cmp x,#$50 */
    if (!spc.z) goto L_0561;  /* 0564 bne spc_0561_02835E */
    spc_ret(); return;  /* 0566 ret  */
}

void abi_SoundCmdFA(void)
{
    spc_call(abi_SoundUpload, 0x056A);  /* 0567 call $0656 */
    abi_SoundCmdFB();  /* on into 056A */
}

void abi_SoundCmdFB(void)
{
    spc_st(spc_dp(0xA1), 0x0D);  /* 056A mov $A1,#$0D */
    spc_st(spc_dp(0xA0), 0x1F);  /* 056D mov $A0,#$1F */
    abi_SongStartKeep();  /* on into 0570 */
}

void abi_SongStartKeep(void)
{
    spc_call(abi_SequencerReset, 0x0573);  /* 0570 call $0549 */
    spc.y = spc_nz(0x00);  /* 0573 mov y,#$00 */
    spc.a = spc_nz(spc.y);  /* 0575 mov a,y */
    spc_movw_st(0xC0);  /* 0576 movw $C0,ya */
    spc_st(spc_dp(0xC4), spc.a);  /* 0578 mov.b $C4,a */
    spc_st(spc_dp(0xC5), spc.a);  /* 057A mov.b $C5,a */
    spc.a = spc_dec(spc.a);  /* 057C dec a */
    spc_st(spc_dp(0xC3), spc.a);  /* 057D mov.b $C3,a */
    spc_call(abi_FadeClear, 0x0582);  /* 057F call $0645 */
    spc.x = spc_nz(0x07);  /* 0582 mov x,#$07 */
L_0584:
    spc.y = spc_nz(0x00);  /* 0584 mov y,#$00 */
    spc_incw(0xA0, 1);  /* 0586 incw $A0 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 0588 mov a,($A0)+y */
    spc_st(spc_dp(0xA3), spc.a);  /* 058A mov.b $A3,a */
    spc_incw(0xA0, 1);  /* 058C incw $A0 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 058E mov a,($A0)+y */
    spc_st(spc_dp(0xA2), spc.a);  /* 0590 mov.b $A2,a */
    spc.a = spc_nz(spc_rd(ind_y(0xA2)));  /* 0592 mov a,($A2)+y */
    spc_cmp(spc.a, 0x17);  /* 0594 cmp a,#$17 */
    if (spc.z) goto L_05AD;  /* 0596 beq spc_05AD_0283AA */
    spc_movw_ld(0xA2);  /* 0598 movw ya,$A2 */
    spc_st(spc_dp((uint8_t)(0x08 + spc.x)), spc.y);  /* 059A mov.b $08+x,y */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 059C mov.b $00+x,a */
    spc.a = spc_nz(0x10);  /* 059E mov a,#$10 */
    spc_st((uint16_t)(0x0100 + spc.x), spc.a);  /* 05A0 mov.w $0100+x,a */
    spc.a = spc_nz(0xB8);  /* 05A3 mov a,#$B8 */
    spc_st((uint16_t)(0x0240 + spc.x), spc.a);  /* 05A5 mov.w $0240+x,a */
    spc.a = spc_nz(0x80);  /* 05A8 mov a,#$80 */
    spc_st((uint16_t)(0x0248 + spc.x), spc.a);  /* 05AA mov.w $0248+x,a */
L_05AD:
    spc.x = spc_dec(spc.x);  /* 05AD dec x */
    if (!spc.n) goto L_0584;  /* 05AE bpl spc_0584_028381 */
    spc_ret(); return;  /* 05B0 ret  */
}

/*
 * @routine 85:83AE SoundCmdFC
 * @spc 05B1
 * WHAT IT DOES: Command $FC: sets the music's tempo offset from port 1, a
 * signed value scaled by 3 x $69 / 256 into $C0-$C1.
 * HOW: Port 1's magnitude times $69 (high byte kept at $A2) plus the
 * magnitude times 3; negated (0 less it) when port 1 was negative. Port 1
 * is written by the S-CPU only in SpcUpload, which ends every upload by
 * writing 0 or 1 there, so the negative branch ($05B7, $05CC) is code the
 * retail game never runs.
 * Entry: reached from the command table (SoundCommand). Exit: YA = the
 * offset, also at $C0; $A3 = 0.
 */
void abi_SoundCmdFC(void)
{
    spc.a = spc_nz(spc_rd(0x00F5U));  /* 05B1 mov.w a,$00F5, not run by the traces */
    spc_push(spc_psw());  /* 05B4 push p, not run by the traces */
    if (!spc.n) goto L_05BA;  /* 05B5 bpl $05BA, not run by the traces */
    spc.a = spc_eor(spc.a, 0xFF);  /* 05B7 eor a,#$FF, not run by the traces */
    spc.a = spc_inc(spc.a);  /* 05B9 inc a, not run by the traces */
L_05BA:
    spc_push(spc.a);  /* 05BA push a, not run by the traces */
    spc.y = spc_nz(0x69);  /* 05BB mov y,#$69, not run by the traces */
    spc_mul();  /* 05BD mul ya, not run by the traces */
    spc_st(spc_dp(0xA3), 0x00);  /* 05BE mov $A3,#$00, not run by the traces */
    spc_st(spc_dp(0xA2), spc.y);  /* 05C1 mov.b $A2,y, not run by the traces */
    spc.a = spc_pull();  /* 05C3 pop a, not run by the traces */
    spc.y = spc_nz(0x03);  /* 05C4 mov y,#$03, not run by the traces */
    spc_mul();  /* 05C6 mul ya, not run by the traces */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 05C7 addw ya,$A2, not run by the traces */
    spc_set_psw(spc_pull());  /* 05C9 pop p, not run by the traces */
    if (!spc.n) goto L_05D3;  /* 05CA bpl $05D3, not run by the traces */
    spc_movw_st(0xA2);  /* 05CC movw $A2,ya, not run by the traces */
    spc.y = spc_nz(0x00);  /* 05CE mov y,#$00, not run by the traces */
    spc.a = spc_nz(spc.y);  /* 05D0 mov a,y, not run by the traces */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 05D1 subw ya,$A2, not run by the traces */
L_05D3:
    spc_movw_st(0xC0);  /* 05D3 movw $C0,ya, not run by the traces */
    spc_ret(); return;  /* 05D5 ret , not run by the traces */
}

void abi_SoundCmdFD(void)
{
    spc.a = spc_nz(spc_rd(0x00F5U));  /* 05D6 mov.w a,$00F5, not run by the traces */
    spc_st(spc_dp(0xC5), spc.a);  /* 05D9 mov.b $C5,a, not run by the traces */
    spc_ret(); return;  /* 05DB ret , not run by the traces */
}

void abi_SoundCmdF3(void)
{
    spc_bit_set(0xB0, 6, 1);  /* 05DC set1 $B0.6 */
    { abi_VoicesReleaseFree(); return; }  /* 05DE bra spc_05F9_0283F6 */
}

void abi_SoundCmdF4(void)
{
    spc_bit_set(0xB0, 6, 0);  /* 05E0 clr1 $B0.6 */
    spc_st(spc_dp(0xBA), 0x00);  /* 05E2 mov $BA,#$00 */
    spc_st(spc_dp(0xB3), 0x80);  /* 05E5 mov $B3,#$80 */
    spc.x = spc_nz(0x57);  /* 05E8 mov x,#$57 */
L_05EA:
    spc.a = spc_nz(spc_rd(spc_dp(0xB8)));  /* 05EA mov.b a,$B8 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0xB3)));  /* 05EC and.b a,$B3 */
    if (!spc.z) goto L_05F3;  /* 05EE bne spc_05F3_0283F0 */
    spc_call(abi_VoiceSfxMark, 0x05F3);  /* 05F0 call $09A7 */
L_05F3:
    spc.x = spc_dec(spc.x);  /* 05F3 dec x */
    spc_rmw(spc_dp(0xB3), spc_lsr);  /* 05F4 lsr.b $B3 */
    if (!spc.z) goto L_05EA;  /* 05F6 bne spc_05EA_0283E7 */
    spc_ret(); return;  /* 05F8 ret  */
}

/*
 * @routine 85:83F6 VoicesReleaseFree
 * @spc 05F9
 * WHAT IT DOES: Releases every voice no sound effect holds.
 * HOW: For voices 7 down to 0 whose bit is clear in $B8: VoiceRelease and
 * its bit set in $BB; $BA and $BD cleared.
 * Entry: called by CALL. Exit: X = $FF; A as the last release leaves it.
 */
void abi_VoicesReleaseFree(void)
{
    spc.x = spc_nz(0x07);  /* 05F9 mov x,#$07 */
    spc_st(spc_dp(0xB3), 0x80);  /* 05FB mov $B3,#$80 */
L_05FE:
    spc.a = spc_nz(spc_rd(spc_dp(0xB8)));  /* 05FE mov.b a,$B8 */
    spc.a = spc_and(spc.a, spc_rd(spc_dp(0xB3)));  /* 0600 and.b a,$B3 */
    if (!spc.z) goto L_060A;  /* 0602 bne spc_060A_028407 */
    spc_call(abi_VoiceRelease, 0x0607);  /* 0604 call $0414 */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xBB)); spc_wr(spc_dp(0xBB), spc_or(d, s)); }  /* 0607 or $BB,$B3 */
L_060A:
    spc.x = spc_dec(spc.x);  /* 060A dec x */
    spc_rmw(spc_dp(0xB3), spc_lsr);  /* 060B lsr.b $B3 */
    if (!spc.z) goto L_05FE;  /* 060D bne spc_05FE_0283FB */
    spc_st(spc_dp(0xBA), 0x00);  /* 060F mov $BA,#$00 */
    spc_st(spc_dp(0xBD), 0x00);  /* 0612 mov $BD,#$00 */
    spc_ret(); return;  /* 0615 ret  */
}

void abi_SoundCmdF5(void)
{
    spc.x = spc_nz(0xFF);  /* 0616 mov x,#$FF, not run by the traces */
    { abi_FadeStart(); return; }  /* 0618 bra $061C, not run by the traces */
}

void abi_SoundCmdF6(void)
{
    spc.x = spc_nz(0x01);  /* 061A mov x,#$01 */
    abi_FadeStart();  /* on into 061C */
}

void abi_FadeStart(void)
{
    spc_cmp(spc.x, spc_rd(spc_dp(0xC6)));  /* 061C cmp.b x,$C6 */
    if (spc.z) goto L_063C;  /* 061E beq spc_063C_028439 */
    spc.a = spc_nz(spc_rd(0x00F6U));  /* 0620 mov.w a,$00F6 */
    spc.a = spc_or(spc.a, spc_rd(0x00F7U));  /* 0623 or.w a,$00F7 */
    if (spc.z) { abi_FadeClear(); return; }  /* 0626 beq spc_0645_028442 */
    spc.a = spc_nz(spc_rd(0x00F6U));  /* 0628 mov.w a,$00F6 */
    spc.y = spc_nz(spc_rd(0x00F7U));  /* 062B mov.w y,$00F7 */
    spc_rmw(spc_dp(0xC6), spc_asl);  /* 062E asl.b $C6 */
    if (spc.z) goto L_063D;  /* 0630 beq spc_063D_02843A */
    spc_call(abi_FadeSet, 0x0635);  /* 0632 call $064B, not run by the traces */
    spc.y = spc_nz(0xFF);  /* 0635 mov y,#$FF, not run by the traces */
    spc.a = spc_nz(spc.y);  /* 0637 mov a,y, not run by the traces */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xC9)) | spc_rd(spc_dp(0xCA)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 0638 subw ya,$C9, not run by the traces */
    spc_movw_st(0xC9);  /* 063A movw $C9,ya, not run by the traces */
L_063C:
    spc_ret(); return;  /* 063C ret  */
L_063D:
    spc_st(spc_dp(0xC9), 0x00);  /* 063D mov $C9,#$00 */
    spc_st(spc_dp(0xCA), 0x00);  /* 0640 mov $CA,#$00 */
    { abi_FadeSet(); return; }  /* 0643 bra spc_064B_028448 */
}

/*
 * @routine 85:8442 FadeClear
 * @spc 0645
 * WHAT IT DOES: Clears the fade: its rate and level ($C6-$CA).
 * HOW: YA = 0, X = 0; then on into FadeSet.
 * Entry: called by CALL or reached. Exit: A = Y = X = 0.
 */
void abi_FadeClear(void)
{
    spc.a = spc_nz(0x00);  /* 0645 mov a,#$00 */
    spc.y = spc_nz(spc.a);  /* 0647 mov y,a */
    spc.x = spc_nz(spc.a);  /* 0648 mov x,a */
    spc_movw_st(0xC9);  /* 0649 movw $C9,ya */
    abi_FadeSet();  /* on into 064B */
}

/*
 * @routine 85:8448 FadeSet
 * @spc 064B
 * WHAT IT DOES: Sets the fade level to YA and its step to X.
 * HOW: MOVW $C7,YA; $C6 = X.
 * Entry: YA = the level; X = the step. Exit: registers unchanged; flags
 * from the MOVW.
 */
void abi_FadeSet(void)
{
    spc_movw_st(0xC7);  /* 064B movw $C7,ya */
    spc_st(spc_dp(0xC6), spc.x);  /* 064D mov.b $C6,x */
    spc_ret(); return;  /* 064F ret  */
}

void abi_SoundCmdF7(void)
{
    spc_bit_set(0xB0, 5, 0);  /* 0650 clr1 $B0.5 */
    spc_ret(); return;  /* 0652 ret  */
}

void abi_SoundCmdF8(void)
{
    spc_bit_set(0xB0, 5, 1);  /* 0653 set1 $B0.5 */
    spc_ret(); return;  /* 0655 ret  */
}

/*
 * @routine 85:8453 SoundUpload
 * @spc 0656
 * WHAT IT DOES: Command $F9: stops all sound and takes a new upload from
 * the S-CPU (song and sample data) by the port handshake.
 * HOW: SoundStopAll; ports 0-1 = $AA, $BB; waits for $CC; then blocks:
 * port 2-3 the address, port 1 the data, port 0 the counter echoed; ends
 * when the S-CPU sends a zero counter, and the timer and sequence count
 * are reset.
 * Entry: reached from the command table. Exit: as its last port write
 * leaves it.
 */
void abi_SoundUpload(void)
{
    spc_call(abi_SoundStopAll, 0x0659);  /* 0656 call $052D */
    spc.a = spc_nz(0xAA);  /* 0659 mov a,#$AA */
    spc_st(0x00F4U, spc.a);  /* 065B mov.w $00F4,a */
    spc.a = spc_nz(0xBB);  /* 065E mov a,#$BB */
    spc_st(0x00F5U, spc.a);  /* 0660 mov.w $00F5,a */
L_0663:
    spc.a = spc_nz(spc_rd(0x00F4U));  /* 0663 mov.w a,$00F4 */
    spc_cmp(spc.a, 0xCC);  /* 0666 cmp a,#$CC */
    if (!spc.z) goto L_0663;  /* 0668 bne spc_0663_028460 */
    goto L_068C;  /* 066A bra spc_068C_028489 */
L_066C:
    spc.y = spc_nz(spc_rd(0x00F4U));  /* 066C mov.w y,$00F4 */
    if (!spc.z) goto L_066C;  /* 066F bne spc_066C_028469 */
L_0671:
    spc_cmp(spc.y, spc_rd(0x00F4U));  /* 0671 cmp.w y,$00F4 */
    if (!spc.z) goto L_0685;  /* 0674 bne spc_0685_028482 */
    spc.a = spc_nz(spc_rd(0x00F5U));  /* 0676 mov.w a,$00F5 */
    spc_st(0x00F4U, spc.y);  /* 0679 mov.w $00F4,y */
    spc_st(ind_y(0xA0), spc.a);  /* 067C mov ($A0)+y,a */
    spc.y = spc_inc(spc.y);  /* 067E inc y */
    if (!spc.z) goto L_0671;  /* 067F bne spc_0671_02846E */
    spc_rmw(spc_dp(0xA1), spc_inc);  /* 0681 inc.b $A1 */
    goto L_0671;  /* 0683 bra spc_0671_02846E */
L_0685:
    if (!spc.n) goto L_0671;  /* 0685 bpl spc_0671_02846E */
    spc_cmp(spc.y, spc_rd(0x00F4U));  /* 0687 cmp.w y,$00F4 */
    if (!spc.n) goto L_0671;  /* 068A bpl spc_0671_02846E */
L_068C:
    spc.a = spc_nz(spc_rd(0x00F6U));  /* 068C mov.w a,$00F6 */
    spc.y = spc_nz(spc_rd(0x00F7U));  /* 068F mov.w y,$00F7 */
    spc_movw_st(0xA0);  /* 0692 movw $A0,ya */
    spc.y = spc_nz(spc_rd(0x00F4U));  /* 0694 mov.w y,$00F4 */
    spc.a = spc_nz(spc_rd(0x00F5U));  /* 0697 mov.w a,$00F5 */
    if (spc.z) goto L_06A1;  /* 069A beq spc_06A1_02849E */
    spc_st(0x00F4U, spc.y);  /* 069C mov.w $00F4,y */
    goto L_066C;  /* 069F bra spc_066C_028469 */
L_06A1:
    spc_st(0x00F6U, spc.a);  /* 06A1 mov.w $00F6,a */
    spc.a = spc_inc(spc.a);  /* 06A4 inc a */
    spc_st(spc_dp(0xB2), spc.a);  /* 06A5 mov.b $B2,a */
    spc.a = spc_nz(spc_rd(0x00FDU));  /* 06A7 mov.w a,$00FD */
    spc_st(spc_dp(0xB1), 0x00);  /* 06AA mov $B1,#$00 */
    spc_st(0x00F4U, spc.y);  /* 06AD mov.w $00F4,y */
    spc_ret(); return;  /* 06B0 ret  */
}

/*
 * @routine 85:84AE VoiceTick
 * @spc 06B1
 * WHAT IT DOES: Steps voice X's track for one tick: its note's time, then
 * its commands and next note when the time runs out.
 * HOW: The note's length counted down by $A8; the release point
 * (VoiceAdsr1Set) when its gate ends; a slide (from $0AC3) while it runs;
 * at 0 the command loop: commands below $20 run from the handler table at
 * $0831, a byte of $20 or more is a note: its pitch, length and key-on.
 * Entry: X = the voice; $B3 its bit; $A8 the tick's length; called by
 * CALL. Exit: as the note or command leaves it.
 */
void abi_VoiceTick(void)
{
    { uint8_t t = spc_rd(spc_dp(0xB3)); spc_wr(spc_dp(0xB4), t); }  /* 06B1 mov $B4,$B3 */
    { uint8_t d = spc_rd(spc_dp(0xB4)); spc_wr(spc_dp(0xB4), spc_eor(d, 0xFF)); }  /* 06B4 eor $B4,#$FF */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x18 + spc.x))));  /* 06B7 mov.b a,$18+x */
    if (spc.z) goto L_06E9;  /* 06B9 beq spc_06E9_0284E6 */
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x28 + spc.x))));  /* 06BB mov.b y,$28+x */
    spc.a = spc_nz(spc.y);  /* 06BD mov a,y */
    spc.a = spc_or(spc.a, spc_rd(spc_dp((uint8_t)(0x20 + spc.x))));  /* 06BE or.b a,$20+x */
    if (spc.z) goto L_06D7;  /* 06C0 beq spc_06D7_0284D4 */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x20 + spc.x))));  /* 06C2 mov.b a,$20+x */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA9)) | spc_rd(spc_dp(0xAA)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 06C4 subw ya,$A9 */
    if (spc.z) goto L_06CA;  /* 06C6 beq spc_06CA_0284C7 */
    if (spc.c) goto L_06D3;  /* 06C8 bcs spc_06D3_0284D0 */
L_06CA:
    if ((spc_rd(spc_dp(0xB9)) >> 7 & 1)) goto L_06D0;  /* 06CA bbs $B9.7,spc_06D0_0284CD */
    spc_call(abi_VoiceAdsr1Set, 0x06D0);  /* 06CD call $041B */
L_06D0:
    spc.y = spc_nz(0x00);  /* 06D0 mov y,#$00 */
    spc.a = spc_nz(spc.y);  /* 06D2 mov a,y */
L_06D3:
    spc_st(spc_dp((uint8_t)(0x28 + spc.x)), spc.y);  /* 06D3 mov.b $28+x,y */
    spc_st(spc_dp((uint8_t)(0x20 + spc.x)), spc.a);  /* 06D5 mov.b $20+x,a */
L_06D7:
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x18 + spc.x))));  /* 06D7 mov.b a,$18+x */
    spc.c = 1;  /* 06D9 setc  */
    spc.a = spc_sbc(spc.a, spc_rd(spc_dp(0xA8)));  /* 06DA sbc.b a,$A8 */
    spc_st(spc_dp((uint8_t)(0x18 + spc.x)), spc.a);  /* 06DC mov.b $18+x,a */
    if (spc.z) goto L_06E9;  /* 06DE beq spc_06E9_0284E6 */
    if (!spc.c) goto L_06E9;  /* 06E0 bcc spc_06E9_0284E6 */
    spc_cmp(spc.a, 0xC1);  /* 06E2 cmp a,#$C1 */
    if (spc.c) goto L_06E9;  /* 06E4 bcs spc_06E9_0284E6 */
    { abi_VoiceSlide(); return; }  /* 06E6 jmp $0AC3 */
L_06E9:
    spc.y = spc_nz(0x06);  /* 06E9 mov y,#$06 */
    spc.a = spc_nz(0xF2);  /* 06EB mov a,#$F2 */
    spc_push(spc.y);  /* 06ED push y */
    spc_push(spc.a);  /* 06EE push a */
    goto L_0803;  /* 06EF jmp $0803 */
L_06F2:
    spc_push(spc.a);  /* 06F2 push a */
    spc.a = spc_xcn(spc.a);  /* 06F3 xcn a */
    spc.a = spc_lsr(spc.a);  /* 06F4 lsr a */
    spc.a = spc_and(spc.a, 0x07);  /* 06F5 and a,#$07 */
    spc.a = spc_dec(spc.a);  /* 06F7 dec a */
    spc.y = spc_nz(spc.a);  /* 06F8 mov y,a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 06F9 mov.b a,$10+x */
    spc.a = spc_and(spc.a, 0x30);  /* 06FB and a,#$30 */
    if (!spc.z) goto L_0704;  /* 06FD bne spc_0704_028501 */
    spc.a = spc_nz(spc_rd((uint16_t)(0x07E5 + spc.y)));  /* 06FF mov.w a,$07E5+y */
    goto L_0716;  /* 0702 bra spc_0716_028513 */
L_0704:
    spc.a = spc_and(spc.a, 0x10);  /* 0704 and a,#$10 */
    if (!spc.z) goto L_070D;  /* 0706 bne spc_070D_02850A */
    spc.a = spc_nz(spc_rd((uint16_t)(0x07DE + spc.y)));  /* 0708 mov.w a,$07DE+y */
    goto L_0716;  /* 070B bra spc_0716_028513 */
L_070D:
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 070D mov.b a,$10+x */
    spc.a = spc_and(spc.a, 0xEF);  /* 070F and a,#$EF */
    spc_st(spc_dp((uint8_t)(0x10 + spc.x)), spc.a);  /* 0711 mov.b $10+x,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x07EC + spc.y)));  /* 0713 mov.w a,$07EC+y */
L_0716:
    spc.c = 0;  /* 0716 clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp((uint8_t)(0x18 + spc.x))));  /* 0717 adc.b a,$18+x */
    spc_st(spc_dp((uint8_t)(0x18 + spc.x)), spc.a);  /* 0719 mov.b $18+x,a */
    spc.a = spc_pull();  /* 071B pop a */
    spc.a = spc_and(spc.a, 0x1F);  /* 071C and a,#$1F */
    if (!spc.z) goto L_0721;  /* 071E bne spc_0721_02851E */
    spc_ret(); return;  /* 0720 ret  */
L_0721:
    spc_push(spc.a);  /* 0721 push a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 0722 mov.b a,$10+x */
    spc.a = spc_and(spc.a, 0x0F);  /* 0724 and a,#$0F */
    spc.y = spc_nz(spc.a);  /* 0726 mov y,a */
    spc.a = spc_pull();  /* 0727 pop a */
    spc.c = 0;  /* 0728 clrc  */
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x07F3 + spc.y)));  /* 0729 adc.w a,$07F3+y */
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_0732;  /* 072C bbs $B0.7,spc_0732_02852F */
    spc.c = 0;  /* 072F clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0xC4)));  /* 0730 adc.b a,$C4 */
L_0732:
    spc.c = 0;  /* 0732 clrc  */
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0210 + spc.x)));  /* 0733 adc.w a,$0210+x */
    spc_st(spc_dp(0xA1), spc.a);  /* 0736 mov.b $A1,a */
    spc_st(spc_dp(0xA0), 0x00);  /* 0738 mov $A0,#$00 */
    spc.y = spc_nz(0x00);  /* 073B mov y,#$00 */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0218 + spc.x)));  /* 073D mov.w a,$0218+x */
    if (!spc.n) goto L_0743;  /* 0740 bpl spc_0743_028540 */
    spc.y = spc_dec(spc.y);  /* 0742 dec y */
L_0743:
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0743 addw ya,$A0 */
    spc_cmp(spc.y, 0x61);  /* 0745 cmp y,#$61 */
    if (!spc.c) goto L_074B;  /* 0747 bcc spc_074B_028548 */
    spc.y = spc_nz(0x60);  /* 0749 mov y,#$60 */
L_074B:
    spc.p = 1;  /* 074B setp  */
    spc_movw_st(0xA2);  /* 074C movw $A2,ya */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 074E mov.b a,$00+x */
    spc_st(spc_dp(0xA4), spc.a);  /* 0750 mov.b $A4,a */
    spc_bit_set(0xA4, 0, 0);  /* 0752 clr1 $A4.0 */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x28 + spc.x))));  /* 0754 mov.b a,$28+x */
    if (spc.z) goto L_0760;  /* 0756 beq spc_0760_02855D */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x40 + spc.x))));  /* 0758 mov.b a,$40+x */
    if (spc.z) goto L_0760;  /* 075A beq spc_0760_02855D */
    spc_cmp(spc.a, spc_rd(spc_dp(0xA3)));  /* 075C cmp.b a,$A3 */
    if (!spc.z) goto L_0764;  /* 075E bne spc_0764_028561 */
L_0760:
    spc.a = spc_nz(spc_rd(spc_dp(0xA2)));  /* 0760 mov.b a,$A2 */
    goto L_0772;  /* 0762 bra spc_0772_02856F */
L_0764:
    if (spc.c) goto L_076A;  /* 0764 bcs spc_076A_028567 */
    spc_bit_set(0xA4, 1, 1);  /* 0766 set1 $A4.1 */
    goto L_076C;  /* 0768 bra spc_076C_028569 */
L_076A:
    spc_bit_set(0xA4, 1, 0);  /* 076A clr1 $A4.1 */
L_076C:
    spc_rmw(spc_dp(0xA4), spc_inc);  /* 076C inc.b $A4 */
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x40 + spc.x))));  /* 076E mov.b y,$40+x */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x48 + spc.x))));  /* 0770 mov.b a,$48+x */
L_0772:
    spc_push(spc.a);  /* 0772 push a */
    spc.a = spc_nz(spc_rd(spc_dp(0xA4)));  /* 0773 mov.b a,$A4 */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 0775 mov.b $00+x,a */
    spc.a = spc_nz(spc_rd(spc_dp(0xA3)));  /* 0777 mov.b a,$A3 */
    spc_st(spc_dp((uint8_t)(0x40 + spc.x)), spc.a);  /* 0779 mov.b $40+x,a */
    spc.a = spc_nz(spc_rd(spc_dp(0xA2)));  /* 077B mov.b a,$A2 */
    spc_st(spc_dp((uint8_t)(0x48 + spc.x)), spc.a);  /* 077D mov.b $48+x,a */
    spc.a = spc_pull();  /* 077F pop a */
    spc_st(spc_dp((uint8_t)(0x30 + spc.x)), spc.y);  /* 0780 mov.b $30+x,y */
    spc_st(spc_dp((uint8_t)(0x38 + spc.x)), spc.a);  /* 0782 mov.b $38+x,a */
    spc.p = 0;  /* 0784 clrp  */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 0785 mov.b a,$10+x */
    if (spc.n) goto L_07B3;  /* 0787 bmi spc_07B3_0285B0 */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0100 + spc.x)));  /* 0789 mov.w a,$0100+x */
    spc.y = spc_nz(spc.a);  /* 078C mov y,a */
    spc.a = spc_and(spc.a, 0x10);  /* 078D and a,#$10 */
    if (spc.z) goto L_079C;  /* 078F beq spc_079C_028599 */
    spc.a = spc_nz(spc.y);  /* 0791 mov a,y */
    spc.a = spc_and(spc.a, 0x3F);  /* 0792 and a,#$3F */
    spc_st((uint16_t)(0x0100 + spc.x), spc.a);  /* 0794 mov.w $0100+x,a */
    spc.a = spc_nz(0x00);  /* 0797 mov a,#$00 */
    spc_st((uint16_t)(0x0120 + spc.x), spc.a);  /* 0799 mov.w $0120+x,a */
L_079C:
    if (!(spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_07A7;  /* 079C bbc $B0.7,spc_07A7_0285A4 */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xB5)); spc_wr(spc_dp(0xB5), spc_or(d, s)); }  /* 079F or $B5,$B3 */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xB6)); spc_wr(spc_dp(0xB6), spc_or(d, s)); }  /* 07A2 or $B6,$B3 */
    goto L_07B0;  /* 07A5 bra spc_07B0_0285AD */
L_07A7:
    if ((spc_rd(spc_dp(0xB9)) >> 7 & 1)) goto L_07BD;  /* 07A7 bbs $B9.7,spc_07BD_0285BA */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xBA)); spc_wr(spc_dp(0xBA), spc_or(d, s)); }  /* 07AA or $BA,$B3 */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xBB)); spc_wr(spc_dp(0xBB), spc_or(d, s)); }  /* 07AD or $BB,$B3 */
L_07B0:
    spc_call(abi_VoiceRelease, 0x07B3);  /* 07B0 call $0414 */
L_07B3:
    if ((spc_rd(spc_dp(0xB9)) >> 7 & 1)) goto L_07BD;  /* 07B3 bbs $B9.7,spc_07BD_0285BA */
    spc.p = 1;  /* 07B6 setp  */
    spc_call(abi_VoicePitchSet, 0x07BA);  /* 07B7 call $0BE9 */
    spc_call(abi_VoiceVolumeSet, 0x07BD);  /* 07BA call $0B17 */
L_07BD:
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 07BD mov.b a,$10+x */
    spc.y = spc_nz(spc.a);  /* 07BF mov y,a */
    spc.a = spc_and(spc.a, 0x7F);  /* 07C0 and a,#$7F */
    spc_st(spc_dp(0xA3), spc.a);  /* 07C2 mov.b $A3,a */
    spc.a = spc_nz(spc.y);  /* 07C4 mov a,y */
    spc.a = spc_and(spc.a, 0x40);  /* 07C5 and a,#$40 */
    spc.a = spc_asl(spc.a);  /* 07C7 asl a */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xA3)));  /* 07C8 or.b a,$A3 */
    spc_st(spc_dp((uint8_t)(0x10 + spc.x)), spc.a);  /* 07CA mov.b $10+x,a */
    if (!spc.n) goto L_07D3;  /* 07CC bpl spc_07D3_0285D0 */
    spc.y = spc_nz(0x00);  /* 07CE mov y,#$00 */
    spc.a = spc_nz(spc.y);  /* 07D0 mov a,y */
    goto L_07D9;  /* 07D1 bra spc_07D9_0285D6 */
L_07D3:
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x18 + spc.x))));  /* 07D3 mov.b y,$18+x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0208 + spc.x)));  /* 07D5 mov.w a,$0208+x */
    spc_mul();  /* 07D8 mul ya */
L_07D9:
    spc_st(spc_dp((uint8_t)(0x28 + spc.x)), spc.y);  /* 07D9 mov.b $28+x,y */
    spc_st(spc_dp((uint8_t)(0x20 + spc.x)), spc.a);  /* 07DB mov.b $20+x,a */
    spc_ret(); return;  /* 07DD ret  */
L_0803:
    /* The command loop. A command below $20 is run by pushing $0803 (this
       loop) and its handler's address and returning into it, so the
       handler's RET comes back here; one that pops two return addresses
       (end of track, $097C) leaves the voice's tick. A byte of $20 or more
       is a note: the RET pulls $06F2, pushed before the loop. */
    for (;;) {
        uint8_t top, lo, hi;
        spc_call(abi_TrackByte, 0x0806);  /* 0803 call $0820 */
        spc_cmp(spc.a, 0x20);  /* 0806 cmp a,#$20 */
        if (spc.c) {  /* 0808 bcs: 0830 ret, to $06F2 */
            spc_ret();
            goto L_06F2;
        }
        spc_st(spc_dp(0xA7), spc.a);  /* 080A mov.b $A7,a */
        spc.y = spc_nz(0x08);  /* 080C mov y,#$08 */
        spc_push(spc.y);  /* 080E push y */
        spc.y = spc_nz(0x03);  /* 080F mov y,#$03 */
        spc_push(spc.y);  /* 0811 push y */
        top = spc.sp;
        spc.a = spc_asl(spc.a);  /* 0812 asl a */
        spc.y = spc_nz(spc.a);  /* 0813 mov y,a */
        hi = spc.a = spc_nz(spc_rd((uint16_t)(0x0832 + spc.y)));  /* 0814 mov.w a,$0832+y */
        spc_push(spc.a);  /* 0817 push a */
        lo = spc.a = spc_nz(spc_rd((uint16_t)(0x0831 + spc.y)));  /* 0818 mov.w a,$0831+y */
        spc_push(spc.a);  /* 081B push a */
        spc_cmp(spc.y, 0x08);  /* 081C cmp y,#$08 */
        if (!spc.c)  /* 081E bcc: 0830 ret, to the handler */
            spc_ret();
        else  /* on into 0820, the parameter byte; its RET to the handler */
            abi_TrackByte();
        command_run((uint16_t)(hi << 8 | lo));
        if (spc.sp != (uint8_t)(top + 2))
            return;
    }
}

/*
 * @routine 85:861D TrackByte
 * @spc 0820
 * WHAT IT DOES: Reads voice X's next track byte.
 * HOW: The track pointer at $00+X/$08+X into $A0, the byte at it, the
 * pointer counted up.
 * Entry: X = the voice; called by CALL. Exit: A = the byte; Y = 0; N, Z
 * from the increment.
 */
void abi_TrackByte(void)
{
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x08 + spc.x))));  /* 0820 mov.b y,$08+x */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 0822 mov.b a,$00+x */
    spc_movw_st(0xA0);  /* 0824 movw $A0,ya */
    spc.y = spc_nz(0x00);  /* 0826 mov y,#$00 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 0828 mov a,($A0)+y */
    spc_rmw(spc_dp((uint8_t)(0x00 + spc.x)), spc_inc);  /* 082A inc.b $00+x */
    if (!spc.z) { abi_TrackByteEnd(); return; }  /* 082C bne spc_0830_02862D */
    spc_rmw(spc_dp((uint8_t)(0x08 + spc.x)), spc_inc);  /* 082E inc.b $08+x */
    abi_TrackByteEnd();  /* on into 0830 */
}

void abi_TrackByteEnd(void)
{
    spc_ret(); return;  /* 0830 ret  */
}

void abi_SeqCmd00(void)
{
    spc.a = spc_nz(0x20);  /* 0871 mov a,#$20 */
    { abi_VoiceFlagsToggle(); return; }  /* 0873 bra spc_0881_02867E */
}

void abi_SeqCmd01(void)
{
    spc.a = spc_nz(0x40);  /* 0875 mov a,#$40 */
    { abi_VoiceFlagsToggle(); return; }  /* 0877 bra spc_0881_02867E */
}

void abi_SeqCmd02(void)
{
    spc.a = spc_nz(0x10);  /* 0879 mov a,#$10 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 087B or.b a,$10+x */
    { abi_VoiceFlagsStore(); return; }  /* 087D bra spc_0883_028680 */
}

void abi_SeqCmd03(void)
{
    spc.a = spc_nz(0x08);  /* 087F mov a,#$08 */
    abi_VoiceFlagsToggle();  /* on into 0881 */
}

void abi_VoiceFlagsToggle(void)
{
    spc.a = spc_eor(spc.a, spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 0881 eor.b a,$10+x */
    abi_VoiceFlagsStore();  /* on into 0883 */
}

void abi_VoiceFlagsStore(void)
{
    spc_st(spc_dp((uint8_t)(0x10 + spc.x)), spc.a);  /* 0883 mov.b $10+x,a */
    spc_ret(); return;  /* 0885 ret  */
}

void abi_SeqCmd05(void)
{
    spc_push(spc.a);  /* 0886 push a */
    spc_call(abi_TrackByte, 0x088A);  /* 0887 call $0820 */
    spc.y = spc_pull();  /* 088A pop y */
    if (!(spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_089B;  /* 088B bbc $B0.7,spc_089B_028698 */
    spc_st((uint16_t)(0x0258 + spc.x), spc.a);  /* 088E mov.w $0258+x,a */
    spc.a = spc_nz(spc.y);  /* 0891 mov a,y */
    spc_st((uint16_t)(0x0260 + spc.x), spc.a);  /* 0892 mov.w $0260+x,a */
    spc.a = spc_nz(0x00);  /* 0895 mov a,#$00 */
    spc_st((uint16_t)(0x0268 + spc.x), spc.a);  /* 0897 mov.w $0268+x,a */
    spc_ret(); return;  /* 089A ret  */
L_089B:
    spc_movw_st(0xBE);  /* 089B movw $BE,ya */
    spc_st(spc_dp(0xC2), 0x00);  /* 089D mov $C2,#$00 */
    spc_ret(); return;  /* 08A0 ret  */
}

void abi_SeqCmd06(void)
{
    spc_st((uint16_t)(0x0208 + spc.x), spc.a);  /* 08A1 mov.w $0208+x,a */
    spc_ret(); return;  /* 08A4 ret  */
}

void abi_SeqCmd07(void)
{
    spc.y = spc_nz(spc.a);  /* 08A5 mov y,a */
    spc_call(abi_CurveAtY, 0x08A9);  /* 08A6 call $0B8E */
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_08B6;  /* 08A9 bbs $B0.7,spc_08B6_0286B3 */
    spc_push(spc.a);  /* 08AC push a */
    spc.y = spc_nz(spc_rd(0x1DB1U));  /* 08AD mov.w y,$1DB1 */
    spc_call(abi_CurveAtY, 0x08B3);  /* 08B0 call $0B8E */
    spc.y = spc_pull();  /* 08B3 pop y */
    spc_mul();  /* 08B4 mul ya */
    spc.a = spc_nz(spc.y);  /* 08B5 mov a,y */
L_08B6:
    spc_st((uint16_t)(0x0230 + spc.x), spc.a);  /* 08B6 mov.w $0230+x,a */
    spc_ret(); return;  /* 08B9 ret  */
}

void abi_SeqCmd08(void)
{
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) { abi_VoiceInstrumentFrom(); return; }  /* 08BA bbs $B0.7,spc_08C5_0286C2 */
    spc.a = spc_inc(spc.a);  /* 08BD inc a */
    spc_st((uint16_t)(0x0250 + spc.x), spc.a);  /* 08BE mov.w $0250+x,a */
    if ((spc_rd(spc_dp(0xB9)) >> 7 & 1)) { abi_VoiceInstrumentEnd(); return; }  /* 08C1 bbs $B9.7,spc_08FB_0286F8 */
    abi_VoiceInstrument();  /* on into 08C4 */
}

/*
 * @routine 85:86C1 VoiceInstrument
 * @spc 08C4
 * WHAT IT DOES: Sets voice X's instrument A less 1 from the instrument
 * table at $3438: its envelope, gain and sample source.
 * HOW: Instrument A less 1, times 6, indexes the table: its ADSR1 kept at
 * $0238, ADSR1-GAIN and the source to the voice's DSP registers 4-7, its
 * pitch multiplier to $0220/$0228.
 * Entry: A = the instrument; X = the voice. Exit: X unchanged.
 */
void abi_VoiceInstrument(void)
{
    spc.a = spc_dec(spc.a);  /* 08C4 dec a */
    abi_VoiceInstrumentFrom();  /* on into 08C5 */
}

void abi_VoiceInstrumentFrom(void)
{
    spc.y = spc_nz(0x06);  /* 08C5 mov y,#$06 */
    spc_mul();  /* 08C7 mul ya */
    spc_movw_st(0xA0);  /* 08C8 movw $A0,ya */
    spc.c = 0;  /* 08CA clrc  */
    { uint8_t d = spc_rd(spc_dp(0xA0)); spc_wr(spc_dp(0xA0), spc_adc(d, 0x38)); }  /* 08CB adc $A0,#$38 */
    { uint8_t d = spc_rd(spc_dp(0xA1)); spc_wr(spc_dp(0xA1), spc_adc(d, 0x34)); }  /* 08CE adc $A1,#$34 */
    spc.y = spc_nz(0x01);  /* 08D1 mov y,#$01 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 08D3 mov a,($A0)+y */
    spc_st((uint16_t)(0x0238 + spc.x), spc.a);  /* 08D5 mov.w $0238+x,a */
    spc.y = spc_dec(spc.y);  /* 08D8 dec y */
    spc_push(spc.x);  /* 08D9 push x */
    spc.a = spc_nz(spc.x);  /* 08DA mov a,x */
    spc.a = spc_xcn(spc.a);  /* 08DB xcn a */
    spc.a = spc_and(spc.a, 0x70);  /* 08DC and a,#$70 */
    spc.a = spc_or(spc.a, 0x04);  /* 08DE or a,#$04 */
    spc.x = spc_nz(spc.a);  /* 08E0 mov x,a */
L_08E1:
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 08E1 mov a,($A0)+y */
    spc_st(0x00F2U, spc.x);  /* 08E3 mov.w $00F2,x */
    spc_st(0x00F3U, spc.a);  /* 08E6 mov.w $00F3,a */
    spc.x = spc_inc(spc.x);  /* 08E9 inc x */
    spc.y = spc_inc(spc.y);  /* 08EA inc y */
    spc_cmp(spc.y, 0x04);  /* 08EB cmp y,#$04 */
    if (!spc.z) goto L_08E1;  /* 08ED bne spc_08E1_0286DE */
    spc.x = spc_pull();  /* 08EF pop x */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 08F0 mov a,($A0)+y */
    spc_st((uint16_t)(0x0220 + spc.x), spc.a);  /* 08F2 mov.w $0220+x,a */
    spc.y = spc_inc(spc.y);  /* 08F5 inc y */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 08F6 mov a,($A0)+y */
    spc_st((uint16_t)(0x0228 + spc.x), spc.a);  /* 08F8 mov.w $0228+x,a */
    abi_VoiceInstrumentEnd();  /* on into 08FB */
}

void abi_VoiceInstrumentEnd(void)
{
    spc_ret(); return;  /* 08FB ret  */
}

void abi_SeqCmd09(void)
{
    spc_st(spc_dp(0xA3), spc.a);  /* 08FC mov.b $A3,a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 08FE mov.b a,$10+x */
    spc.a = spc_and(spc.a, 0xF8);  /* 0900 and a,#$F8 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xA3)));  /* 0902 or.b a,$A3 */
    spc_st(spc_dp((uint8_t)(0x10 + spc.x)), spc.a);  /* 0904 mov.b $10+x,a */
    spc_ret(); return;  /* 0906 ret  */
}

void abi_SeqCmd0A(void)
{
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_090C;  /* 0907 bbs $B0.7,spc_090C_028709 */
    spc_st(spc_dp(0xC4), spc.a);  /* 090A mov.b $C4,a */
L_090C:
    spc_ret(); return;  /* 090C ret  */
}

void abi_SeqCmd0B(void)
{
    spc_st((uint16_t)(0x0210 + spc.x), spc.a);  /* 090D mov.w $0210+x,a */
    spc_ret(); return;  /* 0910 ret  */
}

void abi_SeqCmd0C(void)
{
    spc_st((uint16_t)(0x0218 + spc.x), spc.a);  /* 0911 mov.w $0218+x,a */
    spc_ret(); return;  /* 0914 ret  */
}

void abi_SeqCmd0D(void)
{
    spc_st((uint16_t)(0x0128 + spc.x), spc.a);  /* 0915 mov.w $0128+x,a */
    spc_ret(); return;  /* 0918 ret  */
}

void abi_SeqCmd0E(void)
{
    spc.y = spc_nz(0x30);  /* 0919 mov y,#$30 */
    { abi_SeqLoop(); return; }  /* 091B bra spc_0927_028724 */
}

void abi_SeqCmd0F(void)
{
    spc.y = spc_nz(0x38);  /* 091D mov y,#$38 */
    { abi_SeqLoop(); return; }  /* 091F bra spc_0927_028724 */
}

void abi_SeqCmd10(void)
{
    spc.y = spc_nz(0x40);  /* 0921 mov y,#$40 */
    { abi_SeqLoop(); return; }  /* 0923 bra spc_0927_028724 */
}

void abi_SeqCmd11(void)
{
    spc.y = spc_nz(0x48);  /* 0925 mov y,#$48, not run by the traces */
    abi_SeqLoop();  /* on into 0927 */
}

void abi_SeqLoop(void)
{
    spc_st(spc_dp(0xA3), spc.a);  /* 0927 mov.b $A3,a */
    spc_st(spc_dp(0xA0), spc.y);  /* 0929 mov.b $A0,y */
    spc.a = spc_nz(spc.x);  /* 092B mov a,x */
    spc.c = 0;  /* 092C clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0xA0)));  /* 092D adc.b a,$A0 */
    spc.y = spc_nz(spc.a);  /* 092F mov y,a */
    spc_st(spc_dp(0xA1), 0x00);  /* 0930 mov $A1,#$00 */
    spc_st(spc_dp(0xA0), 0x00);  /* 0933 mov $A0,#$00 */
    { uint8_t d = spc_rd(spc_dp(0xA7)); spc_cmp(d, 0x12); }  /* 0936 cmp $A7,#$12 */
    if (spc.c) goto L_094C;  /* 0939 bcs spc_094C_028749 */
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 093B mov a,($A0)+y */
    if (spc.z) goto L_0946;  /* 093D beq spc_0946_028743 */
    spc.a = spc_dec(spc.a);  /* 093F dec a */
    spc_st(ind_y(0xA0), spc.a);  /* 0940 mov ($A0)+y,a */
    if (spc.z) { abi_SeqLoopSkip(); return; }  /* 0942 beq spc_0965_028762 */
    goto L_0958;  /* 0944 bra spc_0958_028755 */
L_0946:
    spc.a = spc_nz(spc_rd(spc_dp(0xA3)));  /* 0946 mov.b a,$A3 */
    spc_st(ind_y(0xA0), spc.a);  /* 0948 mov ($A0)+y,a */
    goto L_0958;  /* 094A bra spc_0958_028755 */
L_094C:
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));  /* 094C mov a,($A0)+y */
    spc.a = spc_dec(spc.a);  /* 094E dec a */
    if (!spc.z) { abi_SeqLoopSkip(); return; }  /* 094F bne spc_0965_028762 */
    spc_st(ind_y(0xA0), spc.a);  /* 0951 mov ($A0)+y,a */
    spc.a = spc_nz(spc_rd(spc_dp(0xA3)));  /* 0953 mov.b a,$A3 */
    spc_call(abi_VoiceFlagsSet, 0x0958);  /* 0955 call $0971 */
L_0958:
    spc_call(abi_TrackByte, 0x095B);  /* 0958 call $0820 */
    abi_SeqCmd16();  /* on into 095B */
}

void abi_SeqCmd16(void)
{
    spc_push(spc.a);  /* 095B push a */
    spc_call(abi_TrackByte, 0x095F);  /* 095C call $0820 */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 095F mov.b $00+x,a */
    spc.a = spc_pull();  /* 0961 pop a */
    spc_st(spc_dp((uint8_t)(0x08 + spc.x)), spc.a);  /* 0962 mov.b $08+x,a */
    spc_ret(); return;  /* 0964 ret  */
}

void abi_SeqLoopSkip(void)
{
    spc.a = spc_nz(0x02);  /* 0965 mov a,#$02 */
    spc.c = 0;  /* 0967 clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 0968 adc.b a,$00+x */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 096A mov.b $00+x,a */
    if (!spc.c) goto L_0970;  /* 096C bcc spc_0970_02876D */
    spc_rmw(spc_dp((uint8_t)(0x08 + spc.x)), spc_inc);  /* 096E inc.b $08+x */
L_0970:
    spc_ret(); return;  /* 0970 ret  */
}

/*
 * @routine 85:876E VoiceFlagsSet
 * @spc 0971
 * WHAT IT DOES: Sequencer command $04: sets voice X's mode bits (bits 3,
 * 5, 6 of $10+X) from A.
 * HOW: $10+X AND $97, OR A.
 * Entry: A = the bits; X = the voice. Exit: A = the new flags; N, Z from
 * them.
 */
void abi_VoiceFlagsSet(void)
{
    spc_st(spc_dp(0xA3), spc.a);  /* 0971 mov.b $A3,a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 0973 mov.b a,$10+x */
    spc.a = spc_and(spc.a, 0x97);  /* 0975 and a,#$97 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xA3)));  /* 0977 or.b a,$A3 */
    spc_st(spc_dp((uint8_t)(0x10 + spc.x)), spc.a);  /* 0979 mov.b $10+x,a */
    spc_ret(); return;  /* 097B ret  */
}

void abi_SeqCmd17(void)
{
    spc.a = spc_pull();  /* 097C pop a */
    spc.a = spc_pull();  /* 097D pop a */
    spc.a = spc_pull();  /* 097E pop a */
    spc.a = spc_pull();  /* 097F pop a */
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_0996;  /* 0980 bbs $B0.7,spc_0996_028793 */
    spc.a = spc_nz(0x00);  /* 0983 mov a,#$00 */
    spc_st(spc_dp((uint8_t)(0x08 + spc.x)), spc.a);  /* 0985 mov.b $08+x,a */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 0987 mov.b $00+x,a */
    spc_st((uint16_t)(0x0250 + spc.x), spc.a);  /* 0989 mov.w $0250+x,a */
    if ((spc_rd(spc_dp(0xB9)) >> 7 & 1)) goto L_0995;  /* 098C bbs $B9.7,spc_0995_028792 */
    spc_call(abi_VoiceRelease, 0x0992);  /* 098F call $0414 */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xBB)); spc_wr(spc_dp(0xBB), spc_or(d, s)); }  /* 0992 or $BB,$B3 */
L_0995:
    spc_ret(); return;  /* 0995 ret  */
L_0996:
    { uint8_t s = spc_rd(spc_dp(0xB4)); uint8_t d = spc_rd(spc_dp(0xB8)); spc_wr(spc_dp(0xB8), spc_and(d, s)); }  /* 0996 and $B8,$B4 */
    { uint8_t s = spc_rd(spc_dp(0xB4)); uint8_t d = spc_rd(spc_dp(0xB7)); spc_wr(spc_dp(0xB7), spc_and(d, s)); }  /* 0999 and $B7,$B4 */
    abi_VoiceStop();  /* on into 099C */
}

/*
 * @routine 85:8799 VoiceStop
 * @spc 099C
 * WHAT IT DOES: Stops voice X: its priority cleared, released, its key-off
 * marked.
 * HOW: $0250+X = 0; VoiceRelease; $B6 |= $B3; then VoiceSfxMark.
 * Entry: X = the voice; $B3 its bit. Exit: as VoiceSfxMark.
 */
void abi_VoiceStop(void)
{
    spc.a = spc_nz(0x00);  /* 099C mov a,#$00 */
    spc_st((uint16_t)(0x0250 + spc.x), spc.a);  /* 099E mov.w $0250+x,a */
    spc_call(abi_VoiceRelease, 0x09A4);  /* 09A1 call $0414 */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xB6)); spc_wr(spc_dp(0xB6), spc_or(d, s)); }  /* 09A4 or $B6,$B3 */
    abi_VoiceSfxMark();  /* on into 09A7 */
}

/*
 * @routine 85:87A4 VoiceSfxMark
 * @spc 09A7
 * WHAT IT DOES: Marks voice X in $BD when the music's voice under it is
 * still playing.
 * HOW: $0250 of the voice X - $50 nonzero: $BD |= $B3.
 * Entry: X = the voice; $B3 its bit. Exit: X unchanged.
 */
void abi_VoiceSfxMark(void)
{
    spc_push(spc.x);  /* 09A7 push x */
    spc.a = spc_nz(spc.x);  /* 09A8 mov a,x */
    spc.c = 1;  /* 09A9 setc  */
    spc.a = spc_sbc(spc.a, 0x50);  /* 09AA sbc a,#$50 */
    spc.x = spc_nz(spc.a);  /* 09AC mov x,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0250 + spc.x)));  /* 09AD mov.w a,$0250+x */
    if (spc.z) goto L_09B5;  /* 09B0 beq spc_09B5_0287B2 */
    { uint8_t s = spc_rd(spc_dp(0xB3)); uint8_t d = spc_rd(spc_dp(0xBD)); spc_wr(spc_dp(0xBD), spc_or(d, s)); }  /* 09B2 or $BD,$B3 */
L_09B5:
    spc.x = spc_pull();  /* 09B5 pop x */
    spc_ret(); return;  /* 09B6 ret  */
}

void abi_SeqCmd18(void)
{
    spc.c = 0;  /* 09B7 clrc  */
    spc.a = spc_adc(spc.a, 0x80);  /* 09B8 adc a,#$80 */
    spc_st((uint16_t)(0x0248 + spc.x), spc.a);  /* 09BA mov.w $0248+x,a */
    spc_ret(); return;  /* 09BD ret  */
}

void abi_SeqCmd19(void)
{
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_09C7;  /* 09BE bbs $B0.7,spc_09C7_0287C4 */
    spc.y = spc_nz(spc.a);  /* 09C1 mov y,a */
    spc_call(abi_CurveAtY, 0x09C5);  /* 09C2 call $0B8E */
    spc_st(spc_dp(0xC3), spc.a);  /* 09C5 mov.b $C3,a */
L_09C7:
    spc_ret(); return;  /* 09C7 ret  */
}

void abi_SeqCmd1A(void)
{
    spc_push(spc.a);  /* 09C8 push a */
    spc_call(abi_TrackByte, 0x09CC);  /* 09C9 call $0820 */
    spc.y = spc_pull();  /* 09CC pop y */
    spc_cmp(spc.y, 0x03);  /* 09CD cmp y,#$03 */
    if (!spc.z) goto L_09DF;  /* 09CF bne spc_09DF_0287DC */
    spc.a = spc_xcn(spc.a);  /* 09D1 xcn a, not run by the traces */
    spc_st(spc_dp(0xA3), spc.a);  /* 09D2 mov.b $A3,a, not run by the traces */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0100 + spc.x)));  /* 09D4 mov.w a,$0100+x, not run by the traces */
    spc.a = spc_and(spc.a, 0xEF);  /* 09D7 and a,#$EF, not run by the traces */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xA3)));  /* 09D9 or.b a,$A3, not run by the traces */
    spc_st((uint16_t)(0x0100 + spc.x), spc.a);  /* 09DB mov.w $0100+x,a, not run by the traces */
    spc_ret(); return;  /* 09DE ret , not run by the traces */
L_09DF:
    spc_push(spc.a);  /* 09DF push a */
    spc.a = spc_nz(0x08);  /* 09E0 mov a,#$08 */
    spc_mul();  /* 09E2 mul ya */
    spc_st(spc_dp(0xA4), spc.x);  /* 09E3 mov.b $A4,x */
    spc.c = 0;  /* 09E5 clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0xA4)));  /* 09E6 adc.b a,$A4 */
    spc.y = spc_nz(spc.a);  /* 09E8 mov y,a */
    spc.a = spc_pull();  /* 09E9 pop a */
    spc_st((uint16_t)(0x0108 + spc.y), spc.a);  /* 09EA mov.w $0108+y,a */
    spc_ret(); return;  /* 09ED ret  */
}

void abi_SeqCmd1B(void)
{
    spc_call(abi_TrackByte, 0x09F1);  /* 09EE call $0820 */
    spc.y = spc_nz(0x08);  /* 09F1 mov y,#$08 */
    spc_mul();  /* 09F3 mul ya */
    spc_cmp(spc.a, spc_rd(spc_dp(0xCB)));  /* 09F4 cmp.b a,$CB */
    if (spc.z) { abi_EchoSetupEnd(); return; }  /* 09F6 beq spc_0A52_02884F */
    spc_st(spc_dp(0xCB), spc.a);  /* 09F8 mov.b $CB,a */
    abi_EchoSetup();  /* on into 09FA */
}

void abi_EchoSetup(void)
{
    spc_push(spc.x);  /* 09FA push x */
    spc.x = spc_nz(spc.a);  /* 09FB mov x,a */
    spc.y = spc_nz(0x0C);  /* 09FC mov y,#$0C */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D10 + spc.x)));  /* 09FE mov.w a,$0D10+x */
    spc_call(abi_DspWrite, 0x0A04);  /* 0A01 call $0CBB */
    spc.y = spc_nz(0x1C);  /* 0A04 mov y,#$1C */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D11 + spc.x)));  /* 0A06 mov.w a,$0D11+x */
    spc_call(abi_DspWrite, 0x0A0C);  /* 0A09 call $0CBB */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D12 + spc.x)));  /* 0A0C mov.w a,$0D12+x */
    spc_st(spc_dp(0xCE), spc.a);  /* 0A0F mov.b $CE,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D13 + spc.x)));  /* 0A11 mov.w a,$0D13+x */
    spc_st(spc_dp(0xCF), spc.a);  /* 0A14 mov.b $CF,a */
    spc_call(abi_EchoOff, 0x0A19);  /* 0A16 call $0A53 */
    spc.y = spc_nz(0x6D);  /* 0A19 mov y,#$6D */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D14 + spc.x)));  /* 0A1B mov.w a,$0D14+x */
    spc_call(abi_DspWrite, 0x0A21);  /* 0A1E call $0CBB */
    spc.a = spc_nz(0x7D);  /* 0A21 mov a,#$7D */
    spc_st(0x00F2U, spc.a);  /* 0A23 mov.w $00F2,a */
    spc.a = spc_nz(spc_rd(0x00F3U));  /* 0A26 mov.w a,$00F3 */
    spc.a = spc_and(spc.a, 0x0F);  /* 0A29 and a,#$0F */
    spc.a = spc_inc(spc.a);  /* 0A2B inc a */
    spc.a = spc_eor(spc.a, 0xFF);  /* 0A2C eor a,#$FF */
    if (!(spc_rd(spc_dp(0xCD)) >> 7 & 1)) goto L_0A34;  /* 0A2E bbc $CD.7,spc_0A34_028831 */
    spc.c = 0;  /* 0A31 clrc , not run by the traces */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0xCD)));  /* 0A32 adc.b a,$CD, not run by the traces */
L_0A34:
    spc_st(spc_dp(0xCD), spc.a);  /* 0A34 mov.b $CD,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D15 + spc.x)));  /* 0A36 mov.w a,$0D15+x */
    spc_st(0x00F3U, spc.a);  /* 0A39 mov.w $00F3,a */
    spc.a = spc_nz(0x20);  /* 0A3C mov a,#$20 */
    spc.c = 0;  /* 0A3E clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp(0xCC)));  /* 0A3F adc.b a,$CC */
    spc_st(spc_dp(0xCC), spc.a);  /* 0A41 mov.b $CC,a */
    spc.y = spc_nz(0x0D);  /* 0A43 mov y,#$0D */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D16 + spc.x)));  /* 0A45 mov.w a,$0D16+x */
    spc_call(abi_DspWrite, 0x0A4B);  /* 0A48 call $0CBB */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0D17 + spc.x)));  /* 0A4B mov.w a,$0D17+x */
    spc_call(abi_EchoFirSet, 0x0A51);  /* 0A4E call $0A6C */
    spc.x = spc_pull();  /* 0A51 pop x */
    abi_EchoSetupEnd();  /* on into 0A52 */
}

void abi_EchoSetupEnd(void)
{
    spc_ret(); return;  /* 0A52 ret  */
}

/*
 * @routine 85:8850 EchoOff
 * @spc 0A53
 * WHAT IT DOES: Turns echo off: echo volume and feedback 0, then the echo
 * write disabled.
 * HOW: The DSP registers at $0A68 (3) to 0; FLG ($6C) = $20.
 * Entry: called by CALL. Exit: as DspWrite.
 */
void abi_EchoOff(void)
{
    spc.y = spc_nz(0x03);  /* 0A53 mov y,#$03 */
L_0A55:
    spc.a = spc_nz(spc_rd((uint16_t)(0x0A68 + spc.y)));  /* 0A55 mov.w a,$0A68+y */
    spc_st(0x00F2U, spc.a);  /* 0A58 mov.w $00F2,a */
    spc.a = spc_nz(0x00);  /* 0A5B mov a,#$00 */
    spc_st(0x00F3U, spc.a);  /* 0A5D mov.w $00F3,a */
    if ((spc.y = (uint8_t)(spc.y - 1)) != 0) goto L_0A55;  /* 0A60 dbnz y,spc_0A55_028852 */
    spc.y = spc_nz(0x6C);  /* 0A62 mov y,#$6C */
    spc.a = spc_nz(0x20);  /* 0A64 mov a,#$20 */
    { abi_DspWrite(); return; }  /* 0A66 jmp $0CBB */
}

/*
 * @routine 85:8869 EchoFirSet
 * @spc 0A6C
 * WHAT IT DOES: Sets the echo filter's 8 coefficients from filter A of the
 * table at $0A83.
 * HOW: A times 8 indexes the table; the DSP registers $0F, $1F ... $7F.
 * Entry: A = the filter. Exit: X unchanged.
 */
void abi_EchoFirSet(void)
{
    spc_push(spc.x);  /* 0A6C push x */
    spc.y = spc_nz(0x08);  /* 0A6D mov y,#$08 */
    spc_mul();  /* 0A6F mul ya */
    spc.x = spc_nz(spc.a);  /* 0A70 mov x,a */
    spc.y = spc_nz(0x0F);  /* 0A71 mov y,#$0F */
L_0A73:
    spc.a = spc_nz(spc_rd((uint16_t)(0x0A83 + spc.x)));  /* 0A73 mov.w a,$0A83+x */
    spc_call(abi_DspWrite, 0x0A79);  /* 0A76 call $0CBB */
    spc.x = spc_inc(spc.x);  /* 0A79 inc x */
    spc.a = spc_nz(spc.y);  /* 0A7A mov a,y */
    spc.c = 0;  /* 0A7B clrc  */
    spc.a = spc_adc(spc.a, 0x10);  /* 0A7C adc a,#$10 */
    spc.y = spc_nz(spc.a);  /* 0A7E mov y,a */
    if (!spc.n) goto L_0A73;  /* 0A7F bpl spc_0A73_028870 */
    spc.x = spc_pull();  /* 0A81 pop x */
    spc_ret(); return;  /* 0A82 ret  */
}

void abi_SeqCmd1C(void)
{
    spc.a = spc_and(spc.a, 0x01);  /* 0AA3 and a,#$01 */
    spc.a = spc_lsr(spc.a);  /* 0AA5 lsr a */
    if (!spc.c) goto L_0AAA;  /* 0AA6 bcc spc_0AAA_0288A7 */
    spc.a = spc_nz(spc_rd(spc_dp(0xB3)));  /* 0AA8 mov.b a,$B3 */
L_0AAA:
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_0AB5;  /* 0AAA bbs $B0.7,spc_0AB5_0288B2 */
    { uint8_t s = spc_rd(spc_dp(0xB4)); uint8_t d = spc_rd(spc_dp(0xBC)); spc_wr(spc_dp(0xBC), spc_and(d, s)); }  /* 0AAD and $BC,$B4 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xBC)));  /* 0AB0 or.b a,$BC */
    spc_st(spc_dp(0xBC), spc.a);  /* 0AB2 mov.b $BC,a */
    spc_ret(); return;  /* 0AB4 ret  */
L_0AB5:
    { uint8_t s = spc_rd(spc_dp(0xB4)); uint8_t d = spc_rd(spc_dp(0xB7)); spc_wr(spc_dp(0xB7), spc_and(d, s)); }  /* 0AB5 and $B7,$B4 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp(0xB7)));  /* 0AB8 or.b a,$B7 */
    spc_st(spc_dp(0xB7), spc.a);  /* 0ABA mov.b $B7,a */
    spc_ret(); return;  /* 0ABC ret  */
}

void abi_SeqCmd1D(void)
{
    spc.a = spc_or(spc.a, 0xA0);  /* 0ABD or a,#$A0 */
    spc_st((uint16_t)(0x0240 + spc.x), spc.a);  /* 0ABF mov.w $0240+x,a */
    spc_ret(); return;  /* 0AC2 ret  */
}

void abi_VoiceSlide(void)
{
    spc.p = 1;  /* 0AC3 setp  */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x18 + spc.x))));  /* 0AC4 mov.b a,$18+x */
    spc.c = 0;  /* 0AC6 clrc  */
    spc.a = spc_adc(spc.a, spc_rd(spc_dp((uint8_t)(0x20 + spc.x))));  /* 0AC7 adc.b a,$20+x */
    spc_st(spc_dp((uint8_t)(0x20 + spc.x)), spc.a);  /* 0AC9 mov.b $20+x,a */
    if (!spc.c) goto L_0AD4;  /* 0ACB bcc spc_0AD4_0288D1 */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 0ACD mov.b a,$00+x */
    spc.c = 0;  /* 0ACF clrc  */
    spc.a = spc_adc(spc.a, 0x40);  /* 0AD0 adc a,#$40 */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 0AD2 mov.b $00+x,a */
L_0AD4:
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 0AD4 mov.b a,$00+x */
    spc_st(spc_dp(0xA4), spc.a);  /* 0AD6 mov.b $A4,a */
    if (!(spc_rd(spc_dp(0xA4)) >> 0 & 1)) goto L_0B0D;  /* 0AD8 bbc $A4.0,spc_0B0D_02890A */
    spc_st(spc_dp(0xA1), 0x00);  /* 0ADB mov $A1,#$00 */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x28 + spc.x))));  /* 0ADE mov.b a,$28+x */
    spc.a = spc_asl(spc.a);  /* 0AE0 asl a */
    spc_st(spc_dp(0xA0), spc.a);  /* 0AE1 mov.b $A0,a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x40 + spc.x))));  /* 0AE3 mov.b a,$40+x */
    spc_st(spc_dp(0xA3), spc.a);  /* 0AE5 mov.b $A3,a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x48 + spc.x))));  /* 0AE7 mov.b a,$48+x */
    spc_st(spc_dp(0xA2), spc.a);  /* 0AE9 mov.b $A2,a */
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x30 + spc.x))));  /* 0AEB mov.b y,$30+x */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x38 + spc.x))));  /* 0AED mov.b a,$38+x */
    if ((spc_rd(spc_dp(0xA4)) >> 1 & 1)) goto L_0AFC;  /* 0AEF bbs $A4.1,spc_0AFC_0288F9 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 0AF2 subw ya,$A0 */
    if (!spc.c) goto L_0B02;  /* 0AF4 bcc spc_0B02_0288FF */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_cmpw(spc_ya(), w); }  /* 0AF6 cmpw ya,$A2 */
    if (!spc.c) goto L_0B02;  /* 0AF8 bcc spc_0B02_0288FF */
    goto L_0B09;  /* 0AFA bra spc_0B09_028906 */
L_0AFC:
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0AFC addw ya,$A0 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_cmpw(spc_ya(), w); }  /* 0AFE cmpw ya,$A2 */
    if (!spc.c) goto L_0B09;  /* 0B00 bcc spc_0B09_028906 */
L_0B02:
    spc.a = spc_nz(spc_rd(spc_dp(0xA4)));  /* 0B02 mov.b a,$A4 */
    spc.a = spc_dec(spc.a);  /* 0B04 dec a */
    spc_st(spc_dp((uint8_t)(0x00 + spc.x)), spc.a);  /* 0B05 mov.b $00+x,a */
    spc_movw_ld(0xA2);  /* 0B07 movw ya,$A2 */
L_0B09:
    spc_st(spc_dp((uint8_t)(0x30 + spc.x)), spc.y);  /* 0B09 mov.b $30+x,y */
    spc_st(spc_dp((uint8_t)(0x38 + spc.x)), spc.a);  /* 0B0B mov.b $38+x,a */
L_0B0D:
    spc.a = spc_nz(spc_rd(0x00B9U));  /* 0B0D mov.w a,$00B9 */
    if (!spc.n) goto L_0B14;  /* 0B10 bpl spc_0B14_028911 */
    spc.p = 0;  /* 0B12 clrp  */
    spc_ret(); return;  /* 0B13 ret  */
L_0B14:
    spc_call(abi_VoicePitchUpdate, 0x0B17);  /* 0B14 call $0BD6 */
    abi_VoiceVolumeSet();  /* on into 0B17 */
}

/*
 * @routine 85:8914 VoiceVolumeSet
 * @spc 0B17
 * WHAT IT DOES: Sets voice X's left and right volume from its volume, pan
 * and the fade.
 * HOW: The pan ($20+X, by the note's half) through the pan curve
 * (CurveAt8), the music's fade (CurveAt16), the voice's volume ($0230) and
 * $0248 (or $80), each step a multiply; the two products to DSP registers
 * 1 and 0.
 * Entry: X = the voice. Exit: $A5 = $FF.
 */
void abi_VoiceVolumeSet(void)
{
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x10 + spc.x))));  /* 0B17 mov.b y,$10+x */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 0B19 mov.b a,$00+x */
    spc.a = spc_asl(spc.a);  /* 0B1B asl a */
    spc.a = spc_asl(spc.a);  /* 0B1C asl a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x20 + spc.x))));  /* 0B1D mov.b a,$20+x */
    if (spc.c) goto L_0B23;  /* 0B1F bcs spc_0B23_028920 */
    spc.a = spc_eor(spc.a, 0xFF);  /* 0B21 eor a,#$FF */
L_0B23:
    spc_mul();  /* 0B23 mul ya */
    spc_movw_st(0xA2);  /* 0B24 movw $A2,ya */
    spc.y = spc_nz(0x7E);  /* 0B26 mov y,#$7E */
    spc.a = spc_nz(0x81);  /* 0B28 mov a,#$81 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 0B2A subw ya,$A2 */
    spc_call(abi_CurveAt8, 0x0B2F);  /* 0B2C call $0B90 */
    spc.y = spc_nz(spc.a);  /* 0B2F mov y,a */
    spc.p = 0;  /* 0B30 clrp  */
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_0B47;  /* 0B31 bbs $B0.7,spc_0B47_028944 */
    spc.a = spc_nz(spc_rd(spc_dp(0xC3)));  /* 0B34 mov.b a,$C3 */
    spc_mul();  /* 0B36 mul ya */
    spc_push(spc.y);  /* 0B37 push y */
    spc_movw_ld(0xC9);  /* 0B38 movw ya,$C9 */
    if ((spc_rd(spc_dp(0xC6)) >> 7 & 1)) goto L_0B42;  /* 0B3A bbs $C6.7,spc_0B42_02893F */
    spc.y = spc_nz(0xFF);  /* 0B3D mov y,#$FF */
    spc.a = spc_nz(spc.y);  /* 0B3F mov a,y */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xC9)) | spc_rd(spc_dp(0xCA)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 0B40 subw ya,$C9 */
L_0B42:
    spc_call(abi_CurveAt16, 0x0B45);  /* 0B42 call $0B88 */
    spc.y = spc_pull();  /* 0B45 pop y */
    spc_mul();  /* 0B46 mul ya */
L_0B47:
    spc.a = spc_nz(spc_rd((uint16_t)(0x0230 + spc.x)));  /* 0B47 mov.w a,$0230+x */
    spc_mul();  /* 0B4A mul ya */
    spc_st(spc_dp(0xA3), spc.y);  /* 0B4B mov.b $A3,y */
    if (!(spc_rd(spc_dp(0xB0)) >> 5 & 1)) goto L_0B54;  /* 0B4D bbc $B0.5,spc_0B54_028951 */
    spc.a = spc_nz(0x80);  /* 0B50 mov a,#$80 */
    goto L_0B57;  /* 0B52 bra spc_0B57_028954 */
L_0B54:
    spc.a = spc_nz(spc_rd((uint16_t)(0x0248 + spc.x)));  /* 0B54 mov.w a,$0248+x */
L_0B57:
    spc.y = spc_nz(0x14);  /* 0B57 mov y,#$14 */
    spc_mul();  /* 0B59 mul ya */
    spc_movw_st(0xA0);  /* 0B5A movw $A0,ya */
    spc_st(spc_dp(0xA5), 0x01);  /* 0B5C mov $A5,#$01 */
L_0B5F:
    spc.y = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0B5F mov.b y,$A1 */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0BC1 + spc.y)));  /* 0B61 mov.w a,$0BC1+y */
    spc.c = 1;  /* 0B64 setc  */
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0BC0 + spc.y)));  /* 0B65 sbc.w a,$0BC0+y */
    spc.y = spc_nz(spc_rd(spc_dp(0xA0)));  /* 0B68 mov.b y,$A0 */
    spc_mul();  /* 0B6A mul ya */
    spc.a = spc_nz(spc.y);  /* 0B6B mov a,y */
    spc.y = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0B6C mov.b y,$A1 */
    spc.c = 0;  /* 0B6E clrc  */
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0BC0 + spc.y)));  /* 0B6F adc.w a,$0BC0+y */
    spc.y = spc_nz(spc_rd(spc_dp(0xA3)));  /* 0B72 mov.b y,$A3 */
    spc_mul();  /* 0B74 mul ya */
    spc.a = spc_nz(spc.y);  /* 0B75 mov a,y */
    spc.y = spc_nz(spc_rd(spc_dp(0xA5)));  /* 0B76 mov.b y,$A5 */
    spc_call(abi_DspWriteVoice, 0x0B7B);  /* 0B78 call $0CB0 */
    spc.y = spc_nz(0x14);  /* 0B7B mov y,#$14 */
    spc.a = spc_nz(0x00);  /* 0B7D mov a,#$00 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 0B7F subw ya,$A0 */
    spc_movw_st(0xA0);  /* 0B81 movw $A0,ya */
    spc_rmw(spc_dp(0xA5), spc_dec);  /* 0B83 dec.b $A5 */
    if (!spc.n) goto L_0B5F;  /* 0B85 bpl spc_0B5F_02895C */
    spc_ret(); return;  /* 0B87 ret  */
}

/*
 * @routine 85:8985 CurveAt16
 * @spc 0B88
 * WHAT IT DOES: The pan curve at YA / 16 (a 4-bit fraction).
 * HOW: $A1 = Y; four shifts of $A1:A; then as CurveAt8 from its lookup.
 * Entry: YA = the position. Exit: A = the curve's value.
 */
void abi_CurveAt16(void)
{
    spc_st(spc_dp(0xA1), spc.y);  /* 0B88 mov.b $A1,y */
    spc.y = spc_nz(0x04);  /* 0B8A mov y,#$04 */
    { abi_CurveShift(); return; }  /* 0B8C bra spc_0B94_028991 */
}

/*
 * @routine 85:898B CurveAtY
 * @spc 0B8E
 * WHAT IT DOES: The pan curve at Y (with a fraction of $FF).
 * HOW: A = $FF, then CurveAt8.
 * Entry: Y = the position. Exit: as CurveAt8.
 */
void abi_CurveAtY(void)
{
    spc.a = spc_nz(0xFF);  /* 0B8E mov a,#$FF */
    abi_CurveAt8();  /* on into 0B90 */
}

/*
 * @routine 85:898D CurveAt8
 * @spc 0B90
 * WHAT IT DOES: The pan curve at YA / 8: the table at $0BAF interpolated.
 * HOW: Three shifts of $A1:A give the entry and the fraction; the entry
 * plus the fraction of the step to the next.
 * Entry: YA = the position. Exit: A = the value; Y = the entry.
 */
void abi_CurveAt8(void)
{
    spc_st(spc_dp(0xA1), spc.y);  /* 0B90 mov.b $A1,y */
    spc.y = spc_nz(0x03);  /* 0B92 mov y,#$03 */
    abi_CurveShift();  /* on into 0B94 */
}

void abi_CurveShift(void)
{
L_0B94:
    spc_rmw(spc_dp(0xA1), spc_lsr);  /* 0B94 lsr.b $A1 */
    spc.a = spc_ror(spc.a);  /* 0B96 ror a */
    if ((spc.y = (uint8_t)(spc.y - 1)) != 0) goto L_0B94;  /* 0B97 dbnz y,spc_0B94_028991 */
    spc_st(spc_dp(0xA0), spc.a);  /* 0B99 mov.b $A0,a */
    spc.y = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0B9B mov.b y,$A1 */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0BB0 + spc.y)));  /* 0B9D mov.w a,$0BB0+y */
    spc.c = 1;  /* 0BA0 setc  */
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0BAF + spc.y)));  /* 0BA1 sbc.w a,$0BAF+y */
    spc.y = spc_nz(spc_rd(spc_dp(0xA0)));  /* 0BA4 mov.b y,$A0 */
    spc_mul();  /* 0BA6 mul ya */
    spc.a = spc_nz(spc.y);  /* 0BA7 mov a,y */
    spc.y = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0BA8 mov.b y,$A1 */
    spc.c = 0;  /* 0BAA clrc  */
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0BAF + spc.y)));  /* 0BAB adc.w a,$0BAF+y */
    spc_ret(); return;  /* 0BAE ret  */
}

/*
 * @routine 85:89D3 VoicePitchUpdate
 * @spc 0BD6
 * WHAT IT DOES: Updates voice X's pitch when a slide, vibrato or a detune
 * is running.
 * HOW: Nothing when neither its slide nor its vibrato runs and the detune
 * ($C5) is 0; else on into VoicePitchSet.
 * Entry: X = the voice. Exit: as VoicePitchSet.
 */
void abi_VoicePitchUpdate(void)
{
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 0BD6 mov.b a,$00+x */
    spc.a = spc_and(spc.a, 0x01);  /* 0BD8 and a,#$01 */
    spc.a = spc_or(spc.a, spc_rd(spc_dp((uint8_t)(0x08 + spc.x))));  /* 0BDA or.b a,$08+x */
    if (!spc.z) { abi_VoicePitchSet(); return; }  /* 0BDC bne spc_0BE9_0289E6 */
    spc.a = spc_nz(spc_rd(0x00B0U));  /* 0BDE mov.w a,$00B0 */
    if (spc.n) goto L_0BE8;  /* 0BE1 bmi spc_0BE8_0289E5 */
    spc.a = spc_nz(spc_rd(0x00C5U));  /* 0BE3 mov.w a,$00C5 */
    if (!spc.z) { abi_VoicePitchFrom(); return; }  /* 0BE6 bne spc_0BED_0289EA */
L_0BE8:
    spc_ret(); return;  /* 0BE8 ret  */
}

/*
 * @routine 85:89E6 VoicePitchSet
 * @spc 0BE9
 * WHAT IT DOES: Sets voice X's DSP pitch from its note, slide, vibrato and
 * the detune.
 * HOW: The note and fraction ($30/$38+X), the vibrato's offset added or
 * taken, the detune ($C5) added, then the octave table at $0CC2
 * interpolated and scaled by the instrument's multiplier; DSP registers 2
 * and 3.
 * Entry: X = the voice. Exit: as DspWrite.
 */
void abi_VoicePitchSet(void)
{
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x08 + spc.x))));  /* 0BE9 mov.b a,$08+x */
    if (!spc.z) { abi_VoicePitchVibrato(); return; }  /* 0BEB bne spc_0BF3_0289F0 */
    abi_VoicePitchFrom();  /* on into 0BED */
}

void abi_VoicePitchFrom(void)
{
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x30 + spc.x))));  /* 0BED mov.b y,$30+x */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x38 + spc.x))));  /* 0BEF mov.b a,$38+x */
    { abi_VoicePitchOut(); return; }  /* 0BF1 bra spc_0C22_028A1F */
}

void abi_VoicePitchVibrato(void)
{
    spc.a = spc_asl(spc.a);  /* 0BF3 asl a */
    spc.y = spc_nz(0x0C);  /* 0BF4 mov y,#$0C */
    spc_mul();  /* 0BF6 mul ya */
    spc_movw_st(0xA0);  /* 0BF7 movw $A0,ya */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x00 + spc.x))));  /* 0BF9 mov.b a,$00+x */
    spc_push(spc_psw());  /* 0BFB push p */
    spc.a = spc_asl(spc.a);  /* 0BFC asl a */
    spc.a = spc_asl(spc.a);  /* 0BFD asl a */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x20 + spc.x))));  /* 0BFE mov.b a,$20+x */
    if (!spc.c) goto L_0C04;  /* 0C00 bcc spc_0C04_028A01 */
    spc.a = spc_eor(spc.a, 0xFF);  /* 0C02 eor a,#$FF */
L_0C04:
    spc_push(spc.a);  /* 0C04 push a */
    spc.y = spc_nz(spc_rd(spc_dp(0xA0)));  /* 0C05 mov.b y,$A0 */
    spc_mul();  /* 0C07 mul ya */
    spc_st(spc_dp(0xA0), spc.y);  /* 0C08 mov.b $A0,y */
    spc.a = spc_pull();  /* 0C0A pop a */
    spc.y = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0C0B mov.b y,$A1 */
    spc_mul();  /* 0C0D mul ya */
    spc_st(spc_dp(0xA1), 0x00);  /* 0C0E mov $A1,#$00 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0C11 addw ya,$A0 */
    spc_movw_st(0xA0);  /* 0C13 movw $A0,ya */
    spc.y = spc_nz(spc_rd(spc_dp((uint8_t)(0x30 + spc.x))));  /* 0C15 mov.b y,$30+x */
    spc.a = spc_nz(spc_rd(spc_dp((uint8_t)(0x38 + spc.x))));  /* 0C17 mov.b a,$38+x */
    spc_set_psw(spc_pull());  /* 0C19 pop p */
    if (spc.n) goto L_0C20;  /* 0C1A bmi spc_0C20_028A1D */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0C1C addw ya,$A0 */
    { abi_VoicePitchOut(); return; }  /* 0C1E bra spc_0C22_028A1F */
L_0C20:
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 0C20 subw ya,$A0 */
    abi_VoicePitchOut();  /* on into 0C22 */
}

void abi_VoicePitchOut(void)
{
    spc.p = 0;  /* 0C22 clrp  */
    if ((spc_rd(spc_dp(0xB0)) >> 7 & 1)) goto L_0C2E;  /* 0C23 bbs $B0.7,spc_0C2E_028A2B */
    { uint8_t t = spc_rd(spc_dp(0xC5)); spc_wr(spc_dp(0xA1), t); }  /* 0C26 mov $A1,$C5 */
    spc_st(spc_dp(0xA0), 0x00);  /* 0C29 mov $A0,#$00 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0C2C addw ya,$A0 */
L_0C2E:
    spc.p = 1;  /* 0C2E setp  */
    spc.y = spc_dec(spc.y);  /* 0C2F dec y */
    spc_st(spc_dp(0xA1), spc.y);  /* 0C30 mov.b $A1,y */
    spc_st(spc_dp(0xA0), spc.a);  /* 0C32 mov.b $A0,a */
    spc.a = spc_nz(spc.y);  /* 0C34 mov a,y */
    spc.y = spc_nz(0x00);  /* 0C35 mov y,#$00 */
    spc.c = 1;  /* 0C37 setc  */
    spc.a = spc_sbc(spc.a, 0x34);  /* 0C38 sbc a,#$34 */
    if (spc.c) goto L_0C45;  /* 0C3A bcs spc_0C45_028A42 */
    spc.a = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0C3C mov.b a,$A1 */
    spc.c = 1;  /* 0C3E setc  */
    spc.a = spc_sbc(spc.a, 0x13);  /* 0C3F sbc a,#$13 */
    if (spc.c) goto L_0C49;  /* 0C41 bcs spc_0C49_028A46 */
    spc.y = spc_dec(spc.y);  /* 0C43 dec y */
    spc.a = spc_asl(spc.a);  /* 0C44 asl a */
L_0C45:
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA0)) | spc_rd(spc_dp(0xA1)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0C45 addw ya,$A0 */
    spc_movw_st(0xA0);  /* 0C47 movw $A0,ya */
L_0C49:
    spc_push(spc.x);  /* 0C49 push x */
    spc.a = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0C4A mov.b a,$A1 */
    spc.a = spc_asl(spc.a);  /* 0C4C asl a */
    spc.y = spc_nz(0x00);  /* 0C4D mov y,#$00 */
    spc.x = spc_nz(0x18);  /* 0C4F mov x,#$18 */
    spc_div();  /* 0C51 div ya,x */
    spc.x = spc_nz(spc.a);  /* 0C52 mov x,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0CC3 + spc.y)));  /* 0C53 mov.w a,$0CC3+y */
    spc_st(spc_dp(0xA3), spc.a);  /* 0C56 mov.b $A3,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0CC2 + spc.y)));  /* 0C58 mov.w a,$0CC2+y */
    spc_st(spc_dp(0xA2), spc.a);  /* 0C5B mov.b $A2,a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0CC5 + spc.y)));  /* 0C5D mov.w a,$0CC5+y */
    spc_push(spc.a);  /* 0C60 push a */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0CC4 + spc.y)));  /* 0C61 mov.w a,$0CC4+y */
    spc.y = spc_pull();  /* 0C64 pop y */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_set_ya(spc_subw(spc_ya(), w)); }  /* 0C65 subw ya,$A2 */
    spc.y = spc_nz(spc_rd(spc_dp(0xA0)));  /* 0C67 mov.b y,$A0 */
    spc_mul();  /* 0C69 mul ya */
    spc.a = spc_nz(spc.y);  /* 0C6A mov a,y */
    spc.y = spc_nz(0x00);  /* 0C6B mov y,#$00 */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0C6D addw ya,$A2 */
    spc_st(spc_dp(0xA1), spc.y);  /* 0C6F mov.b $A1,y */
    spc.a = spc_asl(spc.a);  /* 0C71 asl a */
    spc_rmw(spc_dp(0xA1), spc_rol);  /* 0C72 rol.b $A1 */
L_0C74:
    spc_cmp(spc.x, 0x08);  /* 0C74 cmp x,#$08 */
    if (spc.z) goto L_0C7E;  /* 0C76 beq spc_0C7E_028A7B */
    spc_rmw(spc_dp(0xA1), spc_lsr);  /* 0C78 lsr.b $A1 */
    spc.a = spc_ror(spc.a);  /* 0C7A ror a */
    spc.x = spc_inc(spc.x);  /* 0C7B inc x */
    goto L_0C74;  /* 0C7C bra spc_0C74_028A71 */
L_0C7E:
    spc_st(spc_dp(0xA0), spc.a);  /* 0C7E mov.b $A0,a */
    spc.x = spc_pull();  /* 0C80 pop x */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0228 + spc.x)));  /* 0C81 mov.w a,$0228+x */
    spc.y = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0C84 mov.b y,$A1 */
    spc_mul();  /* 0C86 mul ya */
    spc_movw_st(0xA2);  /* 0C87 movw $A2,ya */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0228 + spc.x)));  /* 0C89 mov.w a,$0228+x */
    spc.y = spc_nz(spc_rd(spc_dp(0xA0)));  /* 0C8C mov.b y,$A0 */
    spc_mul();  /* 0C8E mul ya */
    spc_push(spc.y);  /* 0C8F push y */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));  /* 0C90 mov.w a,$0220+x */
    spc.y = spc_nz(spc_rd(spc_dp(0xA0)));  /* 0C93 mov.b y,$A0 */
    spc_mul();  /* 0C95 mul ya */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0C96 addw ya,$A2 */
    spc_movw_st(0xA2);  /* 0C98 movw $A2,ya */
    spc.a = spc_nz(spc_rd((uint16_t)(0x0220 + spc.x)));  /* 0C9A mov.w a,$0220+x */
    spc.y = spc_nz(spc_rd(spc_dp(0xA1)));  /* 0C9D mov.b y,$A1 */
    spc_mul();  /* 0C9F mul ya */
    spc.y = spc_nz(spc.a);  /* 0CA0 mov y,a */
    spc.a = spc_pull();  /* 0CA1 pop a */
    { uint16_t w = (uint16_t)(spc_rd(spc_dp(0xA2)) | spc_rd(spc_dp(0xA3)) << 8); spc_set_ya(spc_addw(spc_ya(), w)); }  /* 0CA2 addw ya,$A2 */
    spc.a = spc_and(spc.a, 0xF0);  /* 0CA4 and a,#$F0 */
    spc_push(spc.y);  /* 0CA6 push y */
    spc.y = spc_nz(0x02);  /* 0CA7 mov y,#$02 */
    spc_call(abi_DspWriteVoice, 0x0CAC);  /* 0CA9 call $0CB0 */
    spc.a = spc_pull();  /* 0CAC pop a */
    spc.y = spc_inc(spc.y);  /* 0CAD inc y */
    { abi_DspWrite(); return; }  /* 0CAE bra $0CBB */
}

