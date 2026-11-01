; joypad.asm: reading the joypads once a frame.
;
; The hardware reads the pads by itself after each vertical blank (auto
; joypad read). The game keeps the buttons held ($0377) and the buttons
; newly pressed this frame ($037B), a word each for pads 1 and 2.
; C: src/aladdin/system/joypad.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 81:BDCC JoypadRead
; WHAT IT DOES: Takes this frame's buttons from pads 1 and 2: the ones
; held, and the ones pressed since the last read.
; HOW: Waits for the hardware's automatic read to finish, then for each of
; the four bytes (pad 2 high to pad 1 low): newly pressed = pressed now
; and not held before; held = pressed now.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $4212-$421B are the CPU's registers; called by JSR.
; Exit: X = $FF; A low byte = pad 1's low byte; N set, Z clear (DEX);
; C clear (the wait's LSR); Y, V unchanged.
%at($81BDCC)
JoypadRead:
    lda.w !HVBJOY
    lsr a
    bcs JoypadRead
    ldx.b #3
.byte:
    lda.w !joy_held,x
    eor.b #$FF
    and.w !JOY1L,x
    sta.w !joy_pressed,x
    lda.w !JOY1L,x
    sta.w !joy_held,x
    dex
    bpl .byte
    rts
