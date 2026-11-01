; controls.asm: the game's actions from pad 1, through the buttons the
; options assign them.
; C: src/aladdin/system/controls.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_pad.inc"

arch 65816

!control_masks = $0348          ; the options' button for each of the four actions (a mask word each)
!pad1_pressed_copy = $0381      ; a copy of pad 1's newly pressed buttons (word)
!action_held = $0383            ; per action, nonzero while its button is held
!action_pressed = $0387         ; per action, nonzero when its button was just pressed

; @routine 81:BDE9 ControlsRead
; WHAT IT DOES: Turns pad 1's buttons into the game's four actions, held
; and newly pressed, through the button chosen for each.
; HOW: Clears the eight action flags, copies the pad's held and pressed
; words, and counts each action whose button mask (from the options)
; matches.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
; Exit: M 8-bit; A = the pressed word (16 bits); N, V, Z from the last
; BIT; X, Y, C unchanged.
%at($81BDE9)
ControlsRead:
    stz.w !action_held+1
    stz.w !action_pressed+1
    stz.w !action_held
    stz.w !action_pressed
    stz.w !action_held+2
    stz.w !action_pressed+2
    stz.w !action_held+3
    stz.w !action_pressed+3
    rep.b #$20
    lda.w !joy_held
    sta.w !pad1_held
    lda.w !joy_pressed
    sta.w !pad1_pressed_copy
    lda.w !pad1_held
    bit.w !control_masks+2
    beq .lBE1A
    inc.w !action_held+1
.lBE1A:
    bit.w !control_masks
    beq .lBE22
    inc.w !action_held
.lBE22:
    bit.w !control_masks+4
    beq .lBE2A
    inc.w !action_held+2
.lBE2A:
    bit.w !control_masks+6
    beq .lBE32
    inc.w !action_held+3
.lBE32:
    lda.w !pad1_pressed_copy
    bit.w !control_masks+2
    beq .lBE3D
    inc.w !action_pressed+1
.lBE3D:
    bit.w !control_masks
    beq .lBE45
    inc.w !action_pressed
.lBE45:
    sep.b #$20
    bit.w !control_masks+4
    beq .lBE4F
    inc.w !action_pressed+2
.lBE4F:
    bit.w !control_masks+6
    beq .lBE57
    inc.w !action_pressed+3
.lBE57:
    sep.b #$20
    rts
