; kind83.asm: actor kind $83, a thing that flies up out of the carpet
; levels' scenery to a height and leaves a child of kind $5E there.
; C: src/aladdin/actor/kind83.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K83_16 = $16                   ; word: $D836
!K83_NO_CHILD = $42             ; nonzero (the late variant): no child at the end
!K83_TOP_SPEED = $45            ; the most its vertical speed's fraction may reach
!K83_LINE = $4A                 ; the y (low byte) it goes on until: $C8, or $FF for the late variant
!K83_DELAYS = $D83A             ; by $07: the frames before it shows (in the bank DB names)
!K83_TOP_SPEEDS = $D83D         ; by $07: K83_TOP_SPEED
!K83_PRESETS = $D840            ; by $07: its motion preset
!player_x = !player+!actor_x+1  ; Aladdin's x, whole pixels (word)
!V_1A10 = $1A10                 ; nonzero: it falls instead of flying on
!KIND_5E = $5E

; @routine 84:9538 Kind83Start
; @actor
; WHAT IT DOES: Actor kind $83: by its $07, hidden for a while, then it
; flies off by its motion preset, its vertical speed held to its top
; speed, until its y reaches $C8; it falls instead once $1A10 is set;
; then it leaves a child of kind $5E and ends. With $07 bit 4 set (late)
; it waits until Aladdin is past it, flies until y $FF, and leaves no
; child.
; HOW: ACTOR_09 bits $90 set; K83_16 $D836; AnimStart 0; K83_NO_CHILD 0.
; $07 bit 4: $07 AND $0F, K83_NO_CHILD up, each frame (BRK) until its x
; is below PLAYER_X, K83_LINE $FF. Else ACTOR_08 bit 3 cleared, COP
; K83_DELAYS by $07, the bit set again, K83_LINE $C8. Facing and flip 0;
; K83_TOP_SPEED and the preset (ActorMotionPreset) by $07. Each frame:
; ActorFly; y at K83_LINE or past it, on; $1A10 set: gravity 8 and each
; frame ActorFall until y reaches K83_LINE; else the speed's fraction
; held at K83_TOP_SPEED (gravity 0 when it was over). At the end, unless
; K83_NO_CHILD: each frame ActorCreateChild8 of kind $5E ($21 0) until
; one is made. ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds its tables (bank $84); reached by
; the actor loop's jump for kind $83 in state $0C (the table at $83:8DAB,
; through the stub at $83:92D8). Exit: never returns; ends in
; ActorEndPlaced.
%at($849538)
Kind83Start:
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D836
    sta.b !K83_16
    sep.b #$20
    lda.b #$00
    jsl.l AnimStart
    stz.b !K83_NO_CHILD
    lda.b !actor_07
    bit.b #$10
    beq .shown_later
    and.b #$0F
    sta.b !actor_07
    inc.b !K83_NO_CHILD
.until_past:
    brk.b #$00
    rep.b #$20
    lda.b !actor_x+1
    cmp.w !player_x
    sep.b #$20
    bcs .until_past
    lda.b #$FF
    sta.b !K83_LINE
    bra .off
.shown_later:
    lda.b #$08
    trb.b !actor_08
    ldx.b !actor_07
    lda.w !K83_DELAYS,x
    cop.b #$00
    lda.b #$08
    tsb.b !actor_08
    lda.b #$C8
    sta.b !K83_LINE
.off:
    stz.b !actor_facing
    stz.b !actor_flip
    ldx.b !actor_07
    lda.w !K83_TOP_SPEEDS,x
    sta.b !K83_TOP_SPEED
    ldy.w !K83_PRESETS,x
    jsl.l ActorMotionPreset
.fly:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_y+1
    cmp.b !K83_LINE
    bcs .there
    lda.w !V_1A10
    bne .fall
    lda.b !K83_TOP_SPEED
    cmp.b !actor_speed_y
    bcs .fly
    sta.b !actor_speed_y
    stz.b !actor_accel_y
    bra .fly
.fall:
    lda.b #$08
    sta.b !actor_accel_y
.falling:
    brk.b #$00
    jsl.l ActorFall
    lda.b !actor_y+1
    cmp.b !K83_LINE
    bcc .falling
.there:
    lda.b !K83_NO_CHILD
    bne .end
    brk.b #$00
    stz.b !actor_spawn_at
    lda.b #!KIND_5E
    jsl.l ActorCreateChild8
    bcs .there
.end:
    jml.l ActorEndPlaced
