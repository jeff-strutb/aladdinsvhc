; kind87.asm: actor kind $87, a catcher: when Aladdin's test box comes
; into its zone it takes hold of him, shakes him about, lets him go, drops,
; and breaks into five of its children before it ends; a child ($07
; nonzero) drops to the ground, waits its turn, and floats away.
; C: src/aladdin/actor/kind87.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K87_CAUGHT = $43               ; counted up when it takes hold of Aladdin
!K87_HELD = $45                 ; nonzero while it still holds him
!K87_DONE = $42                 ; nonzero: it does not catch, only shakes and breaks
!K87_CHILD = $44                ; a child's: counted up once it has landed
!K87_FRAMES = $41               ; frames left of a stretch; the children left to make
!K87_STEP = $40                 ; the shake's step (counted down)
!K87_SLOT = $21                 ; the slot byte its children are made with
!K87_ZONE = $7C                 ; its zone for Aladdin's test box
!K87_PROBES = $D852             ; its probe list
!K87_CHILD_PROBES = $D856       ; a child's probe list
!K87_SHAKE_PICKS = $D843        ; (data bank) by the step: the offset in K87_SHAKE_X
!K87_SHAKE_X = $D84C            ; (data bank) x moves of the shake (words)
!K87_CHILD_WAITS = $D859        ; (data bank) by a child's $07: frames before it rises (0: none)
!K87_COUNTS = $7EFF00           ; a count by kind of the children made
!K87_HOLD_CODE = $D6F8          ; Aladdin's code while something holds him
!K87_FREED_CODE = $CDD2         ; his code when it lets him go
!K87_PLAYER_08F0 = $08F0        ; nonzero: Aladdin faces left (its facing is taken from it)
!K87_PLAYER_TAKEN = $1427       ; nonzero while Aladdin is taken
!K87_145A = $145A               ; negative: no catching
!K87_141B = $141B               ; nonzero while Aladdin is hurt (state $8C)
!K87_HOLD_AT = $145D            ; where Aladdin is held: x, y (words)
!ACTOR_PROBES = $16             ; its probe list's address (word)
!K87_26 = $26                   ; set with K87_FRAMES as it blinks out

; @routine 83:F77F Kind87Start
; @actor
; WHAT IT DOES: Actor kind $87. $07 0, the catcher: shown and hittable,
; it waits until Aladdin's test box is in its zone (not while $145A is
; negative or he is hurt); then it holds him ($30 frames), shakes him
; through 8 steps of its table, lets him go (his code $CDD2 if it still
; held him), drops to the ground with sound $3D, makes five children of
; its kind ($07 1-5, counted by kind at $7E:FF00), blinks 11 frames and
; ends. With K87_DONE set it skips the catch. $07 nonzero, a child: it
; drops to the ground, waits by its $07, rises 12 frames and ends.
; HOW: ACTOR_08 bit 1, ACTOR_09 $90; K87_CAUGHT, K87_HELD 0. Catcher:
; K87_DONE 0, probes $D852, AnimStart 3; each frame (BRK) unless
; K87_DONE: $145A negative or $141B: again; ActorZoneHasTestPointLong $7C
; C set: again. Caught: K87_CAUGHT up, facing 0 (1 when $08F0 is 0);
; Aladdin's code $D6F8, K87_HELD up, Kind87HoldAtIt, BRK,
; Kind87PlayerToIt; $30 frames (BRK), K87_HELD 0 once $1427 is 0. Shake:
; 8 steps down: its x + K87_SHAKE_X by the step's pick; unless K87_DONE,
; $1427 and K87_HELD set: Aladdin's x the same, else K87_HELD 0 when
; $1427 is 0; COP 2. Then $1427 and K87_HELD: his code $CDD2. Flip =
; facing; AnimStart 1, ActorMotionReset $38; ActorFall each frame (BRK)
; until ActorOutOfGround; SoundQueueAdd $3D, ActorSnapToGround; 5 times:
; $07 up, slot ($72) up 2, ActorSpawnSameLook with its $06, made: its
; count up. ACTOR_09 bit 7 cleared; $26 and K87_FRAMES $0B; each frame
; (BRK) ACTOR_08 bit 3 flipped, 11 frames; ActorEndPlaced (JML). Child:
; K87_CHILD 0, probes $D856; while not ActorOutOfGround its y + 8;
; ActorSnapToGround; COP by K87_CHILD_WAITS (when nonzero); K87_CHILD up,
; AnimStart 2, COP $18; vertical speed $000200; 12 frames (BRK) of
; ActorStepYBack; ActorEndPlaced (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $87 in state $0C (through its stub at
; $83:930C). Exit: never returns.
%at($83F77F)
Kind87Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    stz.b !K87_CAUGHT
    stz.b !K87_HELD
    lda.b !actor_07
    beq .catcher
    jmp.w .child
.catcher:
    stz.b !K87_DONE
    lda.b #!K87_PROBES
    sta.b !ACTOR_PROBES
    lda.b #!K87_PROBES>>8
    sta.b !ACTOR_PROBES+1
    lda.b #$03
    jsl.l AnimStart
.watch:
    brk.b #$00
    lda.b !K87_DONE
    bne .shake
    lda.w !K87_145A
    bmi .watch
    lda.w !K87_141B
    bne .watch
    ldy.b #!K87_ZONE
    jsl.l ActorZoneHasTestPointLong
    bcs .watch
    inc.b !K87_CAUGHT
    stz.b !actor_facing
    lda.w !K87_PLAYER_08F0
    bne .facing
    inc.b !actor_facing
.facing:
    lda.b #!K87_HOLD_CODE
    sta.w !player+!actor_03
    lda.b #!K87_HOLD_CODE>>8
    sta.w !player+!actor_03+1
    inc.b !K87_HELD
    jsr.w Kind87HoldAtIt
    brk.b #$00
    jsr.w Kind87PlayerToIt
    lda.b #$30
    sta.b !K87_FRAMES
.hold:
    lda.w !K87_PLAYER_TAKEN
    bne .holding
    stz.b !K87_HELD
.holding:
    brk.b #$00
    dec.b !K87_FRAMES
    bne .hold
.shake:
    lda.b #$08
    sta.b !K87_STEP
.shake_step:
    ldx.b !K87_STEP
    ldy.w !K87_SHAKE_PICKS,x
    rep.b #$21
    lda.b !actor_x+1
    adc.w !K87_SHAKE_X,y
    sta.b !actor_x+1
    ldx.b !K87_DONE
    bne .shaken
    ldx.w !K87_PLAYER_TAKEN
    beq .let_go
    ldx.b !K87_HELD
    beq .shaken
    sta.w !player+!actor_x+1
    bra .shaken
.let_go:
    ldx.b #$00
    stx.b !K87_HELD
.shaken:
    sep.b #$20
    lda.b #$02
    cop.b #$00
    dec.b !K87_STEP
    bne .shake_step
    ldx.w !K87_PLAYER_TAKEN
    beq .drop
    ldx.b !K87_HELD
    beq .drop
    lda.b #!K87_FREED_CODE
    sta.w !player+!actor_03
    lda.b #!K87_FREED_CODE>>8
    sta.w !player+!actor_03+1
.drop:
    lda.b !actor_facing
    sta.b !actor_flip
    lda.b #$01
    jsl.l AnimStart
    lda.b #$38
    jsl.l ActorMotionReset
.fall:
    brk.b #$00
    jsl.l ActorFall
    jsl.l ActorOutOfGround
    beq .fall
    lda.b #$3D
    jsl.l SoundQueueAdd
    jsl.l ActorSnapToGround
    lda.b #$05
    sta.b !K87_FRAMES
    lda.b #$72
    sta.b !K87_SLOT
.spawn:
    inc.b !actor_07
    inc.b !K87_SLOT
    inc.b !K87_SLOT
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .spawned
    ldx.b !actor_06
    lda.l !K87_COUNTS,x
    inc a
    sta.l !K87_COUNTS,x
.spawned:
    dec.b !K87_FRAMES
    bne .spawn
    lda.b #$80
    trb.b !actor_09
    lda.b #$0B
    sta.b !K87_FRAMES
    sta.b !K87_26
.blink:
    brk.b #$00
    lda.b !actor_08
    eor.b #$08
    sta.b !actor_08
    dec.b !K87_FRAMES
    bne .blink
    jml.l ActorEndPlaced
.child:
    stz.b !K87_CHILD
    lda.b #!K87_CHILD_PROBES
    sta.b !ACTOR_PROBES
    lda.b #!K87_CHILD_PROBES>>8
    sta.b !ACTOR_PROBES+1
.down:
    jsl.l ActorOutOfGround
    bne .grounded
    rep.b #$21
    lda.b !actor_y+1
    adc.w #$0008
    sta.b !actor_y+1
    sep.b #$20
    bra .down
.grounded:
    jsl.l ActorSnapToGround
    ldx.b !actor_07
    lda.w !K87_CHILD_WAITS,x
    beq .rise_set
    cop.b #$00
.rise_set:
    inc.b !K87_CHILD
    lda.b #$02
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    stz.b !actor_speed_y
    lda.b #$02
    sta.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    lda.b #$0C
    sta.b !K87_FRAMES
.rise:
    brk.b #$00
    jsl.l ActorStepYBack
    dec.b !K87_FRAMES
    bne .rise
    jml.l ActorEndPlaced

; @routine 83:F8E4 Kind87PlayerToIt
; WHAT IT DOES: Puts Aladdin at actor kind $87's place: its x, and its y
; less 8.
; HOW: 16-bit with C clear: its x pixels to the player's; its y pixels
; less 7 (less the clear carry: 8) to the player's.
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M 8-bit; A = the player's new y (16
; bits); N, V, Z, C from the subtraction.
%at($83F8E4)
Kind87PlayerToIt:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    sbc.w #$0007
    sta.w !player+!actor_y+1
    sep.b #$20
    rts

; @routine 83:F8F6 Kind87HoldAtIt
; WHAT IT DOES: Sets where Aladdin is held (K87_HOLD_AT) to actor kind
; $87's place: its x, and its y less 8.
; HOW: As Kind87PlayerToIt, into K87_HOLD_AT and the word after it.
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: as Kind87PlayerToIt.
%at($83F8F6)
Kind87HoldAtIt:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !K87_HOLD_AT
    lda.b !actor_y+1
    sbc.w #$0007
    sta.w !K87_HOLD_AT+2
    sep.b #$20
    rts
%ends_at($83F908)
