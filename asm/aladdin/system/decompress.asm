; decompress.asm: the graphics and map assets packed in ROM, unpacked into
; the second bank of work RAM: groups of a flag byte and a fill byte, then a
; byte for each set flag bit, each group unpacking to 8 bytes (bit 7 first:
; set, the next literal byte; clear, the fill byte). The source runs on
; across banks, past $FFFF at $8000 of the next.
; C: src/aladdin/system/decompress.c.

incsrc "../include/macros.inc"

arch 65816

; Direct page of the unpacking (D = 0).
!decomp_flags = $0C             ; group's flag byte, shifted out bit by bit
!decomp_fill = $0D              ; group's fill byte
!decomp_src = $65               ; source long pointer (low word 0: Y is the offset)
!decomp_groups = $69            ; groups left (word)
!decomp_out = $0000             ; the destination: X is its address in bank $7F

; @routine 81:BB49 DecompressGroup
; WHAT IT DOES: Unpacks one 8-byte group of a packed asset to $7F:X and
; counts it off.
; HOW: Takes the flag and fill bytes from the source, then for each of the
; 8 bytes shifts the flags: a set bit stores the next source byte, a clear
; one the fill byte. Each source step past $FFFF goes on at $8000 of the
; next bank. X moves on 8; the groups left go down by one.
; Entry: M 8-bit, X 16-bit; D = 0; DB = $7F; X = the destination; Y = the
; source offset in the bank at DECOMP_SRC + 2; called by JSR
; (DecompressAsset). Exit: X = the next destination, A = X (16 bits, from
; TXA/ADC), Y the next source offset; N and Z from the groups-left
; decrement (Z set: none left); C from the ADC; M 8-bit, X 16-bit.
%at($81BB49)
DecompressGroup:
    lda.b [!decomp_src],y
    sta.b !decomp_flags
    iny
    bne .bBB55
    ldy.w #$8000
    inc.b !decomp_src+2
.bBB55:
    lda.b [!decomp_src],y
    sta.b !decomp_fill
    iny
    bne .bBB61
    ldy.w #$8000
    inc.b !decomp_src+2
.bBB61:
    asl.b !decomp_flags
    bcs .bBB69
    lda.b !decomp_fill
    bra .bBB73
.bBB69:
    lda.b [!decomp_src],y
    iny
    bne .bBB73
    ldy.w #$8000
    inc.b !decomp_src+2
.bBB73:
    sta.w !decomp_out,x
    asl.b !decomp_flags
    bcs .bBB7E
    lda.b !decomp_fill
    bra .bBB88
.bBB7E:
    lda.b [!decomp_src],y
    iny
    bne .bBB88
    ldy.w #$8000
    inc.b !decomp_src+2
.bBB88:
    sta.w !decomp_out+1,x
    asl.b !decomp_flags
    bcs .bBB93
    lda.b !decomp_fill
    bra .bBB9D
.bBB93:
    lda.b [!decomp_src],y
    iny
    bne .bBB9D
    ldy.w #$8000
    inc.b !decomp_src+2
.bBB9D:
    sta.w !decomp_out+2,x
    asl.b !decomp_flags
    bcs .bBBA8
    lda.b !decomp_fill
    bra .bBBB2
.bBBA8:
    lda.b [!decomp_src],y
    iny
    bne .bBBB2
    ldy.w #$8000
    inc.b !decomp_src+2
.bBBB2:
    sta.w !decomp_out+3,x
    asl.b !decomp_flags
    bcs .bBBBD
    lda.b !decomp_fill
    bra .bBBC7
.bBBBD:
    lda.b [!decomp_src],y
    iny
    bne .bBBC7
    ldy.w #$8000
    inc.b !decomp_src+2
.bBBC7:
    sta.w !decomp_out+4,x
    asl.b !decomp_flags
    bcs .bBBD2
    lda.b !decomp_fill
    bra .bBBDC
.bBBD2:
    lda.b [!decomp_src],y
    iny
    bne .bBBDC
    ldy.w #$8000
    inc.b !decomp_src+2
.bBBDC:
    sta.w !decomp_out+5,x
    asl.b !decomp_flags
    bcs .bBBE7
    lda.b !decomp_fill
    bra .bBBF1
.bBBE7:
    lda.b [!decomp_src],y
    iny
    bne .bBBF1
    ldy.w #$8000
    inc.b !decomp_src+2
.bBBF1:
    sta.w !decomp_out+6,x
    asl.b !decomp_flags
    bcs .bBBFC
    lda.b !decomp_fill
    bra .bBC06
.bBBFC:
    lda.b [!decomp_src],y
    iny
    bne .bBC06
    ldy.w #$8000
    inc.b !decomp_src+2
.bBC06:
    sta.w !decomp_out+7,x
    rep.b #$20
    clc
    txa
    adc.w #$0008
    tax
    dec.b !decomp_groups
    sep.b #$20
    rts
