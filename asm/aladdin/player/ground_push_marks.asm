; ground_push_marks.asm: actor kind $60 (and $5F in state $4C): the mark at
; Aladdin's feet while the ground pushes him or he slides on it, shown
; only then. Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/player/ground_push_marks.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!GROUND_PUSHED = $1429  ; 2 while the player stands on ground that pushes, 1 while he slides on it, else 0
!PLAYER_X = $8E7  ; the player's x pixels (word)
!PLAYER_Y = $8EA  ; the player's y pixels (word)
!MARK_ANIM = $41  ; the animation it shows
!MARK_ANIM_WANTED = $40  ; the animation it is to show

; @routine 83:E480 GroundPushMark
; @actor
; WHAT IT DOES: Actor kind $60 started new (and kind $5F in state $4C): a
; mark that follows Aladdin's feet (his x, 24 below his y) for good; shown
; animated only while GROUND_PUSHED is set: animation 9 while the ground
; pushes him (2), $0A while he slides (1).
; HOW: $09 bits $90 set, its look $0600; $40 and $41 = $0A, AnimStart $0A;
; $08 bit 0 set, bit 3 (animated) cleared. Each frame (BRK first): its
; position from the player's; GROUND_PUSHED zero: bit 3 of $08 cleared;
; else set, the animation wanted 9 (2) or $0A (1), started when it is not
; the one shown ($41).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $60 in state $0C and kind $5F in state
; $4C. Exit: never returns.
%at($83E480)
GroundPushMark:
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    lda.b #$0A
    sta.b !MARK_ANIM_WANTED
    sta.b !MARK_ANIM
    jsl.l AnimStart
    lda.b !actor_08
    ora.b #$01
    and.b #$F7
    sta.b !actor_08
.lE4A1:
    brk.b #$00
    rep.b #$20
    lda.w !PLAYER_X
    sta.b !actor_x+1
    lda.w !PLAYER_Y
    clc
    adc.w #$0018
    sta.b !actor_y+1
    sep.b #$20
    lda.w !GROUND_PUSHED
    bne .lE4C2
    lda.b !actor_08
    and.b #$F7
    sta.b !actor_08
    bra .lE4A1
.lE4C2:
    lda.b !actor_08
    ora.b #$08
    sta.b !actor_08
    lda.w !GROUND_PUSHED
    dec a
    bne .lE4DF
    lda.b #$0A
.lE4D0:
    sta.b !MARK_ANIM_WANTED
    sec
    cmp.b !MARK_ANIM
    beq .lE4A1
    sta.b !MARK_ANIM
    jsl.l AnimStart
    bra .lE4A1
.lE4DF:
    lda.b #$09
    bra .lE4D0
