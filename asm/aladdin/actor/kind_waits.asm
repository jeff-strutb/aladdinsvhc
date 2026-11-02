; kind_waits.asm: the routines some actor kinds run each frame they wait (by the kind
; table at $83:8F4F, or a kind's stub): animation, shots, touches and
; their range. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind_waits.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; Not yet filed: a test of a shot reaching the actor (83:81CF), and a
; step of kind $6E's actor (83:F908).
!SHOT_TEST_81CF = $8381CF
!STEP_F908 = $83F908
; In bank $80: the animation frames on which kind $52 hurts, and the
; zone (box) for each.
!KIND52_HURT_FRAMES = $D382
!KIND52_HURT_ZONES = $D387
!v_0367 = $0367
!v_036c = $036C
!v_1a2c = $1A2C                 ; kind $67's zone is $C0 above it
!TEST_BOX_Y = $1486
!TEST_BOX_HALF_H = $148A
!ACTOR_27 = $27                 ; the actor's animation frame
!ACTOR_43 = $43
!ACTOR_4E = $4E                 ; kind $53: the record it watches (word)

; @routine 83:E645 ActorCount42Down
; WHAT IT DOES: Counts the actor's word at $42 down by 1, not below 0.
; HOW: A 16-bit subtraction; negative becomes 0.
; Entry: M 16-bit; D = the actor's record; called by JSR (83:E60C,
; 83:E633). Exit: M 16-bit; A = the new count; C from the subtraction.
%at($83E645)
ActorCount42Down:
    sec
    lda.b !actor_42
    sbc.w #$0001
    bpl .lE650
    lda.w #$0000
.lE650:
    sta.b !actor_42
    rts


; @routine 83:A012 Kind27Wait
; WHAT IT DOES: Kind $27's wait: animates; once its $42 is set, touching
; the player it plays sound $4F, counts $036C up and ends; it ends at once
; when $0367 is 0, or when out of range.
; HOW: AnimStep; $42 clear returns. $0367 zero: ActorEndOutOfRange with
; box $58. Else ActorHitsTestBoxLong with box $24: touched, SoundQueueAdd
; $4F, $036C up, and into ActorEndOutOfRange past its range test (its
; return dropped, freed); not touched, ActorEndOutOfRange with box $58.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Its $42 is set only by its untraced start for $07 bit 3 (83:9E58). Exit:
; $42 clear: as AnimStep. Else in range: as ActorEndOutOfRange; ended:
; does not return.
%at($83A012)
Kind27Wait:
    jsl.l AnimStep
    lda.b !actor_42
    bne .lA01B
    rts
.lA01B:
    lda.w !v_0367
    beq .lA028
    ldy.b #$24
    jsl.l ActorHitsTestBoxLong
    bcc .lA02D
.lA028:
    ldx.b #$58
    jmp.w ActorEndOutOfRange
.lA02D:
    lda.b #$4F
    jsl.l SoundQueueAdd
    inc.w !v_036c
    jml.l ActorEndOutOfRange_end


; @routine 83:AEEC Kind31Wait
; WHAT IT DOES: Kind $31's wait: with its $42 set it turns round at x
; $0F00 and beyond in levels below 7, and may be downed by a shot; it
; animates and is ended when out of range.
; HOW: $42 set: level below 7 and x pixels $0F00 or more, facing and flip
; toggled; ActorDownedByShot, downed: ActorStompOrHurt. Then AnimStep and
; ActorEndOutOfRange with box $38 (by JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Called by JSR: it ends by JML to ActorEndOutOfRange, which returns to
; the loop. Exit: as ActorEndOutOfRange.
%at($83AEEC)
Kind31Wait:
    lda.b !actor_42
    beq .lAF12
    lda.w !level
    cmp.b #$07
    bcs .lAF0A
    rep.b #$20
    lda.b !actor_x+1
    cmp.w #$0F00
    sep.b #$20
    bcc .lAF0A
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.lAF0A:
    jsr.w ActorDownedByShot
    bcc .lAF12
    jsr.w ActorStompOrHurt
.lAF12:
    jsl.l AnimStep
    ldx.b #$38
    jml.l ActorEndOutOfRange


; @routine 83:B94A Kind30Wait
; WHAT IT DOES: Kind $30's wait: animates; variant 7 (low three bits of
; $07) notes the first shot that reaches it ($43); ended when out of
; range.
; HOW: AnimStep; variant 7 with $43 clear: ActorTakesShot, hit: $43 up.
; Then ActorEndOutOfRange with box $18.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: as ActorEndOutOfRange.
%at($83B94A)
Kind30Wait:
    jsl.l AnimStep
    lda.b !actor_07
    and.b #$07
    cmp.b #$07
    bne .lB961
    lda.b !ACTOR_43
    bne .lB961
    jsr.w ActorTakesShot
    bcs .lB961
    inc.b !ACTOR_43
.lB961:
    ldx.b #$18
    jmp.w ActorEndOutOfRange


; @routine 83:C94A Kind46Wait
; WHAT IT DOES: Kind $46's wait: a shot it stops sets its $43; with $07
; bit 1 it is ended when out of range.
; HOW: ActorStopsShot: stopped (C clear), $43 = 1. $07 bit 1:
; ActorEndOutOfRange with box 0; else returns.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: M 8-bit; A low = $07 AND 2; or as ActorEndOutOfRange.
%at($83C94A)
Kind46Wait:
    jsr.w ActorStopsShot
    bcs .lC953
    lda.b #$01
    sta.b !ACTOR_43
.lC953:
    lda.b !actor_07
    and.b #$02
    bne .lC95A
    rts
.lC95A:
    ldx.b #$00
    jmp.w ActorEndOutOfRange


; @routine 83:D518 Kind52Wait
; WHAT IT DOES: Kind $52's wait: animates; with its $48 set it tests a
; shot; stomped or hurting as touched; on animation frames 0-3 and 10 it
; hurts the player when its zone meets the test box; ended when out of
; range.
; HOW: AnimStep; $48 set: the shot test at 83:81CF. ActorStompOrHurt. Its
; frame ($27) looked up among five; found: ActorZoneHasTestBox with that
; frame's zone, met: PlayerEnter8CFacing. Then ActorEndOutOfRange with box
; 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: as ActorEndOutOfRange.
%at($83D518)
Kind52Wait:
    jsl.l AnimStep
    lda.b !actor_48
    beq .lD523
    jsr.w !SHOT_TEST_81CF
.lD523:
    jsr.w ActorStompOrHurt
    ldx.b #$04
    lda.b !ACTOR_27
.lD52A:
    cmp.w !KIND52_HURT_FRAMES,x
    beq .lD534
    dex
    bmi .lD540
    bra .lD52A
.lD534:
    lda.w !KIND52_HURT_ZONES,x
    tay
    jsr.w ActorZoneHasTestBox
    bcs .lD540
    jsr.w PlayerEnter8CFacing
.lD540:
    ldx.b #$00
    jmp.w ActorEndOutOfRange


; @routine 83:D64A Kind53Wait
; WHAT IT DOES: Kind $53's wait: stomped or hurting as touched; sets its
; $45 once the record its $4E names has $46 set; animates.
; HOW: ActorStompOrHurt; $45 clear: the byte at $46 of the record at $4E
; (16-bit X) nonzero, $45 = 1. AnimStep.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: as AnimStep.
%at($83D64A)
Kind53Wait:
    jsr.w ActorStompOrHurt
    lda.b !actor_45
    bne .lD660
    rep.b #$10
    ldx.b !ACTOR_4E
    lda.w !actor_46,x
    sep.b #$10
    beq .lD660
    lda.b #$01
    sta.b !actor_45
.lD660:
    jsl.l AnimStep
    rts


; @routine 83:E679 Kind67Wait
; WHAT IT DOES: Kind $67's wait: puts the player into state $8C while the
; player is across x $04F8-$0E47 and the test box is within its zone (from
; $1A2C) up and down.
; HOW: The player's x pixels less $09A0 plus $04A8 below $0950; then the
; test box's y against $C0 less $1A2C, by the test box's half-height plus
; $4C (in $000C, doubled in $000E): inside, PlayerEnter8CFacingLong.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor loop (83:94D0) each frame the actor waits, by its kind.
; Exit: M 8-bit, X 8-bit; as PlayerEnter8CFacingLong, or flags from the
; comparison.
%at($83E679)
Kind67Wait:
    rep.b #$20
    sec
    lda.w !player+!actor_x+1
    sbc.w #$09A0
    clc
    adc.w #$04A8
    cmp.w #$0950
    bcs .lE6B8
    clc
    lda.w !TEST_BOX_HALF_H
    adc.w #$004C
    sta.w !scratch
    asl a
    sta.w !scratch+2
    lda.w !v_1a2c
    eor.w #$FFFF
    inc a
    clc
    adc.w #$00C0
    sec
    sbc.w !TEST_BOX_Y
    clc
    adc.w !scratch
    cmp.w !scratch+2
    bcs .lE6B8
    sep.b #$30
    jsl.l PlayerEnter8CFacingLong
    rts
.lE6B8:
    sep.b #$30
    rts


; @routine 83:F991 KindWaitF991
; WHAT IT DOES: The per-frame routine of the kind whose stub at 83:930C
; starts at 83:F77F: animates; variant 0 sets its $42 when it stops a shot
; (until its $43 is set); other variants with $44 set hurt the player on
; touch and take the step at 83:F908; freed when out of range.
; HOW: AnimStep; $07 zero: $43 clear, ActorStopsShotLong, stopped: $42 =
; 1. $07 nonzero: $44 set, ActorHurtsPlayerOnTouchLong and the step at
; 83:F908. Then ActorFreeOutOfRangeLong with box 0 (by JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSL
; from the kind's stub (83:9310). Exit: as ActorFreeOutOfRangeLong.
%at($83F991)
KindWaitF991:
    jsl.l AnimStep
    lda.b !actor_07
    bne .lF9A9
    lda.b !ACTOR_43
    bne .lF9B4
    jsl.l ActorStopsShotLong
    bcs .lF9B4
    lda.b #$01
    sta.b !actor_42
    bra .lF9B4
.lF9A9:
    lda.b !actor_44
    beq .lF9B4
    jsl.l ActorHurtsPlayerOnTouchLong
    jsr.w !STEP_F908
.lF9B4:
    ldx.b #$00
    jml.l ActorFreeOutOfRangeLong
