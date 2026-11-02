; unpack.asm: setting up the unpacking of packed data (graphics) into
; bank $7F.
;
; An unpack reads its source through the long pointer at $65 (offset 0 in
; the source's bank, the offset itself in Y) and writes 8 bytes a group to
; bank $7F at X; $69 counts the groups.
; C: src/aladdin/video/unpack.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; Direct page (D = 0).
!unpack_source = $65             ; the source's pointer (long)
!unpack_groups = $69             ; the 8-byte groups left (word)
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
