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
incsrc "../include/wram_video.inc"

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

; A task no code creates: it runs only where cov_unused_tasks.box creates
; it.
!WOBBLES = $BA76                 ; (bank $80) per wobble, 4 bytes: x step, y step, frames a swing, frames in all
!WOBBLE_DX = $07                 ; (its record) the x step (word)
!WOBBLE_DY = $09                 ; the y step (word)
!WOBBLE_SWING = $0B              ; frames a swing
!WOBBLE_LEFT = $0C               ; frames left of this swing
!WOBBLE_ALL = $0D                ; frames left in all

; @routine 81:9F98 Bg1WobbleTask
; WHAT IT DOES: A task that wobbles layer 1: its x and y shifts move by a
; step each frame, turning back after each swing, for the frames the
; wobble its parameter picks lasts; then the shifts are 0 again and it
; ends. No code creates it.
; HOW: D = its slot's record ($6F + Y, the old D pushed); from WOBBLES at
; the parameter: the x step to $07 (word), the y step to $09 (word), the
; frames a swing to $0B and $0C, the frames in all to $0D. Each frame:
; TaskWait 1, BG1_X_SHIFT and BG1_Y_SHIFT plus the steps; the frames in
; all down (at 0 the shifts cleared, D pulled, TaskExit); the swing's
; frames down, and at 0 the steps negated and the swing begun again.
; Entry: the task's start: M 8-bit, X 8-bit; Y = the task's slot; its
; parameter = the wobble's offset in WOBBLES; DB = $80. Exit: none: the
; task ends.
%at($819F98)
Bg1WobbleTask:
    rep.b #$20
    phd
    clc
    tya
    adc.w #!task_state
    tcd
    sep.b #$20
    ldx.b !task_param-!task_state
    lda.w !WOBBLES,x
    sta.b !WOBBLE_DX
    stz.b !WOBBLE_DX+1
    lda.w !WOBBLES+1,x
    sta.b !WOBBLE_DY
    stz.b !WOBBLE_DY+1
    lda.w !WOBBLES+2,x
    sta.b !WOBBLE_SWING
    lda.w !WOBBLES+3,x
    sta.b !WOBBLE_ALL
.swing:
    lda.b !WOBBLE_SWING
    sta.b !WOBBLE_LEFT
.frame:
    lda.b #$01
    jsl.l TaskWait
    rep.b #$20
    clc
    lda.b !WOBBLE_DX
    adc.w !bg1_x_shift
    sta.w !bg1_x_shift
    clc
    lda.b !WOBBLE_DY
    adc.w !bg1_x_shift+4
    sta.w !bg1_x_shift+4
    sep.b #$20
    dec.b !WOBBLE_ALL
    beq .done
    dec.b !WOBBLE_LEFT
    bne .frame
    rep.b #$20
    lda.b !WOBBLE_DX
    eor.w #$FFFF
    inc a
    sta.b !WOBBLE_DX
    lda.b !WOBBLE_DY
    eor.w #$FFFF
    inc a
    sta.b !WOBBLE_DY
    sep.b #$20
    bra .swing
.done:
    stz.w !bg1_x_shift
    stz.w !bg1_x_shift+1
    stz.w !bg1_x_shift+4
    stz.w !bg1_x_shift+5
    pld
    jml.l TaskExit

