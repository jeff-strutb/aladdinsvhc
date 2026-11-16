; load_list.asm: the list a screen is loaded from: a pointer stepped through
; its bytes (graphics, VRAM copies, palettes). Drafted by cdraft.py; named
; and described by hand.
; C: src/aladdin/video/load_list.c.

incsrc "../include/macros.inc"

arch 65816

!LOAD_LIST_PTR = $4C            ; in the direct page: the list's next byte (word)
!TEXT_TYPE_PTR = $50            ; in the direct page: the typed text's next character (word, in the data bank)

; @routine 81:F303 LoadListNext
; WHAT IT DOES: Moves the load list's pointer on to its next byte.
; HOW: The word at D + $4C stepped up with 16-bit M.
; Entry: M 8-bit; called by JSR from the list's reader (81:F1A4), which
; reads each byte through the pointer. Exit: M 8-bit; A unchanged; N, Z
; from the new pointer.
%at($81F303)
LoadListNext:
    rep.b #$20
    inc.b !LOAD_LIST_PTR
    sep.b #$20
    rts

; @routine 81:F30A TextTypeNext
; WHAT IT DOES: Moves the typed text's pointer on to its next character.
; HOW: The word at D + $50 stepped up with 16-bit M.
; Entry: M 8-bit; called by JSR from the text typing task (81:F243,
; 81:F27E, 81:F292, 81:F299), which reads each character through the
; pointer. Exit: M 8-bit; A unchanged; N, Z from the new pointer.
%at($81F30A)
TextTypeNext:
    rep.b #$20
    inc.b !TEXT_TYPE_PTR
    sep.b #$20
    rts
