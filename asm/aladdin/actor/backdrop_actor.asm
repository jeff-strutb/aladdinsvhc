; backdrop_actor.asm: pieces of the actor of kind $6E (its start at
; $83:F09C, its wait routine at $83:F1D5), drawn as the picture of the
; backdrop layer, not as sprites: it moves right 3 pixels a frame, its
; picture's frames are uploads cycled from a table, and the backdrop is
; shown, scrolled to put the picture where the actor is, only while it is
; near.
; C: src/aladdin/actor/backdrop_actor.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

; The actor's own fields of its record.
!PICTURE_AT = $41               ; the offset of its picture's frame in PICTURE_FRAMES
!PICTURE_TIME = $42             ; frames left before the next

; In the bank DB names: 2 bytes a frame: the upload's number, then its
; frames (bit 7 set on the last of the cycle).
!PICTURE_FRAMES = $D67B

!RANGE_X = $1A44                ; the point actors are kept in range of (words)
!RANGE_Y = $1A46
!TM_SHADOW = $0399              ; TM's shadow: the main screen's layers

; @routine 83:F211 ActorStepRight3
; WHAT IT DOES: Moves the actor 3 pixels to the right.
; HOW: A 24-bit addition of $000300 to its x, a byte at a time.
; Entry: M 8-bit; D = the actor's record. Exit: as the C states.
%at($83F211)
ActorStepRight3:
    clc
    lda.b !actor_x
    adc.b #$00
    sta.b !actor_x
    lda.b !actor_x+1
    adc.b #$03
    sta.b !actor_x+1
    lda.b !actor_x+2
    adc.b #$00
    sta.b !actor_x+2
    rts

; @routine 83:F225 BackdropPictureStep
; WHAT IT DOES: Plays the actor's picture on by a frame of time: when the
; picture's frame has run its time, the next one (after the last, the
; first) is asked for as a fixed VRAM upload.
; HOW: The time steps down; below 0, the offset steps on by 2 (back to 0
; when the frame left was the last) and the new frame's time (low 7 bits)
; and upload number are taken.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB the table's bank,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
BackdropPictureStep:
    dec.b !PICTURE_TIME
    bpl .done
    ldy.b !PICTURE_AT
    iny
    iny
    lda.w !PICTURE_FRAMES-1,y
    bpl .take
    ldy.b #$00
.take:
    lda.w !PICTURE_FRAMES+1,y
    and.b #$7F
    sta.b !PICTURE_TIME
    lda.w !PICTURE_FRAMES,y
    sta.w !vram_upload_number
    sty.b !PICTURE_AT
.done:
    rts

; @routine 83:F244 BackdropActorShow
; WHAT IT DOES: Shows the backdrop, with the actor's picture on it, while
; the actor is within 256 pixels of the range point on both axes: the
; backdrop is scrolled to put its point (128, 128) where the actor is on
; the screen. Farther away the backdrop is not shown.
; HOW: With 16-bit A, each distance plus 256 compared unsigned with 512;
; near: the backdrop's scroll is the play field's plus 128 less the
; actor's pixels, on each axis, and TM's shadow is $17; else $15.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM. Exit: as the C states.
BackdropActorShow:
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.w !RANGE_X
    clc
    adc.w #$0100
    cmp.w #$0200
    bcs .far
    sec
    lda.b !actor_y+1
    sbc.w !RANGE_Y
    clc
    adc.w #$0100
    cmp.w #$0200
    bcs .far
    sec
    lda.b !actor_x+1
    sbc.w !layer0+!LAYER_SCROLL_X
    sec
    sbc.w #$0080
    eor.w #$FFFF
    inc a
    sta.w !layer1+!LAYER_SCROLL_X
    sec
    lda.b !actor_y+1
    sbc.w !layer0+!LAYER_SCROLL_Y
    sec
    sbc.w #$0080
    eor.w #$FFFF
    inc a
    sta.w !layer1+!LAYER_SCROLL_Y
    sep.b #$20
    lda.b #$17
    sta.w !TM_SHADOW
    rts
.far:
    sep.b #$20
    lda.b #$15
    sta.w !TM_SHADOW
    rts
