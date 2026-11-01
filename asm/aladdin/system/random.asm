; random.asm: the game's random numbers.
; C: src/aladdin/system/random.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 81:981B RandomNext
; WHAT IT DOES: Steps the random number: the next in its sequence.
; HOW: random * 3, plus its old low byte into the high byte (random *
; $0103 mod $10000), byte by byte through scratch $1A-$1B.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A low byte = the new high byte; N, V, Z, C from its ADC; scratch
; $1A-$1B = the old random * 2; X, Y unchanged.
%at($81981B)
RandomNext:
    lda.w !random
    pha
    sta.w !scratch+14
    lda.w !random+1
    sta.w !scratch+15
    asl.w !scratch+14
    rol.w !scratch+15
    clc
    lda.w !scratch+14
    adc.w !random
    sta.w !random
    lda.w !random+1
    adc.w !scratch+15
    sta.w !random+1
    pla
    clc
    adc.w !random+1
    sta.w !random+1
    rtl
