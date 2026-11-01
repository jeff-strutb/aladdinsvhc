; options.asm: the option settings work RAM keeps across a reset.
;
; Power-on keeps the three option bytes when work RAM still holds the
; signature and the cartridge title; the sum beside them is kept up to
; date whenever the options screen changes one.
; C: src/aladdin/system/options.c.

incsrc "../include/macros.inc"
incsrc "../include/wram_state.inc"

arch 65816

; @routine 80:80FE KeptOptionsSum
; WHAT IT DOES: Adds up the three kept option bytes.
; HOW: The second byte plus the first, then plus the third, each addition
; with the carry cleared first (decimal when D is set, as the CPU adds).
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C
; states.
%at($8080FE)
KeptOptionsSum:
    clc
    lda.w !kept_options+1
    adc.w !kept_options
    clc
    adc.w !kept_options+2
    rts

; @routine 80:810A KeptOptionsSeal
; WHAT IT DOES: Stores the kept option bytes' sum beside them, after one
; of them has changed.
; HOW: KeptOptionsSum, then the sum to its byte.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSL.
; Exit: as KeptOptionsSum.
KeptOptionsSeal:
    jsr.w KeptOptionsSum
    sta.w !kept_options_sum
    rtl
