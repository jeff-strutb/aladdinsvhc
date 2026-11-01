; helpers.asm: small shared steps other routines call: a flag result, a
; shift, an empty return, a buffer clear.
; C: src/aladdin/system/helpers.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!WRAM_C100 = $7EC100            ; a word table the bank-$82 code keeps

; @routine 82:A78E ShiftRight5
; WHAT IT DOES: Divides the 16-bit A by 32, unsigned, rounding down.
; HOW: Five LSRs of the 16-bit A.
; Entry: M 16-bit. Exit: as the C states.
%at($82A78E)
ShiftRight5:
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    rts

; @routine 82:A978 WramC100Clear
; WHAT IT DOES: Clears the word table at $7E:C100 from its start up to the
; word at X.
; HOW: Stores 0 at $7E:C100 + X, stepping X down two bytes at a time
; while it stays positive.
; Entry: M 16-bit, X 16-bit; X = the last word's offset ($0000-$7FFF).
; Exit: as the C states.
%at($82A978)
WramC100Clear:
    lda.w #0
.clear:
    sta.l !WRAM_C100,x
    dex
    dex
    bpl .clear
    rts

; @routine 82:AA87 CarryClearReturn
; WHAT IT DOES: Returns with the carry clear, the "no" answer of the
; tests that share it.
; HOW: CLC.
; Entry: any. Exit: as the C states.
%at($82AA87)
CarryClearReturn:
    clc
    rts

; @routine A5:F533 LongReturn
; WHAT IT DOES: Nothing: a handler that has no work, called by JSL.
; HOW: RTL.
; Entry: any. Exit: as the C states.
%at($A5F533)
LongReturn:
    rtl

; @routine 83:84C7 ScratchBelowBy10
; WHAT IT DOES: Compares the scratch word at $0012, less 10, with A: C
; clear when it is below A.
; HOW: A is kept in the scratch word at $0014 for the compare.
; Entry: M 16-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($8384C7)
ScratchBelowBy10:
    sta.w !scratch+8
    sec
    lda.w !scratch+6
    sbc.w #10
    cmp.w !scratch+8
    rts
