; display.asm: switching the vertical-blank interrupt and the screen.
;
; The game keeps shadows of NMITIMEN and INIDISP in work RAM; the NMI
; writes INIDISP's shadow to the register every frame.
; C: src/aladdin/system/display.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 81:969F NmiEnable
; WHAT IT DOES: Turns the vertical-blank interrupt on, with the rest of
; NMITIMEN as its shadow says.
; HOW: Writes the shadow with bit 7 set to NMITIMEN.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose $4200
; is NMITIMEN. Exit: A low byte = the value written; N set, Z clear; X, Y,
; C, V unchanged.
%at($81969F)
NmiEnable:
    lda.w !nmitimen_shadow
    ora.b #$80
    sta.w !NMITIMEN
    rtl

; @routine 81:96A8 NmiDisable
; WHAT IT DOES: Turns the vertical-blank interrupt off, keeping the
; automatic joypad read as its shadow says.
; HOW: Writes the shadow's bit 0 alone to NMITIMEN.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose $4200
; is NMITIMEN. Exit: A low byte = the value written; N clear, Z from it;
; X, Y, C, V unchanged.
NmiDisable:
    lda.w !nmitimen_shadow
    and.b #$01
    sta.w !NMITIMEN
    rtl

; @routine 81:96C5 ScreenFullBright
; WHAT IT DOES: Shows the screen at full brightness from the next vertical
; blank.
; HOW: INIDISP's shadow = $0F (no forced blank, brightness 15).
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A low byte = $0F; N, Z clear; X, Y, C, V unchanged.
%at($8196C5)
ScreenFullBright:
    lda.b #$0F
    sta.w !inidisp_shadow
    rtl
