; kind_steps.asm: small per-frame steps of some actor kinds (animate,
; follow a path), and the path an actor can follow.
; C: src/aladdin/actor/kind_steps.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram.inc"

arch 65816

; The path an actor follows (its record, the direct page).
!path_velocity = $48             ; the segment's velocity (word), added to $42
!path_pos = $42                  ; the position the path moves (word)
!path_frames = $47               ; frames left in the segment's step
!path_step_frames = $46          ; frames a step lasts
!path_step_frames0 = $45         ; the steps' frames, reloaded at a segment
!path_at = $4A                   ; the next segment (word)
!path_segments = $4D             ; steps left in the segment
!path_on = $4C                   ; nonzero: the actor follows its path
!path_done = $18                 ; set to 1 at the path's end

; @routine 84:9387 KindAnimateFreeOff
; WHAT IT DOES: One frame of a kind that only animates, freed once it is
; far out of range.
; HOW: AnimStep, then ActorFreeOutOfRangeLong with range $18.
; Entry: M 8-bit, X 8-bit; D = the actor; called by JSL. Exit: as
; ActorFreeOutOfRangeLong.
%at($849387)
KindAnimateFreeOff:
    jsl.l AnimStep
    ldx.b #$18
    jml.l ActorFreeOutOfRangeLong

; @routine 84:9533 KindAnimate9533
; WHAT IT DOES: One frame of an actor kind that only animates.
; HOW: AnimStep.
; Entry: D = the actor; called by JSL. Exit: as AnimStep.
%at($849533)
KindAnimate9533:
    jsl.l AnimStep
    rtl

; @routine 84:9628 KindAnimate9628
; WHAT IT DOES: One frame of an actor kind that only animates.
; HOW: AnimStep.
; Entry: D = the actor; called by JSL. Exit: as AnimStep.
%at($849628)
KindAnimate9628:
    jsl.l AnimStep
    rtl

; @routine 84:9EF0 KindAnimate9EF0
; WHAT IT DOES: One frame of an actor kind that only animates.
; HOW: AnimStep.
; Entry: D = the actor; called by JSL. Exit: as AnimStep.
%at($849EF0)
KindAnimate9EF0:
    jsl.l AnimStep
    rtl

; @routine 84:9F4A KindAnimate9F4A
; WHAT IT DOES: One frame of an actor kind that only animates.
; HOW: AnimStep.
; Entry: D = the actor; called by JSL. Exit: as AnimStep.
%at($849F4A)
KindAnimate9F4A:
    jsl.l AnimStep
    rtl

; @routine 84:903B KindAnimateSend
; WHAT IT DOES: One frame of an actor kind that animates its own picture.
; HOW: AnimStep, then ActorPictureSend.
; Entry: D = the actor; called by JSL. Exit: as ActorPictureSend.
%at($84903B)
KindAnimateSend:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    rtl

; @routine 84:8CAF ActorPathStep
; WHAT IT DOES: One step of the actor along its path: its position moves
; by the segment's velocity, and the path goes on when the step's frames
; run out.
; HOW: ActorParityBitsSet; the velocity added to the position; when the
; path is on, ActorPathNext.
; Entry: M 8-bit; D = the actor; called by JSR. Exit: M 8-bit; as
; ActorPathNext when the path is on.
%at($848CAF)
ActorPathStep:
    jsr.w ActorParityBitsSet
    rep.b #$20
    clc
    lda.b !path_velocity
    adc.b !path_pos
    sta.b !path_pos
    sep.b #$20
    lda.b !path_on
    beq .l8CC4
    jsr.w ActorPathNext
.l8CC4:
    rts


; @routine 84:8DFB ActorPathNext
; WHAT IT DOES: Counts down the path's step; at its end takes the next
; step, the next segment when the segment's steps run out, and marks the
; path done at its end.
; HOW: The frames count down; at 0: the velocity from the segment
; (XORed with the speed word $19, plus $1C), the frames reloaded;
; the segment's steps count down; at 0 the next segment's address from
; the list: zero ends the path ($18 = 1), else its step count and frames.
; Entry: M 8-bit; D = the actor; called by JSR. Exit: M and X 8-bit.
%at($848DFB)
ActorPathNext:
    lda.b !path_frames
    beq .l8E32
    dec.b !path_frames
    bne .l8E32
    rep.b #$10
    ldy.b !path_at
    rep.b #$20
    lda.w 0,y
    eor.b !actor_speed_x
    clc
    adc.b !actor_accel_x
    sta.b !path_velocity
    sep.b #$20
    lda.b !path_step_frames
    sta.b !path_frames
    iny
    iny
    dec.b !path_segments
    bne .l8E2E
    lda.b !path_step_frames0
    sta.b !path_step_frames
    ldx.w 0,y
    beq .l8E33
    lda.w !sound_seq,y
    sta.b !path_segments
    txy
.l8E2E:
    sty.b !path_at
    sep.b #$30
.l8E32:
    rts
.l8E33:
    stz.b !path_frames
    lda.b #$01
    sta.b !path_done
    sep.b #$30
    rts
