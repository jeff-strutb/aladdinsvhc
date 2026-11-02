; dsp_write.asm: the sound driver's writes to DSP registers, and the voice
; envelope settings made through them.
; C: src/aladdin/audio/dsp_write.c.

incsrc "../include/macros.inc"

!DSP_ADDR = $00F2               ; the DSP register to read or write
!DSP_DATA = $00F3
!DSP_VOICE_REG = $A6            ; scratch: the voice register's low nibble
!VOICE_ADSR1 = $0238            ; each voice's ADSR1 (bit 7, the ADSR enable, cleared to use GAIN)
!DSP_V_ADSR1 = $05              ; in a voice's DSP registers
!DSP_V_GAIN = $07
!GAIN_DECREASE = $9E            ; GAIN: exponential decrease, rate $1E

; @routine 85:8211 VoiceRelease
; @spc 0414
; WHAT IT DOES: Fades voice X out: its GAIN to an exponential decrease,
; then its envelope handed to GAIN (VoiceAdsr1Set).
; HOW: $9E to its register 7 (DspWriteVoice), then on into VoiceAdsr1Set.
; Entry: X = the voice; called by CALL. Exit: as DspWriteVoice.
%spc($0414, $858211)
VoiceRelease:
    mov y,#!DSP_V_GAIN
    mov a,#!GAIN_DECREASE
    call DspWriteVoice

; @routine 85:8218 VoiceAdsr1Set
; @spc 041B
; WHAT IT DOES: Sets voice X's DSP ADSR1 from its kept value with the
; ADSR enable bit cleared, so the voice's GAIN takes over its envelope.
; HOW: Its ADSR1 at $0238 plus X, AND $7F, to its register 5
; (DspWriteVoice, by JMP).
; Entry: X = the voice; called by CALL, or reached from VoiceRelease.
; Exit: as DspWriteVoice.
VoiceAdsr1Set:
    mov y,#!DSP_V_ADSR1
    mov.w a,!VOICE_ADSR1+x
    and a,#$7F
    jmp DspWriteVoice
%spc_end()

; @routine 85:8AAD DspWriteVoice
; @spc 0CB0
; WHAT IT DOES: Writes A to register Y (low nibble) of voice X's DSP
; registers.
; HOW: The register in scratch $A6; voice X's nibble (X swapped into the
; high nibble, bits 4-6) or'ed in as the DSP address; then DspWrite.
; Entry: A = the value; X = the voice (0-7); Y = the register ($0-$9);
; called by CALL or JMP. Exit: Y = the DSP address; N, Z from it; A, X
; and the other flags unchanged; $A6 = the register.
%spc($0CB0, $858AAD)
DspWriteVoice:
    push a
    mov.b !DSP_VOICE_REG,y
    mov a,x
    xcn a
    and a,#$70
    or.b a,!DSP_VOICE_REG
    mov y,a
    pop a

; @routine 85:8AB8 DspWrite
; @spc 0CBB
; WHAT IT DOES: Writes A to DSP register Y.
; HOW: Y to the DSP address port $F2, A to the data port $F3.
; Entry: A = the value; Y = the register; called by CALL, or reached from
; DspWriteVoice. Exit: every register and flag unchanged.
DspWrite:
    mov.w !DSP_ADDR,y
    mov.w !DSP_DATA,a
    ret
%spc_end()
