; player_carry.asm: actors and the player beside them: carried along when
; riding, and faced. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/player_carry.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!CARRIES = $43                  ; nonzero: the player rides the actor this frame
!KEPT_X = $48                   ; the actor's x pixels when last kept (word)
!KEPT_Y = $4A                   ; its y pixels when last kept (word)
!v_143b = $143B              ; moved with the player's x (word)
!v_143e = $143E              ; moved with the player's y (word)

; @routine 83:FBDA ActorCarryPlayer
; WHAT IT DOES: Carries the player along with the actor it rides: moves
; the player (and $143B, $143E) by how far the actor moved since its x and
; y were kept.
; HOW: Only when CARRIES is set: its x pixels less KEPT_X added to the
; player's x pixels and to $143B; its y pixels less KEPT_Y added to the
; player's y and to $143E; 16-bit A.
; Entry: M 8-bit; D = the actor's record; called by JSR. Exit: M 8-bit;
; not carrying: A low = 0, Z set; else A = the new $143E.
%at($83FBDA)
ActorCarryPlayer:
    lda.b !CARRIES
    beq .lFC0C
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.b !KEPT_X
    pha
    clc
    adc.w !player+!actor_x+1
    sta.w !player+!actor_x+1
    pla
    clc
    adc.w !v_143b
    sta.w !v_143b
    sec
    lda.b !actor_y+1
    sbc.b !KEPT_Y
    pha
    clc
    adc.w !player+!actor_y+1
    sta.w !player+!actor_y+1
    pla
    clc
    adc.w !v_143e
    sta.w !v_143e
    sep.b #$20
.lFC0C:
    rts


; @routine 83:FC96 ActorFacePlayer
; WHAT IT DOES: Turns the actor to face the player: facing 1 (left) when
; it is at or right of the player, 0 when left of it.
; HOW: Its x pixels compared with the player's (16-bit X); the carry
; rotated into 0 is the facing.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit: M
; 8-bit, X 8-bit; A low = the facing; X = its x pixels' low byte; C clear.
%at($83FC96)
ActorFacePlayer:
    rep.b #$10
    ldx.b !actor_x+1
    cpx.w !player+!actor_x+1
    lda.b #$00
    rol a
    sta.b !actor_facing
    sep.b #$10
    rts
