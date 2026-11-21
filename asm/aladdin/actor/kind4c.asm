; kind4c.asm: actor kind $4C, the five marks at the end of levels $12 and $1C: each shows its
; animation when the gem sets found reach its number, then goes; with $07 bit 4 set (no placement
; has it) it waits, sets the level 0 and shows its animation for ever.
; C: src/aladdin/actor/kind4c.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!K4C_COUNT = $40                ; frames of its animation left
!K4C_WAIT = $41                 ; its long wait's two counters (bytes)
!K4C_FRAMES = $88               ; frames its animation shows
!K4C_SOUND_AT = $50             ; the count at which its sound plays
!K4C_SOUND = $4C                ; its sound
!K4C_PALETTE_ROW = $5A          ; ActorPaletteFromRom's row for it
!K4C_GEM_SETS = $0375           ; the gem sets found

; @routine A5:F43D Kind4CStart
; @actor
; WHAT IT DOES: Actor kind $4C, one of the five placed at the end of
; levels $12 and $1C (its $07 0-4 numbers them; bit 4 of $07 is set by
; no placement). With $07 bit 4 clear it shows its animation 0 for 136
; frames, with sound $4C halfway, when the gem sets found ($0375) less 5
; are at least its $07, else it goes at once; with $07 bit 4 set it
; waits $A0 x $8C frames, sets the level ($035E) to 0 and then shows
; animation 0 again and again, with a 32-frame wait between.
; HOW: $08 bit 1 and $09 bit 4 set. $07 bit 0 clear: the palette byte
; ($2C) 2 higher, and with $07's low four bits clear,
; ActorPaletteFromRom with Y = $5A. $07 bit 4 clear: K4C_COUNT ($40) =
; $88; $0375 less 5 borrowing, or less than $07: JML ActorEndPlaced;
; else AnimStart 0, and each frame (BRK) AnimStep, K4C_COUNT down to 0,
; sound $4C when it is $50 (AnimStart's frames first); then JML
; ActorEndPlaced. $07 bit 4 set: K4C_WAIT (the bytes $41, $42) = $A0,
; $8C; each frame (BRK) $41 down, and at 0 $42 down, until both are 0;
; the level ($035E) cleared; then for ever K4C_COUNT = $88, AnimStart 0,
; each frame (BRK) AnimStep and K4C_COUNT down to 0 with the sound at
; $50, and a COP wait of $20 frames.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML of
; kind $4C's stub (Kind4CEnter, $83:9245) from the actor loop in the
; start state.
; Exit: never returns: the first part ends in ActorEndPlaced, the second
; goes on for ever.
%at($A5F43D)
Kind4CStart:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$10
    sta.b !actor_09
    lda.b !actor_07
    lsr a
    bcs .shows
    lda.b !actor_look+1
    clc
    adc.b #$02
    sta.b !actor_look+1
    lda.b !actor_07
    and.b #$0F
    bne .shows
    ldy.b #!K4C_PALETTE_ROW
    jsl.l ActorPaletteFromRom
.shows:
    lda.b !actor_07
    and.b #$10
    bne .waits
    lda.b #!K4C_FRAMES
    sta.b !K4C_COUNT
    sec
    lda.w !K4C_GEM_SETS
    sbc.b #$05
    bcc .gone
    cmp.b !actor_07
    bcc .gone
    lda.b #$00
    jsl.l AnimStart
.show:
    lda.b !K4C_COUNT
    cmp.b #!K4C_SOUND_AT
    bne .frame
    lda.b #!K4C_SOUND
    jsl.l SoundQueueAdd
.frame:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K4C_COUNT
    bne .show
.gone:
    jml.l ActorEndPlaced
.waits:
    lda.b #$A0
    sta.b !K4C_WAIT
    lda.b #$8C
    sta.b !K4C_WAIT+1
.waiting:
    brk.b #$00
    dec.b !K4C_WAIT
    bne .waiting
    dec.b !K4C_WAIT+1
    bne .waiting
    stz.w !level
.again:
    lda.b #!K4C_FRAMES
    sta.b !K4C_COUNT
    lda.b #$00
    jsl.l AnimStart
.again_show:
    lda.b !K4C_COUNT
    cmp.b #!K4C_SOUND_AT
    bne .again_frame
    lda.b #!K4C_SOUND
    jsl.l SoundQueueAdd
.again_frame:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K4C_COUNT
    bne .again_show
    lda.b #$20
    cop.b #$00
    bra .again
