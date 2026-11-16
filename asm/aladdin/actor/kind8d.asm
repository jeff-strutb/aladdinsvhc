; kind8d.asm: actor kind $8D, which flies in from the view's left, circles
; Aladdin twice, flies off and ends.
; C: src/aladdin/actor/kind8d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!player_y = !player+!actor_y+1  ; the player's y, whole pixels (word)
!VIEW_X = $1773                 ; the view's left edge (pixels, word)
!VIEW_Y = $1777                 ; the view's top edge (pixels, word)
!V_1A12 = $1A12                 ; cleared as it flies off
!KIND8D_ANGLE = $14             ; its direction (0-$7F), for the vector moves
!KIND8D_TURNS = $40             ; circles left; then frames left of its flight off
!KIND8D_12 = $12                ; $18
!KIND8D_29 = $29                ; word: $890B

; @routine 84:99ED Kind8DCircle
; @actor
; WHAT IT DOES: Actor kind $8D: starts 16 pixels left of the view and 256
; below its top, flies on direction $1B until near Aladdin, then circles
; him twice at radius $48 (its flip toward him), flies off on direction
; $1D for 80 frames, and ends off the screen.
; HOW: actor_08 bit 1, actor_09 bits $90 set; KIND8D_12 = $18, KIND8D_29
; = $890B; x = VIEW_X - $10, y = VIEW_Y + $100; AnimStart 0; direction
; $1B, flip 0. Each frame (BRK first) ActorMoveByVector with X = $1E
; until ActorNearPlayer (Y = $2C) clears C. Direction $46, two turns:
; each frame its centre is his place, the direction steps by one (mod
; $80), ActorMoveByVectorTimes (X = $10, A = $48), flip by
; ActorSideOfPlayer; a turn ends at direction $48. Then flip 0, direction
; $1D, $50 frames of ActorMoveByVector (X = $1E); V_1A12 cleared; its x
; and y high bytes $FF; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $8D
; in state $0C (the table at $83:8DAB, through the stub at $83:9344).
; Exit: never returns; ends in ActorEndPlaced.
%at($8499ED)
Kind8DCircle:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$18
    sta.b !KIND8D_12
    rep.b #$20
    lda.w #$890B
    sta.b !KIND8D_29
    lda.w !VIEW_X
    sec
    sbc.w #$0010
    sta.b !actor_x+1
    lda.w !VIEW_Y
    clc
    adc.w #$0100
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$00
    jsl.l AnimStart
    lda.b #$1B
    sta.b !KIND8D_ANGLE
    stz.b !actor_flip
.in:
    brk.b #$00
    ldx.b #$1E
    jsl.l ActorMoveByVector
    ldy.b #$2C
    jsl.l ActorNearPlayer
    bcs .in
    lda.b #$46
    sta.b !KIND8D_ANGLE
    lda.b #$02
    sta.b !KIND8D_TURNS
.circle:
    brk.b #$00
    rep.b #$20
    lda.w !player_x
    sta.b !actor_x+1
    lda.w !player_y
    sta.b !actor_y+1
    sep.b #$20
    lda.b !KIND8D_ANGLE
    inc a
    and.b #$7F
    sta.b !KIND8D_ANGLE
    ldx.b #$10
    lda.b #$48
    jsl.l ActorMoveByVectorTimes
    jsl.l ActorSideOfPlayer
    sta.b !actor_flip
    lda.b !KIND8D_ANGLE
    sec
    cmp.b #$48
    bne .circle
    dec.b !KIND8D_TURNS
    bne .circle
    stz.b !actor_flip
    lda.b #$1D
    sta.b !KIND8D_ANGLE
    lda.b #$50
    sta.b !KIND8D_TURNS
.off:
    brk.b #$00
    ldx.b #$1E
    jsl.l ActorMoveByVector
    dec.b !KIND8D_TURNS
    bne .off
    stz.w !V_1A12
    jmp.w .gone

%at($849B43)
.gone:
    lda.b #$FF
    sta.b !actor_x+2
    sta.b !actor_y+2
    jml.l ActorEndPlaced
