; shot_hit_face.asm: an actor a shot of the second pool touches: it turns to
; face Aladdin and goes to its hit state, keeping where the shot was.
; Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/shot_hit_face.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!HIT_SHOT_X = $48  ; word: the shot's x pixels
!HIT_SHOT_Y = $4A  ; word: the shot's y pixels

; @routine 84:821A ActorShotHitFace
; WHAT IT DOES: When one of the second pool's shots touches the actor: it
; faces Aladdin, takes state $CC (hit), copies $42 to $46 (with $42 zero,
; ActorStateSet8CLong as well), and keeps the shot's position in $48 and
; $4A.
; HOW: ActorTouchesSecondPoolLong; C set (none): out. Its x compared with
; the player's: facing and flip = the carry (1: he is left of it); state
; $CC; $46 = $42, zero: ActorStateSet8CLong; the touching record's (Y) x
; and y pixels to $48 and $4A. M and X 8-bit.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; called by JSR
; (84:8200). Exit: M and X 8-bit; C as ActorTouchesSecondPoolLong left it
; when none touched.
%at($84821A)
ActorShotHitFace:
    jsl.l ActorTouchesSecondPoolLong
    bcs .l8246
    ldx.b !actor_x+1
    cpx.w !player+!actor_x+1
    lda.b #$00
    rol a
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$CC
    sta.b !actor_state
    lda.b !actor_42
    sta.b !actor_46
    bne .l823A
    jsl.l ActorStateSet8CLong
.l823A:
    rep.b #$20
    lda.w !actor_x+1,y
    sta.b !HIT_SHOT_X
    lda.w !actor_y+1,y
    sta.b !HIT_SHOT_Y
.l8246:
    sep.b #$30
    rts
