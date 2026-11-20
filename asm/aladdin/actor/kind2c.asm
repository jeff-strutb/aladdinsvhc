; kind2c.asm: actor kind $2C's start, a ride: when Aladdin comes into its
; zone it takes him (his code sent to $D992) and flies off right and down,
; carrying him, until it reaches the x its $07 gives; there it lets him go
; (his code sent to $CDD2) and stays. Its wait is Kind2CWait
; (kind_waits_rest.asm), which plays its sound while it carries him.
; C: src/aladdin/actor/kind2c.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K2C_END_X = $40                ; the x pixels where the ride ends (word)
!K2C_FRAMES = $43               ; frames to its wait's next sound, from 6
!K2C_SOUNDS = $44               ; nonzero while it carries him: its wait's sound
!K2C_END_FAR = $0F93            ; K2C_END_X with $07 nonzero
!K2C_END_NEAR = $0CF3           ; and with $07 0
!K2C_ZONE = $74                 ; ActorZoneHasTestPoint's zone
!K2C_RIDER_KIND = $146F         ; the kind of the actor carrying Aladdin
!K2C_PLAYER_FLAGS = $0347       ; bits 1-7 set while it carries him
!K2C_STATE_8C_COUNT = $141B     ; counted up as Aladdin enters state $8C
!K2C_V_145A = $145A             ; negative: no ride
!K2C_RIDE_CODE = $D992          ; Aladdin's code while it carries him
!K2C_OFF_CODE = $CDD2           ; and once it lets him go
!K2C_PRESET = $2D               ; ActorMotionPreset's preset for its flight
!K2C_ACCEL_X = $0C              ; its horizontal acceleration
!K2C_TOP_X = $04                ; the horizontal speed's middle byte it stops speeding up at
!K2C_RIDER_Y = $001A            ; Aladdin's y below it while it carries him

; @routine 83:A758 Kind2CStart
; @actor
; WHAT IT DOES: Actor kind $2C: a ride. It waits until Aladdin comes into
; its zone ($145A not negative), then takes him (his code to $D992, its
; wait's sound on) and flies off: speeding up right and down to a speed
; of 4 across, carrying him $1A below it, until it reaches K2C_END_X
; ($0CF3, $0F93 with $07 set); there it puts him and itself at that x and
; lets him go (his code to $CDD2), and stays for good.
; HOW: K2C_END_X by $07; $08 bit 1 and $09 bits 7 and 4 set; AnimStart 0;
; K2C_SOUNDS 0, K2C_FRAMES 6. Each frame (BRK first) until $145A is not
; negative and ActorZoneHasTestPointLong with zone $74 gives C clear.
; Then K2C_RIDER_KIND its kind, K2C_SOUNDS up, $0347 bits 1-7 set, $08
; bit 3 cleared, his facing and flip 0, his resume address $D992;
; ActorMotionPreset $2D, $1C $0C. Each frame (BRK first): its speed's
; middle byte other than 4: ActorAccelerateX, ActorAccelerateY;
; ActorMoveRight, ActorStepY, Kind2CCarry; until its x is K2C_END_X or
; more. Then $0347 bits 1-7 cleared, $141B 0, $08 bit 3 set, its x and
; his = K2C_END_X, his speed's middle and top bytes ours, his resume
; address $CDD2, K2C_SOUNDS 0; a BRK each frame for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $2C in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83A758)
Kind2CStart:
    rep.b #$20
    lda.w #!K2C_END_FAR
    ldy.b !actor_07
    bne .far
    lda.w #!K2C_END_NEAR
.far:
    sta.b !K2C_END_X
    sep.b #$20
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    stz.b !K2C_SOUNDS
    lda.b #$06
    sta.b !K2C_FRAMES
.wait:
    brk.b #$00
    lda.w !K2C_V_145A
    bmi .wait
    ldy.b #!K2C_ZONE
    jsl.l ActorZoneHasTestPointLong
    bcs .wait
    lda.b !actor_06
    sta.w !K2C_RIDER_KIND
    inc.b !K2C_SOUNDS
    lda.b #$FE
    tsb.w !K2C_PLAYER_FLAGS
    lda.b #$08
    trb.b !actor_08
    stz.w !player+!actor_facing
    stz.w !player+!actor_flip
    lda.b #!K2C_RIDE_CODE&255
    sta.w !player+!actor_03
    lda.b #!K2C_RIDE_CODE>>8
    sta.w !player+!actor_03+1
    ldy.b #!K2C_PRESET
    jsl.l ActorMotionPreset
    lda.b #!K2C_ACCEL_X
    sta.b !actor_accel_x
.fly:
    brk.b #$00
    lda.b !actor_speed_x+1
    cmp.b #!K2C_TOP_X
    beq .fast
    jsl.l ActorAccelerateX
    jsl.l ActorAccelerateY
.fast:
    jsl.l ActorMoveRight
    jsl.l ActorStepY
    jsr.w Kind2CCarry
    rep.b #$20
    lda.b !actor_x+1
    cmp.b !K2C_END_X
    sep.b #$20
    bcc .fly
    lda.b #$FE
    trb.w !K2C_PLAYER_FLAGS
    stz.w !K2C_STATE_8C_COUNT
    lda.b #$08
    tsb.b !actor_08
    rep.b #$20
    lda.b !K2C_END_X
    sta.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_speed_x+1
    sta.w !player+!actor_speed_x+1
    sep.b #$20
    lda.b #!K2C_OFF_CODE&255
    sta.w !player+!actor_03
    lda.b #!K2C_OFF_CODE>>8
    sta.w !player+!actor_03+1
    stz.b !K2C_SOUNDS
.stay:
    brk.b #$00
    bra .stay

; @routine 83:A806 Kind2CCarry
; WHAT IT DOES: Keeps Aladdin with the ride: his x its x, his y $1A below
; its y.
; HOW: 16-bit, carry cleared: his x pixels = its x pixels; his y pixels
; = its y pixels + K2C_RIDER_Y.
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M 8-bit; A = his y (16 bits); C from the
; addition.
%at($83A806)
Kind2CCarry:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    adc.w #!K2C_RIDER_Y
    sta.w !player+!actor_y+1
    sep.b #$20
    rts
%ends_at($83A818)
