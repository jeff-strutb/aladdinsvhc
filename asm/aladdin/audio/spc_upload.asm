; spc_upload.asm: sending code and data to the SPC700 through its IPL
; transfer protocol.
;
; The lists are indexed by a multiple of 3 in the pointer table at
; $85:8000 (config/aladdin/spclists.csv): the driver (0), two sample sets,
; then songs. A list is blocks of a length word, an APU address word and
; the bytes; a block of length 0 ends it, its address where the SPC700
; goes on.
; C: src/aladdin/audio/spc_upload.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

!SPC_LISTS = $858000            ; 3 bytes a list: address (bit 15 dropped), bank (bit 7 set)
!IPL_READY = $BBAA              ; ports 0-1 while the SPC700 waits in its IPL
!IPL_START = $CC                ; the first counter value
!SOUND_SETS_LOADED = $000A      ; nonzero once the driver and sample sets are in (cleared at power-on)
!DRIVER_TO_IPL = $F9            ; the command that sends a running driver back to the IPL
!SONG_REQUESTED = $03B9         ; the song SongPlay was asked for
!DRIVER_TO_IPL_SONG = $FA       ; the command that sends the driver back to the IPL for a song

; Direct-page bytes the upload uses: the command, and the list's long
; pointer (its bank steps as the data crosses $8000).
!DP_COMMAND = $34
!DP_LIST = $0C

; @routine 80:B4AA SoundSetsLoad
; WHAT IT DOES: Puts the sound driver and both sample sets into the
; SPC700 the first time it is called after power-on, and restarts the
; sound command sequence.
; HOW: Once marked, SpcUpload of lists 0, 3 and 6 with command $F9; the
; sequence number then starts at 1 either way.
; Entry: M 8-bit, X 8-bit; D a page whose $0C-$0E and $34 SpcUpload may
; use; DB a bank whose $0000-$1FFF is work RAM and whose $2140-$2143 are
; the APU ports; called by JSL. Exit: as the C states.
%at($80B4AA)
SoundSetsLoad:
    lda.w !SOUND_SETS_LOADED
    bne .sequence
    inc.w !SOUND_SETS_LOADED
    ldx.b #0
    lda.b #!DRIVER_TO_IPL
    jsl.l SpcUpload
    ldx.b #3
    lda.b #!DRIVER_TO_IPL
    jsl.l SpcUpload
    ldx.b #6
    lda.b #!DRIVER_TO_IPL
    jsl.l SpcUpload
.sequence:
    lda.b #$01
    sta.w !sound_seq
    rtl

; @routine 80:B4D0 SpcUpload
; WHAT IT DOES: Sends list X (the driver, a sample set or a song) to the
; SPC700. Unless it is the driver, a running driver is first asked back
; into the IPL with command A, once it has taken the last sound command.
; HOW: The IPL protocol: ports 0-1 read $BBAA when it waits; each block's
; address goes to ports 2-3 and a nonzero port 1 says data follows, port 0
; the counter; each byte goes to port 1 with the counter (0 up) to port 0,
; the next sent once port 0 echoes the last; the next block's counter is
; the last plus 3 (4 with the carry), never 0. The last block's address
; goes with port 1 = 0. Then it waits for the new driver to report 1 on
; port 2, the sound sequence number's start. X and Y are 16-bit while it
; works; Y crossing $8000 steps the list's bank.
; Entry: M 8-bit, X 8-bit; A = the command; X = the list's index; D a
; page whose $0C-$0E and $34 it may use; DB a bank whose $0000-$1FFF is
; work RAM and whose $2140-$2143 are the APU ports; called by JSL. Exit: as the C states.
%at($80B4D0)
SpcUpload:
    sta.b !DP_COMMAND
    php
    rep.b #$30
    lda.w #$8000
    sta.b !DP_LIST
    lda.l !SPC_LISTS,x
    and.w #$7FFF
    tay
    sep.b #$20
    lda.l !SPC_LISTS+2,x
    ora.b #$80
    sta.b !DP_LIST+2
    txa
    beq .ipl_ready
    lda.w !sound_seq
.driver_idle:
    cmp.w !APUIO2
    bne .driver_idle
.send_command:
    lda.b !DP_COMMAND
    sta.w !APUIO0
.ipl_ready:
    rep.b #$20
    lda.w #!IPL_READY
    cmp.w !APUIO0
    sep.b #$20
    bne .send_command
    lda.b #!IPL_START
    bra .block
.first_byte:
    lda.b [!DP_LIST],y
    iny
    xba
    lda.b #$00
    bra .put
.next_byte:
    xba
    lda.b [!DP_LIST],y
    iny
    bpl .same_bank
    inc.b !DP_LIST+2
    ldy.w #$0000
.same_bank:
    xba
.echo:
    cmp.w !APUIO0
    bne .echo
    inc a
.put:
    rep.b #$20
    sta.w !APUIO0
    sep.b #$20
    dex
    bne .next_byte
.last_echo:
    cmp.w !APUIO0
    bne .last_echo
.step:
    adc.b #$03
    beq .step
.block:
    pha
    rep.b #$20
    lda.b [!DP_LIST],y
    iny
    iny
    tax
    lda.b [!DP_LIST],y
    iny
    iny
    sta.w !APUIO2
    sep.b #$20
    cpx.w #$0001
    lda.b #$00
    rol a
    sta.w !APUIO1
    adc.b #$7F
    pla
    sta.w !APUIO0
.block_echo:
    cmp.w !APUIO0
    bne .block_echo
    bvs .first_byte
    sep.b #$30
    lda.b #$01
.driver_up:
    cmp.w !APUIO2
    bne .driver_up
    sta.w !sound_seq
    plp
    rtl

; @routine 81:FA23 SongPlay
; WHAT IT DOES: Starts song A: uploads its list (song A's entry, A times
; 3, in the list table) to the SPC700, unless a sound command is being
; sent right now.
; HOW: Sound commands are held while it works; with D 0 (kept on the
; stack meanwhile), SpcUpload with command $FA.
; Entry: M 8-bit, X 8-bit; A = the song's entry number; DB a bank whose
; $0000-$1FFF is work RAM and whose $2140-$2143 are the APU ports; called
; by JSL. Exit: as the C states.
%at($81FA23)
SongPlay:
    ldx.w !sound_sending
    bne .done
    inc.w !sound_hold
    phd
    sta.w !SONG_REQUESTED
    rep.b #$30
    lda.w #$0000
    tcd
    clc
    lda.w !SONG_REQUESTED
    asl a
    adc.w !SONG_REQUESTED
    and.w #$00FF
    tax
    lda.w #!DRIVER_TO_IPL_SONG
    sep.b #$30
    jsl.l SpcUpload
    sep.b #$30
    pld
    stz.w !sound_hold
.done:
    rtl
