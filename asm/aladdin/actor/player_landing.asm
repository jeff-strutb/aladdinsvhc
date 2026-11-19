; player_landing.asm: Aladdin coming down on top of an actor: he is stood
; on it, his code goes on at his landing, and an actor of kind $11 starts
; at his feet; and the pad he lands on and presses down, which stands him
; on itself.
; C: src/aladdin/actor/player_landing.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!V_1448 = $1448                 ; $3C: no landing on actors
!V_0367 = $0367                 ; counted down on entering state $8C; 0: no landing
!PLAYER_STATE_8C_COUNT = $141B   ; counted up on entering state $8C
!V_141C = $141C                 ; 0: no landing
!V_145A = $145A                 ; bit 7 set: no landing
!PLAYER_SPEED_Y_PIXELS = !player+!actor_speed_y+1 ; his vertical speed's whole part (word)
!PLAYER_X_PIXELS = !player+!actor_x+1
!PLAYER_Y_PIXELS = !player+!actor_y+1
!PLAYER_CODE = !player+!actor_03 ; where his code goes on (word, his bank)
!PLAYER_LANDED_CODE = $D77C     ; his code once landed on an actor
!PLAYER_LANDED = $144E          ; 1: he has landed on an actor
!KIND_11 = $11

; The pad's own fields.
!PAD_REST = $40                 ; frames before it can be landed on again
!PAD_PRESSED = $41              ; frames it shows pressed; at 1 it rises again
!PAD_LAND_ANIM = $13            ; its animation at its start and at each landing
!PAD_AFTER_ANIM = $14           ; its animation from 9 frames after a landing
!PAD_SOUND = $12                ; the sound of a landing on it
!PAD_TOP = $0024                ; his y less its y, as he lands: he is above it
!PAD_STAND = $002B              ; he stands this many pixels above its y, less 1 (taken with C clear)
; A byte of work RAM it puts in PLAYER_LANDED (an absolute $0001 where
; the other landings put an immediate 1).
!V_0001 = $0001
!HIT_BOXES = $C29A               ; the hit boxes: 4 bytes each, the half-width first (data bank)
!TEST_BOX_HALF_W = $1488         ; the test box's half-width

; @routine 83:848D PlayerLandsOnActor
; WHAT IT DOES: Tells whether Aladdin has come down on top of the actor,
; and if so stands him on it, his code going on at his landing, with an
; actor of kind $11 at his feet.
; HOW: No when $1448 is $3C, when the actor misses the test box
; (ActorHitsTestBoxIfActive), when the boxes overlap by 10 or more
; pixels of height (ScratchBelowBy10), when he is rising, or when $0367
; is 0, a state $8C is counted, $141C is 0 or bit 7 of $145A is set. Yes:
; $83:84D5 stands him on it, then ActorSpawnKind11AtPlayer.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the hit boxes' bank. Exit: as the C states.
%at($83848D)
PlayerLandsOnActor:
    lda.w !V_1448
    cmp.b #$3C
    beq .no
    jsr.w ActorHitsTestBoxIfActive
    bcs .no
    rep.b #$20
    jsr.w ScratchBelowBy10
    bcs .no
    lda.w !PLAYER_SPEED_Y_PIXELS
    bpl .down
.no:
    sep.b #$32
    rts
.down:
    ldx.w !V_0367
    beq .no
    ldx.w !PLAYER_STATE_8C_COUNT
    bne .no
    ldx.w !V_141C
    beq .no
    ldx.w !V_145A
    bmi .no
    jsr.w PlayerStandOnActor
    sep.b #$30
    jsr.w ActorSpawnKind11AtPlayer
    lda.b #$01
    rts

; @routine 83:84F4 ActorSpawnKind11AtPlayer
; WHAT IT DOES: Starts an actor of kind $11 at Aladdin's feet: at his x,
; 16 pixels below his y, in a record of the second free list.
; HOW: The record taken through $81:EA3D; none free: nothing. Its state
; $0C (to start), kind $11, byte $07 zero, byte $08 $80, and its x and y
; pixels from his.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the C states.
%at($8384F4)
ActorSpawnKind11AtPlayer:
    jsl.l ActorAllocBLong
    bmi .done
    lda.b #$0C
    sta.w !actor_state,x
    lda.b #!KIND_11
    sta.w !actor_06,x
    stz.w !actor_07,x
    lda.b #$80
    sta.w !actor_08,x
    rep.b #$20
    lda.w !PLAYER_X_PIXELS
    sta.w !actor_x+1,x
    lda.w !PLAYER_Y_PIXELS
    clc
    adc.w #$0010
    sta.w !actor_y+1,x
.done:
    sep.b #$30
    rts

; @routine 83:84D5 PlayerStandOnActor
; WHAT IT DOES: Stands Aladdin on the actor he has come down on: his code
; goes on at his landing ($D77C of his bank) and his y is the actor's top.
; HOW: Unless a state $8C is counted ($141B): $D77C as his code address,
; his y pixels = the actor's less scratch $0010 (the boxes' half-heights
; less 4, from ScratchBelowBy10), and $144E = 1. With $141B nonzero it
; pulls its own return address and returns from its caller (M and X
; 8-bit), code the retail game never runs: each of its five callers
; ($83:8382, $83F6, $843E, $847E, $84BC) has just found $141B zero.
; Entry: M 16-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M 16-bit, X 8-bit; A = his y pixels; X =
; 1; N, Z from X; C, V from the subtraction; Y unchanged.
%at($8384D5)
PlayerStandOnActor:
    lda.w #!PLAYER_LANDED_CODE
    ldx.w !PLAYER_STATE_8C_COUNT
    bne .counted
    sta.w !PLAYER_CODE
    sec
    lda.b !actor_y+1
    sbc.w !scratch+4
    sta.w !PLAYER_Y_PIXELS
    ldx.b #1
    stx.w !PLAYER_LANDED
    rts
.counted:
    sep.b #$30
    pla
    pla
    rts

; @routine 83:F9BA LandingPad
; @actor
; WHAT IT DOES: An actor (through the stub at $83:9315): a pad Aladdin
; can land on. Landed on from above, it plays sound $12, starts its
; animation $13 again and $14 nine frames later, stands him on its top at
; his landing code, and cannot be landed on again for 20 frames.
; HOW: actor_08 bit 1 and actor_09 bits $90 set; $2C = 6; animation $13;
; PAD_REST and PAD_PRESSED 0. Each frame (BRK first): PAD_PRESSED counts
; down, and the animation $14 starts as it goes from 1 to 0; with
; PAD_REST nonzero it counts down and the frame ends. Else with the test
; box met (ActorHitsTestBoxIfActive) and $145A not negative: when his y
; plus $24 is below its y, animation $13 and $141C = 1; then with no
; state $8C counted, PAD_REST 20, PAD_PRESSED 10, SoundQueueAdd $12, his
; code $D77C, a yield, his y = its y less $2C (SBC #$2B with the C its
; REP #$21 cleared, kept through the yield), his x = its x, and
; PLAYER_LANDED = the byte at $0001. Met from the side or with a state
; $8C counted it goes on at PadFromSide ($83:FA40): Aladdin put beside
; it (PlayerPushAside), and the next frame.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump through the stub at $83:9315. Exit: never
; returns.
%at($83F9BA)
LandingPad:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$06
    sta.b !actor_2c
    lda.b #!PAD_LAND_ANIM
    jsl.l AnimStart
    stz.b !PAD_REST
    stz.b !PAD_PRESSED
.frame:
    brk.b #$00
    lda.b !PAD_PRESSED
    beq .resting
    dec a
    bne .pressed
    lda.b #!PAD_AFTER_ANIM
    jsl.l AnimStart
.pressed:
    dec.b !PAD_PRESSED
.resting:
    lda.b !PAD_REST
    beq .ready
    dec.b !PAD_REST
    bra .frame
.ready:
    jsr.w ActorHitsTestBoxIfActive
    bcs .frame
    lda.w !V_145A
    bmi .frame
    rep.b #$20
    lda.w !PLAYER_Y_PIXELS
    adc.w #!PAD_TOP
    cmp.b !actor_y+1
    sep.b #$20
    bcs PadFromSide
    lda.b #!PAD_LAND_ANIM
    jsl.l AnimStart
    lda.b #$01
    sta.w !V_141C
    lda.w !PLAYER_STATE_8C_COUNT
    bne PadFromSide
    lda.b #20
    sta.b !PAD_REST
    lda.b #10
    sta.b !PAD_PRESSED
    lda.b #!PAD_SOUND
    jsl.l SoundQueueAdd
    rep.b #$21
    lda.w #!PLAYER_LANDED_CODE
    sta.w !PLAYER_CODE
    brk.b #$00
    lda.b !actor_y+1
    sbc.w #!PAD_STAND
    sta.w !PLAYER_Y_PIXELS
    lda.b !actor_x+1
    sta.w !PLAYER_X_PIXELS
    sep.b #$20
    lda.w !V_0001
    sta.w !PLAYER_LANDED
    bra .frame
PadFromSide:
    jsr.w PlayerPushAside
    bra LandingPad_frame
%ends_at($83FA45)

; @routine 83:80B5 PlayerPushAsideLong
; WHAT IT DOES: PlayerPushAside for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as PlayerPushAside; called by JSL. Exit: as PlayerPushAside.
%at($8380B5)
PlayerPushAsideLong:
    jsr.w PlayerPushAside
    rtl

; @routine 83:80B9 PlayerPushAsideIfTouched
; WHAT IT DOES: PlayerPushAside when Aladdin is in the air ($141C set) and
; touches the active actor; else nothing. Nothing in the game calls it
; (no JSR, JSL, JMP, pointer or word names it; it sits past
; PlayerPushAsideLong's RTL): unit_bank83_pools runs it.
; HOW: $141C zero: out (PlayerPushAside's RTS); ActorHitsTestBoxIfActive C
; set: out; else on into PlayerPushAside.
; Entry: M and X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and whose HIT_BOXES is the box table; called
; by JSR. Exit: out: as the test leaves them; else as PlayerPushAside.
PlayerPushAsideIfTouched:
    lda.w !V_141C
    beq PlayerPushAside_done
    jsr.w ActorHitsTestBoxIfActive
    bcs PlayerPushAside_done

; @routine 83:80C3 PlayerPushAside
; WHAT IT DOES: Puts Aladdin beside the actor, just out of its box: on
; its left when he is left of it (or at its x), else on its right.
; HOW: 16-bit: Y = the actor's box ($06) times 4; its x pixels less his
; ($08E7), the borrow kept (PHP); the box's half-width (HIT_BOXES,Y)
; plus 1 plus TEST_BOX_HALF_W; with no borrow (he is left of it, or at
; its x) that is negated, plus 1 (EOR #$FFFF, INC, INC); his x pixels =
; that plus its x pixels. M and X 8-bit.
; Entry: M and X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and whose HIT_BOXES is the box table; called
; by JSR. Exit: M and X
; 8-bit (Y high byte cleared); A = his new x pixels (word); N, Z, C from
; the last ADC.
%at($8380C3)
PlayerPushAside:
    rep.b #$30
    lda.b !actor_06
    and.w #$00FF
    asl a
    asl a
    tay
    lda.b !actor_x+1
    sec
    sbc.w !PLAYER_X_PIXELS
    php
    lda.w !HIT_BOXES,y
    inc a
    clc
    adc.w !TEST_BOX_HALF_W
    plp
    bcc .beside
    eor.w #$FFFF
    inc a
    inc a
.beside:
    clc
    adc.b !actor_x+1
    sta.w !PLAYER_X_PIXELS
    sep.b #$30
.done:
    rts
