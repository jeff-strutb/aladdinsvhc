; vram_words.asm: single words written to VRAM in the vertical blank: a
; list of address and value pairs the frame's code builds ($81:9A64 adds
; to it).
; C: src/aladdin/video/vram_words.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"

arch 65816

!VRAM_WORDS_PENDING = $0616     ; nonzero: the list waits for the upload
!VRAM_WORDS_COUNT = $0617       ; how many pairs the list holds
!VRAM_WORDS = $0619             ; the list: VRAM address, then the word (4 bytes a pair)
!DMA_FOUR_REGS = $04            ; DMAP: four B-bus registers in turn
!VMAIN_STEP_HIGH = $80          ; VMAIN: step after the high byte

; @routine 80:884C VramWordsUpload
; WHAT IT DOES: Writes the list of single VRAM words, when one is
; pending: each pair's word goes to its own VRAM address.
; HOW: The pending flag is cleared; DMA channel 0, four registers in turn
; from VMADDL (address low, high, data low, high), copies count * 4 bytes
; of the list from bank $00.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose
; $2100-$21FF and $4300-$430F are the PPU's and the DMA's. Exit: as the C states.
%at($80884C)
VramWordsUpload:
    lda.w !VRAM_WORDS_PENDING
    beq .done
    stz.w !VRAM_WORDS_PENDING
    lda.b #!VMAIN_STEP_HIGH
    sta.w !VMAIN
    lda.b #!DMA_FOUR_REGS
    sta.w !DMAP0
    lda.b #!VMADDL&255
    sta.w !BBAD0
    rep.b #$20
    lda.w #!VRAM_WORDS
    sta.w !A1T0L
    lda.w #$0000
    sta.w !A1B0
    sep.b #$20
    lda.w !VRAM_WORDS_COUNT
    asl a
    asl a
    sta.w !DAS0L
    sep.b #$20
    lda.b #$01
    sta.w !MDMAEN
.done:
    rts
