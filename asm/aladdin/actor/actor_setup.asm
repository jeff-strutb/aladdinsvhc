; actor_setup.asm: giving actors their starting values: from a placement
; entry, from tables by an index the actor holds, and down the list of
; actors one owns.
; C: src/aladdin/actor/actor_setup.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

; In the bank DB names: placement entries of 5 bytes (a byte whose low 5
; bits go to !actor_07, then the x and y pixels in the low 14 bits of a
; word each); words and bytes by !actor_40 ($D6EB, $D70B); stream starts
; by !actor_flip (two words, 4 bytes an entry: $D71D, $D71F) and stream
; addresses by X ($D725).
!PLACEMENTS = $8A7B
!BY_40_WORDS = $D6EB
!BY_40_BYTES = $D70B
!STREAM_STARTS = $D71D
!STREAMS = $D725
!PLACED_FLAGS = $7EC600         ; a byte per placement: bit 0 set once placed

; @routine 82:A906 ActorPlace
; WHAT IT DOES: Places the actor whose record X points at from placement
; entry Y: the entry's low 5 bits to its byte at $07, its x and y pixels,
; state $0C; marks the placement as placed and keeps its index (the
; caller's direct-page word at $12) in the actor's byte at $34.
; HOW: The pixels are the low 14 bits of the entry's words. The placed
; flag is the entry's first byte with bit 0 set, at $7E:C600 plus the
; index.
; Entry: M 8-bit, X 16-bit; X = the actor, Y = the entry's offset; D the
; caller's (its word at $12 the index); DB the entries' bank, whose
; $0000-$1FFF is work RAM. Exit: as the C states.
%at($82A906)
ActorPlace:
    lda.w !PLACEMENTS,y
    pha
    and.b #$1F
    sta.w !actor_07,x
    rep.b #$20
    lda.w !PLACEMENTS+1,y
    and.w #$3FFF
    sta.w !actor_x+1,x
    lda.w !PLACEMENTS+3,y
    and.w #$3FFF
    sta.w !actor_y+1,x
    sep.b #$20
    lda.b #$0C
    sta.w !actor_state,x
    pla
    phx
    ldx.b !actor_12
    ora.b #$01
    sta.l !PLACED_FLAGS,x
    txa
    plx
    sta.w !actor_34,x
    rts

; @routine 84:8D41 ActorListStart
; WHAT IT DOES: Starts every actor in the list this actor owns: each gets
; the pair X, Y (in its bytes at $03-$04 and at $35-$36), state 1, its
; counters at $18, $48, $49 and $4C cleared, and at $44 a value that steps
; by A down the list from A.
; HOW: The inputs go to scratch bytes; with 16-bit X and Y the walk
; follows each record's link from this actor's own, starting from the
; first linked record whatever it is, until a link of 0.
; Entry: M 8-bit, X 8-bit; A = the first value and the step; D = the
; actor; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($848D41)
ActorListStart:
    stx.w !scratch+8
    sty.w !scratch+9
    sta.w !scratch+10
    sta.w !scratch
    rep.b #$30
    tdc
    tay
    ldx.w !actor_next,y
    sep.b #$20
.next:
    lda.w !scratch+8
    sta.w !actor_03,x
    sta.w !actor_35,x
    lda.w !scratch+9
    sta.w !actor_04,x
    sta.w !actor_36,x
    stz.w !actor_18,x
    stz.w !actor_4c,x
    stz.w !actor_48,x
    stz.w !actor_49,x
    lda.b #$01
    sta.w !actor_01,x
    lda.w !scratch
    sta.w !actor_44,x
    clc
    adc.w !scratch+10
    sta.w !scratch
    ldy.w !actor_next,x
    tyx
    bne .next
    sep.b #$30
    rts

; @routine 84:8D8F ActorStartBy40
; WHAT IT DOES: Starts the actor from its index at $40: bytes $08 and $09
; to 2 and $A0, its word at $42 from a word table less $0100, its byte at
; $41 from a byte table, and its bytes at $46-$49, $4C and $18 cleared.
; HOW: The index picks a word entry (doubled) and a byte entry, in the
; bank DB names.
; Entry: M 8-bit, X 8-bit; D = the actor; DB the tables' bank. Exit: as the C states.
ActorStartBy40:
    lda.b #$02
    sta.b !actor_08
    lda.b #$A0
    sta.b !actor_09
    lda.b !actor_40
    tay
    asl a
    tax
    rep.b #$20
    sec
    lda.w !BY_40_WORDS,x
    sbc.w #$0100
    sta.b !actor_42
    sep.b #$20
    lda.w !BY_40_BYTES,y
    sta.b !actor_41
    stz.b !actor_46
    stz.b !actor_47
    stz.b !actor_48
    stz.b !actor_49
    stz.b !actor_4c
    stz.b !actor_18
    rts

; @routine 84:8DBB ActorStreamStart
; WHAT IT DOES: Starts the actor on byte stream X: its horizontal speed
; and the word at $1C from the entry its byte at $15 picks, then from the
; stream's head a byte for $45, $46 and $47, a count less one for $4D, and
; a word for $48 (EORed with the speed's low word, plus the $1C word);
; the stream goes on from the byte after, and the counter at $4C steps.
; HOW: The entry is 4 bytes; the stream's address comes from a word table
; by X, read with 16-bit Y, in the bank DB names.
; Entry: M 8-bit, X 8-bit; X = the stream's offset; D = the actor; DB the
; tables' and streams' bank. Exit: as the C states.
ActorStreamStart:
    lda.b !actor_flip
    asl a
    asl a
    tay
    rep.b #$20
    lda.w !STREAM_STARTS,y
    sta.b !actor_speed_x
    lda.w !STREAM_STARTS+2,y
    sta.b !actor_accel_x
    sep.b #$20
    rep.b #$10
    ldy.w !STREAMS,x
    lda.w 0,y
    sta.b !actor_45
    sta.b !actor_46
    sta.b !actor_47
    iny
    lda.w 0,y
    dec a
    sta.b !actor_4d
    iny
    rep.b #$20
    lda.w 0,y
    eor.b !actor_speed_x
    clc
    adc.b !actor_accel_x
    sta.b !actor_48
    sep.b #$20
    iny
    iny
    sty.b !actor_stream
    sep.b #$10
    inc.b !actor_4c
    rts
