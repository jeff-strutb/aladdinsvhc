; load_list.asm: the list a screen is loaded from: a pointer stepped through
; its bytes (graphics, VRAM copies, palettes). Drafted by cdraft.py; named
; and described by hand.
; C: src/aladdin/video/load_list.c.

incsrc "../include/macros.inc"

arch 65816

!LOAD_LIST_PTR = $4C            ; in the direct page: the list's next byte (word)

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
