; level_bits.asm: the eight bytes at $7E:F260, set at a level's start from
; the bits of a table byte picked by $035D.
; C: src/aladdin/level/level_bits.c.

incsrc "../include/macros.inc"

arch 65816

; Sets the byte at $7E:F260 + X from bit X of the table byte by $035D.
!BIT_TO_BYTE = $83C2F0

; @routine 83:C2E7 Bits035DExpand
; WHAT IT DOES: Sets the eight bytes at $7E:F260 from the eight bits of the
; byte for $035D in the table at $80:D27E, last first.
; HOW: The routine at $83:C2F0 for X = 7 down to 0.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: as the C states.
%at($83C2E7)
Bits035DExpand:
    ldx.b #$07
.next:
    jsr.w !BIT_TO_BYTE
    dex
    bpl .next
    rtl
