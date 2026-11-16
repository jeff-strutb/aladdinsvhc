; kind73.asm: actor kind $73's per-frame work while it waits: it follows a
; path, shows the animation its controller picks, and is touched (stomped,
; shots turned back) at a place offset from where it is drawn.
; C: src/aladdin/actor/kind73.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K73_ANIM = $1B20               ; the animation it should show
!K73_ANIM_SHOWN = $1B21         ; the animation it was last started on
!K73_KEPT_X = $1B23             ; its x (fraction, pixels word) kept over the touch tests
!K73_KEPT_X_PIXELS = $1B24
!K73_KEPT_MID = $1B25            ; its x's high byte, then its y's fraction (word)
!K73_KEPT_Y_PIXELS = $1B27
!K73_FLASH = $1B2A              ; frames left of its flash; at 0, $08 bits 4 and 7 cleared
!K73_UNTOUCHABLE = $1B2C        ; nonzero: no touch tests
!K73_POSE = $1B35               ; index (by 4) of the touch place's offsets
!K73_TOUCH_DX = $D7B6           ; per pose: x offset (word), then y offset (word)
!K73_TOUCH_DY = $D7B8

; @routine 84:86F4 Kind73Work
; WHAT IT DOES: What actors of kind $73 do each frame they wait: it
; starts the animation K73_ANIM names when that changes, animates, shows
; its picture from its kind's graphics, steps along its path, counts its
; flash down, and is touched at a place offset from it by its pose:
; Aladdin stomping it and shots turned back there, unless untouchable.
; HOW: K73_ANIM against K73_ANIM_SHOWN: changed, kept and AnimStart.
; AnimStep, ActorPictureSendFromKind, ActorPathStep. K73_FLASH nonzero:
; down by one, at 0 actor_08 AND $6F. Its x and y ($0B-$10) kept at
; $1B23-$1B28 and moved by the pose's offsets (K73_TOUCH_DX/DY by
; K73_POSE); K73_UNTOUCHABLE zero: ActorStompedByPlayerLong and
; ActorTurnsShotBackLong; then $0B-$10 put back.
; Entry: M 8-bit, X 8-bit; D = the actor; DB the bank of the offsets table
; and of work RAM at $0000-$1FFF; called by JSL from the kind's stub
; ($83:925E) through the actor loop's wait table at $83:8F4F. Exit: M
; 8-bit, X 8-bit; A = its y's pixels word; X = its y's fraction; N and Z
; from A; C as the touch tests or the y sum left it.
%at($8486F4)
Kind73Work:
    lda.w !K73_ANIM
    cmp.w !K73_ANIM_SHOWN
    beq .shown
    sta.w !K73_ANIM_SHOWN
    jsl.l AnimStart
.shown:
    jsl.l AnimStep
    jsl.l ActorPictureSendFromKind
    jsr.w ActorPathStep
    lda.w !K73_FLASH
    beq .moved
    dec.w !K73_FLASH
    bne .moved
    lda.b !actor_08
    and.b #$6F
    sta.b !actor_08
.moved:
    rep.b #$20
    ldx.w !K73_POSE
    lda.b !actor_x+1
    sta.w !K73_KEPT_X_PIXELS
    clc
    adc.w !K73_TOUCH_DX,x
    sta.b !actor_x+1
    lda.b !actor_y+1
    sta.w !K73_KEPT_Y_PIXELS
    clc
    adc.w !K73_TOUCH_DY,x
    sta.b !actor_y+1
    ldx.b !actor_x
    stx.w !K73_KEPT_X
    ldx.b !actor_y
    stx.w !K73_KEPT_MID+1
    sep.b #$20
    lda.w !K73_UNTOUCHABLE
    bne .back
    jsl.l ActorStompedByPlayerLong
    jsl.l ActorTurnsShotBackLong
.back:
    rep.b #$20
    lda.w !K73_KEPT_X
    sta.b !actor_x
    lda.w !K73_KEPT_MID
    sta.b !actor_x+2
    lda.w !K73_KEPT_Y_PIXELS
    sta.b !actor_y+1
    sep.b #$20
    rtl
