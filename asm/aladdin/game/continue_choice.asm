; continue_choice.asm: actor kind $26 on the continue screen: the two places
; Aladdin can step down into (yes: continue; no) and the one that watches
; him. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/game/continue_choice.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_state.inc"

arch 65816

!CONTINUE_YES = $76  ; set when Aladdin takes the continue (the screen then counts a continue off)
!CONTINUE_NO = $77  ; set when he turns it down
!V_0380 = $380  ; copied each frame to the watcher's $46
!PLAYER_RESUME = $8DE  ; the player's resume address (its record's $03, word)
!PLAYER_X = $8E7  ; the player's x pixels (its record's $0C)
!CHOICE_TAKEN = $44  ; set once Aladdin has stepped into it
!CHOICE_SINK = $40  ; frames left of its sinking
!WATCHER = $42  ; set: it is the watcher
!WATCHER_ANIM = $45  ; the animation it shows
!WATCHER_46 = $46  ; V_0380, copied each frame

; @routine 83:9D62 ContinueChoiceActor
; @actor
; WHAT IT DOES: An actor of the continue screen; none when no continues
; are left. $07 low four bits zero: a place Aladdin can step down into
; ($07 zero: the yes, facing right; else the no, facing left): when he is
; in its zone it takes him (his code goes on at $D9C5, set below it),
; sinks 16 pixels with him, and marks the choice. Otherwise the watcher:
; its animation by the choice made, or by where Aladdin stands (1 between
; x 80 and 175, 2 left of it, 4 right; 3 once yes, 5 once no).
; HOW: $08 bit 1 and $09 bits $90 set; facing, flip and $42 cleared.
; CONTINUES zero: ActorEndPlaced. Place: animation 0, $44 cleared; each
; frame until ActorZoneHasTestPointLong (zone $84) clears C; PLAYER_RESUME
; = $D9C5, $44 up, PlayerSpotBelowActor 5; 16 frames of its y pixels up by
; one; CONTINUE_YES (or, $07 nonzero, CONTINUE_NO) up; then BRK for good.
; Watcher: $45 cleared, $42 up; each frame a new animation (A) is started
; when not $45, $46 = V_0380, BRK, then A from the choice or the player's
; x.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $26 in state $0C. Exit: never returns;
; with no continues ends in ActorEndPlaced.
%at($839D62)
ContinueChoiceActor:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    stz.b !WATCHER
    lda.w !continues
    bne .l9D7C
    jmp.w ActorEndPlaced
.l9D7C:
    lda.b !actor_07
    and.b #$0F
    bne .l9DCC
    lda.b #$00
    jsl.l AnimStart
    stz.b !CHOICE_TAKEN
    lda.b !actor_07
    beq .l9D94
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
.l9D94:
    brk.b #$00
    ldy.b #$84
    jsl.l ActorZoneHasTestPointLong
    bcs .l9D94
    rep.b #$20
    lda.w #$D9C5
    sta.w !PLAYER_RESUME
    sep.b #$20
    inc.b !CHOICE_TAKEN
    lda.b #$05
    jsl.l PlayerSpotBelowActor
    lda.b #$10
    sta.b !CHOICE_SINK
.l9DB4:
    brk.b #$00
    inc.b !actor_y+1
    dec.b !CHOICE_SINK
    bne .l9DB4
    lda.b !actor_07
    bne .l9DC5
    inc.w !CONTINUE_YES
    bra .l9DC8
.l9DC5:
    inc.w !CONTINUE_NO
.l9DC8:
    brk.b #$00
    bra .l9DC8
.l9DCC:
    stz.b !WATCHER_ANIM
    inc.b !WATCHER
.l9DD0:
    cmp.b !WATCHER_ANIM
    beq .l9DDA
    sta.b !WATCHER_ANIM
    jsl.l AnimStart
.l9DDA:
    lda.w !V_0380
    sta.b !WATCHER_46
    brk.b #$00
    lda.w !CONTINUE_YES
    bne .l9E02
    lda.w !CONTINUE_NO
    bne .l9E06
    lda.w !PLAYER_X
    cmp.b #$50
    bcc .l9DFA
    cmp.b #$B0
    bcs .l9DFE
    lda.b #$01
    bra .l9DD0
.l9DFA:
    lda.b #$02
    bra .l9DD0
.l9DFE:
    lda.b #$04
    bra .l9DD0
.l9E02:
    lda.b #$03
    bra .l9DD0
.l9E06:
    lda.b #$05
    bra .l9DD0
