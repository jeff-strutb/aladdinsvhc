; kind56.asm: actor kind $56 of level 3, the monkey at the market's end:
; it waits until Aladdin is right of x $0528, runs left a while, and waits
; for his walk-out scene ($00BE); then it runs at him, gives its sign
; ($00BE = $80) and runs off. Its wait is Kind56Wait (kind_waits_rest.asm).
; C: src/aladdin/actor/kind56.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K56_LOOKS = $16                ; its look table (word)
!K56_STEPS = $40                ; frames left of its first run
!K56_LOOK_TABLE = $D3B4         ; its look table's address
!K56_X = $05B0                  ; where it starts (x and y pixels)
!K56_Y = $01B0
!K56_PLAYER_X_HIGH = $08E8      ; Aladdin's x pixels, high byte
!K56_PLAYER_X_LOW = $08E7       ; and low byte
!K56_CUE = $00BE                ; set by Aladdin's walk-out scene; it sets it to $80 in turn
!K56_RUN_SPEED = $30            ; ActorSpeedXSet's entry for its runs
!K56_NEAR = $EC                 ; added to its x's low byte: it runs until that is below his

; @routine 83:D8E2 Kind56Start
; @actor
; WHAT IT DOES: Actor kind $56: put at x $05B0, y $01B0 facing left, it
; waits until Aladdin is right of x $0528, then (after 76 frames) runs
; right 240 frames and turns; once his walk-out scene sets $00BE it runs
; at him until it is near (its x's low byte less $14 below his), poses,
; sets $00BE to $80, and runs off for good, 4 pixels higher.
; HOW: $08 bit 0 and $09 bits 7 and 4 set; its look table $D3B4; x pixels
; $05B0, y $01B0; facing and flip 1; AnimStart 0. Each frame (BRK first)
; until his x high byte is 5 and its low byte $28 or more. COP $4C;
; facing, flip 0; AnimStart 1; ActorSpeedXSet $30; K56_STEPS $F0 frames
; (BRK first) of ActorMoveX, ActorSnapToGround. $08 bits 0-1 = 2; facing,
; flip 1; AnimStart 2; each frame (BRK first) until $00BE is set. COP
; $40; AnimStart 1; ActorSpeedXSet $30; each frame (BRK first)
; ActorMoveX, ActorSnapToGround until its x low byte + $EC is below his.
; AnimStart 3; COP $80; $00BE $80; COP $10; facing, flip 0; y pixels less
; 4; AnimStart 1; ActorSpeedXSet $30; each frame (BRK first) ActorMoveX.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $56 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83D8E2)
Kind56Start:
    lda.b #$01
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    rep.b #$20
    lda.w #!K56_LOOK_TABLE
    sta.b !K56_LOOKS
    lda.w #!K56_X
    sta.b !actor_x+1
    lda.w #!K56_Y
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.wait:
    brk.b #$00
    lda.w !K56_PLAYER_X_HIGH
    cmp.b #$05
    bne .wait
    lda.w !K56_PLAYER_X_LOW
    cmp.b #$28
    bcc .wait
    lda.b #$4C
    cop.b #$00
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$01
    jsl.l AnimStart
    ldy.b #!K56_RUN_SPEED
    jsl.l ActorSpeedXSet
    lda.b #$F0
    sta.b !K56_STEPS
.run:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !K56_STEPS
    bne .run
    lda.b !actor_08
    and.b #$FC
    ora.b #$02
    sta.b !actor_08
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$02
    jsl.l AnimStart
.cue:
    brk.b #$00
    lda.w !K56_CUE
    beq .cue
    lda.b #$40
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    ldy.b #!K56_RUN_SPEED
    jsl.l ActorSpeedXSet
.atHim:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    clc
    lda.b !actor_x+1
    adc.b #!K56_NEAR
    cmp.w !K56_PLAYER_X_LOW
    bcs .atHim
    lda.b #$03
    jsl.l AnimStart
    lda.b #$80
    cop.b #$00
    lda.b #$80
    sta.w !K56_CUE
    lda.b #$10
    cop.b #$00
    stz.b !actor_facing
    stz.b !actor_flip
    rep.b #$20
    clc
    lda.b !actor_y+1
    adc.w #$FFFC
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$01
    jsl.l AnimStart
    ldy.b #!K56_RUN_SPEED
    jsl.l ActorSpeedXSet
.off:
    brk.b #$00
    jsl.l ActorMoveX
    bra .off
%ends_at($83D9B5)
