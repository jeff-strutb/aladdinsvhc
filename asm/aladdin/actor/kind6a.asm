; kind6a.asm: actor kind $6A, which stage a3 s1 places as spawners: a
; spawner ($07 0-3) makes a child of its own kind with $07 4 more, again
; and again after a pause by its $07; a child ($07 4-7) moves one of four
; ways for good.
; C: src/aladdin/actor/kind6a.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K6A_PAUSES = $D65B             ; ($80) by $07: the spawner's frames between children
!K6A_SPAWNED = $7EFF00          ; by kind: children spawned
!K6A_FLY_BOX = $002C            ; ActorSpeedXUpFlyAgainst's box ($08)

; @routine 83:EB5E Kind6AStart
; @actor
; WHAT IT DOES: Actor kind $6A: by its $07, a spawner (0-3) that makes a
; child of its own kind with $07 4 more and waits its pause, again and
; again; or a child (4-7), shown in animation 0 facing right, that moves
; one of four ways for good.
; HOW: $07 4 or more: actor_08 bit 1 and actor_09 bits $90 set, flip 0,
; AnimStart 0. JMP (.ways,X) by $07 * 2: 0-3 the spawner: Kind6ASpawnChild,
; COP with K6A_PAUSES by $07, and again; 4-7 Kind6AWay4-Kind6AWay7.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (K6A_PAUSES
; there); reached by the actor loop's jump for kind $6A in state $0C (the
; table at $83:8CAB).
; Exit: never returns.
%at($83EB5E)
Kind6AStart:
    lda.b !actor_07
    sec
    cmp.b #$04
    bcc .way
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    stz.b !actor_flip
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$00
    jsl.l AnimStart
.way:
    lda.b !actor_07
    asl
    tax
    jmp.w (.ways,x)
.ways:
    dw .spawner, .spawner, .spawner, .spawner
    dw Kind6AWay4, Kind6AWay5, Kind6AWay6, Kind6AWay7
.spawner:
    jsr.w Kind6ASpawnChild
    ldx.b !actor_07
    lda.w !K6A_PAUSES,x
    cop.b #$00
    bra .spawner

; @routine 83:EB9C Kind6ASpawnChild
; WHAT IT DOES: A kind $6A spawner makes a child like itself with $07 4
; more, at its spawn point 0, and counts it for its kind.
; HOW: $07 kept (PHA); actor_spawn_at 0; $07 + 4; ActorSpawnSameLook with
; A = actor_06; made (N clear): K6A_SPAWNED by actor_06 up. M 8-bit; $07
; back (PLA).
; Entry: M 8-bit, X 8-bit; D = the spawner; called by JSR. Exit: M 8-bit;
; A = $07, N and Z from it; X as the spawn or the count left it.
%at($83EB9C)
Kind6ASpawnChild:
    lda.b !actor_07
    pha
    stz.b !actor_spawn_at
    lda.b !actor_07
    clc
    adc.b #$04
    sta.b !actor_07
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .kept
    ldx.b !actor_06
    lda.l !K6A_SPAWNED,x
    inc
    sta.l !K6A_SPAWNED,x
.kept:
    sep.b #$20
    pla
    sta.b !actor_07
    rts

; @routine 83:EBC1 Kind6AWay4
; @actor
; WHAT IT DOES: A kind $6A child with $07 4: thrown up with gravity $0C,
; then each frame slowed and moved up and down.
; HOW: ActorSpeedYSetGravity with A = $0C, Y = 0; each frame (BRK first)
; ActorAccelerateYBack, ActorMoveY.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; Kind6AStart's jump. Exit: never returns.
%at($83EBC1)
Kind6AWay4:
    lda.b #$0C
    ldy.b #$00
    jsl.l ActorSpeedYSetGravity
.frame:
    brk.b #$00
    jsl.l ActorAccelerateYBack
    jsl.l ActorMoveY
    bra .frame

; @routine 83:EBD5 Kind6AWay5
; @actor
; WHAT IT DOES: A kind $6A child with $07 5: speed $36 across (gaining
; 6), $60 up or down, and each frame faster.
; HOW: ActorSpeedXSetAccel A = 6, Y = $36; ActorSpeedYSet Y = $60; each
; frame (BRK first) ActorSpeedUpMove.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; Kind6AStart's jump. Exit: never returns.
%at($83EBD5)
Kind6AWay5:
    lda.b #$06
    ldy.b #$36
    jsl.l ActorSpeedXSetAccel
    ldy.b #$60
    jsl.l ActorSpeedYSet
.frame:
    brk.b #$00
    jsl.l ActorSpeedUpMove
    bra .frame

; @routine 83:EBEB Kind6AWay6
; @actor
; WHAT IT DOES: A kind $6A child with $07 6: speed $39 across, $60 up or
; down, moved each frame.
; HOW: ActorSpeedXSet Y = $39; ActorSpeedYSet Y = $60; each frame (BRK
; first) ActorMove.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; Kind6AStart's jump. Exit: never returns.
%at($83EBEB)
Kind6AWay6:
    ldy.b #$39
    jsl.l ActorSpeedXSet
    ldy.b #$60
    jsl.l ActorSpeedYSet
.frame:
    brk.b #$00
    jsl.l ActorMove
    bra .frame

; @routine 83:EBFF Kind6AWay7
; @actor
; WHAT IT DOES: A kind $6A child with $07 7: speed $3C across (gaining 8),
; $60 with gravity 8, and each frame faster across, flown against box 8.
; HOW: ActorSpeedXSetAccel A = 8, Y = $3C; ActorSpeedYSetGravity A = 8,
; Y = $60; each frame (BRK first) K6A_FLY_BOX 8, ActorSpeedXUpFlyAgainst.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached from
; Kind6AStart's jump. Exit: never returns.
%at($83EBFF)
Kind6AWay7:
    lda.b #$08
    ldy.b #$3C
    jsl.l ActorSpeedXSetAccel
    lda.b #$08
    ldy.b #$60
    jsl.l ActorSpeedYSetGravity
.frame:
    brk.b #$00
    lda.b #$08
    sta.w !K6A_FLY_BOX
    jsl.l ActorSpeedXUpFlyAgainst
    bra .frame

; @routine 83:EC1C Kind6ASecond
; @actor
; WHAT IT DOES: Kind $6A's second state: it ends.
; HOW: JML ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $6A with state bit 7 set (the table at $83:8DFD).
; Exit: never returns.
%at($83EC1C)
Kind6ASecond:
    jml.l ActorEndPlaced
