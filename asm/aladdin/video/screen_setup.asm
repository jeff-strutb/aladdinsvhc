; screen_setup.asm: the display's settings and the scroll state put back
; to their defaults before a screen is built.
; C: src/aladdin/video/screen_setup.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; Display shadows the NMI copies whose registers are not yet named.
!display_038b = $038B
!display_038c = $038C
!display_039d = $039D
!display_03a0 = $03A0
!display_03a1 = $03A1
!display_03a2 = $03A2
!display_03a4 = $03A4
!display_03a5 = $03A5
!display_03aa = $03AA
!display_03ab = $03AB
!display_03ad = $03AD
!display_03b1 = $03B1
!display_03b6 = $03B6
!display_0829 = $0829
; Scroll state: the camera's words and the layers' offsets.
!scroll_1772 = $1772
!scroll_1774 = $1774
!scroll_1776 = $1776
!scroll_1778 = $1778
!scroll_18c8 = $18C8
!scroll_18ca = $18CA
!scroll_18cc = $18CC
!scroll_18ce = $18CE
!scroll_19ec = $19EC
!scroll_19ee = $19EE
!scroll_19f0 = $19F0
!scroll_19f2 = $19F2
!scroll_19f4 = $19F4
!scroll_19f6 = $19F6
!scroll_19f8 = $19F8
!scroll_19fa = $19FA
!scroll_19fc = $19FC
!scroll_19fe = $19FE
!scroll_1a00 = $1A00
!scroll_1a02 = $1A02
!scroll_1a1e = $1A1E
!scroll_1a22 = $1A22

; @routine 81:96CB ScrollStateClear
; WHAT IT DOES: Clears the scroll state: the camera's words at $1772-$1779
; and $18C8-$18CF, the BG2 shifts and their neighbours at $1A1E-$1A25, and
; the words at $19EC-$1A03.
; HOW: A 16-bit STZ to each word.
; Entry: DB = $00-$3F or $80-$BF; called by JSL. Exit: M 8-bit; A, X, Y
; and flags but M as at entry.
%at($8196CB)
ScrollStateClear:
    rep.b #$20
    stz.w !scroll_1772
    stz.w !scroll_1774
    stz.w !scroll_1776
    stz.w !scroll_1778
    stz.w !scroll_18c8
    stz.w !scroll_18ca
    stz.w !scroll_18cc
    stz.w !scroll_18ce
    stz.w !scroll_1a1e
    stz.w !bg2_x_shift
    stz.w !scroll_1a22
    stz.w !bg2_y_shift
    stz.w !scroll_19ec
    stz.w !scroll_19ee
    stz.w !scroll_19f0
    stz.w !scroll_19f2
    stz.w !scroll_19f4
    stz.w !scroll_19f6
    stz.w !scroll_19f8
    stz.w !scroll_19fa
    stz.w !scroll_19fc
    stz.w !scroll_19fe
    stz.w !scroll_1a00
    stz.w !scroll_1a02
    sep.b #$20
    rtl


; @routine 81:9865 DisplayDefaults
; WHAT IT DOES: Puts the display's settings back to their defaults:
; sprites from VRAM $6000 (OBSEL = 3), colour math's shadows to their
; start values, the fixed colour black, windows, HDMA and mode 7's
; settings off, and only the NMI enabled.
; HOW: Writes OBSEL, COLDATA, SETINI, HDMAEN and M7SEL, and the shadows
; the NMI copies.
; Entry: M 8-bit; DB = $00-$3F or $80-$BF; called by JSL. Exit: A low
; byte = 1; X, Y unchanged.
%at($819865)
DisplayDefaults:
    lda.b #$03
    sta.w !OBSEL
    lda.b #$30
    sta.w !cgwsel_shadow
    lda.b #$09
    sta.w !display_039d
    lda.b #$49
    sta.w !display_03a0
    lda.b #$41
    sta.w !display_03a1
    lda.b #$5C
    sta.w !display_03a2
    lda.b #$00
    sta.w !display_03a4
    lda.b #$05
    sta.w !display_03a5
    lda.b #$E0
    sta.w !COLDATA
    stz.w !coldata_red
    stz.w !coldata_green
    stz.w !coldata_blue
    stz.w !display_038b
    stz.w !display_038c
    stz.w !display_03aa
    stz.w !display_03ab
    stz.w !wobjsel_shadow
    stz.w !display_03ad
    stz.w !display_03b1
    stz.w !SETINI
    stz.w !display_03b6
    stz.w !HDMAEN
    stz.w !M7SEL
    stz.w !display_0829
    lda.b #$01
    sta.w !nmitimen_shadow
    rtl
