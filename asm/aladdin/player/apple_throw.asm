; apple_throw.asm: Aladdin throws an apple: a shot (kind $10) from the
; second pool, from his hand by his pose, one apple fewer. Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/player/apple_throw.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_video.inc"

arch 65816

!APPLES = $369  ; apples left, two decimal digits
!V_0389 = $389  ; zero: no apple is thrown
!THROW_X = $C718  ; by pose (the wanted box, low two bits), word: the hand's x from the player's, along his facing
!THROW_Y = $C720  ; by pose, word: the hand's height above his y
!THROW_COURSE = $C728  ; by pose, word (low byte used): the shot's course ($07)

; The end of the routine before ($81:CDD2 up, not yet filed), the way out
; PlayerAppleThrow takes when it throws nothing: C clear.
%at($81CDE7)
PlayerThrowNone:
    clc
    rts

; @routine 81:CDE9 PlayerAppleThrow
; WHAT IT DOES: Throws an apple when one can be thrown: a shot (kind $10)
; starts at Aladdin's hand for his pose, on the course the pose gives,
; facing his way; one apple fewer, sound 3, and his horizontal motion
; stopped unless $141C is set. C set when thrown.
; HOW: V_0389 zero, APPLES zero, LEVEL $25, or no free record
; (ActorAllocBLong, N set): out with C clear (the clc at $81:CDE7). Else
; the record: state $0C, facing and flip his flip; the pose
; (PLAYER_BOX_WANTED and 3) picks THROW_X (added to his x, or taken when
; his flip is 0), THROW_Y (taken from his y) and THROW_COURSE (its $07);
; kind $10. APPLES down by one in decimal (a low digit 0 takes 7 off);
; SoundQueueAdd 3; V_141C zero: ActorStopX. C set.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $81; called by
; JSR from the player's code. Exit: M 8-bit, X 8-bit; C set when thrown,
; clear when not.
%at($81CDE9)
PlayerAppleThrow:
    lda.w !V_0389
    beq PlayerThrowNone
    lda.w !APPLES
    beq PlayerThrowNone
    lda.w !level
    cmp.b #$25
    beq PlayerThrowNone
    jsl.l ActorAllocBLong
    bmi PlayerThrowNone
    lda.b #$0C
    sta.w !actor_state,x
    lda.b !actor_flip
    sta.w !actor_facing,x
    sta.w !actor_flip,x
    rep.b #$20
    lda.w !player_box_wanted
    and.w #$0003
    pha
    asl a
    tay
    lda.b !actor_flip
    and.w #$00FF
    beq .lCE2A
    lda.b !actor_x+1
    clc
    adc.w !THROW_X,y
    sta.w !actor_x+1,x
    bra .lCE33
.lCE2A:
    lda.b !actor_x+1
    sec
    sbc.w !THROW_X,y
    sta.w !actor_x+1,x
.lCE33:
    lda.b !actor_y+1
    sec
    sbc.w !THROW_Y,y
    sta.w !actor_y+1,x
    sep.b #$20
    ply
    lda.b #$10
    sta.w !actor_06,x
    lda.w !THROW_COURSE,y
    sta.w !actor_07,x
    sep.b #$10
    lda.w !APPLES
    pha
    and.b #$0F
    bne .lCE5A
    pla
    sec
    sbc.b #$07
    bra .lCE5C
.lCE5A:
    pla
    dec a
.lCE5C:
    sta.w !APPLES
    lda.b #$03
    jsl.l SoundQueueAdd
    lda.w !v_141c
    bne .lCE6E
    jsl.l ActorStopX
.lCE6E:
    sec
    rts
