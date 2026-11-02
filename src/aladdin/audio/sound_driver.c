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

/* A byte or word (low first) of the direct page, and stores to it. */
static uint8_t dp_rd(uint8_t d) { return spc_rd(spc_dp(d)); }
static void dp_wr(uint8_t d, uint8_t v) { spc_put(spc_dp(d), v); }
static uint16_t dp_rdw(uint8_t d) { uint16_t v = dp_rd(d); return (uint16_t)(v | dp_rd((uint8_t)(d + 1)) << 8); }
static void dp_wrw(uint8_t d, uint16_t v) { dp_wr(d, (uint8_t)v); dp_wr((uint8_t)(d + 1), (uint8_t)(v >> 8)); }

/* OR and AND of direct-page byte `src` into `dst`, the source read first
   (as OR dd,ds and AND dd,ds); N and Z from the result. */
static void dp_or(uint8_t dst, uint8_t src) { uint8_t v = dp_rd(src); dp_wr(dst, spc_or(dp_rd(dst), v)); }
static void dp_and(uint8_t dst, uint8_t src) { uint8_t v = dp_rd(src); dp_wr(dst, spc_and(dp_rd(dst), v)); }

/* DSP register `reg` = `v` (DspWrite; its CALL returning to `ret`). */
static void dsp_set(uint8_t reg, uint8_t v, uint16_t ret)
{
    spc.y = reg;
    spc.a = v;
    spc_call(abi_DspWrite, ret);
}

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
    uint16_t at;
    uint8_t v, x, y, mask;

    /* the stack at $01CF; the direct pages and $0200-$02BF cleared */
    spc.p = 0;
    spc.sp = 0xCF;
    for (at = 0x0000; at < 0x00E0; at++)
        spc_wr(at, 0);
    for (at = 0x0100; at < 0x01B0; at++)
        spc_wr(at, 0);
    for (at = 0x0200; at < 0x02C0; at++)
        spc_wr(at, 0);
    spc.a = 0;
    spc.x = 0xC0;
    spc_call(abi_DspSetup, 0x02E0);
    /* timer 0 at 8 ms, the ports cleared, the sequence count 1 */
    spc_st(0x00FA, 0x40);
    spc_st(0x00F1, 0xF1);
    dp_wr(0xB2, 0x01);
    spc_st(0x00F6, 0x01);
    spc_st(0x00F7, 0x00);
    for (;;) {
        /* commands from the S-CPU until the timer has ticked */
        spc_call(abi_PortCommandPoll, 0x02F8);
        spc.c = 0;
        v = spc_rd(0x00FD);
        v = spc_adc(v, dp_rd(0xB1));
        dp_wr(0xB1, v);
        if (spc.z)
            continue;
        dp_wr(0xB1, (uint8_t)(v - 1));
        /* each tick ends in DspWrite, whose RET comes back here */
        spc_push(0x02);
        spc_push(0xF5);
        spc.c = 0;
        v = spc_adc(dp_rd(0xB0), 0x80);
        dp_wr(0xB0, v);
        if (!(v & 0x80)) {
            /* every other tick: the DSP brought up to date */
            v = dp_rd(0xCD);
            if (dp_rd(0xCC) != v) {
                /* muting: key-off all, then echo off, $CD ticks */
                if (!(dp_rd(0xCD) & 0x80)) {
                    dsp_set(0x6C, 0x00, 0x0322);
                    dsp_set(0x4D, 0xFF, 0x0329);
                }
                dp_wr(0xCD, (uint8_t)(dp_rd(0xCD) + 1));
            } else {
                v = (uint8_t)~dp_rd(0xB8);
                v &= dp_rd(0xBC);
                v |= dp_rd(0xB7);
                dsp_set(0x4D, v, 0x033A);
                dsp_set(0x2C, dp_rd(0xCE), 0x0341);
                dsp_set(0x3C, dp_rd(0xCF), 0x0348);
            }
            /* each voice whose envelope changed: its GAIN and ADSR1 */
            x = 0x07;
            y = 0x57;
            v = dp_rd(0xBA);
            spc_push((uint8_t)(v | dp_rd(0xB5)));
            do {
                spc_push(y);
                spc.x = x;
                if (dp_rd(0xB5) & 0x80) {
                    dp_wr(0xA3, spc_rd((uint16_t)(0x0238 + y)));
                    spc.a = spc_rd((uint16_t)(0x0240 + y));
                    goto envelope;
                }
                if (dp_rd(0xBD) & 0x80) {
                    spc.a = spc_rd((uint16_t)(0x0250 + x));
                    spc_call(abi_VoiceInstrument, 0x0368);
                    goto next;
                }
                if (!(dp_rd(0xBA) & 0x80))
                    goto next;
                dp_wr(0xA3, spc_rd((uint16_t)(0x0238 + x)));
                spc.a = spc_rd((uint16_t)(0x0240 + x));
envelope:
                spc.y = 0x07;
                spc_call(abi_DspWriteVoice, 0x037A);
                spc.y = 0x05;
                spc.a = dp_rd(0xA3);
                spc_call(abi_DspWriteVoice, 0x0381);
next:
                dp_wr(0xB5, (uint8_t)(dp_rd(0xB5) << 1));
                dp_wr(0xBD, (uint8_t)(dp_rd(0xBD) << 1));
                dp_wr(0xBA, (uint8_t)(dp_rd(0xBA) << 1));
                y = (uint8_t)(spc_pull() - 1);
                x = (uint8_t)(spc.x - 1);
                spc.x = x;
                spc.y = y;
            } while (!(x & 0x80));
            dsp_set(0x5C, 0x00, 0x0393);
            spc.a = spc_pull();
            spc.y = 0x4C;
            spc_nz(0x4C);
            abi_DspWrite();
            continue;
        }
        /* the other ticks: the voices' tracks */
        dp_wr(0xB0, (uint8_t)(dp_rd(0xB0) & 0x7F));
        if (!(dp_rd(0xB0) & 0x40)) {
            /* the song's voices, at the song's tempo (with its slide) */
            dp_wr(0xB3, 0x80);
            dp_wr(0xB9, dp_rd(0xB8));
            spc.x = 0x07;
            spc_movw_ld(0xBE);
            spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xC0)));
            dp_wrw(0xA9, spc_ya());
            spc.y = 0x00;
            spc.a = dp_rd(0xC2);
            spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA9)));
            dp_wr(0xA8, spc.y);
            dp_wr(0xC2, spc.a);
            do {
                v = dp_rd((uint8_t)(0x08 + spc.x));
                if (v | dp_rd((uint8_t)(0x00 + spc.x)))
                    spc_call(abi_VoiceTick, 0x03BF);
                spc.x--;
                dp_wr(0xB9, (uint8_t)(dp_rd(0xB9) << 1));
                v = spc_lsr(dp_rd(0xB3));
                dp_wr(0xB3, v);
            } while (v);
            /* the fade */
            spc_movw_ld(0xC7);
            spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xC9)));
            if (spc.c) {
                dp_wr(0xC7, 0x00);
                dp_wr(0xC8, 0x00);
                spc.y = 0xFF;
                spc.a = 0xFF;
            }
            dp_wrw(0xC9, spc_ya());
        }
        /* the sound effects' voices, each at its own tempo */
        dp_wr(0xB0, (uint8_t)(dp_rd(0xB0) | 0x80));
        dp_wr(0xB3, 0x80);
        dp_wr(0xB9, 0x00);
        spc.x = 0x57;
        do {
            v = dp_rd(0xB8);
            if (v & dp_rd(0xB3)) {
                dp_wr(0xAA, spc_rd((uint16_t)(0x0260 + spc.x)));
                dp_wr(0xA9, spc_rd((uint16_t)(0x0258 + spc.x)));
                spc.y = 0x00;
                spc.a = spc_rd((uint16_t)(0x0268 + spc.x));
                spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA9)));
                dp_wr(0xA8, spc.y);
                spc_put((uint16_t)(0x0268 + spc.x), spc.a);
                spc_call(abi_VoiceTick, 0x0400);
            }
            spc.x--;
            mask = spc_lsr(dp_rd(0xB3));
            dp_wr(0xB3, mask);
        } while (mask);
        /* key-offs of both */
        v = dp_rd(0xB6);
        spc.a = spc_or(v, dp_rd(0xBB));
        dp_wr(0xB6, 0x00);
        dp_wr(0xBB, 0x00);
        spc.y = spc_nz(0x5C);
        abi_DspWrite();
    }
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
    uint8_t y;

    /* three DSP registers from the tables at $0438 and $043B */
    for (y = 3; y; y--) {
        spc_st(0x00F2, spc_rd((uint16_t)(0x0437 + y)));
        spc.a = spc_rd((uint16_t)(0x043A + y));
        spc_st(0x00F3, spc.a);
    }
    spc.y = 0;
    spc_nz(spc.a);
    abi_EchoSetup();
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
    uint8_t cmd;

    /* port 0 read until two reads agree */
    do {
        cmd = spc_rd(0x00F4);
        spc_cmp(cmd, spc_rd(0x00F4));
    } while (!spc.z);
    spc.a = cmd;
    spc.y = spc_nz(cmd);
    if (!cmd) {
        abi_PortCommandPollEnd();
        return;
    }
    /* the command run; then the count and the command back on ports 2-3 */
    spc_push(cmd);
    dp_wr(0xB2, (uint8_t)(dp_rd(0xB2) + 1));
    spc_call(abi_SoundCommand, 0x044F);
    spc_st(0x00F1, 0x31);
    spc.a = spc_nz(dp_rd(0xB2));
    spc_st(0x00F6, spc.a);
    spc.a = spc_pull();
    spc_st(0x00F7, spc.a);
    abi_PortCommandPollEnd();
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
    uint8_t mask;

    spc_cmp(spc.a, 0xF0);
    if (spc.c) {
        abi_SoundCommandTable();
        return;
    }
    /* the sound's number within the bank's count, its sequence address */
    for (;;) {
        spc_cmp(spc.a, spc_rd(0x1DB0));
        if (!spc.c)
            break;
        spc.c = 1;
        spc.a = spc_sbc(spc.a, spc_rd(0x1DB0));
    }
    spc.a = spc_asl(spc.a);
    spc.x = spc.a;
    dp_wr(0xA1, spc_rd((uint16_t)(0x1DB2 + spc.x)));
    spc.a = spc_rd((uint16_t)(0x1DB3 + spc.x));
    dp_wr(0xA0, spc.a);
    spc.a = spc_or(spc.a, dp_rd(0xA1));
    if (spc.z) {
        abi_PortCommandPollEnd();
        return;
    }
    spc.y = 0;
    spc.a = spc_nz(spc_rd(ind_y(0xA0)));
    if (spc.z) {
        abi_SongStartKeep();
        return;
    }
    /* a sound effect: its priority, then a track per voice it uses; each
       voice taken when the effect is not below the voice's own */
    dp_wr(0xA4, spc.a);
    dp_wr(0xB3, 0x80);
    spc.x = 0x57;
    do {
        spc.y = 0;
        spc_incw(0xA0, 1);
        dp_wr(0xA3, spc_rd(ind_y(0xA0)));
        spc_incw(0xA0, 1);
        dp_wr(0xA2, spc_rd(ind_y(0xA0)));
        spc.a = spc_rd(ind_y(0xA2));
        spc_cmp(spc.a, 0x17);
        if (spc.z)
            goto next;
        spc.a = dp_rd(0xA4);
        spc_cmp(spc.a, spc_rd((uint16_t)(0x0250 + spc.x)));
        if (!spc.c)
            goto next;
        dp_or(0xB8, 0xB3);
        mask = (uint8_t)~dp_rd(0xB3);
        dp_wr(0xA5, mask);
        dp_and(0xB7, 0xA5);
        dp_and(0xBA, 0xA5);
        spc_call(abi_VoiceStateClear, 0x04BD);
        spc_call(abi_VoiceRelease, 0x04C0);
        spc_put((uint16_t)(0x0250 + spc.x), dp_rd(0xA4));
        spc_movw_ld(0xA2);
        dp_wr((uint8_t)(0x08 + spc.x), spc.y);
        dp_wr((uint8_t)(0x00 + spc.x), spc.a);
        spc_put((uint16_t)(0x0100 + spc.x), 0x10);
        spc_put((uint16_t)(0x0240 + spc.x), 0xB8);
        spc.c = 0;
        spc.a = spc_adc(spc_rd(0x00F5), 0x80);
        spc_put((uint16_t)(0x0248 + spc.x), spc.a);
next:
        spc.x--;
        mask = spc_lsr(dp_rd(0xB3));
        dp_wr(0xB3, mask);
    } while (mask);
    spc_ret();
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
    /* the voice's bytes in the $0200 page, 8 apart */
    spc.c = 0;
    spc.a = spc_adc(spc.x, 0x08);
    do {
        spc.y = spc.a;
        spc_put((uint16_t)(0x0200 + spc.y), 0x00);
        spc.c = 0;
        spc.a = spc_adc(spc.y, 0x08);
        spc_cmp(spc.a, 0xC0);
    } while (!spc.c);
    /* and in both direct pages */
    spc.p = 1;
    spc_call(abi_VoiceDpClear, 0x04FA);
    spc.p = 0;
    abi_VoiceDpClear();
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
    uint8_t x = spc.x;

    spc_push(x);
    do {
        dp_wr(x, 0x00);
        spc.c = 0;
        spc.a = spc_adc(x, 0x08);
        x = spc.a;
        spc_cmp(x, 0xA0);
    } while (!spc.c);
    spc.x = spc_pull();
    spc_ret();
}

void abi_SoundCommandTable(void)
{
    uint16_t t;

    /* commands $F0-$FF by the table at $0511 */
    spc.a = spc_asl((uint8_t)(spc.a & 0x0F));
    spc.x = spc.a;
    t = spc_rd((uint16_t)(0x0511 + spc.x));
    t |= (uint16_t)(spc_rd((uint16_t)(0x0512 + spc.x)) << 8);
    switch (t) {
    case 0x052D: abi_SoundStopAll(); return;
    case 0x0530: abi_SoundCmdF1(); return;
    case 0x0549: abi_SequencerReset(); return;
    case 0x05DC: abi_SoundCmdF3(); return;
    case 0x05E0: abi_SoundCmdF4(); return;
    case 0x0616: abi_SoundCmdF5(); return;
    case 0x061A: abi_SoundCmdF6(); return;
    case 0x0650: abi_SoundCmdF7(); return;
    case 0x0653: abi_SoundCmdF8(); return;
    case 0x0656: abi_SoundUpload(); return;
    case 0x0567: abi_SoundCmdFA(); return;
    case 0x056A: abi_SoundCmdFB(); return;
    case 0x05B1: abi_SoundCmdFC(); return;
    case 0x05D6: abi_SoundCmdFD(); return;
    }
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
    spc_call(abi_SequencerReset, 0x0530);
    abi_SoundCmdF1();
}

void abi_SoundCmdF1(void)
{
    uint8_t mask;

    /* every effect voice stopped */
    dp_wr(0xB3, 0x80);
    spc.x = 0x57;
    do {
        if (dp_rd(0xB8) & 0x80)
            spc_call(abi_VoiceStop, 0x053B);
        spc.x--;
        dp_wr(0xB8, (uint8_t)(dp_rd(0xB8) << 1));
        mask = spc_lsr(dp_rd(0xB3));
        dp_wr(0xB3, mask);
    } while (mask);
    dp_wr(0xB5, 0x00);
    dp_wr(0xB7, 0x00);
    spc_ret();
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
    spc_call(abi_VoicesReleaseFree, 0x054C);
    spc.a = 0;
    dp_wr(0xBC, 0x00);
    for (spc.x = 0x08; spc.x != 0x58; spc.x++)
        spc_put((uint16_t)(0x0200 + spc.x), 0x00);
    spc.p = 1;
    spc_call(abi_PageClear50, 0x055E);
    spc.p = 0;
    abi_PageClear50();
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
    uint8_t x;

    for (x = 0; x < 0x50; x++)
        spc_wr(spc_dp(x), spc.a);
    spc.x = 0x50;
    spc_cmp(0x50, 0x50);
    spc_ret();
}

void abi_SoundCmdFA(void)
{
    spc_call(abi_SoundUpload, 0x056A);
    abi_SoundCmdFB();
}

void abi_SoundCmdFB(void)
{
    dp_wr(0xA1, 0x0D);
    dp_wr(0xA0, 0x1F);
    abi_SongStartKeep();
}

void abi_SongStartKeep(void)
{
    spc_call(abi_SequencerReset, 0x0573);
    /* tempo slide, transpose and volume cleared, the volume full */
    spc.y = 0;
    spc.a = 0;
    dp_wrw(0xC0, 0x0000);
    dp_wr(0xC4, 0x00);
    dp_wr(0xC5, 0x00);
    spc.a = 0xFF;
    dp_wr(0xC3, 0xFF);
    spc_call(abi_FadeClear, 0x0582);
    /* the song's 8 tracks */
    spc.x = 0x07;
    do {
        spc.y = 0;
        spc_incw(0xA0, 1);
        dp_wr(0xA3, spc_rd(ind_y(0xA0)));
        spc_incw(0xA0, 1);
        dp_wr(0xA2, spc_rd(ind_y(0xA0)));
        spc.a = spc_rd(ind_y(0xA2));
        spc_cmp(spc.a, 0x17);
        if (!spc.z) {
            spc_movw_ld(0xA2);
            dp_wr((uint8_t)(0x08 + spc.x), spc.y);
            dp_wr((uint8_t)(0x00 + spc.x), spc.a);
            spc_put((uint16_t)(0x0100 + spc.x), 0x10);
            spc_put((uint16_t)(0x0240 + spc.x), 0xB8);
            spc.a = 0x80;
            spc_put((uint16_t)(0x0248 + spc.x), 0x80);
        }
        spc.x = spc_dec(spc.x);
    } while (!spc.n);
    spc_ret();
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
    uint8_t v;

    /* the tempo slide: port 1 (signed) * $69 / 256 + * 3 */
    v = spc_nz(spc_rd(0x00F5));
    spc.a = v;
    spc_push(spc_psw());
    if (v & 0x80)
        spc.a = (uint8_t)((v ^ 0xFF) + 1);
    spc_push(spc.a);
    spc.y = 0x69;
    spc_mul();
    dp_wr(0xA3, 0x00);
    dp_wr(0xA2, spc.y);
    spc.a = spc_pull();
    spc.y = 0x03;
    spc_mul();
    spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA2)));
    spc_set_psw(spc_pull());
    if (spc.n) {
        dp_wrw(0xA2, spc_ya());
        spc.y = 0;
        spc.a = 0;
        spc_set_ya(spc_subw(0x0000, dp_rdw(0xA2)));
    }
    dp_wrw(0xC0, spc_ya());
    spc_ret();
}

void abi_SoundCmdFD(void)
{
    spc.a = spc_nz(spc_rd(0x00F5));
    dp_wr(0xC5, spc.a);
    spc_ret();
}

void abi_SoundCmdF3(void)
{
    spc_bit_set(0xB0, 6, 1);
    abi_VoicesReleaseFree();
}

void abi_SoundCmdF4(void)
{
    uint8_t mask;

    spc_bit_set(0xB0, 6, 0);
    dp_wr(0xBA, 0x00);
    /* each voice no effect holds: back to the song's settings */
    dp_wr(0xB3, 0x80);
    spc.x = 0x57;
    do {
        spc.a = dp_rd(0xB8);
        spc.a = spc_and(spc.a, dp_rd(0xB3));
        if (spc.z)
            spc_call(abi_VoiceSfxMark, 0x05F3);
        spc.x--;
        mask = spc_lsr(dp_rd(0xB3));
        dp_wr(0xB3, mask);
    } while (mask);
    spc_ret();
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
    uint8_t mask;

    /* each voice no effect holds: released, and its key-off asked for */
    spc.x = 0x07;
    dp_wr(0xB3, 0x80);
    do {
        spc.a = dp_rd(0xB8);
        spc.a = spc_and(spc.a, dp_rd(0xB3));
        if (spc.z) {
            spc_call(abi_VoiceRelease, 0x0607);
            dp_or(0xBB, 0xB3);
        }
        spc.x--;
        mask = spc_lsr(dp_rd(0xB3));
        dp_wr(0xB3, mask);
    } while (mask);
    dp_wr(0xBA, 0x00);
    dp_wr(0xBD, 0x00);
    spc_ret();
}

void abi_SoundCmdF5(void)
{
    spc.x = 0xFF;
    abi_FadeStart();
}

void abi_SoundCmdF6(void)
{
    spc.x = 0x01;
    abi_FadeStart();
}

void abi_FadeStart(void)
{
    uint8_t was;

    /* a fade out (X = $FF) or in (1) at the speed in ports 2-3 */
    spc_cmp(spc.x, dp_rd(0xC6));
    if (spc.z) {
        spc_ret();
        return;
    }
    spc.a = spc_rd(0x00F6);
    spc.a = spc_or(spc.a, spc_rd(0x00F7));
    if (spc.z) {
        abi_FadeClear();
        return;
    }
    spc.a = spc_rd(0x00F6);
    spc.y = spc_rd(0x00F7);
    was = spc_asl(dp_rd(0xC6));
    dp_wr(0xC6, was);
    if (spc.z) {
        dp_wr(0xC9, 0x00);
        dp_wr(0xCA, 0x00);
        abi_FadeSet();
        return;
    }
    /* turned round: from where it had got to */
    spc_call(abi_FadeSet, 0x0635);
    spc.y = 0xFF;
    spc.a = 0xFF;
    spc_set_ya(spc_subw(0xFFFF, dp_rdw(0xC9)));
    dp_wrw(0xC9, spc_ya());
    spc_ret();
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
    spc.a = spc_nz(0x00);
    spc.y = 0;
    spc.x = 0;
    dp_wrw(0xC9, 0x0000);
    abi_FadeSet();
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
    dp_wrw(0xC7, spc_ya());
    dp_wr(0xC6, spc.x);
    spc_ret();
}

void abi_SoundCmdF7(void)
{
    spc_bit_set(0xB0, 5, 0);
    spc_ret();
}

void abi_SoundCmdF8(void)
{
    spc_bit_set(0xB0, 5, 1);
    spc_ret();
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
    uint8_t y, v;

    spc_call(abi_SoundStopAll, 0x0659);
    /* ready: $AA, $BB on ports 0-1, until the S-CPU answers $CC */
    spc_st(0x00F4, 0xAA);
    spc_st(0x00F5, 0xBB);
    do
        spc_cmp(spc_rd(0x00F4), 0xCC);
    while (!spc.z);
    for (;;) {
        /* a block: its address (ports 2-3); port 1 zero ends the upload */
        spc.a = spc_rd(0x00F6);
        spc.y = spc_rd(0x00F7);
        dp_wrw(0xA0, spc_ya());
        y = spc_rd(0x00F4);
        spc.a = spc_nz(spc_rd(0x00F5));
        if (spc.z)
            break;
        spc_st(0x00F4, y);
        do
            y = spc_rd(0x00F4);
        while (y);
        /* each byte: port 1 when port 0 shows its count, echoed back */
        for (;;) {
            spc_cmp(y, spc_rd(0x00F4));
            if (spc.z) {
                v = spc_rd(0x00F5);
                spc_st(0x00F4, y);
                spc_put((uint16_t)(dp_rdw(0xA0) + y), v);
                if (!++y)
                    dp_wr(0xA1, (uint8_t)(dp_rd(0xA1) + 1));
                continue;
            }
            if (!spc.n)
                continue;
            spc_cmp(y, spc_rd(0x00F4));
            if (spc.n)
                break;
        }
    }
    spc_st(0x00F6, spc.a);
    dp_wr(0xB2, (uint8_t)(spc.a + 1));
    spc.a = spc_nz(spc_rd(0x00FD));
    dp_wr(0xB1, 0x00);
    spc.y = y;
    spc_st(0x00F4, y);
    spc_ret();
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
    uint8_t x = spc.x, len, f, v, a;
    uint16_t w;

    /* $B4: the voice's bit inverted (copied, then flipped in place) */
    dp_wr(0xB4, dp_rd(0xB3));
    dp_wr(0xB4, (uint8_t)~dp_rd(0xB4));
    len = dp_rd((uint8_t)(0x18 + x));
    if (len) {
        /* the note's gate: at its end, the release point */
        spc.y = dp_rd((uint8_t)(0x28 + x));
        if (spc.y | dp_rd((uint8_t)(0x20 + x))) {
            w = (uint16_t)(spc.y << 8 | dp_rd((uint8_t)(0x20 + x)));
            spc_set_ya(spc_subw(w, dp_rdw(0xA9)));
            if (spc.z || !spc.c) {
                if (!(dp_rd(0xB9) & 0x80))
                    spc_call(abi_VoiceAdsr1Set, 0x06D0);
                spc.y = 0;
                spc.a = spc_nz(0);
            }
            dp_wr((uint8_t)(0x28 + x), spc.y);
            dp_wr((uint8_t)(0x20 + x), spc.a);
        }
        /* the note's time down by the tick; while it runs, its slide */
        spc.c = 1;
        spc.a = dp_rd((uint8_t)(0x18 + x));
        spc.a = spc_sbc(spc.a, dp_rd(0xA8));
        dp_wr((uint8_t)(0x18 + x), spc.a);
        if (!spc.z && spc.c) {
            spc_cmp(spc.a, 0xC1);
            if (!spc.c) {
                abi_VoiceSlide();
                return;
            }
        }
    }
    /* the track's commands, up to its next note; a note comes back to
       $06F2 by the RET of the loop */
    spc.y = 0x06;
    spc.a = 0xF2;
    spc_push(0x06);
    spc_push(0xF2);
    goto commands;
note:
    /* the note's length: by its top 3 bits from one of three tables (by
       the voice's flags, the dotted one once) */
    spc_push(spc.a);
    spc.y = (uint8_t)(((spc.a >> 5) & 0x07) - 1);
    f = dp_rd((uint8_t)(0x10 + x));
    if (!(f & 0x30)) {
        v = spc_rd((uint16_t)(0x07E5 + spc.y));
    } else if (!(f & 0x10)) {
        v = spc_rd((uint16_t)(0x07DE + spc.y));
    } else {
        dp_wr((uint8_t)(0x10 + x), (uint8_t)(f & 0xEF));
        v = spc_rd((uint16_t)(0x07EC + spc.y));
    }
    spc.c = 0;
    dp_wr((uint8_t)(0x18 + x), spc_adc(v, dp_rd((uint8_t)(0x18 + x))));
    /* its pitch: 0 is a rest */
    spc.a = spc_and(spc_pull(), 0x1F);
    if (spc.z) {
        spc_ret();
        return;
    }
    spc_push(spc.a);
    spc.y = (uint8_t)(dp_rd((uint8_t)(0x10 + x)) & 0x0F);
    a = spc_pull();
    spc.c = 0;
    a = spc_adc(a, spc_rd((uint16_t)(0x07F3 + spc.y)));
    if (!(dp_rd(0xB0) & 0x80)) {
        spc.c = 0;
        a = spc_adc(a, dp_rd(0xC4));
    }
    spc.c = 0;
    a = spc_adc(a, spc_rd((uint16_t)(0x0210 + x)));
    /* the semitone and the voice's fine tune (signed), at most $60 */
    dp_wr(0xA1, a);
    dp_wr(0xA0, 0x00);
    spc.a = spc_rd((uint16_t)(0x0218 + x));
    spc.y = spc.a & 0x80 ? 0xFF : 0x00;
    spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA0)));
    spc_cmp(spc.y, 0x61);
    if (spc.c)
        spc.y = 0x60;
    /* in page 1: the pitch slide's start, from the last note's pitch
       ($40/$48) when it slides (bit 1: down) */
    spc.p = 1;
    dp_wrw(0xA2, spc_ya());
    dp_wr(0xA4, dp_rd((uint8_t)(0x00 + x)));
    dp_wr(0xA4, (uint8_t)(dp_rd(0xA4) & 0xFE));
    v = dp_rd((uint8_t)(0x28 + x));
    if (v)
        v = dp_rd((uint8_t)(0x40 + x));
    if (v)
        spc_cmp(v, dp_rd(0xA3));
    if (v && !spc.z) {
        if (spc.c)
            dp_wr(0xA4, (uint8_t)(dp_rd(0xA4) & ~0x02));
        else
            dp_wr(0xA4, (uint8_t)(dp_rd(0xA4) | 0x02));
        dp_wr(0xA4, (uint8_t)(dp_rd(0xA4) + 1));
        spc.y = dp_rd((uint8_t)(0x40 + x));
        spc.a = dp_rd((uint8_t)(0x48 + x));
    } else {
        spc.a = dp_rd(0xA2);
    }
    spc_push(spc.a);
    dp_wr((uint8_t)(0x00 + x), dp_rd(0xA4));
    dp_wr((uint8_t)(0x40 + x), dp_rd(0xA3));
    dp_wr((uint8_t)(0x48 + x), dp_rd(0xA2));
    spc.a = spc_pull();
    dp_wr((uint8_t)(0x30 + x), spc.y);
    dp_wr((uint8_t)(0x38 + x), spc.a);
    spc.p = 0;
    /* key-on, unless tied: the envelope reset, the voice's key bits */
    if (!(dp_rd((uint8_t)(0x10 + x)) & 0x80)) {
        v = spc_rd((uint16_t)(0x0100 + x));
        spc.y = v;
        if (v & 0x10) {
            spc_put((uint16_t)(0x0100 + x), (uint8_t)(v & 0x3F));
            spc.a = 0;
            spc_put((uint16_t)(0x0120 + x), 0x00);
        }
        if (dp_rd(0xB0) & 0x80) {
            dp_or(0xB5, 0xB3);
            dp_or(0xB6, 0xB3);
        } else {
            if (dp_rd(0xB9) & 0x80)
                goto gate;
            dp_or(0xBA, 0xB3);
            dp_or(0xBB, 0xB3);
        }
        spc_call(abi_VoiceRelease, 0x07B3);
    }
    if (!(dp_rd(0xB9) & 0x80)) {
        spc.p = 1;
        spc_call(abi_VoicePitchSet, 0x07BA);
        spc_call(abi_VoiceVolumeSet, 0x07BD);
    }
gate:
    /* the tie flag moved from bit 6 to bit 7; the gate: the note's time
       by the voice's gate fraction ($0208), none when tied */
    f = dp_rd((uint8_t)(0x10 + x));
    spc.y = f;
    dp_wr(0xA3, (uint8_t)(f & 0x7F));
    spc.c = 0;
    spc.a = spc_or((uint8_t)((f & 0x40) << 1), dp_rd(0xA3));
    dp_wr((uint8_t)(0x10 + x), spc.a);
    if (spc.n) {
        spc.y = 0;
        spc.a = spc_nz(0);
    } else {
        spc.y = dp_rd((uint8_t)(0x18 + x));
        spc.a = spc_rd((uint16_t)(0x0208 + x));
        spc_mul();
    }
    dp_wr((uint8_t)(0x28 + x), spc.y);
    dp_wr((uint8_t)(0x20 + x), spc.a);
    spc_ret();
    return;
commands:
    /* The command loop. A command below $20 is run by pushing $0803 (this
       loop) and its handler's address and returning into it, so the
       handler's RET comes back here; one that pops two return addresses
       (end of track, $097C) leaves the voice's tick. A byte of $20 or more
       is a note: the RET pulls $06F2, pushed before the loop. */
    for (;;) {
        uint8_t top, lo, hi;
        spc_call(abi_TrackByte, 0x0806);
        spc_cmp(spc.a, 0x20);
        if (spc.c) {  /* 0808 bcs: 0830 ret, to $06F2 */
            spc_ret();
            goto note;
        }
        dp_wr(0xA7, spc.a);
        spc_push(0x08);
        spc_push(0x03);
        top = spc.sp;
        spc.a = spc_asl(spc.a);
        spc.y = spc.a;
        hi = spc_rd((uint16_t)(0x0832 + spc.y));
        spc_push(hi);
        lo = spc.a = spc_nz(spc_rd((uint16_t)(0x0831 + spc.y)));
        spc_push(lo);
        spc_cmp(spc.y, 0x08);
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
    uint8_t x = spc.x, lo;

    /* the byte at the voice's track pointer; the pointer on one */
    spc.y = dp_rd((uint8_t)(0x08 + x));
    spc.a = dp_rd((uint8_t)(0x00 + x));
    dp_wrw(0xA0, spc_ya());
    spc.y = 0;
    spc.a = spc_rd(dp_rdw(0xA0));
    lo = spc_inc(dp_rd((uint8_t)(0x00 + x)));
    dp_wr((uint8_t)(0x00 + x), lo);
    if (!lo)
        dp_wr((uint8_t)(0x08 + x), spc_inc(dp_rd((uint8_t)(0x08 + x))));
    abi_TrackByteEnd();
}

void abi_TrackByteEnd(void)
{
    spc_ret(); return;  /* 0830 ret  */
}

void abi_SeqCmd00(void)
{
    spc.a = 0x20;
    abi_VoiceFlagsToggle();
}

void abi_SeqCmd01(void)
{
    spc.a = 0x40;
    abi_VoiceFlagsToggle();
}

void abi_SeqCmd02(void)
{
    spc.a = spc_or(0x10, dp_rd((uint8_t)(0x10 + spc.x)));
    abi_VoiceFlagsStore();
}

void abi_SeqCmd03(void)
{
    spc.a = 0x08;
    abi_VoiceFlagsToggle();
}

void abi_VoiceFlagsToggle(void)
{
    spc.a = spc_eor(spc.a, dp_rd((uint8_t)(0x10 + spc.x)));
    abi_VoiceFlagsStore();
}

void abi_VoiceFlagsStore(void)
{
    dp_wr((uint8_t)(0x10 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd05(void)
{
    /* the tempo: an effect's own, or the song's */
    spc_push(spc.a);
    spc_call(abi_TrackByte, 0x088A);
    spc.y = spc_pull();
    if (dp_rd(0xB0) & 0x80) {
        spc_put((uint16_t)(0x0258 + spc.x), spc.a);
        spc.a = spc.y;
        spc_put((uint16_t)(0x0260 + spc.x), spc.y);
        spc.a = spc_nz(0x00);
        spc_put((uint16_t)(0x0268 + spc.x), 0x00);
    } else {
        dp_wrw(0xBE, spc_ya());
        dp_wr(0xC2, 0x00);
    }
    spc_ret();
}

void abi_SeqCmd06(void)
{
    spc_put((uint16_t)(0x0208 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd07(void)
{
    /* the voice's volume by the curve; a song's scaled by the bank's */
    spc.y = spc.a;
    spc_call(abi_CurveAtY, 0x08A9);
    if (!(dp_rd(0xB0) & 0x80)) {
        spc_push(spc.a);
        spc.y = spc_rd(0x1DB1);
        spc_call(abi_CurveAtY, 0x08B3);
        spc.y = spc_pull();
        spc_mul();
        spc.a = spc_nz(spc.y);
    }
    spc_put((uint16_t)(0x0230 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd08(void)
{
    if (dp_rd(0xB0) & 0x80) {
        abi_VoiceInstrumentFrom();
        return;
    }
    /* a song voice keeps its instrument (+1) for when effects let go */
    spc.a = spc_inc(spc.a);
    spc_put((uint16_t)(0x0250 + spc.x), spc.a);
    if (dp_rd(0xB9) & 0x80) {
        abi_VoiceInstrumentEnd();
        return;
    }
    abi_VoiceInstrument();
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
    spc.a = spc_dec(spc.a);
    abi_VoiceInstrumentFrom();
}

void abi_VoiceInstrumentFrom(void)
{
    uint16_t at;
    uint8_t reg, i;

    /* instrument A's 6 bytes at $3438 + 6A: its source, ADSR1, ADSR2,
       GAIN to the voice's DSP registers 4-7 (its ADSR1 kept), its pitch
       multiplier */
    spc.y = 0x06;
    spc_mul();
    dp_wrw(0xA0, spc_ya());
    spc.c = 0;
    dp_wr(0xA0, spc_adc(dp_rd(0xA0), 0x38));
    dp_wr(0xA1, spc_adc(dp_rd(0xA1), 0x34));
    at = dp_rdw(0xA0);
    spc_put((uint16_t)(0x0238 + spc.x), spc_rd((uint16_t)(at + 1)));
    spc_push(spc.x);
    reg = (uint8_t)(((spc.x << 4 | spc.x >> 4) & 0x70) | 0x04);
    for (i = 0; i < 4; i++) {
        spc.a = spc_rd((uint16_t)(at + i));
        spc_st(0x00F2, (uint8_t)(reg + i));
        spc_st(0x00F3, spc.a);
    }
    spc_cmp(4, 4);
    spc.x = spc_pull();
    spc_put((uint16_t)(0x0220 + spc.x), spc_rd((uint16_t)(at + 4)));
    spc.y = 0x05;
    spc.a = spc_nz(spc_rd((uint16_t)(at + 5)));
    spc_put((uint16_t)(0x0228 + spc.x), spc.a);
    abi_VoiceInstrumentEnd();
}

void abi_VoiceInstrumentEnd(void)
{
    spc_ret(); return;  /* 08FB ret  */
}

void abi_SeqCmd09(void)
{
    dp_wr(0xA3, spc.a);
    spc.a = (uint8_t)(dp_rd((uint8_t)(0x10 + spc.x)) & 0xF8);
    spc.a = spc_or(spc.a, dp_rd(0xA3));
    dp_wr((uint8_t)(0x10 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd0A(void)
{
    if (!(dp_rd(0xB0) & 0x80))
        dp_wr(0xC4, spc.a);
    spc_ret();
}

void abi_SeqCmd0B(void)
{
    spc_put((uint16_t)(0x0210 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd0C(void)
{
    spc_put((uint16_t)(0x0218 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd0D(void)
{
    spc_put((uint16_t)(0x0128 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd0E(void)
{
    spc.y = 0x30;
    abi_SeqLoop();
}

void abi_SeqCmd0F(void)
{
    spc.y = 0x38;
    abi_SeqLoop();
}

void abi_SeqCmd10(void)
{
    spc.y = 0x40;
    abi_SeqLoop();
}

void abi_SeqCmd11(void)
{
    spc.y = 0x48;
    abi_SeqLoop();
}

void abi_SeqLoop(void)
{
    uint8_t count;

    /* the voice's loop counter (at Y + X): commands below $12 count down
       and jump back while it runs; the others jump on its last pass */
    dp_wr(0xA3, spc.a);
    dp_wr(0xA0, spc.y);
    spc.c = 0;
    spc.a = spc_adc(spc.x, dp_rd(0xA0));
    spc.y = spc.a;
    dp_wr(0xA1, 0x00);
    dp_wr(0xA0, 0x00);
    spc_cmp(dp_rd(0xA7), 0x12);
    if (!spc.c) {
        count = spc_rd(spc.y);
        if (!count) {
            spc.a = dp_rd(0xA3);
            spc_put(spc.y, spc.a);
        } else {
            spc.a = spc_dec(count);
            spc_put(spc.y, spc.a);
            if (spc.z) {
                abi_SeqLoopSkip();
                return;
            }
        }
    } else {
        spc.a = spc_dec(spc_rd(spc.y));
        if (!spc.z) {
            abi_SeqLoopSkip();
            return;
        }
        spc_put(spc.y, spc.a);
        spc.a = dp_rd(0xA3);
        spc_call(abi_VoiceFlagsSet, 0x0958);
    }
    spc_call(abi_TrackByte, 0x095B);
    abi_SeqCmd16();
}

void abi_SeqCmd16(void)
{
    /* the track pointer to the next two bytes (low first) */
    spc_push(spc.a);
    spc_call(abi_TrackByte, 0x095F);
    dp_wr((uint8_t)(0x00 + spc.x), spc.a);
    spc.a = spc_pull();
    dp_wr((uint8_t)(0x08 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqLoopSkip(void)
{
    /* past the jump's two bytes */
    spc.c = 0;
    spc.a = spc_adc(0x02, dp_rd((uint8_t)(0x00 + spc.x)));
    dp_wr((uint8_t)(0x00 + spc.x), spc.a);
    if (spc.c)
        dp_wr((uint8_t)(0x08 + spc.x), spc_inc(dp_rd((uint8_t)(0x08 + spc.x))));
    spc_ret();
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
    dp_wr(0xA3, spc.a);
    spc.a = (uint8_t)(dp_rd((uint8_t)(0x10 + spc.x)) & 0x97);
    spc.a = spc_or(spc.a, dp_rd(0xA3));
    dp_wr((uint8_t)(0x10 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd17(void)
{
    /* the end of the track: out of the command loop and the tick */
    spc_pull();
    spc_pull();
    spc_pull();
    spc.a = spc_pull();
    if (dp_rd(0xB0) & 0x80) {
        dp_and(0xB8, 0xB4);
        dp_and(0xB7, 0xB4);
        abi_VoiceStop();
        return;
    }
    spc.a = spc_nz(0x00);
    dp_wr((uint8_t)(0x08 + spc.x), 0x00);
    dp_wr((uint8_t)(0x00 + spc.x), 0x00);
    spc_put((uint16_t)(0x0250 + spc.x), 0x00);
    if (!(dp_rd(0xB9) & 0x80)) {
        spc_call(abi_VoiceRelease, 0x0992);
        dp_or(0xBB, 0xB3);
    }
    spc_ret();
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
    spc.a = 0;
    spc_put((uint16_t)(0x0250 + spc.x), 0x00);
    spc_call(abi_VoiceRelease, 0x09A4);
    dp_or(0xB6, 0xB3);
    abi_VoiceSfxMark();
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
    /* the song voice under it ($50 lower) to be brought back */
    spc_push(spc.x);
    spc.c = 1;
    spc.x = spc_sbc(spc.x, 0x50);
    spc.a = spc_nz(spc_rd((uint16_t)(0x0250 + spc.x)));
    if (!spc.z)
        dp_or(0xBD, 0xB3);
    spc.x = spc_pull();
    spc_ret();
}

void abi_SeqCmd18(void)
{
    spc.c = 0;
    spc.a = spc_adc(spc.a, 0x80);
    spc_put((uint16_t)(0x0248 + spc.x), spc.a);
    spc_ret();
}

void abi_SeqCmd19(void)
{
    if (!(dp_rd(0xB0) & 0x80)) {
        spc.y = spc.a;
        spc_call(abi_CurveAtY, 0x09C5);
        dp_wr(0xC3, spc.a);
    }
    spc_ret();
}

void abi_SeqCmd1A(void)
{
    uint8_t which, v;

    spc_push(spc.a);
    spc_call(abi_TrackByte, 0x09CC);
    which = spc.y = spc_pull();
    spc_cmp(which, 0x03);
    if (spc.z) {
        /* 3: the noise bit (bit 4) of the voice's flags */
        spc.a = spc_xcn(spc.a);
        dp_wr(0xA3, spc.a);
        spc.a = (uint8_t)(spc_rd((uint16_t)(0x0100 + spc.x)) & 0xEF);
        spc.a = spc_or(spc.a, dp_rd(0xA3));
        spc_put((uint16_t)(0x0100 + spc.x), spc.a);
        spc_ret();
        return;
    }
    /* else byte Y of the voice's settings at $0108 (8 voices apart) */
    v = spc.a;
    spc_push(v);
    spc.a = 0x08;
    spc_mul();
    dp_wr(0xA4, spc.x);
    spc.c = 0;
    spc.a = spc_adc(spc.a, dp_rd(0xA4));
    spc.y = spc.a;
    spc.a = spc_pull();
    spc_put((uint16_t)(0x0108 + spc.y), spc.a);
    spc_ret();
}

void abi_SeqCmd1B(void)
{
    /* echo set-up N (8 bytes each), when not the one in use */
    spc_call(abi_TrackByte, 0x09F1);
    spc.y = 0x08;
    spc_mul();
    spc_cmp(spc.a, dp_rd(0xCB));
    if (spc.z) {
        abi_EchoSetupEnd();
        return;
    }
    dp_wr(0xCB, spc.a);
    abi_EchoSetup();
}

void abi_EchoSetup(void)
{
    uint8_t x = spc.a, v;

    /* from the set-up at $0D10 + A: echo volumes, the feedback's
       levels, the delay, the feedback and the filter */
    spc_push(spc.x);
    spc.x = x;
    dsp_set(0x0C, spc_rd((uint16_t)(0x0D10 + x)), 0x0A04);
    dsp_set(0x1C, spc_rd((uint16_t)(0x0D11 + x)), 0x0A0C);
    dp_wr(0xCE, spc_rd((uint16_t)(0x0D12 + x)));
    dp_wr(0xCF, spc_rd((uint16_t)(0x0D13 + x)));
    spc_call(abi_EchoOff, 0x0A19);
    dsp_set(0x6D, spc_rd((uint16_t)(0x0D14 + x)), 0x0A21);
    /* the delay: the old delay's time to wait ($CD counts up to it), the
       new one written */
    spc_st(0x00F2, 0x7D);
    v = (uint8_t)~((spc_rd(0x00F3) & 0x0F) + 1);
    spc.a = v;
    if (dp_rd(0xCD) & 0x80) {
        spc.c = 0;
        spc.a = spc_adc(v, dp_rd(0xCD));
    }
    dp_wr(0xCD, spc.a);
    spc.a = spc_rd((uint16_t)(0x0D15 + x));
    spc_st(0x00F3, spc.a);
    spc.c = 0;
    dp_wr(0xCC, spc_adc(0x20, dp_rd(0xCC)));
    dsp_set(0x0D, spc_rd((uint16_t)(0x0D16 + x)), 0x0A4B);
    spc.a = spc_rd((uint16_t)(0x0D17 + x));
    spc_call(abi_EchoFirSet, 0x0A51);
    spc.x = spc_pull();
    abi_EchoSetupEnd();
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
    uint8_t y;

    /* three echo registers cleared, the echo writes off */
    for (y = 3; y; y--) {
        spc_st(0x00F2, spc_rd((uint16_t)(0x0A68 + y)));
        spc_st(0x00F3, 0x00);
    }
    spc.y = 0x6C;
    spc.a = spc_nz(0x20);
    abi_DspWrite();
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
    uint8_t reg;

    /* filter A's 8 coefficients (8 bytes at $0A83) to FIR0-FIR7 */
    spc_push(spc.x);
    spc.y = 0x08;
    spc_mul();
    spc.x = spc.a;
    for (reg = 0x0F; ; ) {
        spc.y = reg;
        spc.a = spc_rd((uint16_t)(0x0A83 + spc.x));
        spc_call(abi_DspWrite, 0x0A79);
        spc.x++;
        spc.c = 0;
        reg = spc_adc(reg, 0x10);
        if (spc.n)
            break;
    }
    spc.y = reg;
    spc.a = reg;
    spc.x = spc_pull();
    spc_ret();
}

void abi_SeqCmd1C(void)
{
    /* the voice's echo bit on (A bit 0) or off: the song's or the
       effects' echo voices */
    spc.a = spc_lsr((uint8_t)(spc.a & 0x01));
    if (spc.c)
        spc.a = spc_nz(dp_rd(0xB3));
    if (!(dp_rd(0xB0) & 0x80)) {
        dp_and(0xBC, 0xB4);
        spc.a = spc_or(spc.a, dp_rd(0xBC));
        dp_wr(0xBC, spc.a);
    } else {
        dp_and(0xB7, 0xB4);
        spc.a = spc_or(spc.a, dp_rd(0xB7));
        dp_wr(0xB7, spc.a);
    }
    spc_ret();
}

void abi_SeqCmd1D(void)
{
    spc.a = spc_or(spc.a, 0xA0);
    spc_put((uint16_t)(0x0240 + spc.x), spc.a);
    spc_ret();
}

void abi_VoiceSlide(void)
{
    uint8_t x = spc.x, f;
    uint16_t step;
    int down, reached;

    /* in page 1: the slide's phase; each overflow a step ($40) on */
    spc.p = 1;
    spc.c = 0;
    spc.a = dp_rd((uint8_t)(0x18 + x));
    spc.a = spc_adc(spc.a, dp_rd((uint8_t)(0x20 + x)));
    dp_wr((uint8_t)(0x20 + x), spc.a);
    if (spc.c) {
        spc.c = 0;
        spc.a = spc_adc(dp_rd((uint8_t)(0x00 + x)), 0x40);
        dp_wr((uint8_t)(0x00 + x), spc.a);
    }
    f = dp_rd((uint8_t)(0x00 + x));
    spc.a = f;
    dp_wr(0xA4, f);
    if (f & 0x01) {
        /* a pitch slide: the pitch ($30/$38) up or down (bit 1) by twice
           the rate, stopping at the target ($40/$48) */
        dp_wr(0xA1, 0x00);
        dp_wr(0xA0, (uint8_t)(dp_rd((uint8_t)(0x28 + x)) << 1));
        dp_wr(0xA3, dp_rd((uint8_t)(0x40 + x)));
        dp_wr(0xA2, dp_rd((uint8_t)(0x48 + x)));
        spc.y = dp_rd((uint8_t)(0x30 + x));
        spc.a = dp_rd((uint8_t)(0x38 + x));
        down = !(dp_rd(0xA4) & 0x02);
        step = dp_rdw(0xA0);
        if (down) {
            spc_set_ya(spc_subw(spc_ya(), step));
            reached = !spc.c;
            if (!reached) {
                spc_cmpw(spc_ya(), dp_rdw(0xA2));
                reached = !spc.c;
            }
        } else {
            spc_set_ya(spc_addw(spc_ya(), step));
            spc_cmpw(spc_ya(), dp_rdw(0xA2));
            reached = spc.c;
        }
        if (reached) {
            /* reached: the slide off, at the target */
            spc.a = spc_dec(dp_rd(0xA4));
            dp_wr((uint8_t)(0x00 + x), spc.a);
            spc_movw_ld(0xA2);
        }
        dp_wr((uint8_t)(0x30 + x), spc.y);
        dp_wr((uint8_t)(0x38 + x), spc.a);
    }
    spc.a = spc_nz(spc_rd(0x00B9));
    if (spc.n) {
        spc.p = 0;
        spc_ret();
        return;
    }
    spc_call(abi_VoicePitchUpdate, 0x0B17);
    abi_VoiceVolumeSet();
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
    uint8_t x = spc.x, v, phase, level, pan;

    /* the slide's position on the curve (bit 6 of $00+X: falling) */
    spc.y = dp_rd((uint8_t)(0x10 + x));
    phase = dp_rd((uint8_t)(0x00 + x));
    v = dp_rd((uint8_t)(0x20 + x));
    if (!(phase & 0x40))
        v = (uint8_t)~v;
    spc.a = v;
    spc_mul();
    dp_wrw(0xA2, spc_ya());
    spc_set_ya(spc_subw(0x7E81, dp_rdw(0xA2)));
    spc_call(abi_CurveAt8, 0x0B2F);
    spc.y = spc.a;
    spc.p = 0;
    if (!(dp_rd(0xB0) & 0x80)) {
        /* a song voice: by the song's volume, then the fade */
        spc.a = dp_rd(0xC3);
        spc_mul();
        spc_push(spc.y);
        spc_movw_ld(0xC9);
        if (!(dp_rd(0xC6) & 0x80))
            spc_set_ya(spc_subw(0xFFFF, dp_rdw(0xC9)));
        spc_call(abi_CurveAt16, 0x0B45);
        spc.y = spc_pull();
        spc_mul();
    }
    /* by the voice's volume; then left and right by its pan (on the
       curve at $0BC0, $14 steps) */
    spc.a = spc_rd((uint16_t)(0x0230 + x));
    spc_mul();
    dp_wr(0xA3, spc.y);
    pan = dp_rd(0xB0) & 0x20 ? 0x80 : spc_rd((uint16_t)(0x0248 + x));
    spc.y = 0x14;
    spc.a = pan;
    spc_mul();
    dp_wrw(0xA0, spc_ya());
    dp_wr(0xA5, 0x01);
    do {
        spc.y = dp_rd(0xA1);
        spc.c = 1;
        spc.a = spc_rd((uint16_t)(0x0BC1 + spc.y));
        spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0BC0 + spc.y)));
        spc.y = dp_rd(0xA0);
        spc_mul();
        spc.a = spc.y;
        spc.y = dp_rd(0xA1);
        spc.c = 0;
        level = spc_adc(spc.a, spc_rd((uint16_t)(0x0BC0 + spc.y)));
        spc.y = dp_rd(0xA3);
        spc.a = level;
        spc_mul();
        spc.a = spc.y;
        spc.y = dp_rd(0xA5);
        spc_call(abi_DspWriteVoice, 0x0B7B);
        spc_set_ya(spc_subw(0x1400, dp_rdw(0xA0)));
        dp_wrw(0xA0, spc_ya());
        v = spc_dec(dp_rd(0xA5));
        dp_wr(0xA5, v);
    } while (!(v & 0x80));
    spc_ret();
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
    dp_wr(0xA1, spc.y);
    spc.y = 0x04;
    abi_CurveShift();
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
    spc.a = 0xFF;
    abi_CurveAt8();
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
    dp_wr(0xA1, spc.y);
    spc.y = 0x03;
    abi_CurveShift();
}

void abi_CurveShift(void)
{
    uint8_t i, f = spc.a, n;

    /* ($A1:A) shifted right Y times, $A1 in place a step at a time */
    for (n = spc.y; n; n--) {
        i = dp_rd(0xA1);
        dp_wr(0xA1, (uint8_t)(i >> 1));
        f = (uint8_t)((i & 1) << 7 | f >> 1);
    }
    /* the curve at $0BAF, between entry i and i + 1 by the fraction f */
    dp_wr(0xA0, f);
    i = dp_rd(0xA1);
    spc.c = 1;
    spc.a = spc_rd((uint16_t)(0x0BB0 + i));
    spc.a = spc_sbc(spc.a, spc_rd((uint16_t)(0x0BAF + i)));
    spc.y = f;
    spc_mul();
    spc.a = spc.y;
    spc.y = i;
    spc.c = 0;
    spc.a = spc_adc(spc.a, spc_rd((uint16_t)(0x0BAF + i)));
    spc_ret();
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
    spc.a = (uint8_t)(dp_rd((uint8_t)(0x00 + spc.x)) & 0x01);
    spc.a = spc_or(spc.a, dp_rd((uint8_t)(0x08 + spc.x)));
    if (!spc.z) {
        abi_VoicePitchSet();
        return;
    }
    /* a song voice with a transpose set: its pitch again */
    spc.a = spc_nz(spc_rd(0x00B0));
    if (!spc.n) {
        spc.a = spc_nz(spc_rd(0x00C5));
        if (!spc.z) {
            abi_VoicePitchFrom();
            return;
        }
    }
    spc_ret();
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
    spc.a = spc_nz(dp_rd((uint8_t)(0x08 + spc.x)));
    if (!spc.z) {
        abi_VoicePitchVibrato();
        return;
    }
    abi_VoicePitchFrom();
}

void abi_VoicePitchFrom(void)
{
    spc.y = dp_rd((uint8_t)(0x30 + spc.x));
    spc.a = spc_nz(dp_rd((uint8_t)(0x38 + spc.x)));
    abi_VoicePitchOut();
}

void abi_VoicePitchVibrato(void)
{
    uint8_t x = spc.x, phase, depth;
    uint16_t w;

    /* the vibrato's depth (A * 2 * $0C) by its phase (bit 6 of $00+X:
       the second half), added or (bit 7) taken from the pitch */
    spc.a = spc_asl(spc.a);
    spc.y = 0x0C;
    spc_mul();
    dp_wrw(0xA0, spc_ya());
    phase = spc.a = spc_nz(dp_rd((uint8_t)(0x00 + x)));
    spc_push(spc_psw());
    depth = dp_rd((uint8_t)(0x20 + x));
    if (phase & 0x40)
        depth = (uint8_t)~depth;
    spc_push(depth);
    spc.y = dp_rd(0xA0);
    spc.a = depth;
    spc_mul();
    dp_wr(0xA0, spc.y);
    spc.a = spc_pull();
    spc.y = dp_rd(0xA1);
    spc_mul();
    dp_wr(0xA1, 0x00);
    spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA0)));
    dp_wrw(0xA0, spc_ya());
    spc.y = dp_rd((uint8_t)(0x30 + x));
    spc.a = dp_rd((uint8_t)(0x38 + x));
    spc_set_psw(spc_pull());
    w = dp_rdw(0xA0);
    if (spc.n)
        spc_set_ya(spc_subw(spc_ya(), w));
    else
        spc_set_ya(spc_addw(spc_ya(), w));
    abi_VoicePitchOut();
}

void abi_VoicePitchOut(void)
{
    uint8_t x = spc.x, octave, r, hi, lo;

    /* the song's transpose (fine) added */
    spc.p = 0;
    if (!(dp_rd(0xB0) & 0x80)) {
        dp_wr(0xA1, dp_rd(0xC5));
        dp_wr(0xA0, 0x00);
        spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA0)));
    }
    /* in page 1: semitones from $34 below (low notes doubled down) */
    spc.p = 1;
    spc.y = spc_dec(spc.y);
    dp_wr(0xA1, spc.y);
    dp_wr(0xA0, spc.a);
    spc.c = 1;
    spc.a = spc_sbc(spc.y, 0x34);
    spc.y = 0;
    if (!spc.c) {
        spc.c = 1;
        spc.a = spc_sbc(dp_rd(0xA1), 0x13);
        if (spc.c)
            goto octave;
        spc.y = 0xFF;
        spc.a = spc_asl(spc.a);
    }
    spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA0)));
    dp_wrw(0xA0, spc_ya());
octave:
    /* the octave and the semitone in it (12 a octave, words at $0CC2);
       the fraction between it and the next */
    spc_push(x);
    spc.a = spc_asl(dp_rd(0xA1));
    spc.y = 0;
    spc.x = 0x18;
    spc_div();
    octave = spc.a;
    r = spc.y;
    dp_wr(0xA3, spc_rd((uint16_t)(0x0CC3 + r)));
    dp_wr(0xA2, spc_rd((uint16_t)(0x0CC2 + r)));
    spc_push(spc_rd((uint16_t)(0x0CC5 + r)));
    spc.a = spc_rd((uint16_t)(0x0CC4 + r));
    spc.y = spc_pull();
    spc_set_ya(spc_subw(spc_ya(), dp_rdw(0xA2)));
    spc.y = dp_rd(0xA0);
    spc_mul();
    spc.a = spc.y;
    spc.y = 0;
    spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA2)));
    /* doubled, then shifted to the octave ($A1 in place a step at a time) */
    dp_wr(0xA1, spc.y);
    hi = dp_rd(0xA1);
    dp_wr(0xA1, (uint8_t)(hi << 1 | spc.a >> 7));
    lo = (uint8_t)(spc.a << 1);
    for (spc.x = octave; spc.x != 0x08; spc.x++) {
        hi = dp_rd(0xA1);
        dp_wr(0xA1, (uint8_t)(hi >> 1));
        lo = (uint8_t)((hi & 1) << 7 | lo >> 1);
    }
    dp_wr(0xA0, lo);
    spc.a = lo;
    spc.x = spc_pull();
    x = spc.x;
    /* by the instrument's pitch multiplier ($0228.$0220) */
    spc.a = spc_rd((uint16_t)(0x0228 + x));
    spc.y = dp_rd(0xA1);
    spc_mul();
    dp_wrw(0xA2, spc_ya());
    spc.a = spc_rd((uint16_t)(0x0228 + x));
    spc.y = dp_rd(0xA0);
    spc_mul();
    spc_push(spc.y);
    spc.a = spc_rd((uint16_t)(0x0220 + x));
    spc.y = dp_rd(0xA0);
    spc_mul();
    spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA2)));
    dp_wrw(0xA2, spc_ya());
    spc.a = spc_rd((uint16_t)(0x0220 + x));
    spc.y = dp_rd(0xA1);
    spc_mul();
    spc.y = spc.a;
    spc.a = spc_pull();
    spc_set_ya(spc_addw(spc_ya(), dp_rdw(0xA2)));
    spc.a = spc_and(spc.a, 0xF0);
    /* to the voice's PITCHL and PITCHH */
    spc_push(spc.y);
    spc.y = 0x02;
    spc_call(abi_DspWriteVoice, 0x0CAC);
    spc.a = spc_pull();
    spc.y = spc_inc(spc.y);
    abi_DspWrite();
}

