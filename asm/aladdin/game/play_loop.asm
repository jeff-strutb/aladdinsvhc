; play_loop.asm: the game task's play loop: a frame of play, the frame's
; checks, the wait for the next frame; a demo ends on Start or its time.
; C: src/aladdin/game/play_loop.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"

arch 65816

!V_03FA = $03FA                 ; counted up each time play starts; purpose not yet established
!WAIT_FRAMES = $03FB            ; nonzero: the frames each turn of the loop waits (else 1)
!DEMO_PRESSED_HIGH = $0382      ; while a demo plays: the presses it stops on, high byte (bit 4: Start)
!DEMO_FRAMES_LEFT = $0341       ; frames the demo still plays (word)

; @routine 81:8191 PlayLoopStart
; WHAT IT DOES: The game task's play loop from its start: the level
; loaded, the fade-in task started, then the loop.
; HOW: LevelLoad; TaskCreate (slot 1, parameter 4, BrightnessUpTask);
; V_03FA counted up; on into PlayLoop.
; Entry: M 8-bit, X 8-bit; the task's start (TaskExec). Exit: never
; returns.
%at($818191)
PlayLoopStart:
    jsr.w LevelLoad
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl BrightnessUpTask
    inc.w !V_03FA
    jmp.w PlayLoop

; @routine 81:82CF PlayLoop
; WHAT IT DOES: The play loop: each turn a frame of play and the frame's
; checks, then the wait for the next frame (and on while the VRAM queue
; is still waiting); a demo ends when Start is pressed or its frames run
; out, and the title screen comes back.
; HOW: FrameStep, RestartPointCheck, LevelExitCheck, HudRefresh,
; PauseCheck; TaskWait with WAIT_FRAMES (1 when 0), again while
; VRAM_QUEUE_PENDING is set. In a demo (PLAY_MODE 2): bit 4 of
; DEMO_PRESSED_HIGH, or DEMO_FRAMES_LEFT counted down to 0, ends it:
; TaskExec TitleAfterFadeOut.
; Entry: M 8-bit, X 8-bit; reached by JMP from PlayLoopStart (the game
; task). Exit: never returns.
%at($8182CF)
PlayLoop:
    jsr.w FrameStep
    jsr.w RestartPointCheck
    jsr.w LevelExitCheck
    jsr.w HudRefresh
    jsr.w PauseCheck
.wait:
    lda.b #$01
    ldx.w !WAIT_FRAMES
    beq .frames
    txa
.frames:
    jsl.l TaskWait
    ldy.w !vram_queue_pending
    bne .wait
    lda.w !play_mode
    cmp.b #$02
    bne PlayLoop
    lda.w !DEMO_PRESSED_HIGH
    bit.b #$10
    bne .demo_end
    rep.b #$20
    dec.w !DEMO_FRAMES_LEFT
    sep.b #$20
    bne PlayLoop
.demo_end:
    jsl.l TaskExec
    dl TitleAfterFadeOut
