; facing.asm: the way the joypad's Left and Right point.
; C: src/aladdin/player/facing.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_pad.inc"

arch 65816

; The facing for each pair of the joypad's Left and Right bits (Right 1,
; Left 2): 0 right, 1 left, $FF neither or both.
!pad_facing = $80C72C

; @routine 81:CE7C PadFacing
; WHAT IT DOES: Gives the way joypad 1's Left and Right point: 0 right,
; 1 left, or $FF (negative) when neither or both are held.
; HOW: Looks the held Left and Right bits up in a table of four.
; Entry: M 8-bit, X 8-bit; DB = $80. Exit: A low byte = the facing, N and
; Z from it; X = the Left and Right bits; C and the rest unchanged.
%at($81CE7C)
PadFacing:
    lda.w !pad1_held+1
    and.b #$03
    tax
    lda.w !pad_facing,x
    rts
