; unpack.asm: setting up the unpacking of packed data (graphics) into
; bank $7F.
;
; An unpack reads its source through the long pointer at $65 (offset 0 in
; the source's bank, the offset itself in Y) and writes 8 bytes a group to
; bank $7F at X; $69 counts the groups. A group is a flag byte and a fill
; byte, then a byte for each set flag bit (bit 7 first: set, the next
; literal byte; clear, the fill byte). The source runs on across banks,
; past $FFFF at $8000 of the next.
; C: src/aladdin/video/unpack.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; Direct page (D = 0).
!unpack_source = $65             ; the source's pointer (long)
!unpack_groups = $69             ; the 8-byte groups left (word)
!unpack_flags = $0C              ; the group's flag byte, shifted out bit by bit
!unpack_fill = $0D               ; the group's fill byte
!unpack_out = $0000              ; the destination: X is its address in bank $7F
; Bank $80: per set, 7 bytes: destination, source offset (words), bank,
; length (word).
!unpack_sets = $F039
!UNPACK_SET_BANK = $80
!UNPACK_DEST_BANK = $7F

; @routine 81:BB04 UnpackSetBegin
; WHAT IT DOES: Sets up the unpacking of packed set A (the graphics sets
; $81:BAF1 unpacks) into bank $7F.
; HOW: Entry A of the 7-byte table at $80:F039 (A * 7, counting in scratch
; $0C, with DB = $80 and D = 0): X = where it goes in bank $7F, the source
; pointer at $65 = its bank (+4) with offset 0, $69 = its length (+5)
; rounded up to 8-byte groups; then the source offset (+2) goes on into
; UnpackBegin.
; Entry: M 16-bit, X 16-bit; A low byte = the set; called by JSR. Exit:
; as UnpackBegin; X = where the set goes; D = 0.
%at($81BB04)
UnpackSetBegin:
    and.w #$00FF
    sta.w !scratch
    sep.b #$20
    lda.b #!UNPACK_SET_BANK
    pha
    plb
    rep.b #$30
    lda.w !scratch
    asl a
    asl a
    asl a
    sec
    sbc.w !scratch
    tay
    lda.w #$0000
    tcd
    ldx.w !unpack_sets,y
    stz.b !unpack_source
    lda.w !unpack_sets+4,y
    sta.b !unpack_source+2
    lda.w !unpack_sets+5,y
    clc
    adc.w #$0007
    and.w #$FFF8
    lsr a
    lsr a
    lsr a
    sta.b !unpack_groups
    lda.w !unpack_sets+2,y
; @routine 81:BB3D UnpackBegin
; WHAT IT DOES: Starts an unpack into bank $7F: the source offset in Y
; and the data bank at $7F, and says whether there is anything to unpack.
; HOW: Y = A; DB = $7F; A = the group count's two bytes ORed.
; Entry: M 16-bit, X 16-bit; A = the source offset; D = 0; $65-$67 = the
; source's bank pointer; $69 = the 8-byte groups to unpack (word); called
; by JSR. Exit: M 8-bit; Y = the source offset; DB = $7F; Z set when
; there are no groups, N from the OR; X, C, V unchanged.
UnpackBegin:
    tay
    sep.b #$20
    lda.b #!UNPACK_DEST_BANK
    pha
    plb
    lda.b !unpack_groups
    ora.b !unpack_groups+1
    rts

; @routine 81:BB49 UnpackGroup
; WHAT IT DOES: Unpacks one 8-byte group of packed data to $7F:X and
; counts it off.
; HOW: Takes the flag and fill bytes from the source, then for each of the
; 8 bytes shifts the flags: a set bit stores the next source byte, a clear
; one the fill byte. Each source step past $FFFF goes on at $8000 of the
; next bank. X moves on 8; the groups left go down by one.
; Entry: M 8-bit, X 16-bit; D = 0; DB = $7F; X = the destination; Y = the
; source offset in the bank at UNPACK_SOURCE + 2; called by JSR
; (UnpackSet). Exit: X = the next destination, A = X (16 bits, from
; TXA/ADC), Y the next source offset; N and Z from the groups-left
; decrement (Z set: none left); C from the ADC; M 8-bit, X 16-bit.
%at($81BB49)
UnpackGroup:
    lda.b [!unpack_source],y
    sta.b !unpack_flags
    iny
    bne .bBB55
    ldy.w #$8000
    inc.b !unpack_source+2
.bBB55:
    lda.b [!unpack_source],y
    sta.b !unpack_fill
    iny
    bne .bBB61
    ldy.w #$8000
    inc.b !unpack_source+2
.bBB61:
    asl.b !unpack_flags
    bcs .bBB69
    lda.b !unpack_fill
    bra .bBB73
.bBB69:
    lda.b [!unpack_source],y
    iny
    bne .bBB73
    ldy.w #$8000
    inc.b !unpack_source+2
.bBB73:
    sta.w !unpack_out,x
    asl.b !unpack_flags
    bcs .bBB7E
    lda.b !unpack_fill
    bra .bBB88
.bBB7E:
    lda.b [!unpack_source],y
    iny
    bne .bBB88
    ldy.w #$8000
    inc.b !unpack_source+2
.bBB88:
    sta.w !unpack_out+1,x
    asl.b !unpack_flags
    bcs .bBB93
    lda.b !unpack_fill
    bra .bBB9D
.bBB93:
    lda.b [!unpack_source],y
    iny
    bne .bBB9D
    ldy.w #$8000
    inc.b !unpack_source+2
.bBB9D:
    sta.w !unpack_out+2,x
    asl.b !unpack_flags
    bcs .bBBA8
    lda.b !unpack_fill
    bra .bBBB2
.bBBA8:
    lda.b [!unpack_source],y
    iny
    bne .bBBB2
    ldy.w #$8000
    inc.b !unpack_source+2
.bBBB2:
    sta.w !unpack_out+3,x
    asl.b !unpack_flags
    bcs .bBBBD
    lda.b !unpack_fill
    bra .bBBC7
.bBBBD:
    lda.b [!unpack_source],y
    iny
    bne .bBBC7
    ldy.w #$8000
    inc.b !unpack_source+2
.bBBC7:
    sta.w !unpack_out+4,x
    asl.b !unpack_flags
    bcs .bBBD2
    lda.b !unpack_fill
    bra .bBBDC
.bBBD2:
    lda.b [!unpack_source],y
    iny
    bne .bBBDC
    ldy.w #$8000
    inc.b !unpack_source+2
.bBBDC:
    sta.w !unpack_out+5,x
    asl.b !unpack_flags
    bcs .bBBE7
    lda.b !unpack_fill
    bra .bBBF1
.bBBE7:
    lda.b [!unpack_source],y
    iny
    bne .bBBF1
    ldy.w #$8000
    inc.b !unpack_source+2
.bBBF1:
    sta.w !unpack_out+6,x
    asl.b !unpack_flags
    bcs .bBBFC
    lda.b !unpack_fill
    bra .bBC06
.bBBFC:
    lda.b [!unpack_source],y
    iny
    bne .bBC06
    ldy.w #$8000
    inc.b !unpack_source+2
.bBC06:
    sta.w !unpack_out+7,x
    rep.b #$20
    clc
    txa
    adc.w #$0008
    tax
    dec.b !unpack_groups
    sep.b #$20
    rts
