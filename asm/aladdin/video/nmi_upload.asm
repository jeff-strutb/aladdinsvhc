; nmi_upload.asm: what the NMI sends to the PPU each frame: the text buffer
; when asked for, and the scroll positions.
; C: src/aladdin/video/nmi_upload.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram_video.inc"

arch 65816

; @routine 80:8469 TextBufferUpload
; WHAT IT DOES: Sends the text buffer to VRAM when one was asked for since
; the last NMI.
; HOW: When the pending flag is set it is cleared, and DMA channel 0
; copies the requested number of bytes from $7E:D800 to VMDATA at the
; requested VRAM address, the VRAM address stepping after each high byte.
; The bank store writes the low byte of the count too ($00), before the
; count is stored.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $2100-$43FF are the registers. Exit: as the C states.
%at($808469)
TextBufferUpload:
    lda.w !text_upload_pending
    beq .done
    lda.b #$80
    sta.w !VMAIN
    stz.w !text_upload_pending
    rep.b #$20
    lda.w !text_upload_vram
    sta.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    lda.w #!text_buffer&65535
    sta.w !A1T0L
    lda.w #!text_buffer>>16
    sta.w !A1B0
    lda.w !text_upload_size
    sta.w !DAS0L
    sep.b #$20
    lda.b #$01
    sta.w !MDMAEN
.done:
    rts

; @routine 80:8793 ScrollRegistersWrite
; WHAT IT DOES: Sets the PPU's scroll registers for BG1, BG2 and BG3 from
; the game's scroll positions.
; HOW: BG1's and BG2's positions go out with their shifts added (each a
; 16-bit sum, low byte then high, to the write-twice register); BG2's
; horizontal register is skipped while it is held. For BG2's horizontal
; high byte the original adds the shift's low byte again, not its high
; byte. BG3's positions go out as they are.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM and whose
; $2100-$21FF are the PPU registers. Exit: as the C states.
%at($808793)
ScrollRegistersWrite:
    clc
    lda.w !bg1_x
    adc.w !bg1_x_shift
    sta.w !BG1HOFS
    lda.w !bg1_x+1
    adc.w !bg1_x_shift+1
    sta.w !BG1HOFS
    clc
    lda.w !bg1_y
    adc.w !bg1_y_shift
    sta.w !BG1VOFS
    lda.w !bg1_y+1
    adc.w !bg1_y_shift+1
    sta.w !BG1VOFS
    lda.w !bg2_x_held
    bne .bg2_y
    clc
    lda.w !bg2_x
    adc.w !bg2_x_shift
    sta.w !BG2HOFS
    lda.w !bg2_x+1
    adc.w !bg2_x_shift
    sta.w !BG2HOFS
.bg2_y:
    clc
    lda.w !bg2_y
    adc.w !bg2_y_shift
    sta.w !BG2VOFS
    lda.w !bg2_y+1
    adc.w !bg2_y_shift+1
    sta.w !BG2VOFS
    clc
    lda.w !bg3_x
    sta.w !BG3HOFS
    lda.w !bg3_x+1
    sta.w !BG3HOFS
    lda.w !bg3_y
    sta.w !BG3VOFS
    lda.w !bg3_y+1
    sta.w !BG3VOFS
    rts
