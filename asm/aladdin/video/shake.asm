; shake.asm: shaking a layer up and down.
;
; A shake task runs with its slot as the direct page. Its record holds the
; layer (its byte offset in the layer offsets), how many sways, the frames
; each half-sway lasts and the pixels it moves each frame. Each frame of a
; half-sway moves the layer's vertical offset by the step and waits one
; frame.
; C: src/aladdin/video/shake.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; The shake task's record, from its slot (the direct page).
!shake_layer = $75              ; the layer's byte offset in layer_offset_y
!shake_sways = $76              ; sways left
!shake_frames = $77             ; frames in a half-sway
!shake_step = $78               ; pixels a frame
; Per shake, 4 bytes: the layer, sways, frames and step.
!shake_table = $BAA3

; @routine 81:A1D5 ShakeTask
; WHAT IT DOES: A task that shakes a layer up and down: the shake its
; parameter picks, then it ends.
; HOW: Its slot becomes the direct page; the parameter (a byte offset in
; the shake table at $BAA3) is replaced by the shake's four bytes: the
; layer, the sways, the frames of a half-sway and the step; then
; ShakeUp and ShakeDown for each sway; then TaskExit.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; DB =
; $81. Exit: none: the task ends.
%at($81A1D5)
ShakeTask:
    lda.b #0
    xba
    tya
    tcd
    ldx.b !shake_layer
    lda.w !shake_table,x
    sta.b !shake_layer
    lda.w !shake_table+1,x
    sta.b !shake_sways
    lda.w !shake_table+2,x
    sta.b !shake_frames
    lda.w !shake_table+3,x
    sta.b !shake_step
.sway:
    jsr.w ShakeUp
    jsr.w ShakeDown
    dec.b !shake_sways
    bne .sway
    jml.l TaskExit

; @routine 81:A1FE ShakeUp
; WHAT IT DOES: The upward half of a sway: the layer moves up by the step
; each frame for the half-sway's frames.
; HOW: Each frame takes the step from the layer's 16-bit vertical offset
; and waits one frame.
; Entry: M 8-bit, X 8-bit; D = the shake task's slot; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from the task. Exit: Y = 0; Z set,
; N clear; X = the layer; A, D, DB and the other flags as the last wait
; restores them.
ShakeUp:
    ldx.b !shake_layer
    ldy.b !shake_frames
.frame:
    sec
    lda.w !layer_offset_y,x
    sbc.b !shake_step
    sta.w !layer_offset_y,x
    lda.w !layer_offset_y+1,x
    sbc.b #0
    sta.w !layer_offset_y+1,x
    lda.b #1
    jsl.l TaskWait
    dey
    bne .frame
    rts

; @routine 81:A21D ShakeDown
; WHAT IT DOES: The downward half of a sway: the layer moves down by the
; step each frame for the half-sway's frames.
; HOW: Each frame adds the step to the layer's 16-bit vertical offset and
; waits one frame.
; Entry: M 8-bit, X 8-bit; D = the shake task's slot; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from the task. Exit: Y = 0; Z set,
; N clear; X = the layer; A, D, DB and the other flags as the last wait
; restores them.
ShakeDown:
    ldx.b !shake_layer
    ldy.b !shake_frames
.frame:
    clc
    lda.w !layer_offset_y,x
    adc.b !shake_step
    sta.w !layer_offset_y,x
    lda.w !layer_offset_y+1,x
    adc.b #0
    sta.w !layer_offset_y+1,x
    lda.b #1
    jsl.l TaskWait
    dey
    bne .frame
    rts
