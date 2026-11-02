; palette_cycles.asm: the level's palette cycles: six records of 14
; bytes from $15F1, each playing a script of colour frames from the ROM's
; table at $84:E000 into the palette buffer, round and round.
;
; A record, through the direct page (D = the record): $00 nonzero while it
; runs; $01 its cycle (a word index into the table); $02 the frames in the
; script and $03 the colours in a frame; $05 the wait before the next
; frame; $06 the script's first frame and $08 the next one (offsets from
; $84:E004); $0A the first colour's offset in the palette buffer; $0C the
; step (0 load, 1 restart, 2 frame, 3 wait); $0D the frames left.
; C: src/aladdin/video/palette_cycles.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!PALETTE_CYCLES = $15F1         ; the six records, 14 bytes apart
!PALETTE_CYCLES_END = $1645
!PALETTE_CYCLE_SIZE = 14
!PCYC_ACTIVE = $00
!PCYC_ID = $01
!PCYC_FRAMES = $02
!PCYC_COLOURS = $03
!PCYC_WAIT = $05
!PCYC_START = $06
!PCYC_NEXT = $08
!PCYC_AT = $0A
!PCYC_STEP = $0C
!PCYC_LEFT = $0D
!PALETTE_CYCLE_TABLE = $84E000  ; by cycle: a script's offset (words); a script: frames, colours (bytes)
!PALETTE_CYCLE_DATA = $84E004   ; a frame: its wait (byte), then its colours (words)

; @routine 81:EC8A PaletteCycleStartLong
; WHAT IT DOES: PaletteCycleStart for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as PaletteCycleStart; called by JSL. Exit: as PaletteCycleStart.
;
%at($81EC8A)
PaletteCycleStartLong:
    jsr.w PaletteCycleStart
    rtl

; @routine 81:EC8E PaletteCycleStart
; WHAT IT DOES: Starts palette cycle A in record X.
; HOW: Its step 0 (load), its cycle A, and its active byte counted up.
; Entry: M 8-bit; A low = the cycle; X = the record's offset from $15F1;
; DB a bank whose $0000-$1FFF is work RAM; called by JSR. Exit: N, Z
; from the active byte; the registers unchanged.
;
%at($81EC8E)
PaletteCycleStart:
    stz.w !PALETTE_CYCLES+!PCYC_STEP,x
    sta.w !PALETTE_CYCLES+!PCYC_ID,x
    inc.w !PALETTE_CYCLES+!PCYC_ACTIVE,x
    rts

; @routine 81:EC98 PaletteCyclesStep
; WHAT IT DOES: Steps every running palette cycle, once a frame.
; HOW: With D at each of the six records in turn, one with its active
; byte nonzero runs the code its step names (the table at $81:ECBB).
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
; Exit: M and X 8-bit; D unchanged; N, Z from it (PLD); A = $1645 (16
; bits); C set, V unchanged; X as the last step leaves it (low byte).
;
%at($81EC98)
PaletteCyclesStep:
    phd
    lda.b #!PALETTE_CYCLES>>8
    xba
    lda.b #!PALETTE_CYCLES&$FF
    tcd
.record:
    lda.b !PCYC_ACTIVE
    beq .next
    lda.b !PCYC_STEP
    asl a
    tax
    jsr.w (PaletteCycleSteps,x)
.next:
    rep.b #$20
    tdc
    clc
    adc.w #!PALETTE_CYCLE_SIZE
    tcd
    cmp.w #!PALETTE_CYCLES_END
    sep.b #$30
    bne .record
    pld
    rts

; The steps' code, by step: load, restart, frame, wait.
PaletteCycleSteps:
    dw PaletteCycleLoad, PaletteCycleRestart, PaletteCycleFrame, PaletteCycleWait

; @routine 81:ECC3 PaletteCycleLoad
; WHAT IT DOES: A palette cycle's first step: finds its script.
; HOW: The cycle's entry in the table at $84:E000 is the script's offset,
; kept as its first frame; the script's first word is its frames and
; colours, its second the palette buffer offset; the step goes on.
; Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep.
; Exit: M and X 16-bit; A = the palette offset; X = the script's offset;
; N, Z from the step's increment; C, V, Y unchanged.
;
%at($81ECC3)
PaletteCycleLoad:
    lda.b !PCYC_ID
    asl a
    tax
    rep.b #$30
    lda.l !PALETTE_CYCLE_TABLE,x
    sta.b !PCYC_START
    tax
    lda.l !PALETTE_CYCLE_TABLE,x
    sta.b !PCYC_FRAMES
    lda.l !PALETTE_CYCLE_TABLE+2,x
    sta.b !PCYC_AT
    inc.b !PCYC_STEP
    rts

; @routine 81:ECDF PaletteCycleRestart
; WHAT IT DOES: Starts a palette cycle's script again from its first
; frame, and puts that frame in the buffer.
; HOW: Its frames left = the script's frames, its next frame = its first;
; the step goes on; then PaletteCycleFrame (the code that follows).
; Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep.
; Exit: as PaletteCycleFrame.
;
%at($81ECDF)
PaletteCycleRestart:
    rep.b #$10
    lda.b !PCYC_FRAMES
    sta.b !PCYC_LEFT
    lda.b !PCYC_START
    sta.b !PCYC_NEXT
    lda.b !PCYC_START+1
    sta.b !PCYC_NEXT+1
    inc.b !PCYC_STEP

; @routine 81:ECEF PaletteCycleFrame
; WHAT IT DOES: Puts a palette cycle's next frame in the palette buffer.
; HOW: The frame's wait byte, then its colours (the script's count of
; them) copied as words from $84:E004 plus the next frame's offset to the
; buffer from the cycle's offset; the next frame's offset kept, the step
; goes on (to the wait) and the buffer is marked for upload. Scratch
; $000C holds the colours left.
; Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep,
; or reached from PaletteCycleRestart. Exit: M 8-bit, X 16-bit; X = the
; next frame's offset; Y = past the last colour's offset; A = the last
; colour (16 bits); N, Z from the upload flag's increment; C, V from the
; last offset step; scratch $000C = 0.
;
PaletteCycleFrame:
    rep.b #$10
    lda.b !PCYC_COLOURS
    sta.w !scratch
    stz.w !scratch+1
    ldx.b !PCYC_NEXT
    lda.l !PALETTE_CYCLE_DATA,x
    sta.b !PCYC_WAIT
    inx
    rep.b #$20
    ldy.b !PCYC_AT
.colour:
    lda.l !PALETTE_CYCLE_DATA,x
    phx
    tyx
    sta.l !palette_buffer,x
    plx
    inx
    inx
    iny
    iny
    dec.w !scratch
    bne .colour
    stx.b !PCYC_NEXT
    sep.b #$20
    inc.b !PCYC_STEP
    inc.w !palette_upload
    rts

; @routine 81:ED24 PaletteCycleWait
; WHAT IT DOES: Waits out a palette cycle's frame, then goes on to its
; next frame, or back to its first once every frame has played.
; HOW: The wait counted down; at 0 the frames left counted down: step 2
; (frame) while some are left, else step 1 (restart).
; Entry: M 8-bit; D = the record; called by JSR from PaletteCyclesStep.
; Exit: N, Z from the last decrement or A; A low = the step when the
; wait ran out; C, V, X, Y unchanged.
;
%at($81ED24)
PaletteCycleWait:
    dec.b !PCYC_WAIT
    bne .done
    lda.b #2
    dec.b !PCYC_LEFT
    bne .step
    dec a
.step:
    sta.b !PCYC_STEP
.done:
    rts
