; sound_driver.asm: the SPC700 sound driver: its start and main loop, the
; S-CPU's commands, the sequencer and the voices' pitch and volume.
; C: src/aladdin/audio/sound_driver.c.

incsrc "../include/macros.inc"

; The SPC700's I/O registers.
!CONTROL = $00F1               ; timers and port clears
!DSP_ADDR = $00F2              ; the DSP register to read or write
!DSP_DATA = $00F3
!PORT0 = $00F4                 ; the S-CPU's ports, read here; written back for the S-CPU
!PORT1 = $00F5
!PORT2 = $00F6
!PORT3 = $00F7
!T0_TARGET = $00FA             ; timer 0's divider
!T0_OUT = $00FD                ; timer 0's count since the last read

; The driver's RAM and tables, by address (sd_ = sound driver): direct page
; bytes $00-$CF (per voice: $00+X, $08+X ... ), and words in pages 1-$1D.
!sd_00 = $00
!sd_08 = $08
!sd_10 = $10
!sd_18 = $18
!sd_20 = $20
!sd_28 = $28
!sd_30 = $30
!sd_38 = $38
!sd_40 = $40
!sd_48 = $48
!sd_a0 = $A0
!sd_a1 = $A1
!sd_a2 = $A2
!sd_a3 = $A3
!sd_a4 = $A4
!sd_a5 = $A5
!sd_a7 = $A7
!sd_a8 = $A8
!sd_a9 = $A9
!sd_aa = $AA
!sd_b0 = $B0
!sd_b1 = $B1
!sd_b2 = $B2
!sd_b3 = $B3
!sd_b4 = $B4
!sd_b5 = $B5
!sd_b6 = $B6
!sd_b7 = $B7
!sd_b8 = $B8
!sd_b9 = $B9
!sd_ba = $BA
!sd_bb = $BB
!sd_bc = $BC
!sd_bd = $BD
!sd_be = $BE
!sd_c0 = $C0
!sd_c2 = $C2
!sd_c3 = $C3
!sd_c4 = $C4
!sd_c5 = $C5
!sd_c6 = $C6
!sd_c7 = $C7
!sd_c8 = $C8
!sd_c9 = $C9
!sd_ca = $CA
!sd_cb = $CB
!sd_cc = $CC
!sd_cd = $CD
!sd_ce = $CE
!sd_cf = $CF
!sd_00b0 = $00B0
!sd_00b9 = $00B9
!sd_00c5 = $00C5
!sd_0100 = $0100
!sd_0108 = $0108
!sd_0120 = $0120
!sd_0128 = $0128
!sd_0200 = $0200
!sd_0208 = $0208
!sd_0210 = $0210
!sd_0218 = $0218
!sd_0220 = $0220
!sd_0228 = $0228
!sd_0230 = $0230
!sd_0238 = $0238
!sd_0240 = $0240
!sd_0248 = $0248
!sd_0250 = $0250
!sd_0258 = $0258
!sd_0260 = $0260
!sd_0268 = $0268
!sd_0437 = $0437
!sd_043a = $043A
!sd_07de = $07DE
!sd_07e5 = $07E5
!sd_07ec = $07EC
!sd_07f3 = $07F3
!sd_0a68 = $0A68
!sd_0a83 = $0A83
!sd_0baf = $0BAF
!sd_0bb0 = $0BB0
!sd_0bc0 = $0BC0
!sd_0bc1 = $0BC1
!sd_0cc2 = $0CC2
!sd_0cc3 = $0CC3
!sd_0cc4 = $0CC4
!sd_0cc5 = $0CC5
!sd_0d10 = $0D10
!sd_0d11 = $0D11
!sd_0d12 = $0D12
!sd_0d13 = $0D13
!sd_0d14 = $0D14
!sd_0d15 = $0D15
!sd_0d16 = $0D16
!sd_0d17 = $0D17
!sd_1db0 = $1DB0
!sd_1db1 = $1DB1
!sd_1db2 = $1DB2
!sd_1db3 = $1DB3

%spc($02C0, $8580BD)

; @routine 85:80BD SoundDriverStart
; @spc 02C0
; WHAT IT DOES: The sound driver's start and main loop: clears its RAM,
; sets up the DSP and timer 0, then forever takes the S-CPU's commands
; and, on each timer tick, steps the music and sound-effect voices.
; HOW: Clears direct pages 0 and 1 and $0200-$02BF; DspSetup; timer 0 at
; $40, the timers on; then each loop polls the ports (PortCommandPoll) and
; counts timer 0 into $B1; on a tick, with the return address $02F5
; pushed, either the music half (every other tick, $B0 bit 7: each voice
; with a track, VoiceTick, then the fade) or the effects half (the voices
; $B8 marks), then the key-on and key-off writes; their last DSP write
; returns to $02F5.
; Entry: jumped to by the boot ROM after the upload. Exit: never returns.
SoundDriverStart:
    clrp
    mov x,#$CF
    mov sp,x
    mov a,#$00
    mov x,a
.l02C7:
    mov (x+),a
    cmp x,#$E0
    bne .l02C7
    setp
    mov x,a
.l02CE:
    mov (x+),a
    cmp x,#$B0
    bne .l02CE
    clrp
    mov x,a
.l02D5:
    mov.w !sd_0200+x,a
    inc x
    cmp x,#$C0
    bne .l02D5
    call DspSetup
    mov a,#$40
    mov.w !T0_TARGET,a
    mov a,#$F1
    mov.w !CONTROL,a
    mov a,#$01
    mov.b !sd_b2,a
    mov.w !PORT2,a
    dec a
    mov.w !PORT3,a
.l02F5:
    call PortCommandPoll
    mov.w a,!T0_OUT
    clrc
    adc.b a,!sd_b1
    mov.b !sd_b1,a
    beq .l02F5
    dec.b !sd_b1
    mov y,#$02
    mov a,#$F5
    push y
    push a
    clrc
    adc !sd_b0,#$80
    bpl .l0313
    jmp .l0399
.l0313:
    cmp !sd_cc,!sd_cd
    beq .l032D
    bbs !sd_cd.7,.l0329
    mov y,#$6C
    mov a,#$00
    call DspWrite
    mov y,#$4D
    mov a,#$FF
    call DspWrite
.l0329:
    inc.b !sd_cd
    bra .l0348
.l032D:
    mov y,#$4D
    mov.b a,!sd_b8
    eor a,#$FF
    and.b a,!sd_bc
    or.b a,!sd_b7
    call DspWrite
    mov y,#$2C
    mov.b a,!sd_ce
    call DspWrite
    mov y,#$3C
    mov.b a,!sd_cf
    call DspWrite
.l0348:
    mov x,#$07
    mov y,#$57
    mov.b a,!sd_ba
    or.b a,!sd_b5
    push a
.l0351:
    push y
    bbc !sd_b5.7,.l035F
    mov.w a,!sd_0238+y
    mov.b !sd_a3,a
    mov.w a,!sd_0240+y
    bra .l0375
.l035F:
    bbc !sd_bd.7,.l036A
    mov.w a,!sd_0250+x
    call VoiceInstrument
    bra .l0381
.l036A:
    bbc !sd_ba.7,.l0381
    mov.w a,!sd_0238+x
    mov.b !sd_a3,a
    mov.w a,!sd_0240+x
.l0375:
    mov y,#$07
    call DspWriteVoice
    mov y,#$05
    mov.b a,!sd_a3
    call DspWriteVoice
.l0381:
    asl.b !sd_b5
    asl.b !sd_bd
    asl.b !sd_ba
    pop y
    dec y
    dec x
    bpl .l0351
    mov y,#$5C
    mov a,#$00
    call DspWrite
    pop a
    mov y,#$4C
    jmp DspWrite
.l0399:
    clr1 !sd_b0.7
    bbs !sd_b0.6,.l03D7
    mov !sd_b3,#$80
    mov !sd_b9,!sd_b8
    mov x,#$07
    movw ya,!sd_be
    addw ya,!sd_c0
    movw !sd_a9,ya
    mov y,#$00
    mov.b a,!sd_c2
    addw ya,!sd_a9
    mov.b !sd_a8,y
    mov.b !sd_c2,a
.l03B6:
    mov.b a,!sd_08+x
    or.b a,!sd_00+x
    beq .l03BF
    call VoiceTick
.l03BF:
    dec x
    asl.b !sd_b9
    lsr.b !sd_b3
    bne .l03B6
    movw ya,!sd_c7
    addw ya,!sd_c9
    bcc .l03D5
    mov !sd_c7,#$00
    mov !sd_c8,#$00
    mov y,#$FF
    mov a,y
.l03D5:
    movw !sd_c9,ya
.l03D7:
    set1 !sd_b0.7
    mov !sd_b3,#$80
    mov !sd_b9,#$00
    mov x,#$57
.l03E1:
    mov.b a,!sd_b8
    and.b a,!sd_b3
    beq .l0400
    mov.w a,!sd_0260+x
    mov.b !sd_aa,a
    mov.w a,!sd_0258+x
    mov.b !sd_a9,a
    mov y,#$00
    mov.w a,!sd_0268+x
    addw ya,!sd_a9
    mov.b !sd_a8,y
    mov.w !sd_0268+x,a
    call VoiceTick
.l0400:
    dec x
    lsr.b !sd_b3
    bne .l03E1
    mov.b a,!sd_b6
    or.b a,!sd_bb
    mov !sd_b6,#$00
    mov !sd_bb,#$00
    mov y,#$5C
    jmp DspWrite

%spc_end()

%spc($0425, $858222)

; @routine 85:8222 DspSetup
; @spc 0425
; WHAT IT DOES: Sets the DSP's main volume and echo settings at start.
; HOW: The DSP registers and values of the tables at $0438 (3 pairs) to
; $F2/$F3, then the echo setup (its code from $09FA).
; Entry: called by CALL. Exit: as the echo setup.
DspSetup:
    mov y,#$03
.l0427:
    mov.w a,!sd_0437+y
    mov.w !DSP_ADDR,a
    mov.w a,!sd_043a+y
    mov.w !DSP_DATA,a
    dbnz y,.l0427
    jmp EchoSetup
    db $2D,$3D,$5D,$00,$00,$33

; @routine 85:823B PortCommandPoll
; @spc 043E
; WHAT IT DOES: Takes the S-CPU's next command from port 0, if any, and
; acknowledges it.
; HOW: Reads port 0 until two reads agree; nonzero: counts $B2, runs it
; (SoundCommand), clears the ports' inputs ($F1 = $31) and writes $B2 and
; the command back to ports 2 and 3.
; Entry: called by CALL. Exit: A = the command (or 0); Y = the command.
PortCommandPoll:
    mov.w a,!PORT0
    cmp.w a,!PORT0
    bne PortCommandPoll
    mov y,a
    beq PortCommandPollEnd
    push a
    inc.b !sd_b2
    call SoundCommand
    mov a,#$31
    mov.w !CONTROL,a
    mov.b a,!sd_b2
    mov.w !PORT2,a
    pop a
    mov.w !PORT3,a

PortCommandPollEnd:
    ret

; @routine 85:825B SoundCommand
; @spc 045E
; WHAT IT DOES: Runs one S-CPU command: below $F0 starts song or sound
; effect A, $F0 and up is a driver command.
; HOW: $F0 and up: the command table at $0511 by the low nibble (JMP
; (abs+X)). Else A, less the song count at $1DB0 until below it, indexes
; the song table at $1DB2; a song's header gives its priority and each
; voice's track; voices it may take (priority at or above theirs) are
; cleared and given their track.
; Entry: A = the command; called by CALL. Exit: as the command's code
; leaves it.
SoundCommand:
    cmp a,#$F0
    bcc .l0465
    jmp SoundCommandTable
.l0465:
    cmp.w a,!sd_1db0
    bcc .l0470
    setc
    sbc.w a,!sd_1db0
    bra .l0465
.l0470:
    asl a
    mov x,a
    mov.w a,!sd_1db2+x
    mov.b !sd_a1,a
    mov.w a,!sd_1db3+x
    mov.b !sd_a0,a
    or.b a,!sd_a1
    beq PortCommandPollEnd
    mov y,#$00
    mov a,(!sd_a0)+y
    bne .l0489
    jmp SongStartKeep
.l0489:
    mov.b !sd_a4,a
    mov !sd_b3,#$80
    mov x,#$57
.l0490:
    mov y,#$00
    incw !sd_a0
    mov a,(!sd_a0)+y
    mov.b !sd_a3,a
    incw !sd_a0
    mov a,(!sd_a0)+y
    mov.b !sd_a2,a
    mov a,(!sd_a2)+y
    cmp a,#$17
    beq .l04DE
    mov.b a,!sd_a4
    cmp.w a,!sd_0250+x
    bcc .l04DE
    or !sd_b8,!sd_b3
    mov.b a,!sd_b3
    eor a,#$FF
    mov.b !sd_a5,a
    and !sd_b7,!sd_a5
    and !sd_ba,!sd_a5
    call VoiceStateClear
    call VoiceRelease
    mov.b a,!sd_a4
    mov.w !sd_0250+x,a
    movw ya,!sd_a2
    mov.b !sd_08+x,y
    mov.b !sd_00+x,a
    mov a,#$10
    mov.w !sd_0100+x,a
    mov a,#$B8
    mov.w !sd_0240+x,a
    mov.w a,!PORT1
    clrc
    adc a,#$80
    mov.w !sd_0248+x,a
.l04DE:
    dec x
    lsr.b !sd_b3
    bne .l0490
    ret

; @routine 85:82E1 VoiceStateClear
; @spc 04E4
; WHAT IT DOES: Clears voice X's sequencer state.
; HOW: Every eighth byte of $0200-$02BF from $0208 plus X; then, in page 1
; and page 0 (VoiceDpClear), every eighth byte from X.
; Entry: X = the voice; called by CALL. Exit: A = $A0 or more, flags from
; the compare; X unchanged; P clear.
VoiceStateClear:
    mov a,x
    clrc
    adc a,#$08
.l04E8:
    mov y,a
    mov a,#$00
    mov.w !sd_0200+y,a
    mov a,y
    clrc
    adc a,#$08
    cmp a,#$C0
    bcc .l04E8
    setp
    call VoiceDpClear
    clrp

; @routine 85:82F8 VoiceDpClear
; @spc 04FB
; WHAT IT DOES: Clears every eighth byte of the direct page from X below
; $A0: voice X's direct-page state.
; HOW: A loop of MOV (X),A stepping X by 8; X kept on the stack.
; Entry: X = the voice; called by CALL. Exit: A = the last X stepped; X
; unchanged.
VoiceDpClear:
    push x
.l04FC:
    mov a,#$00
    mov (x),a
    mov a,x
    clrc
    adc a,#$08
    mov x,a
    cmp x,#$A0
    bcc .l04FC
    pop x
    ret

SoundCommandTable:
    and a,#$0F
    asl a
    mov x,a
    jmp (SoundCommandHandlers+x)
; The driver commands $F0-$FD, by the low nibble (commands $FE and $FF would read the code that follows).
SoundCommandHandlers:
    dw SoundStopAll, SoundCmdF1, SequencerReset, SoundCmdF3
    dw SoundCmdF4, SoundCmdF5, SoundCmdF6, SoundCmdF7
    dw SoundCmdF8, SoundUpload, SoundCmdFA, SoundCmdFB
    dw SoundCmdFC, SoundCmdFD

; @routine 85:832A SoundStopAll
; @spc 052D
; WHAT IT DOES: Command $F0: stops the music and every sound effect.
; HOW: SequencerReset; then each voice $B8 marks is stopped (VoiceStop);
; $B5 and $B7 cleared.
; Entry: reached from the command table or by CALL. Exit: X = the last
; voice; A = 0.
SoundStopAll:
    call SequencerReset

SoundCmdF1:
    mov !sd_b3,#$80
    mov x,#$57
.l0535:
    bbc !sd_b8.7,.l053B
    call VoiceStop
.l053B:
    dec x
    asl.b !sd_b8
    lsr.b !sd_b3
    bne .l0535
    mov !sd_b5,#$00
    mov !sd_b7,#$00
    ret

; @routine 85:8346 SequencerReset
; @spc 0549
; WHAT IT DOES: Command $F2: the music's state cleared and its voices
; released.
; HOW: VoicesReleaseFree; $BC and $0208-$0257 cleared; page 1's first $50
; bytes and page 0's (PageClear50).
; Entry: reached from the command table or by CALL. Exit: A = 0; X = $50;
; P clear.
SequencerReset:
    call VoicesReleaseFree
    mov a,#$00
    mov.b !sd_bc,a
    mov x,#$08
.l0552:
    mov.w !sd_0200+x,a
    inc x
    cmp x,#$58
    bne .l0552
    setp
    call PageClear50
    clrp

; @routine 85:835C PageClear50
; @spc 055F
; WHAT IT DOES: Clears the first $50 bytes of the direct page.
; HOW: MOV (X+),A from X = 0.
; Entry: A = 0; called by CALL. Exit: X = $50; flags from the compare.
PageClear50:
    mov x,#$00
.l0561:
    mov (x+),a
    cmp x,#$50
    bne .l0561
    ret

SoundCmdFA:
    call SoundUpload

SoundCmdFB:
    mov !sd_a1,#$0D
    mov !sd_a0,#$1F

SongStartKeep:
    call SequencerReset
    mov y,#$00
    mov a,y
    movw !sd_c0,ya
    mov.b !sd_c4,a
    mov.b !sd_c5,a
    dec a
    mov.b !sd_c3,a
    call FadeClear
    mov x,#$07
.l0584:
    mov y,#$00
    incw !sd_a0
    mov a,(!sd_a0)+y
    mov.b !sd_a3,a
    incw !sd_a0
    mov a,(!sd_a0)+y
    mov.b !sd_a2,a
    mov a,(!sd_a2)+y
    cmp a,#$17
    beq .l05AD
    movw ya,!sd_a2
    mov.b !sd_08+x,y
    mov.b !sd_00+x,a
    mov a,#$10
    mov.w !sd_0100+x,a
    mov a,#$B8
    mov.w !sd_0240+x,a
    mov a,#$80
    mov.w !sd_0248+x,a
.l05AD:
    dec x
    bpl .l0584
    ret

; @routine 85:83AE SoundCmdFC
; @spc 05B1
; WHAT IT DOES: Command $FC: sets the music's tempo offset from port 1, a
; signed value scaled by 3 x $69 / 256 into $C0-$C1.
; HOW: Port 1's magnitude times $69 (high byte kept at $A2) plus the
; magnitude times 3; negated (0 less it) when port 1 was negative. Port 1
; is written by the S-CPU only in SpcUpload, which ends every upload by
; writing 0 or 1 there, so the negative branch ($05B7, $05CC) is code the
; retail game never runs.
; Entry: reached from the command table (SoundCommand). Exit: YA = the
; offset, also at $C0; $A3 = 0.
SoundCmdFC:
    mov.w a,!PORT1
    push p
    bpl .l05BA
    eor a,#$FF
    inc a
.l05BA:
    push a
    mov y,#$69
    mul ya
    mov !sd_a3,#$00
    mov.b !sd_a2,y
    pop a
    mov y,#$03
    mul ya
    addw ya,!sd_a2
    pop p
    bpl .l05D3
    movw !sd_a2,ya
    mov y,#$00
    mov a,y
    subw ya,!sd_a2
.l05D3:
    movw !sd_c0,ya
    ret

SoundCmdFD:
    mov.w a,!PORT1
    mov.b !sd_c5,a
    ret

SoundCmdF3:
    set1 !sd_b0.6
    bra VoicesReleaseFree

SoundCmdF4:
    clr1 !sd_b0.6
    mov !sd_ba,#$00
    mov !sd_b3,#$80
    mov x,#$57
.l05EA:
    mov.b a,!sd_b8
    and.b a,!sd_b3
    bne .l05F3
    call VoiceSfxMark
.l05F3:
    dec x
    lsr.b !sd_b3
    bne .l05EA
    ret

; @routine 85:83F6 VoicesReleaseFree
; @spc 05F9
; WHAT IT DOES: Releases every voice no sound effect holds.
; HOW: For voices 7 down to 0 whose bit is clear in $B8: VoiceRelease and
; its bit set in $BB; $BA and $BD cleared.
; Entry: called by CALL. Exit: X = $FF; A as the last release leaves it.
VoicesReleaseFree:
    mov x,#$07
    mov !sd_b3,#$80
.l05FE:
    mov.b a,!sd_b8
    and.b a,!sd_b3
    bne .l060A
    call VoiceRelease
    or !sd_bb,!sd_b3
.l060A:
    dec x
    lsr.b !sd_b3
    bne .l05FE
    mov !sd_ba,#$00
    mov !sd_bd,#$00
    ret

SoundCmdF5:
    mov x,#$FF
    bra SoundCmdF6_l061C

SoundCmdF6:
    mov x,#$01
.l061C:
    cmp.b x,!sd_c6
    beq .l063C
    mov.w a,!PORT2
    or.w a,!PORT3
    beq FadeClear
    mov.w a,!PORT2
    mov.w y,!PORT3
    asl.b !sd_c6
    beq .l063D
    call FadeSet
    mov y,#$FF
    mov a,y
    subw ya,!sd_c9
    movw !sd_c9,ya
.l063C:
    ret
.l063D:
    mov !sd_c9,#$00
    mov !sd_ca,#$00
    bra FadeSet

; @routine 85:8442 FadeClear
; @spc 0645
; WHAT IT DOES: Clears the fade: its rate and level ($C6-$CA).
; HOW: YA = 0, X = 0; then on into FadeSet.
; Entry: called by CALL or reached. Exit: A = Y = X = 0.
FadeClear:
    mov a,#$00
    mov y,a
    mov x,a
    movw !sd_c9,ya

; @routine 85:8448 FadeSet
; @spc 064B
; WHAT IT DOES: Sets the fade level to YA and its step to X.
; HOW: MOVW $C7,YA; $C6 = X.
; Entry: YA = the level; X = the step. Exit: registers unchanged; flags
; from the MOVW.
FadeSet:
    movw !sd_c7,ya
    mov.b !sd_c6,x
    ret

SoundCmdF7:
    clr1 !sd_b0.5
    ret

SoundCmdF8:
    set1 !sd_b0.5
    ret

; @routine 85:8453 SoundUpload
; @spc 0656
; WHAT IT DOES: Command $F9: stops all sound and takes a new upload from
; the S-CPU (song and sample data) by the port handshake.
; HOW: SoundStopAll; ports 0-1 = $AA, $BB; waits for $CC; then blocks:
; port 2-3 the address, port 1 the data, port 0 the counter echoed; ends
; when the S-CPU sends a zero counter, and the timer and sequence count
; are reset.
; Entry: reached from the command table. Exit: as its last port write
; leaves it.
SoundUpload:
    call SoundStopAll
    mov a,#$AA
    mov.w !PORT0,a
    mov a,#$BB
    mov.w !PORT1,a
.l0663:
    mov.w a,!PORT0
    cmp a,#$CC
    bne .l0663
    bra .l068C
.l066C:
    mov.w y,!PORT0
    bne .l066C
.l0671:
    cmp.w y,!PORT0
    bne .l0685
    mov.w a,!PORT1
    mov.w !PORT0,y
    mov (!sd_a0)+y,a
    inc y
    bne .l0671
    inc.b !sd_a1
    bra .l0671
.l0685:
    bpl .l0671
    cmp.w y,!PORT0
    bpl .l0671
.l068C:
    mov.w a,!PORT2
    mov.w y,!PORT3
    movw !sd_a0,ya
    mov.w y,!PORT0
    mov.w a,!PORT1
    beq .l06A1
    mov.w !PORT0,y
    bra .l066C
.l06A1:
    mov.w !PORT2,a
    inc a
    mov.b !sd_b2,a
    mov.w a,!T0_OUT
    mov !sd_b1,#$00
    mov.w !PORT0,y
    ret

; @routine 85:84AE VoiceTick
; @spc 06B1
; WHAT IT DOES: Steps voice X's track for one tick: its note's time, then
; its commands and next note when the time runs out.
; HOW: The note's length counted down by $A8; the release point
; (VoiceAdsr1Set) when its gate ends; a slide (from $0AC3) while it runs;
; at 0 the command loop: commands below $20 run from the handler table at
; $0831, a byte of $20 or more is a note: its pitch, length and key-on.
; Entry: X = the voice; $B3 its bit; $A8 the tick's length; called by
; CALL. Exit: as the note or command leaves it.
VoiceTick:
    mov !sd_b4,!sd_b3
    eor !sd_b4,#$FF
    mov.b a,!sd_18+x
    beq .l06E9
    mov.b y,!sd_28+x
    mov a,y
    or.b a,!sd_20+x
    beq .l06D7
    mov.b a,!sd_20+x
    subw ya,!sd_a9
    beq .l06CA
    bcs .l06D3
.l06CA:
    bbs !sd_b9.7,.l06D0
    call VoiceAdsr1Set
.l06D0:
    mov y,#$00
    mov a,y
.l06D3:
    mov.b !sd_28+x,y
    mov.b !sd_20+x,a
.l06D7:
    mov.b a,!sd_18+x
    setc
    sbc.b a,!sd_a8
    mov.b !sd_18+x,a
    beq .l06E9
    bcc .l06E9
    cmp a,#$C1
    bcs .l06E9
    jmp VoiceSlide
.l06E9:
    mov y,#$06
    mov a,#$F2
    push y
    push a
    jmp .l0803
    push a
    xcn a
    lsr a
    and a,#$07
    dec a
    mov y,a
    mov.b a,!sd_10+x
    and a,#$30
    bne .l0704
    mov.w a,!sd_07e5+y
    bra .l0716
.l0704:
    and a,#$10
    bne .l070D
    mov.w a,!sd_07de+y
    bra .l0716
.l070D:
    mov.b a,!sd_10+x
    and a,#$EF
    mov.b !sd_10+x,a
    mov.w a,!sd_07ec+y
.l0716:
    clrc
    adc.b a,!sd_18+x
    mov.b !sd_18+x,a
    pop a
    and a,#$1F
    bne .l0721
    ret
.l0721:
    push a
    mov.b a,!sd_10+x
    and a,#$0F
    mov y,a
    pop a
    clrc
    adc.w a,!sd_07f3+y
    bbs !sd_b0.7,.l0732
    clrc
    adc.b a,!sd_c4
.l0732:
    clrc
    adc.w a,!sd_0210+x
    mov.b !sd_a1,a
    mov !sd_a0,#$00
    mov y,#$00
    mov.w a,!sd_0218+x
    bpl .l0743
    dec y
.l0743:
    addw ya,!sd_a0
    cmp y,#$61
    bcc .l074B
    mov y,#$60
.l074B:
    setp
    movw !sd_a2,ya
    mov.b a,!sd_00+x
    mov.b !sd_a4,a
    clr1 !sd_a4.0
    mov.b a,!sd_28+x
    beq .l0760
    mov.b a,!sd_40+x
    beq .l0760
    cmp.b a,!sd_a3
    bne .l0764
.l0760:
    mov.b a,!sd_a2
    bra .l0772
.l0764:
    bcs .l076A
    set1 !sd_a4.1
    bra .l076C
.l076A:
    clr1 !sd_a4.1
.l076C:
    inc.b !sd_a4
    mov.b y,!sd_40+x
    mov.b a,!sd_48+x
.l0772:
    push a
    mov.b a,!sd_a4
    mov.b !sd_00+x,a
    mov.b a,!sd_a3
    mov.b !sd_40+x,a
    mov.b a,!sd_a2
    mov.b !sd_48+x,a
    pop a
    mov.b !sd_30+x,y
    mov.b !sd_38+x,a
    clrp
    mov.b a,!sd_10+x
    bmi .l07B3
    mov.w a,!sd_0100+x
    mov y,a
    and a,#$10
    beq .l079C
    mov a,y
    and a,#$3F
    mov.w !sd_0100+x,a
    mov a,#$00
    mov.w !sd_0120+x,a
.l079C:
    bbc !sd_b0.7,.l07A7
    or !sd_b5,!sd_b3
    or !sd_b6,!sd_b3
    bra .l07B0
.l07A7:
    bbs !sd_b9.7,.l07BD
    or !sd_ba,!sd_b3
    or !sd_bb,!sd_b3
.l07B0:
    call VoiceRelease
.l07B3:
    bbs !sd_b9.7,.l07BD
    setp
    call VoicePitchSet
    call VoiceVolumeSet
.l07BD:
    mov.b a,!sd_10+x
    mov y,a
    and a,#$7F
    mov.b !sd_a3,a
    mov a,y
    and a,#$40
    asl a
    or.b a,!sd_a3
    mov.b !sd_10+x,a
    bpl .l07D3
    mov y,#$00
    mov a,y
    bra .l07D9
.l07D3:
    mov.b y,!sd_18+x
    mov.w a,!sd_0208+x
    mul ya
.l07D9:
    mov.b !sd_28+x,y
    mov.b !sd_20+x,a
    ret
    db $02,$04,$08,$10,$20,$40,$80,$03,$06,$0C,$18,$30
    db $60,$C0,$00,$09,$12,$24,$48,$90,$00,$00,$0C,$18
    db $24,$30,$3C,$48,$54,$18,$24,$30,$3C,$48,$54,$60
    db $6C
.l0803:
    call TrackByte
    cmp a,#$20
    bcs TrackByteEnd
    mov.b !sd_a7,a
    mov y,#$08
    push y
    mov y,#$03
    push y
    asl a
    mov y,a
    mov.w a,SeqCommandHandlers+1+y
    push a
    mov.w a,SeqCommandHandlers+y
    push a
    cmp y,#$08
    bcc TrackByteEnd

; @routine 85:861D TrackByte
; @spc 0820
; WHAT IT DOES: Reads voice X's next track byte.
; HOW: The track pointer at $00+X/$08+X into $A0, the byte at it, the
; pointer counted up.
; Entry: X = the voice; called by CALL. Exit: A = the byte; Y = 0; N, Z
; from the increment.
TrackByte:
    mov.b y,!sd_08+x
    mov.b a,!sd_00+x
    movw !sd_a0,ya
    mov y,#$00
    mov a,(!sd_a0)+y
    inc.b !sd_00+x
    bne TrackByteEnd
    inc.b !sd_08+x

TrackByteEnd:
    ret
; The sequencer commands $00-$1F: each handler is pushed and reached by RET, after $0803.
SeqCommandHandlers:
    dw SeqCmd00, SeqCmd01, SeqCmd02, SeqCmd03
    dw VoiceFlagsSet, SeqCmd05, SeqCmd06, SeqCmd07
    dw SeqCmd08, SeqCmd09, SeqCmd0A, SeqCmd0B
    dw SeqCmd0C, SeqCmd0D, SeqCmd0E, SeqCmd0F
    dw SeqCmd10, SeqCmd11, SeqCmd0E, SeqCmd0F
    dw SeqCmd10, SeqCmd11, SeqCmd16, SeqCmd17
    dw SeqCmd18, SeqCmd19, SeqCmd1A, SeqCmd1B
    dw SeqCmd1C, SeqCmd1D, TrackByteEnd, TrackByteEnd

SeqCmd00:
    mov a,#$20
    bra SeqCmd03_l0881

SeqCmd01:
    mov a,#$40
    bra SeqCmd03_l0881

SeqCmd02:
    mov a,#$10
    or.b a,!sd_10+x
    bra SeqCmd03_l0883

SeqCmd03:
    mov a,#$08
.l0881:
    eor.b a,!sd_10+x
.l0883:
    mov.b !sd_10+x,a
    ret

SeqCmd05:
    push a
    call TrackByte
    pop y
    bbc !sd_b0.7,.l089B
    mov.w !sd_0258+x,a
    mov a,y
    mov.w !sd_0260+x,a
    mov a,#$00
    mov.w !sd_0268+x,a
    ret
.l089B:
    movw !sd_be,ya
    mov !sd_c2,#$00
    ret

SeqCmd06:
    mov.w !sd_0208+x,a
    ret

SeqCmd07:
    mov y,a
    call CurveAtY
    bbs !sd_b0.7,.l08B6
    push a
    mov.w y,!sd_1db1
    call CurveAtY
    pop y
    mul ya
    mov a,y
.l08B6:
    mov.w !sd_0230+x,a
    ret

SeqCmd08:
    bbs !sd_b0.7,VoiceInstrumentFrom
    inc a
    mov.w !sd_0250+x,a
    bbs !sd_b9.7,VoiceInstrumentEnd

; @routine 85:86C1 VoiceInstrument
; @spc 08C4
; WHAT IT DOES: Sets voice X's instrument A less 1 from the instrument
; table at $3438: its envelope, gain and sample source.
; HOW: Instrument A less 1, times 6, indexes the table: its ADSR1 kept at
; $0238, ADSR1-GAIN and the source to the voice's DSP registers 4-7, its
; pitch multiplier to $0220/$0228.
; Entry: A = the instrument; X = the voice. Exit: X unchanged.
VoiceInstrument:
    dec a

VoiceInstrumentFrom:
    mov y,#$06
    mul ya
    movw !sd_a0,ya
    clrc
    adc !sd_a0,#$38
    adc !sd_a1,#$34
    mov y,#$01
    mov a,(!sd_a0)+y
    mov.w !sd_0238+x,a
    dec y
    push x
    mov a,x
    xcn a
    and a,#$70
    or a,#$04
    mov x,a
.l08E1:
    mov a,(!sd_a0)+y
    mov.w !DSP_ADDR,x
    mov.w !DSP_DATA,a
    inc x
    inc y
    cmp y,#$04
    bne .l08E1
    pop x
    mov a,(!sd_a0)+y
    mov.w !sd_0220+x,a
    inc y
    mov a,(!sd_a0)+y
    mov.w !sd_0228+x,a

VoiceInstrumentEnd:
    ret

SeqCmd09:
    mov.b !sd_a3,a
    mov.b a,!sd_10+x
    and a,#$F8
    or.b a,!sd_a3
    mov.b !sd_10+x,a
    ret

SeqCmd0A:
    bbs !sd_b0.7,.l090C
    mov.b !sd_c4,a
.l090C:
    ret

SeqCmd0B:
    mov.w !sd_0210+x,a
    ret

SeqCmd0C:
    mov.w !sd_0218+x,a
    ret

SeqCmd0D:
    mov.w !sd_0128+x,a
    ret

SeqCmd0E:
    mov y,#$30
    bra SeqCmd11_l0927

SeqCmd0F:
    mov y,#$38
    bra SeqCmd11_l0927

SeqCmd10:
    mov y,#$40
    bra SeqCmd11_l0927

SeqCmd11:
    mov y,#$48
.l0927:
    mov.b !sd_a3,a
    mov.b !sd_a0,y
    mov a,x
    clrc
    adc.b a,!sd_a0
    mov y,a
    mov !sd_a1,#$00
    mov !sd_a0,#$00
    cmp !sd_a7,#$12
    bcs .l094C
    mov a,(!sd_a0)+y
    beq .l0946
    dec a
    mov (!sd_a0)+y,a
    beq SeqCmd16_l0965
    bra .l0958
.l0946:
    mov.b a,!sd_a3
    mov (!sd_a0)+y,a
    bra .l0958
.l094C:
    mov a,(!sd_a0)+y
    dec a
    bne SeqCmd16_l0965
    mov (!sd_a0)+y,a
    mov.b a,!sd_a3
    call VoiceFlagsSet
.l0958:
    call TrackByte

SeqCmd16:
    push a
    call TrackByte
    mov.b !sd_00+x,a
    pop a
    mov.b !sd_08+x,a
    ret
.l0965:
    mov a,#$02
    clrc
    adc.b a,!sd_00+x
    mov.b !sd_00+x,a
    bcc .l0970
    inc.b !sd_08+x
.l0970:
    ret

; @routine 85:876E VoiceFlagsSet
; @spc 0971
; WHAT IT DOES: Sequencer command $04: sets voice X's mode bits (bits 3,
; 5, 6 of $10+X) from A.
; HOW: $10+X AND $97, OR A.
; Entry: A = the bits; X = the voice. Exit: A = the new flags; N, Z from
; them.
VoiceFlagsSet:
    mov.b !sd_a3,a
    mov.b a,!sd_10+x
    and a,#$97
    or.b a,!sd_a3
    mov.b !sd_10+x,a
    ret

SeqCmd17:
    pop a
    pop a
    pop a
    pop a
    bbs !sd_b0.7,.l0996
    mov a,#$00
    mov.b !sd_08+x,a
    mov.b !sd_00+x,a
    mov.w !sd_0250+x,a
    bbs !sd_b9.7,.l0995
    call VoiceRelease
    or !sd_bb,!sd_b3
.l0995:
    ret
.l0996:
    and !sd_b8,!sd_b4
    and !sd_b7,!sd_b4

; @routine 85:8799 VoiceStop
; @spc 099C
; WHAT IT DOES: Stops voice X: its priority cleared, released, its key-off
; marked.
; HOW: $0250+X = 0; VoiceRelease; $B6 |= $B3; then VoiceSfxMark.
; Entry: X = the voice; $B3 its bit. Exit: as VoiceSfxMark.
VoiceStop:
    mov a,#$00
    mov.w !sd_0250+x,a
    call VoiceRelease
    or !sd_b6,!sd_b3

; @routine 85:87A4 VoiceSfxMark
; @spc 09A7
; WHAT IT DOES: Marks voice X in $BD when the music's voice under it is
; still playing.
; HOW: $0250 of the voice X - $50 nonzero: $BD |= $B3.
; Entry: X = the voice; $B3 its bit. Exit: X unchanged.
VoiceSfxMark:
    push x
    mov a,x
    setc
    sbc a,#$50
    mov x,a
    mov.w a,!sd_0250+x
    beq .l09B5
    or !sd_bd,!sd_b3
.l09B5:
    pop x
    ret

SeqCmd18:
    clrc
    adc a,#$80
    mov.w !sd_0248+x,a
    ret

SeqCmd19:
    bbs !sd_b0.7,.l09C7
    mov y,a
    call CurveAtY
    mov.b !sd_c3,a
.l09C7:
    ret

SeqCmd1A:
    push a
    call TrackByte
    pop y
    cmp y,#$03
    bne .l09DF
    xcn a
    mov.b !sd_a3,a
    mov.w a,!sd_0100+x
    and a,#$EF
    or.b a,!sd_a3
    mov.w !sd_0100+x,a
    ret
.l09DF:
    push a
    mov a,#$08
    mul ya
    mov.b !sd_a4,x
    clrc
    adc.b a,!sd_a4
    mov y,a
    pop a
    mov.w !sd_0108+y,a
    ret

SeqCmd1B:
    call TrackByte
    mov y,#$08
    mul ya
    cmp.b a,!sd_cb
    beq EchoSetup_l0A52
    mov.b !sd_cb,a

EchoSetup:
    push x
    mov x,a
    mov y,#$0C
    mov.w a,!sd_0d10+x
    call DspWrite
    mov y,#$1C
    mov.w a,!sd_0d11+x
    call DspWrite
    mov.w a,!sd_0d12+x
    mov.b !sd_ce,a
    mov.w a,!sd_0d13+x
    mov.b !sd_cf,a
    call EchoOff
    mov y,#$6D
    mov.w a,!sd_0d14+x
    call DspWrite
    mov a,#$7D
    mov.w !DSP_ADDR,a
    mov.w a,!DSP_DATA
    and a,#$0F
    inc a
    eor a,#$FF
    bbc !sd_cd.7,.l0A34
    clrc
    adc.b a,!sd_cd
.l0A34:
    mov.b !sd_cd,a
    mov.w a,!sd_0d15+x
    mov.w !DSP_DATA,a
    mov a,#$20
    clrc
    adc.b a,!sd_cc
    mov.b !sd_cc,a
    mov y,#$0D
    mov.w a,!sd_0d16+x
    call DspWrite
    mov.w a,!sd_0d17+x
    call EchoFirSet
    pop x
.l0A52:
    ret

; @routine 85:8850 EchoOff
; @spc 0A53
; WHAT IT DOES: Turns echo off: echo volume and feedback 0, then the echo
; write disabled.
; HOW: The DSP registers at $0A68 (3) to 0; FLG ($6C) = $20.
; Entry: called by CALL. Exit: as DspWrite.
EchoOff:
    mov y,#$03
.l0A55:
    mov.w a,!sd_0a68+y
    mov.w !DSP_ADDR,a
    mov a,#$00
    mov.w !DSP_DATA,a
    dbnz y,.l0A55
    mov y,#$6C
    mov a,#$20
    jmp DspWrite
    db $2C,$3C,$4D

; @routine 85:8869 EchoFirSet
; @spc 0A6C
; WHAT IT DOES: Sets the echo filter's 8 coefficients from filter A of the
; table at $0A83.
; HOW: A times 8 indexes the table; the DSP registers $0F, $1F ... $7F.
; Entry: A = the filter. Exit: X unchanged.
EchoFirSet:
    push x
    mov y,#$08
    mul ya
    mov x,a
    mov y,#$0F
.l0A73:
    mov.w a,!sd_0a83+x
    call DspWrite
    inc x
    mov a,y
    clrc
    adc a,#$10
    mov y,a
    bpl .l0A73
    pop x
    ret
    db $7F,$00,$00,$00,$00,$00,$00,$00,$58,$BF,$DB,$F0
    db $FE,$07,$0C,$0C,$0C,$21,$2B,$2B,$13,$FE,$F3,$F9
    db $34,$33,$00,$D9,$E5,$01,$FC,$EB

SeqCmd1C:
    and a,#$01
    lsr a
    bcc .l0AAA
    mov.b a,!sd_b3
.l0AAA:
    bbs !sd_b0.7,.l0AB5
    and !sd_bc,!sd_b4
    or.b a,!sd_bc
    mov.b !sd_bc,a
    ret
.l0AB5:
    and !sd_b7,!sd_b4
    or.b a,!sd_b7
    mov.b !sd_b7,a
    ret

SeqCmd1D:
    or a,#$A0
    mov.w !sd_0240+x,a
    ret

VoiceSlide:
    setp
    mov.b a,!sd_18+x
    clrc
    adc.b a,!sd_20+x
    mov.b !sd_20+x,a
    bcc .l0AD4
    mov.b a,!sd_00+x
    clrc
    adc a,#$40
    mov.b !sd_00+x,a
.l0AD4:
    mov.b a,!sd_00+x
    mov.b !sd_a4,a
    bbc !sd_a4.0,.l0B0D
    mov !sd_a1,#$00
    mov.b a,!sd_28+x
    asl a
    mov.b !sd_a0,a
    mov.b a,!sd_40+x
    mov.b !sd_a3,a
    mov.b a,!sd_48+x
    mov.b !sd_a2,a
    mov.b y,!sd_30+x
    mov.b a,!sd_38+x
    bbs !sd_a4.1,.l0AFC
    subw ya,!sd_a0
    bcc .l0B02
    cmpw ya,!sd_a2
    bcc .l0B02
    bra .l0B09
.l0AFC:
    addw ya,!sd_a0
    cmpw ya,!sd_a2
    bcc .l0B09
.l0B02:
    mov.b a,!sd_a4
    dec a
    mov.b !sd_00+x,a
    movw ya,!sd_a2
.l0B09:
    mov.b !sd_30+x,y
    mov.b !sd_38+x,a
.l0B0D:
    mov.w a,!sd_00b9
    bpl .l0B14
    clrp
    ret
.l0B14:
    call VoicePitchUpdate

; @routine 85:8914 VoiceVolumeSet
; @spc 0B17
; WHAT IT DOES: Sets voice X's left and right volume from its volume, pan
; and the fade.
; HOW: The pan ($20+X, by the note's half) through the pan curve
; (CurveAt8), the music's fade (CurveAt16), the voice's volume ($0230) and
; $0248 (or $80), each step a multiply; the two products to DSP registers
; 1 and 0.
; Entry: X = the voice. Exit: $A5 = $FF.
VoiceVolumeSet:
    mov.b y,!sd_10+x
    mov.b a,!sd_00+x
    asl a
    asl a
    mov.b a,!sd_20+x
    bcs .l0B23
    eor a,#$FF
.l0B23:
    mul ya
    movw !sd_a2,ya
    mov y,#$7E
    mov a,#$81
    subw ya,!sd_a2
    call CurveAt8
    mov y,a
    clrp
    bbs !sd_b0.7,.l0B47
    mov.b a,!sd_c3
    mul ya
    push y
    movw ya,!sd_c9
    bbs !sd_c6.7,.l0B42
    mov y,#$FF
    mov a,y
    subw ya,!sd_c9
.l0B42:
    call CurveAt16
    pop y
    mul ya
.l0B47:
    mov.w a,!sd_0230+x
    mul ya
    mov.b !sd_a3,y
    bbc !sd_b0.5,.l0B54
    mov a,#$80
    bra .l0B57
.l0B54:
    mov.w a,!sd_0248+x
.l0B57:
    mov y,#$14
    mul ya
    movw !sd_a0,ya
    mov !sd_a5,#$01
.l0B5F:
    mov.b y,!sd_a1
    mov.w a,!sd_0bc1+y
    setc
    sbc.w a,!sd_0bc0+y
    mov.b y,!sd_a0
    mul ya
    mov a,y
    mov.b y,!sd_a1
    clrc
    adc.w a,!sd_0bc0+y
    mov.b y,!sd_a3
    mul ya
    mov a,y
    mov.b y,!sd_a5
    call DspWriteVoice
    mov y,#$14
    mov a,#$00
    subw ya,!sd_a0
    movw !sd_a0,ya
    dec.b !sd_a5
    bpl .l0B5F
    ret

; @routine 85:8985 CurveAt16
; @spc 0B88
; WHAT IT DOES: The pan curve at YA / 16 (a 4-bit fraction).
; HOW: $A1 = Y; four shifts of $A1:A; then as CurveAt8 from its lookup.
; Entry: YA = the position. Exit: A = the curve's value.
CurveAt16:
    mov.b !sd_a1,y
    mov y,#$04
    bra CurveShift

; @routine 85:898B CurveAtY
; @spc 0B8E
; WHAT IT DOES: The pan curve at Y (with a fraction of $FF).
; HOW: A = $FF, then CurveAt8.
; Entry: Y = the position. Exit: as CurveAt8.
CurveAtY:
    mov a,#$FF

; @routine 85:898D CurveAt8
; @spc 0B90
; WHAT IT DOES: The pan curve at YA / 8: the table at $0BAF interpolated.
; HOW: Three shifts of $A1:A give the entry and the fraction; the entry
; plus the fraction of the step to the next.
; Entry: YA = the position. Exit: A = the value; Y = the entry.
CurveAt8:
    mov.b !sd_a1,y
    mov y,#$03

CurveShift:
    lsr.b !sd_a1
    ror a
    dbnz y,CurveShift
    mov.b !sd_a0,a
    mov.b y,!sd_a1
    mov.w a,!sd_0bb0+y
    setc
    sbc.w a,!sd_0baf+y
    mov.b y,!sd_a0
    mul ya
    mov a,y
    mov.b y,!sd_a1
    clrc
    adc.w a,!sd_0baf+y
    ret
    db $00,$0C,$19,$26,$33,$40,$4C,$59,$66,$73,$80,$8C
    db $99,$B3,$CC,$E6,$FF,$00,$01,$03,$07,$0D,$15,$1E
    db $29,$34,$42,$51,$5E,$67,$6E,$73,$77,$7A,$7C,$7D
    db $7E,$7F,$7F

; @routine 85:89D3 VoicePitchUpdate
; @spc 0BD6
; WHAT IT DOES: Updates voice X's pitch when a slide, vibrato or a detune
; is running.
; HOW: Nothing when neither its slide nor its vibrato runs and the detune
; ($C5) is 0; else on into VoicePitchSet.
; Entry: X = the voice. Exit: as VoicePitchSet.
VoicePitchUpdate:
    mov.b a,!sd_00+x
    and a,#$01
    or.b a,!sd_08+x
    bne VoicePitchSet
    mov.w a,!sd_00b0
    bmi .l0BE8
    mov.w a,!sd_00c5
    bne VoicePitchFrom
.l0BE8:
    ret

; @routine 85:89E6 VoicePitchSet
; @spc 0BE9
; WHAT IT DOES: Sets voice X's DSP pitch from its note, slide, vibrato and
; the detune.
; HOW: The note and fraction ($30/$38+X), the vibrato's offset added or
; taken, the detune ($C5) added, then the octave table at $0CC2
; interpolated and scaled by the instrument's multiplier; DSP registers 2
; and 3.
; Entry: X = the voice. Exit: as DspWrite.
VoicePitchSet:
    mov.b a,!sd_08+x
    bne VoicePitchFrom_l0BF3

VoicePitchFrom:
    mov.b y,!sd_30+x
    mov.b a,!sd_38+x
    bra .l0C22
.l0BF3:
    asl a
    mov y,#$0C
    mul ya
    movw !sd_a0,ya
    mov.b a,!sd_00+x
    push p
    asl a
    asl a
    mov.b a,!sd_20+x
    bcc .l0C04
    eor a,#$FF
.l0C04:
    push a
    mov.b y,!sd_a0
    mul ya
    mov.b !sd_a0,y
    pop a
    mov.b y,!sd_a1
    mul ya
    mov !sd_a1,#$00
    addw ya,!sd_a0
    movw !sd_a0,ya
    mov.b y,!sd_30+x
    mov.b a,!sd_38+x
    pop p
    bmi .l0C20
    addw ya,!sd_a0
    bra .l0C22
.l0C20:
    subw ya,!sd_a0
.l0C22:
    clrp
    bbs !sd_b0.7,.l0C2E
    mov !sd_a1,!sd_c5
    mov !sd_a0,#$00
    addw ya,!sd_a0
.l0C2E:
    setp
    dec y
    mov.b !sd_a1,y
    mov.b !sd_a0,a
    mov a,y
    mov y,#$00
    setc
    sbc a,#$34
    bcs .l0C45
    mov.b a,!sd_a1
    setc
    sbc a,#$13
    bcs .l0C49
    dec y
    asl a
.l0C45:
    addw ya,!sd_a0
    movw !sd_a0,ya
.l0C49:
    push x
    mov.b a,!sd_a1
    asl a
    mov y,#$00
    mov x,#$18
    div ya,x
    mov x,a
    mov.w a,!sd_0cc3+y
    mov.b !sd_a3,a
    mov.w a,!sd_0cc2+y
    mov.b !sd_a2,a
    mov.w a,!sd_0cc5+y
    push a
    mov.w a,!sd_0cc4+y
    pop y
    subw ya,!sd_a2
    mov.b y,!sd_a0
    mul ya
    mov a,y
    mov y,#$00
    addw ya,!sd_a2
    mov.b !sd_a1,y
    asl a
    rol.b !sd_a1
.l0C74:
    cmp x,#$08
    beq .l0C7E
    lsr.b !sd_a1
    ror a
    inc x
    bra .l0C74
.l0C7E:
    mov.b !sd_a0,a
    pop x
    mov.w a,!sd_0228+x
    mov.b y,!sd_a1
    mul ya
    movw !sd_a2,ya
    mov.w a,!sd_0228+x
    mov.b y,!sd_a0
    mul ya
    push y
    mov.w a,!sd_0220+x
    mov.b y,!sd_a0
    mul ya
    addw ya,!sd_a2
    movw !sd_a2,ya
    mov.w a,!sd_0220+x
    mov.b y,!sd_a1
    mul ya
    mov y,a
    pop a
    addw ya,!sd_a2
    and a,#$F0
    push y
    mov y,#$02
    call DspWriteVoice
    pop a
    inc y
    bra DspWrite
%spc_end()
