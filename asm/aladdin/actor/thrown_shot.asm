; thrown_shot.asm: actor kinds $10 and $14-$17 of the second pool: a shot
; that flies its course, splashing (kind $13) where it lands, and kind
; $10's end when it hits (kind $11). Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/actor/thrown_shot.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"

arch 65816

!SHOT_16 = $16  ; word: $C9FF
!SHOT_11 = $11
!SHOT_LAUNCH = $40  ; frames left before bit 7 of $08 is cleared
!V_1774 = $1774  ; the high byte of the word at $1773 (the camera's x)

; @routine 81:EABA ShotFly
; @actor
; WHAT IT DOES: A shot (kinds $10 and $14-$17) started new: it flies the
; course its $07 picks until it lands in the ground, where it splashes (an
; actor of kind $13 in its place) and ends; it ends at once when it can no
; longer be hit, and, in levels 8 and $10, when it is above the level's
; top, in level $0E, when it is at the top and the camera is left of x
; $0700.
; HOW: Position fractions cleared, $09 = $50, animation 7, $2C = 4,
; ActorMotionPreset by $07; fractions cleared again, $16 = $C9FF, $11 and
; $18 cleared, bit 7 of $08 set, $40 = 2. Each frame (BRK first): the
; level tests (LEVEL 8 or $10: y's high byte negative; $0E: V_1774 below 7
; and y's high byte 0) and bit 6 of $09 clear end it (ActorEnd); else
; AnimStep, ActorMove; after 2 frames bit 7 of $08 cleared; ActorInGround
; with C set and bit 6 of V_0012 clear: Spawn13AtYLong at its own record
; (Y = D), then ActorEnd.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
; the actor loop's jump for kinds $10, $14-$17 in state $0C (through the
; stub at $83:920E). Exit: never returns; ends in ActorEnd.
%at($81EABA)
ShotFly:
    stz.b !actor_x
    stz.b !actor_y
    lda.b #$50
    sta.b !actor_09
    lda.b #$07
    jsl.l AnimStart
    lda.b #$04
    sta.b !actor_2c
    ldy.b !actor_07
    jsl.l ActorMotionPreset
    stz.b !actor_x
    stz.b !actor_y
    rep.b #$20
    lda.w #$C9FF
    sta.b !SHOT_16
    sep.b #$20
    stz.b !SHOT_11
    stz.b !actor_18
    lda.b #$80
    tsb.b !actor_08
    lda.b #$02
    sta.b !SHOT_LAUNCH
.lEAEB:
    brk.b #$00
    lda.w !level
    cmp.b #$08
    beq .lEB09
    cmp.b #$10
    beq .lEB09
    cmp.b #$0E
    bne .lEB0D
    lda.w !V_1774
    cmp.b #$07
    bcs .lEB0D
    lda.b !actor_y+2
    beq ShotHit_lEB76
    bra .lEB0D
.lEB09:
    lda.b !actor_y+2
    bmi ShotHit_lEB76
.lEB0D:
    lda.b !actor_09
    and.b #$40
    beq ShotHit_lEB76
    jsl.l AnimStep
    jsl.l ActorMove
    lda.b !SHOT_LAUNCH
    beq .lEB29
    dec.b !SHOT_LAUNCH
    bne .lEAEB
    lda.b !actor_08
    and.b #$7F
    sta.b !actor_08
.lEB29:
    jsl.l ActorInGround
    bcc .lEAEB
    bit.w !v_0012
    bvs .lEAEB
    rep.b #$30
    tdc
    tay
    sep.b #$20
    jsl.l Spawn13AtYLong
    sep.b #$30
    jml.l ActorEnd


; @routine 81:EB44 ShotHit
; @actor
; WHAT IT DOES: Actor kind $10 in state $4C (it hit something): a frame
; later, sound 8 and an actor of kind $11 in its place from the second
; pool, then its end.
; HOW: BRK; SoundQueueAdd 8; bit 7 of $08 set; ActorAllocBLong; a record
; (N clear): state $0C, kind $11, $07 0, $08 $80, its x and y pixels; then
; (also with none) ActorEnd, M and X 8-bit.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
; the actor loop's jump for kind $10 in state $4C and kind $B9 in state
; $0C (through the stub at $83:9212). Exit: never returns; ends in
; ActorEnd.
%at($81EB44)
ShotHit:
    brk.b #$00
    lda.b #$08
    jsl.l SoundQueueAdd
    lda.b !actor_08
    ora.b #$80
    sta.b !actor_08
    jsl.l ActorAllocBLong
    bmi .lEB76
    lda.b #$0C
    sta.w !actor_state,x
    lda.b #$11
    sta.w !actor_06,x
    stz.w !actor_07,x
    lda.b #$80
    sta.w !actor_08,x
    rep.b #$20
    lda.b !actor_x+1
    sta.w !actor_x+1,x
    lda.b !actor_y+1
    sta.w !actor_y+1,x
.lEB76:
    sep.b #$30
    jml.l ActorEnd
    ldy.b #$75
    jsl.l ActorMotionPreset
.lEB82:
    brk.b #$00
    jsl.l ActorFly
    bit.b !actor_09
    bvs .lEB82
    jml.l ActorEnd
