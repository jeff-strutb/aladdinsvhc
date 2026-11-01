; actor_fields.asm: small steps actor code shares that set fields of the
; actor's record, reached through the direct page (D = the record).
; C: src/aladdin/actor/actor_fields.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_state.inc"

arch 65816

; Tables in the bank DB names: two bytes for !actor_08's low bits, by the
; frame's parity ($84:D6B9); three-byte values for !actor_speed_x and !actor_speed_y
; ($84:D8D2 for one family of actors, $A5:CFB7 and $A5:CFCF for
; another).
!PARITY_BITS = $D6B9
!TRIPLES_84 = $D8D2
!TRIPLES_A5_19 = $CFB7
!TRIPLES_A5_1D = $CFCF

; The actor being worked on, as some bank-$84 code keeps it (word).
!ACTOR_CURRENT = $1B14

; @routine 83:824B ActorStateSetCC
; WHAT IT DOES: Puts the actor into state $CC.
; HOW: LDA #$CC, STA to its state byte.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($83824B)
ActorStateSetCC:
    lda.b #$CC
    sta.b !actor_state
    rts

; @routine 83:8267 ActorStateSet8C
; WHAT IT DOES: Puts the actor whose record Y points at into state $8C,
; with bit 7 of its byte at $08 set and of its byte at $09 cleared.
; HOW: ORA and AND on the two bytes, then the state byte.
; Entry: M 8-bit, X 16-bit; Y = the record (in the bank DB names, whose
; $0000-$1FFF is work RAM). Exit: as the C states.
%at($838267)
ActorStateSet8C:
    lda.w !actor_08,y
    ora.b #$80
    sta.w !actor_08,y
    lda.w !actor_09,y
    and.b #$7F
    sta.w !actor_09,y
    lda.b #$8C
    sta.w !actor_state,y
    rts

; @routine 83:9A34 ActorXTo40
; WHAT IT DOES: Copies the actor's x pixel position to its word at $40.
; HOW: A byte at a time, low then high.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($839A34)
ActorXTo40:
    lda.b !actor_x+1
    sta.b !actor_40
    lda.b !actor_x+1+1
    sta.b !actor_40+1
    rts

; @routine 83:9A3D ActorSpeedXSet1
; WHAT IT DOES: Sets the actor's horizontal speed to 1 pixel a frame.
; HOW: The speed's three bytes to 0 (fraction), 1, 0.
; Entry: M 8-bit; D = the record. Exit: as the C states.
ActorSpeedXSet1:
    stz.b !actor_speed_x
    lda.b #$01
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    rts

; @routine 83:9BE5 Actor44SetBy07
; WHAT IT DOES: Sets the actor's words at $44 and $46 to $0020 and $0040,
; or to $0028 and $0050 when bit 1 of its byte at $07 is set.
; HOW: The value in X, doubled through A for $46; both high bytes 0.
; Entry: M 8-bit, X 8-bit; D = the record. Exit: as the C states.
%at($839BE5)
Actor44SetBy07:
    ldx.b #$20
    lda.b !actor_07
    and.b #$02
    beq .set
    ldx.b #$28
.set:
    stx.b !actor_44
    txa
    asl a
    sta.b !actor_46
    stz.b !actor_45
    stz.b !actor_47
    rts

; @routine 83:9BFA Actor44Set40And80
; WHAT IT DOES: Sets the actor's words at $44 and $46 to $0040 and $0080.
; HOW: $40 to $44, shifted left to $80 for $46, then both high bytes 0.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($839BFA)
Actor44Set40And80:
    lda.b #$40
    sta.b !actor_44
    asl a
    sta.b !actor_46
    stz.b !actor_45
    stz.b !actor_47
    rts

; @routine 83:A167 ActorFallStart
; WHAT IT DOES: Starts the actor falling from rest: no vertical speed,
; gravity 8 (a fraction of a pixel a frame each frame), and its bytes at
; $47 and $48 set to $28 and 1.
; HOW: Three STZs, then the three stores, in that order.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($83A167)
ActorFallStart:
    stz.b !actor_speed_y
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    lda.b #$08
    sta.b !actor_accel_y
    lda.b #$28
    sta.b !actor_47
    lda.b #$01
    sta.b !actor_48
    rts

; @routine 83:B61A ActorFallSpeedCap
; WHAT IT DOES: Limits the actor's falling speed to 6 pixels a frame:
; while it moves down (the speed's top byte not negative) a speed whose
; fraction and low pixel byte, as a word, pass $0600 becomes $0600 and its
; gravity is cleared.
; HOW: Compares $0600 with the word (16-bit A); stores it and clears the
; gravity when the word is larger.
; Entry: M 8-bit; D = the record. Exit: as the C states.
%at($83B61A)
ActorFallSpeedCap:
    lda.b !actor_speed_y+2
    bmi .done
    rep.b #$20
    lda.w #$0600
    cmp.b !actor_speed_y
    bcs .not_over
    sta.b !actor_speed_y
    sep.b #$20
    stz.b !actor_accel_y
.not_over:
    sep.b #$20
.done:
    rts

; @routine 84:8A52 ActorListFindIdle
; WHAT IT DOES: Walks the actor's list, from the record its link points
; at, to the first whose byte at $18 is 0.
; HOW: With 16-bit X and Y each record's $18 byte is read; when not 0 the
; walk goes on through that record's own link, until a link of 0.
; Entry: M 8-bit, X 8-bit; D = the record; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($848A52)
ActorListFindIdle:
    rep.b #$11
    ldx.b !actor_next
.next:
    lda.w !actor_18,x
    beq .done
    ldy.w !actor_next,x
    tyx
    bne .next
    sec
.done:
    sep.b #$10
    rts

; @routine 84:8CD2 ActorParityBitsSet
; WHAT IT DOES: Sets the low two bits of the actor's byte at $08 from a
; two-entry table by whether the frame count is odd or even, so the
; actor alternates between two settings frame by frame.
; HOW: The frame count's bit 0 picks the table byte, ORed into the byte
; with its low two bits cleared.
; Entry: M 8-bit, X 8-bit; D = the record; DB the table's bank ($84,
; with work RAM at $0000-$1FFF). Exit: as the C states.
%at($848CD2)
ActorParityBitsSet:
    lda.w !frame_count
    and.b #$01
    tax
    lda.b !actor_08
    and.b #$FC
    ora.w !PARITY_BITS,x
    sta.b !actor_08
    rts

; @routine 84:8CE2 ActorPriorityByHeight
; WHAT IT DOES: Sets the sprite priority (bits 4-5 of the byte at $09) of
; the actor $1B14 points at to 1 when its y position is less than 128,
; else to 2.
; HOW: With 16-bit X and Y, the byte's two bits are cleared and the value
; ORed in.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: as the C states.
%at($848CE2)
ActorPriorityByHeight:
    rep.b #$10
    ldx.w !ACTOR_CURRENT
    lda.w !actor_09,x
    and.b #$CF
    sta.w !actor_09,x
    lda.b #$10
    ldy.w !actor_y+1,x
    cpy.w #128
    bcc .set
    lda.b #$20
.set:
    ora.w !actor_09,x
    sta.w !actor_09,x
    sep.b #$10
    rts

; @routine 84:9ED0 ActorSpeedXFrom84
; WHAT IT DOES: Sets the actor's horizontal speed from the table at $D8D2,
; entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($84). Exit: as the C states.
%at($849ED0)
ActorSpeedXFrom84:
    lda.w !TRIPLES_84,y
    sta.b !actor_speed_x
    lda.w !TRIPLES_84+1,y
    sta.b !actor_speed_x+1
    lda.w !TRIPLES_84+2,y
    sta.b !actor_speed_x+2
    rts

; @routine 84:9EE0 ActorSpeedYFrom84
; WHAT IT DOES: Sets the actor's vertical speed from the table at $D8D2,
; entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($84). Exit: as the C states.
%at($849EE0)
ActorSpeedYFrom84:
    lda.w !TRIPLES_84,y
    sta.b !actor_speed_y
    lda.w !TRIPLES_84+1,y
    sta.b !actor_speed_y+1
    lda.w !TRIPLES_84+2,y
    sta.b !actor_speed_y+2
    rts

; @routine A5:EA93 ActorSpeedXFromA5
; WHAT IT DOES: Sets the actor's horizontal speed from the table at $CFB7,
; entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($A5). Exit: as the C states.
%at($A5EA93)
ActorSpeedXFromA5:
    lda.w !TRIPLES_A5_19,y
    sta.b !actor_speed_x
    lda.w !TRIPLES_A5_19+1,y
    sta.b !actor_speed_x+1
    lda.w !TRIPLES_A5_19+2,y
    sta.b !actor_speed_x+2
    rts

; @routine A5:EAA3 ActorSpeedYFromA5
; WHAT IT DOES: Sets the actor's vertical speed from the table at $CFCF,
; entry Y.
; HOW: Three byte copies.
; Entry: M 8-bit, X 8-bit; Y = the entry's offset; D = the record; DB
; the table's bank ($A5). Exit: as the C states.
%at($A5EAA3)
ActorSpeedYFromA5:
    lda.w !TRIPLES_A5_1D,y
    sta.b !actor_speed_y
    lda.w !TRIPLES_A5_1D+1,y
    sta.b !actor_speed_y+1
    lda.w !TRIPLES_A5_1D+2,y
    sta.b !actor_speed_y+2
    rts
