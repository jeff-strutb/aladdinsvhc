/*
 * dsp_write.c: the sound driver's writes to DSP registers, and the voice
 * envelope settings made through them.
 */
#include "spc.h"
#include "spc_ops.h"

#define DSP_ADDR 0x00F2u            /* the DSP register to read or write */
#define DSP_DATA 0x00F3u
#define DSP_VOICE_REG 0xA6          /* scratch: the voice register's low nibble */
#define VOICE_ADSR1 0x0238u         /* each voice's ADSR1 (bit 7, the ADSR enable, cleared to use GAIN) */
#define DSP_V_ADSR1 0x05            /* in a voice's DSP registers */
#define DSP_V_GAIN 0x07
#define GAIN_DECREASE 0x9E          /* GAIN: exponential decrease, rate $1E */

void abi_DspWrite(void);
void abi_DspWriteVoice(void);

/*
 * @routine 85:8AB8 DspWrite
 * @spc 0CBB
 * WHAT IT DOES: Writes A to DSP register Y.
 * HOW: Y to the DSP address port $F2, A to the data port $F3.
 * Entry: A = the value; Y = the register; called by CALL, or reached from
 * DspWriteVoice. Exit: every register and flag unchanged.
 */
void abi_DspWrite(void)
{
    spc_st(DSP_ADDR, spc.y);
    spc_st(DSP_DATA, spc.a);
    spc_ret();
}

/*
 * @routine 85:8AAD DspWriteVoice
 * @spc 0CB0
 * WHAT IT DOES: Writes A to register Y (low nibble) of voice X's DSP
 * registers.
 * HOW: The register in scratch $A6; voice X's nibble (X swapped into the
 * high nibble, bits 4-6) or'ed in as the DSP address; then DspWrite.
 * Entry: A = the value; X = the voice (0-7); Y = the register ($0-$9);
 * called by CALL or JMP. Exit: Y = the DSP address; N, Z from it; A, X
 * and the other flags unchanged; $A6 = the register.
 */
void abi_DspWriteVoice(void)
{
    /* the voice's nibble (bits 4-6) with the register's (A kept on the
       stack meanwhile, as the original) */
    spc_push(spc.a);
    spc_put(spc_dp(DSP_VOICE_REG), spc.y);
    spc.y = spc_nz((uint8_t)(((spc.x << 4 | spc.x >> 4) & 0x70) | spc_rd(spc_dp(DSP_VOICE_REG))));
    spc.a = spc_pull();
    abi_DspWrite();
}

/*
 * @routine 85:8218 VoiceAdsr1Set
 * @spc 041B
 * WHAT IT DOES: Sets voice X's DSP ADSR1 from its kept value with the
 * ADSR enable bit cleared, so the voice's GAIN takes over its envelope.
 * HOW: Its ADSR1 at $0238 plus X, AND $7F, to its register 5
 * (DspWriteVoice, by JMP).
 * Entry: X = the voice; called by CALL, or reached from VoiceRelease.
 * Exit: as DspWriteVoice.
 */
void abi_VoiceAdsr1Set(void)
{
    spc.y = DSP_V_ADSR1;
    spc.a = spc_rd((uint16_t)(VOICE_ADSR1 + spc.x)) & 0x7F;
    abi_DspWriteVoice();
}

/*
 * @routine 85:8211 VoiceRelease
 * @spc 0414
 * WHAT IT DOES: Fades voice X out: its GAIN to an exponential decrease,
 * then its envelope handed to GAIN (VoiceAdsr1Set).
 * HOW: $9E to its register 7 (DspWriteVoice), then on into VoiceAdsr1Set.
 * Entry: X = the voice; called by CALL. Exit: as DspWriteVoice.
 */
void abi_VoiceRelease(void)
{
    spc.y = DSP_V_GAIN;
    spc.a = GAIN_DECREASE;
    spc_call(abi_DspWriteVoice, 0x041B);
    abi_VoiceAdsr1Set();
}
