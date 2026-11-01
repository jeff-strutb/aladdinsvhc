; actor_steps.asm: small steps of some actors' code: rising after a
; delay, a height that follows the animation's frame, a flag each of eight
; actors sets once, and a test picked by X.
; C: src/aladdin/actor/actor_steps.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

; In bank $80 (DB): the y change for each frame of the animation (words),
; by !ACTOR_FRAME.
!FRAME_Y_STEPS = $D673
; In bank $80 (DB): a bit mask for each of 8 (by the low 3 bits of $07).
!BIT_MASKS = $D316
!V_0359 = $0359                 ; eight flags, set by the actors that use these
; Jump tables in the code's bank: 4 cases by bits 3-4 of $07; the tests
; picked by X.
!FLAG_CASES = $CF29
!TESTS_BY_X = $FBA9

; @routine 83:A17A ActorRiseAfterDelay
; WHAT IT DOES: Counts its delay ($47, a word) down; once it is out, the
; actor moves up by its vertical speed, and the speed grows by its
; gravity while its whole part is below 2.
; HOW: DEC on the word while it is not 0; else ActorStepYBack, then
; ActorAccelerateY when the speed's whole low byte is below 2.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($83A17A)
ActorRiseAfterDelay:
    lda.b !actor_47
    ora.b !actor_48
    beq .rise
    rep.b #$20
    dec.b !actor_47
    sep.b #$20
    rts
.rise:
    jsl.l ActorStepYBack
    lda.b !actor_speed_y+1
    cmp.b #$02
    bcs .done
    jsl.l ActorAccelerateY
.done:
    rts

; @routine 83:CF10 ActorFlagOnce
; WHAT IT DOES: Each of eight such actors (by the low 3 bits of $07) has a
; flag in $0359: when its flag is set already, it is set again and back;
; else the case by bits 3-4 of $07 runs (from the table at $83:CF29).
; HOW: The flag's mask (bank $80, $D316) goes in $41 and is tested
; against $0359; set: TSB; clear: JMP through the case table.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR. Exit: as the C states.
%at($83CF10)
ActorFlagOnce:
    lda.b !actor_07
    and.b #$07
    tax
    lda.w !BIT_MASKS,x
    sta.b !actor_41
    and.w !V_0359
    bne .set
    lda.b !actor_07
    and.b #$18
    lsr a
    lsr a
    tax
    jmp.w (!FLAG_CASES,x)
; (the case table and the first cases, not yet filed, lie between)
%at($83CF40)
.set:
    bra .mark
%at($83CF61)
.mark:
    lda.b !actor_41
    tsb.w !V_0359
    rts

; @routine 83:F031 ActorYByFrame
; WHAT IT DOES: When the actor's animation frame has changed, moves it up
; or down by that frame's step, so its height follows the animation.
; HOW: The frame is compared with the one kept in $49; when new, the
; word of the frame-steps table (bank $80, $D673) by it is added to the
; y pixels; then $49 = the frame.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($83F031)
ActorYByFrame:
    ldy.b !ACTOR_FRAME
    cpy.b !actor_49
    beq .keep
    tya
    asl a
    tax
    rep.b #$20
    clc
    lda.b !actor_y+1
    adc.w !FRAME_Y_STEPS,x
    sta.b !actor_y+1
    sep.b #$20
.keep:
    sty.b !actor_49
    rts

; @routine 83:FBA6 ActorTestByX
; WHAT IT DOES: Runs the actor test X picks, from the table at $83:FBA9,
; for callers in any bank.
; HOW: JMP (table,X); the tests end in RTL.
; Entry: M 8-bit, X 8-bit; X = the test's offset (0 or 2); called by JSL.
; Exit: as the C states.
%at($83FBA6)
ActorTestByX:
    jmp.w (!TESTS_BY_X,x)
