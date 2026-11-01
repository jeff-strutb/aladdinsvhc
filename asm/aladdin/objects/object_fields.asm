; object_fields.asm: small steps object code shares that set fields of the
; object's record, reached through the direct page (D = the record).
; C: src/aladdin/objects/object_fields.c.

incsrc "../include/macros.inc"
incsrc "../include/object.inc"
incsrc "../include/wram_state.inc"

arch 65816

; Tables in the bank DB names: two bytes for !obj_08's low bits, by the
; frame's parity ($84:D6B9); three-byte values for !obj_19 and !obj_1d
; ($84:D8D2 for one family of objects, $A5:CFB7 and $A5:CFCF for
; another).
!PARITY_BITS = $D6B9
!TRIPLES_84 = $D8D2
!TRIPLES_A5_19 = $CFB7
!TRIPLES_A5_1D = $CFCF

; The object being worked on, as some bank-$84 code keeps it (word).
!OBJECT_CURRENT = $1B14

; @routine 83:824B ObjectMarkCC
; WHAT IT DOES: Sets the record's first byte to $CC.
; HOW: LDA #$CC, STA to direct page 0.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($83824B)
ObjectMarkCC:
    lda.b #$CC
    sta.b !obj_00
    rts

; @routine 83:8267 ObjectStateSet8C
; WHAT IT DOES: Puts the object whose record Y points at into state $8C,
; with bit 7 of its byte at $08 set and of its byte at $09 cleared.
; HOW: ORA and AND on the two bytes, then the state byte.
; Entry: M 8-bit, X 16-bit; Y = the record (in the bank DB names, whose
; $0000-$1FFF is work RAM). Exit: as the C states.
%at($838267)
ObjectStateSet8C:
    lda.w !obj_08,y
    ora.b #$80
    sta.w !obj_08,y
    lda.w !obj_09,y
    and.b #$7F
    sta.w !obj_09,y
    lda.b #$8C
    sta.w !obj_00,y
    rts

; @routine 83:9A34 ObjectCopy0CTo40
; WHAT IT DOES: Copies the record's word at $0C to its word at $40.
; HOW: A byte at a time, low then high.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($839A34)
ObjectCopy0CTo40:
    lda.b !obj_0c
    sta.b !obj_40
    lda.b !obj_0c+1
    sta.b !obj_40+1
    rts

; @routine 83:9A3D ObjectSet19To010000
; WHAT IT DOES: Sets the record's three bytes at $19 to 0, 1, 0.
; HOW: STZ, LDA #1 and STA, STZ.
; Entry: M 8-bit; D = the record. Exit: as the C states.
ObjectSet19To010000:
    stz.b !obj_19
    lda.b #$01
    sta.b !obj_19+1
    stz.b !obj_19+2
    rts

; @routine 83:9BE5 ObjectSet44By07
; WHAT IT DOES: Sets the record's words at $44 and $46 to $0020 and
; $0040, or to $0028 and $0050 when bit 1 of its byte at $07 is set.
; HOW: The value in X, doubled through A for $46; both high bytes 0.
; Entry: M 8-bit, X 8-bit; D = the record. Exit: as the C states.
%at($839BE5)
ObjectSet44By07:
    ldx.b #$20
    lda.b !obj_07
    and.b #$02
    beq .set
    ldx.b #$28
.set:
    stx.b !obj_44
    txa
    asl a
    sta.b !obj_46
    stz.b !obj_45
    stz.b !obj_47
    rts

; @routine 83:9BFA ObjectSet44To40And80
; WHAT IT DOES: Sets the record's words at $44 and $46 to $0040 and
; $0080.
; HOW: $40 to $44, shifted left to $80 for $46, then both high bytes 0.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($839BFA)
ObjectSet44To40And80:
    lda.b #$40
    sta.b !obj_44
    asl a
    sta.b !obj_46
    stz.b !obj_45
    stz.b !obj_47
    rts

; @routine 83:A167 ObjectSet1DTo20
; WHAT IT DOES: Clears the record's three bytes at $1D and sets its bytes
; at $20, $47 and $48 to 8, $28 and 1.
; HOW: Three STZs and three stores, in that order.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($83A167)
ObjectSet1DTo20:
    stz.b !obj_1d
    stz.b !obj_1d+1
    stz.b !obj_1d+2
    lda.b #$08
    sta.b !obj_20
    lda.b #$28
    sta.b !obj_47
    lda.b #$01
    sta.b !obj_48
    rts

; @routine 83:B61A ObjectCap1D
; WHAT IT DOES: Limits the low word of the record's value at $1D to
; $0600 while the value is not negative (bit 7 of $1F clear), clearing
; its byte at $20 when it does.
; HOW: Compares $0600 with the word (16-bit A); stores it and clears $20
; when the word is larger.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($83B61A)
ObjectCap1D:
    lda.b !obj_1d+2
    bmi .done
    rep.b #$20
    lda.w #$0600
    cmp.b !obj_1d
    bcs .not_over
    sta.b !obj_1d
    sep.b #$20
    stz.b !obj_20
.not_over:
    sep.b #$20
.done:
    rts

; @routine 84:8A52 ObjectListFindFree
; WHAT IT DOES: Walks a list of records from the one the record's word at
; $37 points at, to the first whose byte at $18 is 0.
; HOW: With 16-bit X and Y each record's $18 byte is read; when not 0 the
; walk goes on to the record its own $37 word links, until a link of 0.
; Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($848A52)
ObjectListFindFree:
    rep.b #$11
    ldx.b !obj_37
.next:
    lda.w !obj_18,x
    beq .done
    ldy.w !obj_37,x
    tyx
    bne .next
    sec
.done:
    sep.b #$10
    rts

; @routine 84:8CD2 ObjectParityBitsSet
; WHAT IT DOES: Sets the low two bits of the record's byte at $08 from a
; two-entry table by whether the frame count is odd or even, so the
; object alternates between two settings frame by frame.
; HOW: The frame count's bit 0 picks the table byte, ORed into the byte
; with its low two bits cleared.
; Entry: M 8-bit, X 8-bit; D = the record; DB the table's bank ($84,
; with work RAM at $0000-$1FFF). Exit: as the C states.
%at($848CD2)
ObjectParityBitsSet:
    lda.w !frame_count
    and.b #$01
    tax
    lda.b !obj_08
    and.b #$FC
    ora.w !PARITY_BITS,x
    sta.b !obj_08
    rts

; @routine 84:8CE2 ObjectLayerByHeight
; WHAT IT DOES: Sets bits 4-5 of the current object's byte at $09 to 1
; when its Y position is less than 128, else to 2.
; HOW: The record is the one $1B14 points at; with 16-bit X and Y, the
; byte's two bits are cleared and the value ORed in.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the C states.
%at($848CE2)
ObjectLayerByHeight:
    rep.b #$10
    ldx.w !OBJECT_CURRENT
    lda.w !obj_09,x
    and.b #$CF
    sta.w !obj_09,x
    lda.b #$10
    ldy.w !obj_0f,x
    cpy.w #128
    bcc .set
    lda.b #$20
.set:
    ora.w !obj_09,x
    sta.w !obj_09,x
    sep.b #$10
    rts

; @routine 84:9ED0 Object19FromTable84
; WHAT IT DOES: Loads the record's three bytes at $19 from the table at
; $D8D2, entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($84). Exit: as the C states.
%at($849ED0)
Object19FromTable84:
    lda.w !TRIPLES_84,y
    sta.b !obj_19
    lda.w !TRIPLES_84+1,y
    sta.b !obj_19+1
    lda.w !TRIPLES_84+2,y
    sta.b !obj_19+2
    rts

; @routine 84:9EE0 Object1DFromTable84
; WHAT IT DOES: Loads the record's three bytes at $1D from the table at
; $D8D2, entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($84). Exit: as the C states.
%at($849EE0)
Object1DFromTable84:
    lda.w !TRIPLES_84,y
    sta.b !obj_1d
    lda.w !TRIPLES_84+1,y
    sta.b !obj_1d+1
    lda.w !TRIPLES_84+2,y
    sta.b !obj_1d+2
    rts

; @routine A5:EA93 Object19FromTableA5
; WHAT IT DOES: Loads the record's three bytes at $19 from the table at
; $CFB7, entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($A5). Exit: as the C states.
%at($A5EA93)
Object19FromTableA5:
    lda.w !TRIPLES_A5_19,y
    sta.b !obj_19
    lda.w !TRIPLES_A5_19+1,y
    sta.b !obj_19+1
    lda.w !TRIPLES_A5_19+2,y
    sta.b !obj_19+2
    rts

; @routine A5:EAA3 Object1DFromTableA5
; WHAT IT DOES: Loads the record's three bytes at $1D from the table at
; $CFCF, entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($A5). Exit: as the C states.
%at($A5EAA3)
Object1DFromTableA5:
    lda.w !TRIPLES_A5_1D,y
    sta.b !obj_1d
    lda.w !TRIPLES_A5_1D+1,y
    sta.b !obj_1d+1
    lda.w !TRIPLES_A5_1D+2,y
    sta.b !obj_1d+2
    rts
