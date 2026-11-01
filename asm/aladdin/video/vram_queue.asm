; vram_queue.asm: the VRAM uploads the game asks for between frames, which
; the NMI makes by DMA (channel 0) or by VMDATA writes while the screen is
; in V-blank.
; C: src/aladdin/video/vram_queue.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram_video.inc"

arch 65816

; In the bank DB names (the NMI's, $80): text buffer parts, a byte offset
; for each of 3 parts and then their 6-byte records (VRAM address, source
; in bank $7E, byte count); uploads by number, a byte offset for each and
; then their 10-byte records (VMAIN, VRAM address, DMAP0 and BBAD0,
; source address, then its bank and the byte count's low byte as one
; word, then the count's high byte).
!TEXT_PART_TABLE = $B61B
!UPLOAD_TABLE_INDEX = $B62F
!UPLOAD_TABLE = $B630

; @routine 80:841E VramQueueFlush
; WHAT IT DOES: Makes every DMA upload queued since the last NMI.
; HOW: When the queue is marked pending, each of its count records (8
; bytes: VRAM address, source address, bank, byte count, VMAIN) sets DMA
; channel 0 to VMDATA and starts it, VMAIN set last; then the count and
; the mark are cleared. The bank store writes the count's low byte too,
; before the count is stored.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $2100-$43FF are the registers. Exit: as the C states.
%at($80841E)
VramQueueFlush:
    lda.w !vram_queue_pending
    beq .done
    ldy.w !vram_queue_count
    beq .clear
    ldx.b #0
    rep.b #$20
.record:
    lda.w !vram_queue,x
    sta.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    lda.w !vram_queue+2,x
    sta.w !A1T0L
    lda.w !vram_queue+4,x
    sta.w !A1B0
    lda.w !vram_queue+5,x
    sta.w !DAS0L
    phy
    ldy.w !vram_queue+7,x
    sty.w !VMAIN
    ldy.b #$01
    sty.w !MDMAEN
    ply
    clc
    txa
    adc.w #8
    tax
    dey
    bne .record
    sep.b #$20
    stz.w !vram_queue_count
.clear:
    stz.w !vram_queue_pending
.done:
    rts

; @routine 80:84AA TextBufferPartUpload
; WHAT IT DOES: Sends one part of the text buffer to VRAM when it was
; asked for since the last NMI.
; HOW: X picks the part and its pending flag. When the flag is set it is
; cleared, and the part's record gives the VRAM address, the source in
; bank $7E and the byte count for DMA channel 0 to VMDATA, the address
; stepping after each high byte. The bank store writes the count's low
; byte too ($00), before the count is stored.
; Entry: M 8-bit, X 8-bit; X = the part (0-2); DB $80 (the table's bank,
; with work RAM at $0000-$1FFF and the registers). Exit: as the C states.
%at($8084AA)
TextBufferPartUpload:
    lda.w !text_parts_pending,x
    beq .done
    lda.b #$80
    sta.w !VMAIN
    stz.w !text_parts_pending,x
    ldy.w !TEXT_PART_TABLE,x
    rep.b #$20
    lda.w !TEXT_PART_TABLE,y
    sta.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    lda.w !TEXT_PART_TABLE+2,y
    sta.w !A1T0L
    lda.w #!text_buffer>>16
    sta.w !A1B0
    lda.w !TEXT_PART_TABLE+4,y
    sta.w !DAS0L
    sep.b #$20
    lda.b #$01
    sta.w !MDMAEN
.done:
    rts

; @routine 80:84E2 VramUploadByNumber
; WHAT IT DOES: Makes the VRAM upload the game asked for by number since
; the last NMI, if any.
; HOW: The number (1 up) picks a record whose bytes set VMAIN, the VRAM
; address, DMA channel 0's mode and B-bus register, source address, bank
; and byte count; the request is cleared and the DMA started. The source
; bank and the count's low byte go out as one word, the count's high byte
; on its own.
; Entry: M 8-bit, X 8-bit; DB $80 (the table's bank, with work RAM at
; $0000-$1FFF and the registers). Exit: as the C states.
VramUploadByNumber:
    ldy.w !vram_upload_number
    beq .done
    stz.w !vram_upload_number
    ldx.w !UPLOAD_TABLE_INDEX,y
    rep.b #$20
    ldy.w !UPLOAD_TABLE,x
    sty.w !VMAIN
    lda.w !UPLOAD_TABLE+1,x
    sta.w !VMADDL
    lda.w !UPLOAD_TABLE+3,x
    sta.w !DMAP0
    lda.w !UPLOAD_TABLE+5,x
    sta.w !A1T0L
    lda.w !UPLOAD_TABLE+7,x
    sta.w !A1B0
    ldy.w !UPLOAD_TABLE+9,x
    sty.w !DAS0H
    sep.b #$20
    lda.b #$01
    sta.w !MDMAEN
.done:
    rts

; @routine 80:87FE TileBlockQueueFlush
; WHAT IT DOES: Writes every 16x16 tilemap block queued since the last
; NMI into VRAM.
; HOW: When blocks are pending, each 10-byte record gives a VRAM address
; and four tilemap words: two at the address, two one row (32 words) on.
; The writes go through VMDATA, the address stepping after each high
; byte. Then the queue's length and the pending flag are cleared.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $2100-$21FF are the PPU registers. Exit: as the C states.
%at($8087FE)
TileBlockQueueFlush:
    lda.w !tile_blocks_pending
    beq .done
    ldx.w !tile_blocks_length
    beq .clear
    lda.b #$80
    sta.w !VMAIN
    ldx.b #0
    rep.b #$20
.block:
    lda.w !tile_blocks,x
    pha
    sta.w !VMADDL
    lda.w !tile_blocks+2,x
    sta.w !VMDATAL
    lda.w !tile_blocks+4,x
    sta.w !VMDATAL
    pla
    clc
    adc.w #32
    sta.w !VMADDL
    lda.w !tile_blocks+6,x
    sta.w !VMDATAL
    lda.w !tile_blocks+8,x
    sta.w !VMDATAL
    clc
    txa
    adc.w #10
    tax
    cpx.w !tile_blocks_length
    bne .block
    sep.b #$20
    stz.w !tile_blocks_length
.clear:
    stz.w !tile_blocks_pending
.done:
    rts

; @routine 80:8899 VramListUpload
; WHAT IT DOES: Makes a list of DMA uploads to VRAM that a sprite or
; effect slot has asked for.
; HOW: Y picks the slot. When its pending byte is set it is cleared, and
; the slot's pointer (in the bank DB names) leads to 8-byte records: VRAM
; address, source address, bank, byte count, and a byte that is nonzero
; when another record follows. Each sets DMA channel 0 to VMDATA, VMAIN
; $80, and starts it. X and Y are 16-bit while it works.
; Entry: M 8-bit, X 8-bit; Y = the slot's offset; DB a bank whose
; $0000-$1FFF is work RAM and whose $2100-$43FF are the registers, the
; bank of the records. Exit: as the C states.
%at($808899)
VramListUpload:
    lda.w !vram_list_pending,y
    bne .pending
    rts
.pending:
    lda.b #0
    sta.w !vram_list_pending,y
    rep.b #$10
    ldx.w !vram_list_pointer,y
.record:
    ldy.w 0,x
    sty.w !VMADDL
    ldy.w #$1801
    sty.w !DMAP0
    ldy.w 2,x
    sty.w !A1T0L
    lda.w 4,x
    sta.w !A1B0
    ldy.w 5,x
    sty.w !DAS0L
    lda.b #$80
    sta.w !VMAIN
    lda.b #$01
    sta.w !MDMAEN
    lda.w 7,x
    beq .done
    clc
    rep.b #$20
    txa
    adc.w #8
    tax
    sep.b #$20
    bra .record
.done:
    sep.b #$10
    rts
