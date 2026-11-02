; actor_y_steps.asm: actors' heights stepped from tables in bank $80 (the
; bank DB names while actors run): by the animation frame, and the bob of
; the actor whose code is at $83:C363 (up 4 pixels a frame for 14 frames,
; back 1 a frame for 56, or the other way by its byte at $07).
; C: src/aladdin/actor/actor_y_steps.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

; In the bank DB names: a word per animation frame, added to the y pixels
; when the frame changes (the actor at $83:BE01's).
!FRAME_Y_STEPS_47 = $D1CE
; 4 bytes an entry, by the actor's byte at $07 (0 or 1), the return's
; entries 2 on: a word added to the y pixels, then a byte added (with the
; carry) to their high byte.
!BOB_STEPS = $D286

; @routine 83:BDE9 ActorYByFrame47
; WHAT IT DOES: When the actor's animation frame has changed, moves it up
; or down by that frame's step, so its height follows the animation.
; HOW: The frame is compared with the one kept in $47; when new, the
; word of the frame-steps table (bank $80, $D1CE) by it is added to the
; y pixels; then $47 = the frame.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($83BDE9)
ActorYByFrame47:
    lda.b !ACTOR_FRAME
    cmp.b !actor_47
    beq .keep
    asl a
    tax
    rep.b #$21
    lda.w !FRAME_Y_STEPS_47,x
    adc.b !actor_y+1
    sta.b !actor_y+1
    sep.b #$20
    lda.b !ACTOR_FRAME
.keep:
    sta.b !actor_47
    rts

; @routine 83:C3C5 ActorBobReturnStep
; WHAT IT DOES: One frame of the bob's return: moves the actor by the
; step of entry 2 or 3 (by its byte at $07) of the bob's table.
; HOW: $07 plus 2, then on into ActorBobStep past its LDA.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($83C3C5)
ActorBobReturnStep:
    lda.b !actor_07
    inc a
    inc a
    bra ActorBobStepByA

; @routine 83:C3CB ActorBobStep
; WHAT IT DOES: One frame of the bob's first move: moves the actor by the
; step of entry 0 or 1 (by its byte at $07) of the bob's table.
; HOW: The entry's word is added to the y pixels (16-bit A), then its
; third byte, with the carry, to their high byte.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
ActorBobStep:
    lda.b !actor_07
ActorBobStepByA:
    asl a
    asl a
    tax
    rep.b #$20
    clc
    lda.b !actor_y+1
    adc.w !BOB_STEPS,x
    sta.b !actor_y+1
    sep.b #$20
    lda.b !actor_y+2
    adc.w !BOB_STEPS+2,x
    sta.b !actor_y+2
    rts
