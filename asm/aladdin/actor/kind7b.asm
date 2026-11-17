; kind7b.asm: actor kind $7B, a thing that waits for the player to land
; on it, then sends him on. Written from the bytes; named and described
; by hand.
; C: src/aladdin/actor/kind7b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"

arch 65816

!K7B_PLAYER_1F = !player+$1F     ; bit 7 set: the player is not taken
!K7B_SENT_TO = $DA14             ; where it sends the player's code
!K7B_SOUND = $41

; @routine 83:C572 Kind7BStart
; @actor
; WHAT IT DOES: Actor kind $7B, a thing that waits for the player to land
; on it from above: then it changes pose (animation 1), sends the player
; on in his code at $DA14, waits 32 frames, sounds $41, waits 16 more and
; goes back to waiting; without health left the touch sets its own state
; to $8C instead.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; AnimStart 0. Each frame
; (BRK first): on when V_141C is nonzero, the player's $1F has bit 7
; clear and ActorHitsTestBoxLong says C clear. Health 0: ACTOR_STATE =
; $8C, AnimStart 0, again. Else, his y pixels + 16 less its y pixels
; below 0 (he is above it): AnimStart 1, his resume address ($03-$04) =
; $DA14, COP $20, SoundQueueAdd $41, COP $10, again; otherwise again. No
; script runs past the first BRK: written from its bytes. The JML to
; ActorEndPlaced after it (at $83:C5D1) is never reached.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $7B
; in state $0C (the table at $83:8CAB). Exit: never returns.
%at($83C572)
Kind7BStart:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
.pose:
    lda.b #$00
    jsl.l AnimStart
.frame:
    brk.b #$00
    lda.w !v_141c
    beq .frame
    lda.w !K7B_PLAYER_1F
    bmi .frame
    jsl.l ActorHitsTestBoxLong
    bcs .frame
    lda.w !health
    bne .alive
    lda.b #$8C
    sta.b !actor_state
    bra .pose
.alive:
    rep.b #$20
    lda.w !player+!actor_y+1
    clc
    adc.w #$0010
    sec
    sbc.b !actor_y+1
    sep.b #$20
    bcs .frame
    lda.b #$01
    jsl.l AnimStart
    lda.b #!K7B_SENT_TO
    sta.w !player+!actor_03
    lda.b #!K7B_SENT_TO>>8
    sta.w !player+!actor_03+1
    lda.b #$20
    cop.b #$00
    lda.b #!K7B_SOUND
    jsl.l SoundQueueAdd
    lda.b #$10
    cop.b #$00
    bra .frame
    jml.l ActorEndPlaced
