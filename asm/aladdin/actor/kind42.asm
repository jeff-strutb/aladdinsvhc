; kind42.asm: actor kind $42, the pot a kind $20 guard with $07 bit 4
; carries over its head (Kind20Carrier, kind20_carrier.asm): it rides
; there until the guard's throw, then flies at Aladdin, rolls or bounces
; along the ground, and breaks; it drops when the guard is struck. Its
; wait is Kind42Wait in kind_waits_rest.asm. Not in
; the catalog: no level places such a guard.
; C: src/aladdin/actor/kind42.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!K42_PROBES = $16               ; its probe list (word): $D2B2
!K42_LOOK_HIGH = $2C            ; its sprite attributes' high byte
!K42_CARRIER = $40              ; the guard's record (word)
!K42_LEFT = $41                 ; frames left of its breaking
!K42_FRAME = $27                ; (in the guard's record) the frame its animation shows
!K42_SIGNAL = $4F               ; (in the guard's record) 0; 1 the pot hit Aladdin; 2 it passed him
!K42_AHEAD = $D2B6              ; (data bank $80) x offsets ahead, facing right then left (words)

; @routine 83:C5DE Kind42Start
; @actor
; WHAT IT DOES: The pot's start: it rides on its guard until the guard's
; animation shows frame $15 (the throw), then flies at Aladdin and, by
; its $07, rolls along the ground (0) or bounces (else) until it meets a
; wall or falls too fast; then it breaks (Kind42Break). Its guard gone,
; it breaks at once; its guard struck (frame $0E or $11), it drops.
; HOW: AnimStart 0; ACTOR_08 bit 1, ACTOR_09 bit 4; look high byte 6;
; probes $D2B2. Each frame (BRK first): ActorDownedByHarmlessShot; the
; guard's state 0: Kind42Break; its frame $0E or $11: the drop; $15: on.
; ACTOR_09 bit 7 set, ActorMotionPreset $23. $07 0: each frame (BRK
; first) ActorFly, Kind42PastPlayer, until ActorOutOfGround says Z
; clear; speed x $0180; vertical speed $0800 or more: Kind42Break; else
; sound $14 when ACTOR_09 bit 6, ActorSnapToGround, then each frame
; (BRK first) ActorMoveX, Kind42PastPlayer, Kind42Ahead (a block:
; Kind42Break), ActorSnapToGround (none: vertical speed 0 and the flight
; again). $07 else: Kind42Bounces (JMP). The drop: ACTOR_09 bit 7
; cleared, ActorMotionPreset $24; each frame (BRK first) AnimStep,
; ActorFly until ActorOutOfGround; Kind42Break.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $42 (the table at $83:8CAB), or by
; Kind20Carrier's JMP once it swapped its record with the pot's.
; Exit: never returns.
%at($83C5DE)
Kind42Start:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$02
    tsb.b !actor_08
    lda.b #$10
    tsb.b !actor_09
    lda.b #$06
    sta.b !K42_LOOK_HIGH
    lda.b #$B2
    sta.b !K42_PROBES
    lda.b #$D2
    sta.b !K42_PROBES+1
.held:
    brk.b #$00
    jsr.w ActorDownedByHarmlessShot
    rep.b #$10
    ldx.b !K42_CARRIER
    lda.w !actor_state,x
    bne .carried
    jmp.w Kind42Break
.carried:
    lda.w !K42_FRAME,x
    sep.b #$10
    cmp.b #$0E
    beq .dropped
    cmp.b #$11
    beq .dropped
    cmp.b #$15
    bne .held
    lda.b #$80
    tsb.b !actor_09
    ldy.b #$23
    jsl.l ActorMotionPreset
    lda.b !actor_07
    beq .thrown
    jmp.w Kind42Bounces
.thrown:
    brk.b #$00
    jsl.l ActorFly
    jsr.w Kind42PastPlayer
    jsl.l ActorOutOfGround
    beq .thrown
    rep.b #$20
    lda.w #$0180
    sta.b !actor_speed_x
    lda.b !actor_speed_y
    cmp.w #$0800
    sep.b #$20
    bcs .breaks
    lda.b !actor_09
    and.b #$40
    beq .quiet
    lda.b #$14
    jsl.l SoundQueueAdd
.quiet:
    jsl.l ActorSnapToGround
    brk.b #$00
.rolls:
    jsl.l ActorMoveX
    jsr.w Kind42PastPlayer
    jsr.w Kind42Ahead
    bne .breaks
    jsl.l ActorSnapToGround
    bcc .off
    brk.b #$00
    bra .rolls
.off:
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    bra .thrown
.breaks:
    jmp.w Kind42Break
.dropped:
    lda.b #$80
    trb.b !actor_09
    ldy.b #$24
    jsl.l ActorMotionPreset
.drop:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFly
    jsl.l ActorOutOfGround
    beq .drop
    jmp.w Kind42Break

; @routine 83:C69A Kind42Ahead
; WHAT IT DOES: Gives the class of the map's block ahead of the pot (by
; the table at $80:D2B6 and its facing) at its own height.
; HOW: Its x pixels plus the table's word by facing into SCRATCH, its y
; pixels into SCRATCH + 2 (an LDA #$0010 the code loads over), then
; TileAtProbeLong.
; Entry: M 8-bit, X 8-bit; D = the pot; DB = $80; called by JSR.
; Exit: as TileAtProbeLong.
Kind42Ahead:
    lda.b !actor_facing
    asl a
    tax
    rep.b #$21
    lda.w !K42_AHEAD,x
    adc.b !actor_x+1
    sta.w !scratch
    lda.w #$0010
    clc
    lda.b !actor_y+1
    sta.w !scratch+2
    jsl.l TileAtProbeLong
    rts

; @routine 83:C6B6 Kind42PastPlayer
; WHAT IT DOES: Once the flying pot is past Aladdin (more than 16 pixels
; beyond him the way it faces), tells its guard (its SIGNAL 2) unless the
; guard has been told already.
; HOW: 16-bit registers, C clear. Facing right: Aladdin's x + $10 below
; the pot's x; facing left: Aladdin's x - $10 (SBC #$0F with C clear) at
; or above it. Then the guard's SIGNAL 0: 2. M and X 8-bit.
; Entry: M 8-bit, X 8-bit; D = the pot; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M and X 8-bit.
Kind42PastPlayer:
    lda.b !actor_facing
    rep.b #$31
    bne .left
    lda.w !player+!actor_x+1
    adc.w #$0010
    cmp.b !actor_x+1
    bcs .out
.past:
    ldx.b !K42_CARRIER
    sep.b #$20
    lda.w !K42_SIGNAL,x
    bne .out
    lda.b #$02
    sta.w !K42_SIGNAL,x
    bra .out
.left:
    lda.w !player+!actor_x+1
    sbc.w #$000F
    cmp.b !actor_x+1
    bcs .past
.out:
    sep.b #$30
    rts

%ends_at($83C6E3)
; (83:C6E3-C713: Kind42Wait, in kind_waits_rest.asm.)

; @routine 83:C714 Kind42Bounces
; @actor
; WHAT IT DOES: The thrown pot with $07 nonzero bounces along: each frame
; it flies on; a block ahead breaks it (Kind42Break); landed, it rests 2
; frames and springs up again.
; HOW: Each frame (BRK first): ActorFly; Kind42Ahead nonzero: Kind42Break;
; Kind42PastPlayer; ActorOutOfGround Z: again; else ActorSnapToGround,
; COP 2, ActorMotionPreset $25, again.
; Entry: M 8-bit, X 8-bit; D = the pot; DB = $80; reached by Kind42Start's
; JMP. Exit: never returns.
%at($83C714)
Kind42Bounces:
    brk.b #$00
    jsl.l ActorFly
    jsr.w Kind42Ahead
    bne Kind42Break
    jsr.w Kind42PastPlayer
    jsl.l ActorOutOfGround
    beq Kind42Bounces
    jsl.l ActorSnapToGround
    lda.b #$02
    cop.b #$00
    ldy.b #$25
    jsl.l ActorMotionPreset
    bra Kind42Bounces

; @routine 83:C738 Kind42Break
; @actor
; WHAT IT DOES: The pot breaks: sound $15 if it could be hit, then its
; breaking animation for 16 frames, and its end.
; HOW: 8-bit registers; ACTOR_09 bit 7 cleared; bit 6 set: sound $15.
; AnimStart 1; LEFT $10; each frame (BRK first) AnimStep until LEFT runs
; out; ActorEndPlaced (JMP).
; Entry: D = the pot; DB = $80; reached by the actor loop's jump for kind
; $42 in a state with bit 7 set (the table at $83:8DFD) and by
; Kind42Start's jumps. Exit: never returns.
Kind42Break:
    sep.b #$30
    lda.b #$80
    trb.b !actor_09
    lda.b !actor_09
    and.b #$40
    beq .quiet
    lda.b #$15
    jsl.l SoundQueueAdd
.quiet:
    lda.b #$01
    jsl.l AnimStart
    lda.b #$10
    sta.b !K42_LEFT
.fade:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K42_LEFT
    bne .fade
    jmp.w ActorEndPlaced
%ends_at($83C761)
