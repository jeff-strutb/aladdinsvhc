; logo_screen.asm: the logo screen the game shows at power on, before the
; title.
; C: src/aladdin/game/logo_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_pad.inc"

arch 65816

!tm_shadow = $0399               ; TM's shadow: the main screen's layers
!ts_shadow = $039A               ; TS's shadow: the sub screen's layers
!LOGO_KEYS = $002C               ; scratch: pad 1's A, X, L and R held, as the high nibble

; @routine A5:E000 LogoScreen
; WHAT IT DOES: The logo screen at power on: the logo shown with its tune,
; faded in, its palettes played through, held for two seconds or until a
; button, and faded out.
; HOW: With the NMI off and the screen blanked: scroll and sprites
; cleared, the display's defaults, graphics set 3 unpacked and VRAM copies
; 7 and $0E, layer 1 alone on the main and sub screens, palette 3; NMI on,
; song $10. After 60 frames ColourFadeInTask in slot 1 (parameter 2); when
; it has ended, INIDISP full bright and 31 frames; PaletteSequenceTask in
; slot 2, waited out; then up to 120 frames (TASK_PARAM counts) or until
; pad 1 holds B, Y, Select, Start, A, X, L or R; ColourFadeOutTask in
; slot 1 (parameter 1), waited out.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSL from a task (it waits). Exit: M 8-bit, X 8-bit; A = slot 1's
; state (0), Z set; TASK_PARAM (slot 0's parameter, the count) changed.
%at($A5E000)
LogoScreen:
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsl.l ScrollStateClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    lda.b #$03
    jsl.l UnpackSet
    ldx.b #$07
    jsl.l VramCopy
    ldx.b #$0E
    jsl.l VramCopy
    lda.b #$01
    sta.w !tm_shadow
    sta.w !ts_shadow
    lda.b #$03
    jsl.l PaletteLoad
    jsl.l NmiEnable
    lda.b #$10
    jsl.l SongPlay
    lda.b #$3C
    jsl.l TaskWait
    jsl.l TaskCreate
    db 1*24, 2                  ; slot 1, parameter 2
    dl ColourFadeInTask
.fade_in:
    lda.b #$01
    jsl.l TaskWait
    lda.w !task_state+(1*24)
    bne .fade_in
    lda.b #$0F
    sta.w !inidisp_shadow
    lda.b #$1F
    jsl.l TaskWait
    jsl.l TaskCreate
    db 2*24, 0                  ; slot 2, parameter 0
    dl PaletteSequenceTask
.shine:
    lda.b #$01
    jsl.l TaskWait
    lda.w !task_state+(2*24)
    bne .shine
    lda.b #$78
    sta.w !task_param
.hold:
    lda.b #$01
    jsl.l TaskWait
    lda.w !pad1_held+1
    and.b #$F0
    sta.w !LOGO_KEYS
    lda.w !pad1_held
    and.b #$F0
    ora.w !LOGO_KEYS
    bne .fade_out
    dec.w !task_param
    bne .hold
.fade_out:
    jsl.l TaskCreate
    db 1*24, 1                  ; slot 1, parameter 1
    dl ColourFadeOutTask
.faded:
    lda.b #$01
    jsl.l TaskWait
    lda.w !task_state+(1*24)
    bne .faded
    rtl
