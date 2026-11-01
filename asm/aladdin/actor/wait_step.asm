; wait_step.asm: what some kinds of actor do on each frame they wait. The
; actor loop calls a kind's routine (by JSR through the kind's table at
; $83:8F4F or $83:904F) on every frame an actor of it waits, when bit 7
; of its $09 is set: most play its animation on and end the actor once it
; is out of the camera's range.
; C: src/aladdin/actor/wait_step.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!SUB_81949E = $81949E
!SUB_8383C9 = $8383C9
!SUB_838135 = $838135
!SUB_83840F = $83840F
!SUB_8381E8 = $8381E8
!SUB_83865E = $83865E
!SUB_83848D = $83848D
!SUB_83FBDA = $83FBDA
!SUB_83FB89 = $83FB89
!SUB_82BB26 = $82BB26
!V_1428 = $1428
!V_141C = $141C
!V_032D = $032D
; The tail that ends an actor once it is left behind: when the word at
; $1773 less 31 is not negative and not below its x pixel (else back with
; M 8-bit). Not yet filed.
!END_IF_LEFT_BEHIND = $839738
; In bank $80 (DB): sound commands by the actor's x pixel, low 4 bits.
!STEP_SOUNDS = $D3FC
!FLOOR_Y = $B0                  ; the lowest the player is held at

; @routine 83:9C06 ActorWaitTurnAnim08
; WHAT IT DOES: Each frame it waits: the tests at $83:83C9 and $83:8135,
; its animation on, the update at $81:949E, and it ends once out of range
; (box 8).
; HOW: Calls them in turn, then on into ActorEndOutOfRange with X = 8.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($839C06)
ActorWaitTurnAnim08:
    jsr.w !SUB_8383C9
    jsr.w !SUB_838135
    jsl.l AnimStep
    jsl.l !SUB_81949E
    ldx.b #$08
    jmp.w ActorEndOutOfRange

; @routine 83:9E0A ActorWaitHoldPlayer
; WHAT IT DOES: Each frame it waits: its animation on; then, with $42 set,
; the player falling to or below y $B0 is held there ($1428 = 1, $141C =
; 0); else, with $44 set, the call at $82:BB26 with A = 5.
; HOW: The player's y pixel (word) is compared unsigned with $B0 by its
; low byte, and his vertical speed's sign tested, before y is set to $B0.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($839E0A)
ActorWaitHoldPlayer:
    jsl.l AnimStep
    lda.b !actor_42
    beq .not_holding
    lda.w !player+!actor_y+1
    cmp.b #!FLOOR_Y
    bcc .done
    lda.w !player+!actor_speed_y+2
    bmi .done
    lda.b #!FLOOR_Y
    sta.w !player+!actor_y+1
    stz.w !player+!actor_y+2
    lda.b #$01
    sta.w !V_1428
    stz.w !V_141C
.done:
    rts
.not_holding:
    lda.b !actor_44
    beq .out
    lda.b #$05
    jsl.l !SUB_82BB26
.out:
    rts

; @routine 83:B0BB ActorWaitAnimIf07Bit4
; WHAT IT DOES: Each frame it waits, its animation plays on when bit 4 of
; its $07 is set.
; HOW: BIT #$10 on $07, then AnimStep.
; Entry: M 8-bit, X 8-bit; D = the actor; called by JSR from the actor
; loop. Exit: as the C states.
%at($83B0BB)
ActorWaitAnimIf07Bit4:
    lda.b !actor_07
    bit.b #$10
    bne .anim
    rts
.anim:
    jsl.l AnimStep
    rts

; @routine 83:B4B9 ActorWaitAnim
; WHAT IT DOES: Each frame it waits, its animation plays on.
; HOW: AnimStep.
; Entry: M 8-bit, X 8-bit; D = the actor; called by JSR from the actor
; loop. Exit: as the C states.
%at($83B4B9)
ActorWaitAnim:
    jsl.l AnimStep
    rts

; @routine 83:BD0B ActorWaitAnimEnd10
; WHAT IT DOES: Each frame it waits: its animation on; then, where
; LEVEL_035C is 6, it is ended once left behind (the tail at $83:9738);
; elsewhere once out of range (box $10).
; HOW: AnimStep, the compare, then a jump to either.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83BD0B)
ActorWaitAnimEnd10:
    jsl.l AnimStep
    lda.w !level_035c
    cmp.b #$06
    bne .range
    jmp.w !END_IF_LEFT_BEHIND
.range:
    ldx.b #$10
    jmp.w ActorEndOutOfRange

; @routine 83:CF8C ActorWaitTest42Anim
; WHAT IT DOES: Each frame it waits: while $42 is 0, the tests at
; $83:840F (true: $42 up by 1) and $83:81E8 (C clear: up by 2); then its
; animation on, and it ends once out of range (box $10).
; HOW: The tests, AnimStep, then on into ActorEndOutOfRange with X = $10.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83CF8C)
ActorWaitTest42Anim:
    lda.b !actor_42
    bne .anim
    jsr.w !SUB_83840F
    beq .second
    inc.b !actor_42
.second:
    jsr.w !SUB_8381E8
    bcs .anim
    inc.b !actor_42
    inc.b !actor_42
.anim:
    jsl.l AnimStep
    ldx.b #$10
    jmp.w ActorEndOutOfRange

; @routine 83:D0EF ActorWaitTest42Anim2
; WHAT IT DOES: Each frame it waits: while $42 is 0, the test at $83:840F
; (true: $42 up by 1); the call at $83:865E with Y = $20; its animation
; on, and it ends once out of range (box $10).
; HOW: In turn, then on into ActorEndOutOfRange with X = $10.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83D0EF)
ActorWaitTest42Anim2:
    lda.b !actor_42
    bne .call
    jsr.w !SUB_83840F
    beq .call
    inc.b !actor_42
.call:
    ldy.b #$20
    jsr.w !SUB_83865E
    jsl.l AnimStep
    ldx.b #$10
    jmp.w ActorEndOutOfRange

; @routine 83:D2A2 ActorWaitWake
; WHAT IT DOES: Each frame it waits: when bit 1 of its $07 is clear, $41
; is set and the test at $83:848D is true, it wakes: state bits $8C set,
; bit 7 of $09 (the wait routine) cleared. It ends once out of range (box
; $10).
; HOW: TSB/TRB on its fields, then on into ActorEndOutOfRange with X =
; $10.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83D2A2)
ActorWaitWake:
    lda.b !actor_07
    and.b #$02
    bne .range
    lda.b !actor_41
    beq .range
    jsr.w !SUB_83848D
    beq .range
    lda.b #$8C
    tsb.b !actor_state
    lda.b #$80
    trb.b !actor_09
.range:
    ldx.b #$10
    jmp.w ActorEndOutOfRange

; @routine 83:D8CE ActorWaitHurtOnTouch
; WHAT IT DOES: Each frame it waits: its animation on; unless bit 3 of
; its $07 is set, it hurts the player on touch; it is ended once left
; behind.
; HOW: AnimStep, ActorHurtsPlayerOnTouch, then on into the tail at
; $83:9738.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83D8CE)
ActorWaitHurtOnTouch:
    jsl.l AnimStep
    lda.b !actor_07
    and.b #$08
    bne .tail
    jsr.w ActorHurtsPlayerOnTouch
.tail:
    jmp.w !END_IF_LEFT_BEHIND

; @routine 83:DFCD ActorWaitStepSounds
; WHAT IT DOES: Each frame it waits: its animation on. With bit 7 of $07
; set: while $42 is set, every 8th count of $032D a sound by its x (the
; table at $80:D3FC); it hurts the player on touch. Bit 7 clear: the
; calls at $83:FBDA and $83:FB89 (X = 2, $4C = $3C). It is ended once left
; behind.
; HOW: AnimStep, the cases, then on into the tail at $83:9738.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83DFCD)
ActorWaitStepSounds:
    jsl.l AnimStep
    lda.b !actor_07
    bpl .other
    lda.b !actor_42
    beq .touch
    lda.w !V_032D
    and.b #$07
    bne .touch
    lda.b !actor_x+1
    and.b #$0F
    tax
    lda.w !STEP_SOUNDS,x
    jsl.l SoundQueueAdd
.touch:
    jsr.w ActorHurtsPlayerOnTouch
    bra .tail
.other:
    jsr.w !SUB_83FBDA
    ldy.b #$3C
    sty.b !actor_4c
    ldx.b #$02
    jsr.w !SUB_83FB89
.tail:
    jmp.w !END_IF_LEFT_BEHIND

; @routine 83:E458 ActorWaitAnimRange10
; WHAT IT DOES: Each frame it waits: its animation on, and it ends once
; out of range (box $10).
; HOW: AnimStep, then on into ActorEndOutOfRange with X = $10.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83E458)
ActorWaitAnimRange10:
    jsl.l AnimStep
    ldx.b #$10
    jmp.w ActorEndOutOfRange

; @routine 83:E4E7 ActorWaitAnimB
; WHAT IT DOES: Each frame it waits, its animation plays on (a second
; copy of ActorWaitAnim, for other kinds).
; HOW: AnimStep.
; Entry: M 8-bit, X 8-bit; D = the actor; called by JSR from the actor
; loop. Exit: as the C states.
%at($83E4E7)
ActorWaitAnimB:
    jsl.l AnimStep
    rts

; @routine 83:E5B6 ActorWaitRange00
; WHAT IT DOES: Each frame it waits, it ends once out of range (box 0).
; HOW: X = 0, then on into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83E5B6)
ActorWaitRange00:
    ldx.b #$00
    jmp.w ActorEndOutOfRange

; @routine 83:F487 ActorWaitAnimRange18
; WHAT IT DOES: Each frame it waits: its animation on, the update at
; $81:949E, and it ends once out of range (box $18).
; HOW: AnimStep, the call, then on into ActorEndOutOfRange with X = $18.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop. Exit: as the C states.
%at($83F487)
ActorWaitAnimRange18:
    jsl.l AnimStep
    jsl.l !SUB_81949E
    ldx.b #$18
    jmp.w ActorEndOutOfRange
