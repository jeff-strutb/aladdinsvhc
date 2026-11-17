; kind_a7.asm: actor kind $A7, a figure of the ending: it waits (for a
; cue, by its $07), is placed, fades its palette up out of black with
; colour math, and then stays, or (with $07 set) rises 64 pixels and stays.
; C: src/aladdin/actor/kind_a7.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"

arch 65816

!CHAIN_HEAD = $1B14             ; cleared, then awaited by a $07 figure (set by another actor)
!KA7_FADE = $40                 ; the fade's step: -$20 up to 8
!KA7_TOP = $42                  ; a $07 figure: the y it rises to (word)
!KA7_PLACES = $D99E             ; (data bank) by $07 * 4: x, y (words; on the screen for $07 > 0)
!SUB_LAYERS = $039A             ; layers kept for the subscreen while the fade runs

; @routine 84:AC66 KindA7Start
; @actor
; WHAT IT DOES: Actor kind $A7, a figure of the ending. $07 0: three
; waits of 255 frames; $07 set: CHAIN_HEAD cleared, then awaited, and 96
; frames. Then pose 0 at its place (KA7_PLACES by $07; for $07 set, from
; the screen's corner), its sprite palette faded up from black in steps
; of 6 frames (4 for $07 set) under colour math, and: $07 0 waits 255 and
; 192 frames and stays; $07 set waits 255 and 152 frames, rises 64 pixels
; half a pixel a frame, and stays.
; HOW: $07 set ($84:AC74): COP $60 after CHAIN_HEAD. AnimStart 0; X =
; $07 * 4; $0C, $0F from KA7_PLACES (+ the scroll for X > 0, $84:AC95);
; ACTOR_09 $20; PaletteSave of one palette (8 + its palette bits);
; KA7_FADE $E0, PaletteFadeStep; CGWSEL 2, CGADSUB $12, SUB_LAYERS $10;
; each step COP 6 (4: $84:ACE5), KA7_FADE up, PaletteFadeStep, until 8;
; CGWSEL $30, CGADSUB 0; ACTOR_09 bits 4-5 cleared. $07 0: COP $FF, COP
; $C0, BRK for good. $07 set ($84:AD19): COP $FF, COP $98, KA7_TOP = y -
; $40; each frame (BRK) y less $0080 (24-bit, from $0E) until it is below
; KA7_TOP; BRK for good. The $07 set paths no script meets yet: written
; from their bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds KA7_PLACES (bank $84); reached by
; the actor loop's jump for kind $A7 in state $0C (the table at $83:8DAB,
; through the stub at $83:9442). Exit: never returns.
%at($84AC66)
KindA7Start:
    lda.b !actor_07
    beq .long_waits
    stz.w !CHAIN_HEAD
.until_head:
    brk.b #$00
    lda.w !CHAIN_HEAD
    beq .until_head
    lda.b #$60
    cop.b #$00
    bra .shown
.long_waits:
    lda.b #$FF
    cop.b #$00
    lda.b #$FF
    cop.b #$00
    lda.b #$FF
    cop.b #$00
.shown:
    lda.b #$00
    jsl.l AnimStart
    rep.b #$20
    lda.b !actor_07
    asl a
    asl a
    tax
    beq .absolute
    clc
    lda.w !layer0+!LAYER_SCROLL_X
    adc.w !KA7_PLACES,x
    sta.b !actor_x+1
    clc
    lda.w !layer0+!LAYER_SCROLL_Y
    adc.w !KA7_PLACES+2,x
    sta.b !actor_y+1
    bra .placed
.absolute:
    lda.w !KA7_PLACES,x
    sta.b !actor_x+1
    lda.w !KA7_PLACES+2,x
    sta.b !actor_y+1
.placed:
    sep.b #$20
    lda.b #$20
    sta.b !actor_09
    lda.b !actor_look+1
    and.b #$0E
    lsr a
    ora.b #$08
    ldy.b #$01
    jsl.l PaletteSave
    lda.b #$E0
    sta.b !KA7_FADE
    ldy.b #$01
    jsl.l PaletteFadeStep
    lda.b #$02
    sta.w !cgwsel_shadow
    lda.b #$12
    sta.w !cgadsub_shadow
    lda.b #$10
    sta.w !SUB_LAYERS
.fade:
    lda.b #$06
    ldx.b !actor_07
    beq .step
    lda.b #$04
.step:
    cop.b #$00
    inc.b !KA7_FADE
    lda.b !KA7_FADE
    ldy.b #$01
    jsl.l PaletteFadeStep
    lda.b !KA7_FADE
    cmp.b #$08
    bne .fade
    lda.b #$30
    sta.w !cgwsel_shadow
    stz.w !cgadsub_shadow
    lda.b !actor_09
    and.b #$CF
    ora.b #$00
    sta.b !actor_09
    lda.b !actor_07
    bne .rise
    lda.b #$FF
    cop.b #$00
    lda.b #$C0
    cop.b #$00
.stay:
    brk.b #$00
    bra .stay
.rise:
    lda.b #$FF
    cop.b #$00
    lda.b #$98
    cop.b #$00
    rep.b #$20
    sec
    lda.b !actor_y+1
    sbc.w #$0040
    sta.b !KA7_TOP
    sep.b #$20
.rising:
    brk.b #$00
    sec
    lda.b !actor_y
    sbc.b #$80
    sta.b !actor_y
    lda.b !actor_y+1
    sbc.b #$00
    sta.b !actor_y+1
    lda.b !actor_y+2
    sbc.b #$00
    sta.b !actor_y+2
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !KA7_TOP
    sep.b #$20
    bcs .rising
.risen:
    brk.b #$00
    bra .risen
