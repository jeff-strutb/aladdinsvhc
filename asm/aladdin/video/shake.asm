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
!shake_frames = $77             ; frames in a half-sway
!shake_step = $78               ; pixels a frame

; @routine 81:A1FE ShakeUp
; WHAT IT DOES: The upward half of a sway: the layer moves up by the step
; each frame for the half-sway's frames.
; HOW: Each frame takes the step from the layer's 16-bit vertical offset
; and waits one frame.
; Entry: M 8-bit, X 8-bit; D = the shake task's slot; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from the task. Exit: Y = 0; Z set,
; N clear; X = the layer; A, D, DB and the other flags as the last wait
; restores them.
%at($81A1FE)
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
