; sequences.asm: the level's value sequences: ten records of 16 bytes
; from $1645, each holding a word for some frames, then the next of its
; list.
;
; A record, through the direct page (D = the record): $00 nonzero while it
; runs; $01 its number (an index into the table at $CA17 of its bank); $02
; its list's length in bytes and $03 the index of its entry; $04 = 2 when
; a new word is out; $05 the frames left of the word; $06 its list's
; address; $08 the word; $0C nonzero once started. An entry: the frames
; (byte, negative ends the list), then the word.
; C: src/aladdin/level/sequences.c.

incsrc "../include/macros.inc"

arch 65816

!SEQUENCES = $1645
!SEQUENCES_END = $16E5
!SEQUENCE_SIZE = 16
!SEQ_ACTIVE = $00
!SEQ_ID = $01
!SEQ_LENGTH = $02
!SEQ_INDEX = $03
!SEQ_NEW = $04
!SEQ_WAIT = $05
!SEQ_LIST = $06
!SEQ_WORD = $08
!SEQ_STARTED = $0C
!SEQUENCE_TABLE = $CA17         ; by number: the length (byte), the list's address (word)

; @routine 81:EC23 SequencesStep
; WHAT IT DOES: Steps every running value sequence, once a frame: each
; holds a word for a number of frames, then the next word of its list.
; HOW: With D at each of the ten records in turn, one with its active byte
; nonzero is started (SequenceStart) if not yet; its wait counted down; at
; 0 its index steps 3 (back to 0 at its length) and the entry there gives
; the wait and the word; a negative wait ends the sequence (active byte
; cleared). $04 = 2 marks a new word.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM, the
; sequences' bank; called by JSR. Exit: M 8-bit; D unchanged; N, Z from it
; (PLD); A = $16E5 (16 bits); C set; X as the last start leaves it; Y the
; last entry's index plus 1, or as on entry.
%at($81EC23)
SequencesStep:
    phd
    lda.b #!SEQUENCES>>8
    xba
    lda.b #!SEQUENCES&$FF
    tcd
.record:
    lda.b !SEQ_ACTIVE
    beq .next
    lda.b !SEQ_STARTED
    bne .started
    jsr.w SequenceStart
.started:
    dec.b !SEQ_WAIT
    bne .next
    lda.b !SEQ_INDEX
    clc
    adc.b #3
    cmp.b !SEQ_LENGTH
    bcc .index
    lda.b #0
.index:
    sta.b !SEQ_INDEX
    tay
    lda.b (!SEQ_LIST),y
    sta.b !SEQ_WAIT
    bpl .word
    stz.b !SEQ_ACTIVE
    bra .next
.word:
    iny
    rep.b #$20
    lda.b (!SEQ_LIST),y
    sta.b !SEQ_WORD
    sep.b #$20
    lda.b #2
    sta.b !SEQ_NEW
.next:
    rep.b #$20
    tdc
    clc
    adc.w #!SEQUENCE_SIZE
    tcd
    cmp.w #!SEQUENCES_END
    sep.b #$20
    bne .record
    pld
    rts

; @routine 81:EC6F SequenceStart
; WHAT IT DOES: Starts a value sequence: its list and length from the
; table at $CA17 by its number, a first step on the next frame.
; HOW: Its started byte and index $80, its wait 1; its number ($01) indexes
; the table: the length byte, then the list's address (word).
; Entry: M 8-bit, X 8-bit; D = the record; DB the table's bank; called by
; JSR from SequencesStep. Exit: M 8-bit; A = the list's address (16 bits);
; X = the number; N, Z from the address; C, V, Y unchanged.
SequenceStart:
    lda.b #$80
    sta.b !SEQ_STARTED
    sta.b !SEQ_INDEX
    lda.b #1
    sta.b !SEQ_WAIT
    ldx.b !SEQ_ID
    lda.w !SEQUENCE_TABLE,x
    sta.b !SEQ_LENGTH
    rep.b #$20
    lda.w !SEQUENCE_TABLE+1,x
    sta.b !SEQ_LIST
    sep.b #$20
    rts
