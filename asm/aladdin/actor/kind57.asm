; kind57.asm: actor kind $57 of level 3, the guard at the market's end:
; it waits until Aladdin is right of x $0528, shows itself, then in rounds
; stands, and when he is near steps at him (charging and slashing) or
; walks off; stomped it hops back and flashes, with no health left it is
; knocked away; shot it stops a round. Its wait is Kind57Wait
; (kind_waits_rest.asm).
; C: src/aladdin/actor/kind57.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!K57_HEALTH = $11               ; stomps left: 4 at its start
!K57_LOOKS = $16                ; its look table (word)
!K57_29 = $29                   ; set to K57_29_START at its start; purpose not yet established (word)
!K57_STEPS = $40                ; frames left of a move
!K57_TOUCH = $42                ; Kind57Wait: 0 touching hurts Aladdin; positive: stomp or hurt; negative: nothing
!K57_MODE = $43                 ; Kind57Wait: 0 in play; 1 still; 2 up posed by facing
!K57_FACES = $44                ; Kind57Wait: nonzero, it turns to face Aladdin
!K57_LOOK_TABLE = $D3B8         ; its look table's address
!K57_29_START = $8913           ; K57_29's value at its start
!K57_PLAYER_X_HIGH = $08E8      ; Aladdin's x pixels, high byte
!K57_PLAYER_X_LOW = $08E7       ; and low byte
!K57_HEALTH_P = $0367           ; Aladdin's health
!K57_C0 = $00C0                 ; nonzero: it goes on hurting (its hurt round again)
!K57_KNOCKED = $00BD            ; counted up as one is knocked away
!K57_NEAR_Y = $08               ; ActorNearPlayerY's span
!K57_NEAR_X = $06               ; ActorNearPlayerX's span
!K57_STEP_SPEED = $3F           ; ActorSpeedXSet's entry for its step at him
!K57_WALK_SPEED = $33           ; and for its walk
!K57_STEP_SOUND = $20           ; the sound of its step at him
!K57_HURT_SOUND = $21           ; the sound of its hurt round
!K57_STOMP_SOUND = $0F          ; the sound as it is stomped
!K57_HOP_SOUND = $22            ; the sound between its hops back
!K57_SHOT_SOUND = $17           ; the sound as it is shot
!K57_STOMPED = $0C              ; its state after a stomp: started again
!K57_KNOCK_PICTURE = $09        ; ActorKnockedAway's animation
!K57_KNOCK_SOUND = $45          ; its sound
!K57_KNOCK_PRESET = $03         ; its motion preset

; @routine 83:D9BA Kind57Start
; @actor
; WHAT IT DOES: Actor kind $57: shows itself once Aladdin is right of x
; $0528 (its poses 2 and 1), then goes on in its rounds.
; HOW: $08 bit 1 and $09 bits 7 and 4 set; redraw; K57_HEALTH 4; its look
; table $D3B8, $29 $8913; facing, flip, K57_TOUCH, K57_FACES, K57_MODE 0;
; AnimStart 0. Each frame (BRK first) until Aladdin's x high byte is 5 and
; its low byte $28 or more. COP $18, AnimStart 2, COP $28, AnimStart 1,
; COP $90; on into Kind57Round.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $57 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83D9BA)
Kind57Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    lda.b #$04
    sta.b !K57_HEALTH
    rep.b #$20
    lda.w #!K57_LOOK_TABLE
    sta.b !K57_LOOKS
    lda.w #!K57_29_START
    sta.b !K57_29
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    stz.b !K57_TOUCH
    stz.b !K57_FACES
    stz.b !K57_MODE
    lda.b #$00
    jsl.l AnimStart
.wait:
    brk.b #$00
    lda.w !K57_PLAYER_X_HIGH
    cmp.b #$05
    bne .wait
    lda.w !K57_PLAYER_X_LOW
    cmp.b #$28
    bcc .wait
    lda.b #$18
    cop.b #$00
    lda.b #$02
    jsl.l AnimStart
    lda.b #$28
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    lda.b #$90
    cop.b #$00

; @routine 83:DA14 Kind57Round
; @actor
; WHAT IT DOES: A round of kind $57: it stands facing Aladdin (pose 3, 24
; frames); then each frame while he has health it looks for him: near in
; height and across it steps at him (poses 5 and 6, sound $20, 8 frames
; of moving), rests 32 frames and starts a round again; near in height
; only, it walks (pose 4, 48 frames) and starts a round again.
; HOW: K57_FACES 1, K57_TOUCH 0; AnimStart 3; COP $18. Each frame (BRK
; first) with Aladdin's health nonzero: ActorNearPlayerY 8 C set: the
; walk; ActorNearPlayerX 6 C set: the walk; else K57_TOUCH 1, AnimStart
; 5, COP $18, K57_FACES 0, AnimStart 6, SoundQueueAdd $20, K57_STEPS 8,
; ActorSpeedXSet $3F; K57_STEPS frames (BRK first) of ActorMoveX,
; ActorOutOfWallProbe4, ActorSnapToGround; COP $20; again. The walk:
; K57_STEPS $30, ActorSpeedXSet $33, AnimStart 4, K57_FACES 0; K57_STEPS
; frames of the same moves; again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached from
; Kind57Start (on into it) and by jumps from its other states.
; Exit: never returns.
%at($83DA14)
Kind57Round:
    lda.b #$01
    sta.b !K57_FACES
    stz.b !K57_TOUCH
    lda.b #$03
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
.look:
    brk.b #$00
    lda.w !K57_HEALTH_P
    beq .look
    ldy.b #!K57_NEAR_Y
    jsl.l ActorNearPlayerY
    bcs .walk
    ldy.b #!K57_NEAR_X
    jsl.l ActorNearPlayerX
    bcs .walk
    lda.b #$01
    sta.b !K57_TOUCH
    lda.b #$05
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    stz.b !K57_FACES
    lda.b #$06
    jsl.l AnimStart
    lda.b #!K57_STEP_SOUND
    jsl.l SoundQueueAdd
    lda.b #$08
    sta.b !K57_STEPS
    ldy.b #!K57_STEP_SPEED
    jsl.l ActorSpeedXSet
.step:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorSnapToGround
    dec.b !K57_STEPS
    bne .step
    lda.b #$20
    cop.b #$00
    bra Kind57Round
.walk:
    lda.b #$30
    sta.b !K57_STEPS
    ldy.b #!K57_WALK_SPEED
    jsl.l ActorSpeedXSet
    lda.b #$04
    jsl.l AnimStart
    stz.b !K57_FACES
.walking:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorSnapToGround
    dec.b !K57_STEPS
    bne .walking
    jmp.w Kind57Round

; @routine 83:DAA0 Kind57Hurt
; @actor
; WHAT IT DOES: Kind $57 hurt: it walks back in pose $0C for 128 frames
; with sound $21, again while $00C0 is set; then a round again.
; HOW: K57_FACES 1, K57_TOUCH 0; ActorSpeedXSet $33; K57_STEPS $80;
; SoundQueueAdd $21; AnimStart $0C; K57_STEPS frames (BRK first) of
; ActorMoveX, ActorOutOfWallProbe4, ActorSnapToGround; $00C0 nonzero:
; again; else JMP Kind57Round.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; jumps from its second and third states.
; Exit: never returns.
%at($83DAA0)
Kind57Hurt:
    lda.b #$01
    sta.b !K57_FACES
    stz.b !K57_TOUCH
    ldy.b #!K57_WALK_SPEED
    jsl.l ActorSpeedXSet
    lda.b #$80
    sta.b !K57_STEPS
    lda.b #!K57_HURT_SOUND
    jsl.l SoundQueueAdd
    lda.b #$0C
    jsl.l AnimStart
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorSnapToGround
    dec.b !K57_STEPS
    bne .walk
    lda.w !K57_C0
    bne Kind57Hurt
    jmp.w Kind57Round
%ends_at($83DAD6)

; @routine 83:DB43 Kind57Second
; @actor
; WHAT IT DOES: Kind $57 stomped (its second state): with health left it
; plays sound $0F, poses, hops back twice (sound $22 between), flashes 32
; frames and goes on hurt (Kind57Hurt); with none left it is knocked away
; (counted at $00BD).
; HOW: K57_HEALTH zero: $00BD up, ActorKnockedAway with A = 9, X = $45,
; Y = 3 (JMP). Else K57_MODE up; SoundQueueAdd $0F; $09 bit 7 set; state
; $0C; AnimStart 7; COP 8; K57_MODE up; ActorHopFromTable $23,
; SoundQueueAdd $22, ActorHopFromTable $28; K57_MODE down; K57_STEPS $20
; frames (BRK first) of $08 bit 3 flipped; $08 bit 3 set; K57_MODE 0; JMP
; Kind57Hurt.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $57 in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($83DB43)
Kind57Second:
    lda.b !K57_HEALTH
    beq .knocked
    inc.b !K57_MODE
    lda.b #!K57_STOMP_SOUND
    jsl.l SoundQueueAdd
    lda.b #$80
    tsb.b !actor_09
    lda.b #!K57_STOMPED
    sta.b !actor_state
    lda.b #$07
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    inc.b !K57_MODE
    ldx.b #$23
    jsr.w ActorHopFromTable
    lda.b #!K57_HOP_SOUND
    jsl.l SoundQueueAdd
    ldx.b #$28
    jsr.w ActorHopFromTable
    dec.b !K57_MODE
    lda.b #$20
    sta.b !K57_STEPS
.flash:
    brk.b #$00
    lda.b !actor_08
    eor.b #$08
    sta.b !actor_08
    dec.b !K57_STEPS
    bne .flash
    lda.b #$08
    tsb.b !actor_08
    stz.b !K57_MODE
    jmp.w Kind57Hurt
.knocked:
    inc.w !K57_KNOCKED
    lda.b #!K57_KNOCK_PICTURE
    ldx.b #!K57_KNOCK_SOUND
    ldy.b #!K57_KNOCK_PRESET
    jmp.w ActorKnockedAway

; @routine 83:DB9A Kind57Third
; @actor
; WHAT IT DOES: Kind $57 shot (its third state): sound $17; while $00C0
; is set it goes on hurt; else it stands still in pose $0A for 32 frames,
; touching him does nothing, and a round again.
; HOW: SoundQueueAdd $17; $00C0 nonzero: JMP Kind57Hurt. K57_FACES 0,
; K57_TOUCH $FF; AnimStart $0A; COP $20; JMP Kind57Round.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $57 in a state with bits 7 and 6 set
; (the table at $83:90A1). Exit: never returns.
%at($83DB9A)
Kind57Third:
    lda.b #!K57_SHOT_SOUND
    jsl.l SoundQueueAdd
    lda.w !K57_C0
    beq .still
    jmp.w Kind57Hurt
.still:
    stz.b !K57_FACES
    lda.b #$FF
    sta.b !K57_TOUCH
    lda.b #$0A
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    jmp.w Kind57Round
%ends_at($83DBBB)
