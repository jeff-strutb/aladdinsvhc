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
!K73_HEALTH = $11               ; its health: 7, or $7F on a copier; below 0, beaten
!K73_28 = $28                   ; $FF
!K73_29 = $29                   ; word: $8919
!K73_FRAMES = $44               ; frames left of a pause in its own turn
!K73_HITS = $1B2B               ; counted up each time it is struck below 0
!K73_HIT_FRAMES = $1B29         ; frames left of its show of being struck
!K73_SLOTS = $7EFE60            ; words 1-15 set to $FFFF at its start
!K73_COPIER_TEST = $0000        ; in bank $70 (by X = $80): cartridge RAM a copier has
!K73_COPIER_TEST_2 = $0080
!K73_COPIER_BANK = $80          ; less $10: the bank of the copier test
!CHAIN_GO = $1B18               ; nonzero once the chain is built (chain_start.asm)
!CHAIN_ROUND_OR = $1B1E         ; ORed with the round for the chain's pick lists
!HEAD_CUE = $1B22               ; set when its own turn is done (chain_start.asm)
!V_0844 = $0844                 ; counted up at its start

; @routine 84:8629 Kind73Start
; @actor
; WHAT IT DOES: Actor kind $73, the head of the nine-piece chain: once the
; chain is built it takes its health (7, or $7F on a copier, whose
; cartridge RAM holds what is written there), starts and animates by its
; number, clears its touch pose and the words at K73_SLOTS, saves the
; palette reversed, shows no animation yet, and waits for good (its turns
; come from the chain's eighth piece, which writes its resume address).
; HOW: Each frame (BRK) until CHAIN_GO. With DB $70 (PHB, $80 less $10,
; PHA, PLB): $67 written at $0080 and $89 at $0100 and read back, both
; equal: A = $7F, else 7; PLB; K73_HEALTH = A; actor_spawn_at $88;
; K73_UNTOUCHABLE up; K73_POSE 0; ActorStartBy40, ActorAnimBy40; K73_29
; = $8919; K73_SLOTS words $1E down to 2 = $FFFF; PaletteSaveReversed 2;
; V_0844 up; K73_28 $FF; K73_ANIM 0, K73_ANIM_SHOWN $FF;
; ActorYieldForever.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $73
; in state $0C. Exit: never returns.
%at($848629)
Kind73Start:
    brk.b #$00
    lda.w !CHAIN_GO
    beq Kind73Start
    ldx.b #$80
    phb
    sec
    lda.b #!K73_COPIER_BANK
    sbc.b #$10
    pha
    plb
    lda.b #$67
    sta.w !K73_COPIER_TEST,x
    cmp.w !K73_COPIER_TEST,x
    bne .cartridge
    lda.b #$89
    sta.w !K73_COPIER_TEST_2,x
    cmp.w !K73_COPIER_TEST_2,x
    bne .cartridge
    lda.b #$7F
    bra .health
.cartridge:
    lda.b #$07
.health:
    plb
    sta.b !K73_HEALTH
    lda.b #$88
    sta.b !actor_spawn_at
    inc.w !K73_UNTOUCHABLE
    stz.w !K73_POSE
    jsr.w ActorStartBy40
    jsr.w ActorAnimBy40
    rep.b #$20
    lda.w #$8919
    sta.b !K73_29
    ldx.b #$1E
    lda.w #$FFFF
.slots:
    sta.l !K73_SLOTS,x
    dex
    dex
    bne .slots
    sep.b #$20
    lda.b #$02
    jsl.l PaletteSaveReversed
    inc.w !V_0844
    lda.b #$FF
    sta.b !K73_28
    lda.b #$00
    sta.w !K73_ANIM
    dec
    sta.w !K73_ANIM_SHOWN
    jmp.w ActorYieldForever

; @routine 84:8696 Kind73OwnTurn
; @actor
; WHAT IT DOES: Kind $73's own turn, where the chain's eighth piece sends
; it (its resume address): twice animation 4, a child of hit box $A1, a
; pause; then HEAD_CUE set for the piece, and it waits for good.
; HOW: K73_ANIM 4, $14 frames (BRK, K73_FRAMES down), ActorCreateChild8
; $A1, $1F frames; K73_ANIM 0, $18 frames; K73_ANIM 4, $14 frames,
; ActorCreateChild8 $A1, $1F frames; K73_ANIM 0, $81 frames; HEAD_CUE up;
; ActorYieldForever.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's RTI at the resume
; address chain_start.asm writes (HEAD_OWN_CODE). Exit: never returns.
%at($848696)
Kind73OwnTurn:
    lda.b #$04
    sta.w !K73_ANIM
    lda.b #$14
    sta.b !K73_FRAMES
.first:
    brk.b #$00
    dec.b !K73_FRAMES
    bne .first
    lda.b #$A1
    jsl.l ActorCreateChild8
    lda.b #$1F
    sta.b !K73_FRAMES
.after_first:
    brk.b #$00
    dec.b !K73_FRAMES
    bne .after_first
    stz.w !K73_ANIM
    lda.b #$18
    sta.b !K73_FRAMES
.between:
    brk.b #$00
    dec.b !K73_FRAMES
    bne .between
    lda.b #$04
    sta.w !K73_ANIM
    lda.b #$14
    sta.b !K73_FRAMES
.second:
    brk.b #$00
    dec.b !K73_FRAMES
    bne .second
    lda.b #$A1
    jsl.l ActorCreateChild8
    lda.b #$1F
    sta.b !K73_FRAMES
.after_second:
    brk.b #$00
    dec.b !K73_FRAMES
    bne .after_second
    stz.w !K73_ANIM
    lda.b #$81
    sta.b !K73_FRAMES
.last:
    brk.b #$00
    dec.b !K73_FRAMES
    bne .last
    inc.w !HEAD_CUE
    jmp.w ActorYieldForever

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

; @routine 84:8766 Kind73Hit
; @actor
; WHAT IT DOES: Kind $73 struck: below 0 health counted in K73_HITS; it
; shows being struck (animation 2, flashing) for $1F frames. Still
; standing: it flashes for $5F frames more (K73_FLASH), the chain's pick
; lists go to their second set (CHAIN_ROUND_OR 4) once its health is
; below 3, its animation is started again, and it goes back to what it
; was doing (ChainPieceEnd). Beaten (health negative): animation 3, its
; $09 bit 7 set, and it stays for good.
; HOW: K73_HEALTH negative: K73_HITS up. AnimStart 2,
; ActorPictureSendFromKind; K73_HIT_FRAMES $1F; each frame (BRK)
; K73_HIT_FRAMES down, ActorLookFlash, PaletteFlash, ActorParityBitsSet,
; AnimStep, ActorPictureSendFromKind until it is 0. K73_HEALTH positive:
; K73_FLASH $5F; below 3, CHAIN_ROUND_OR 4; K73_ANIM_SHOWN $FF; on into
; ChainPieceEnd. Negative: K73_ANIM 3, actor_09 OR $80, BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $73
; when struck. Exit: never returns.
%at($848766)
Kind73Hit:
    lda.b !K73_HEALTH
    bpl .shown
    inc.w !K73_HITS
.shown:
    lda.b #$02
    jsl.l AnimStart
    jsl.l ActorPictureSendFromKind
    lda.b #$1F
    sta.w !K73_HIT_FRAMES
.flash:
    brk.b #$00
    dec.w !K73_HIT_FRAMES
    jsr.w ActorLookFlash
    jsr.w PaletteFlash
    jsr.w ActorParityBitsSet
    jsl.l AnimStep
    jsl.l ActorPictureSendFromKind
    lda.w !K73_HIT_FRAMES
    bne .flash
    lda.b !K73_HEALTH
    bmi .beaten
    lda.b #$5F
    sta.w !K73_FLASH
    lda.b !K73_HEALTH
    cmp.b #$03
    bcs .again
    lda.b #$04
    sta.w !CHAIN_ROUND_OR
.again:
    lda.b #$FF
    sta.w !K73_ANIM_SHOWN
    jmp.w ChainPieceEnd
.beaten:
    lda.b #$03
    sta.w !K73_ANIM
    lda.b !actor_09
    ora.b #$80
    sta.b !actor_09
.stay:
    brk.b #$00
    bra .stay
