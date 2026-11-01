; frame_steps.asm: a task's frame end: the frame's update committed, then a
; frame's wait; once or X times.
; C: src/aladdin/system/frame_steps.c.

incsrc "../include/macros.inc"

arch 65816

; Not yet filed: the frame's update (the player's position kept, the
; sprites and blocks for the NMI).
!FRAME_UPDATE = $81830D

; @routine 80:AE9A FrameEnd
; WHAT IT DOES: Ends a task's frame: the frame's update, then the wait
; for the next frame.
; HOW: The update at $81:830D, then TaskWait with A = 1.
; Entry: M 8-bit, X 8-bit; called by JSR from a task; DB a bank whose
; $0000-$1FFF is work RAM. Exit: as the C states.
%at($80AE9A)
FrameEnd:
    jsl.l !FRAME_UPDATE
    lda.b #$01
    jsl.l TaskWait
    rts

; @routine 80:B2D3 FrameEndTimes
; WHAT IT DOES: Lets X frames go by, with the frame's update after each.
; HOW: X kept on the stack across a frame's wait (TaskWait, A = 1) and
; the update at $81:830D, then counted down.
; Entry: M 8-bit, X 8-bit; X = the frames (0: 256); called by JSR from a
; task; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($80B2D3)
FrameEndTimes:
    phx
    lda.b #$01
    jsl.l TaskWait
    jsl.l !FRAME_UPDATE
    plx
    dex
    bne FrameEndTimes
    rts
