; ppu_init.asm: putting PPU registers into a known state.
; C: src/aladdin/video/ppu_init.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"

arch 65816

; @routine 80:80F6 PpuRegistersZero
; WHAT IT DOES: Writes 0 to a run of consecutive PPU registers.
; HOW: Stores 0 at $2100 + X, steps X and counts Y down, until the count
; goes negative: Y + 1 registers for a count of $00-$7F.
; Entry: M 8-bit, X 8-bit; X = the first register's offset from $2100,
; Y = the count less one; DB a bank whose $2100-$21FF are the PPU
; registers. Exit: as the C states.
%at($8080F6)
PpuRegistersZero:
    stz.w !INIDISP,x
    inx
    dey
    bpl PpuRegistersZero
    rts
