; kind7d_wait.asm: actor kind $7D's wait: a spot that takes hold of the
; player. Each frame it follows its kept height, steps its state (take
; hold, keep him there or put him back, a pause), and ends once it is far
; from the point actors are kept in range of.
; C: src/aladdin/actor/kind7d_wait.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K7D_Y_BASE = $40               ; its height before the scroll (word, kept from its start)
!K7D_SCROLL_Y = $1A2C           ; subtracted from it for its y (word)
!K7D_STATE = $13                ; its state: the entry of Kind7DStep's table
!K7D_PROBE_SIDE = $18           ; GroundInFrontLong's side: 0, then 4
!K7D_PAUSE = $42                ; frames left in state 2
!K7D_RANGE_X = $1A44            ; the x of the point actors are kept in range of (word)
!K7D_PUT_BACK = $1458           ; counted up each time the player is put back
!K7D_KEEP = $1427               ; nonzero: the player is kept at the spot
!K7D_HELD_CODE = $D6F8          ; the player's code word while held
!player_x_before = $143B        ; the player's x before this frame's move (word)
!player_y_before = $143E        ; his y before it (word)
!player_resume = !player+!actor_03 ; the player's resume address (word)

; @routine 83:929F ActorKind7DWait
; WHAT IT DOES: The routine actors of kind $7D run each frame they wait
; (not the last), when bit 7 of their byte at $09 is set: Kind7DWait.
; HOW: JSL Kind7DWait ($84:90F4), then RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; X = twice its kind
; (its low 7 bits); called by JSR through the actor loop's table by kind.
; Exit: as Kind7DWait leaves them (it may end the actor: never returns).
%at($83929F)
ActorKind7DWait:
    jsl.l Kind7DWait
    rts

; @routine 84:90F4 Kind7DWait
; WHAT IT DOES: What actors of kind $7D do each frame they wait: follow
; their kept height as the view scrolls, take a step of their state, and
; end once more than 224 pixels from the point actors are kept in range
; of.
; HOW: 16-bit: its y = K7D_Y_BASE - K7D_SCROLL_Y; Kind7DStep (JSR).
; 16-bit: its x - K7D_RANGE_X + $E0 below $1C0 (within 224 either way):
; RTL. Else its JSL's and
; the loop's JSR return addresses dropped (five pulls) and on into
; ActorEndPlaced at the loop's level.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's stub ($83:929F)
; through the actor loop's wait table at $83:8F4F. Exit: M 8-bit; in
; range: C clear, N and Z from the difference; else never returns.
%at($8490F4)
Kind7DWait:
    rep.b #$20
    sec
    lda.b !K7D_Y_BASE
    sbc.w !K7D_SCROLL_Y
    sta.b !actor_y+1
    sep.b #$20
    jsr.w Kind7DStep
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.w !K7D_RANGE_X
    clc
    adc.w #$00E0
    cmp.w #$01C0
    sep.b #$20
    bcs .far
    rtl
.far:
    pla
    pla
    pla
    pla
    pla
    jml.l ActorEndPlaced

; @routine 84:9120 Kind7DStep
; WHAT IT DOES: Kind $7D's state step: 0 waits for the player to step
; onto it and takes hold of him; 1 keeps him there, or puts him back
; where he was when ground is in front of it; 2 a 16-frame pause, then 0
; again.
; HOW: JMP through the table at $84:9127 by K7D_STATE. 0 ($84:912D):
; ActorGrabbableByPlayerLong C set, or GroundInFrontLong on either side
; (K7D_PROBE_SIDE 0, then 4) C set: RTS. Else 16-bit: the player's code
; word K7D_HELD_CODE, PLAYER_POS_SAVED = its x and y; K7D_STATE up. 1
; ($84:915C): GroundInFrontLong on either side C set: the player back at
; his x and y before this frame's move, K7D_PUT_BACK up, then to state 2.
; Else K7D_KEEP set: the player at its x and y, RTS; clear: to state 2
; (K7D_STATE up, K7D_PAUSE $10). 2 ($84:919E): K7D_PAUSE down; at 0,
; K7D_STATE 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR from Kind7DWait. Exit: M 8-bit.
%at($849120)
Kind7DStep:
    lda.b !K7D_STATE
    asl a
    tax
    jmp.w (.states,x)
.states:
    dw .wait, .hold, .pause
.wait:
    jsl.l ActorGrabbableByPlayerLong
    bcs .out
    stz.b !K7D_PROBE_SIDE
    jsl.l GroundInFrontLong
    bcs .out
    lda.b #$04
    sta.b !K7D_PROBE_SIDE
    jsl.l GroundInFrontLong
    bcs .out
    rep.b #$20
    lda.w #!K7D_HELD_CODE
    sta.w !player_resume
    lda.b !actor_x+1
    sta.w !player_pos_saved
    lda.b !actor_y+1
    sta.w !player_pos_saved+2
    sep.b #$20
    inc.b !K7D_STATE
.out:
    rts
.hold:
    stz.b !K7D_PROBE_SIDE
    jsl.l GroundInFrontLong
    bcs .back
    lda.b #$04
    sta.b !K7D_PROBE_SIDE
    jsl.l GroundInFrontLong
    bcc .keep
.back:
    rep.b #$20
    lda.w !player_x_before
    sta.w !player+!actor_x+1
    lda.w !player_y_before
    sta.w !player+!actor_y+1
    sep.b #$20
    inc.w !K7D_PUT_BACK
    bra .next
.keep:
    lda.w !K7D_KEEP
    beq .next
    rep.b #$20
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    sta.w !player+!actor_y+1
    sep.b #$20
    rts
.next:
    inc.b !K7D_STATE
    lda.b #$10
    sta.b !K7D_PAUSE
    rts
.pause:
    dec.b !K7D_PAUSE
    bne .done
    stz.b !K7D_STATE
.done:
    rts
assert pc() == $8491A5
