; carrier_pair.asm: an actor that sends in a pair: a carrier (kind $38) and
; the thing it holds (kind $53, actor/kind53.c), each knowing the other.
; Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/carrier_pair.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!NEW_RECORD = $20  ; the record ActorCreateIn last took (word)
!RANGE_X = $1A44  ; the point actors are kept in range of (words)
!RANGE_Y = $1A46
!PAIR_OTHER = $4E  ; word: the record of the other of the pair
!PAIR_SENDER = $4C  ; word: the record of the actor that sent them, 0 here
!PAIRS_LIVE = $4B  ; actors it has sent that are live
!PAIR_WAIT = $40  ; frames until it sends the next

; @routine 84:A056 CarrierPairSend
; WHAT IT DOES: Sends in a carrier (kind $38) 40 pixels above the point
; actors are kept in range of, and a thing it holds (kind $53) at the
; carrier's spawn point $44, both new and facing as the sender, each with
; the other's record in $4E; counts them live and waits $3C frames before
; the next.
; HOW: Its own position kept on the stack and set to the range point (y
; less 40) for the placing; kind $38 by ActorCreateIn (Y = $4A), $4A up by
; 2; the new record (from $0020) in its $4E; the carrier's $07 = its own
; plus one, ActorSpawnPlace at $44, state $0C, facing and flip as its own,
; $45, $46 and $4C (16-bit) cleared; $41 up, $40 = $3C. Then kind $53 the
; same way, placed at the carrier's spawn point $44 less 4 across and 12
; down, state $0C, its facing, $4C cleared; the two records crossed into
; each other's $4E. Its position back, $4B up, and the free list's top up
; by 4 (both records taken).
; Entry: M 8-bit, X 8-bit; D = the sender's record; DB = $84; called by
; JSR (84:9FE7). Exit: M and X 8-bit; A = the free list's new top (low
; byte); C from its ADC.
%at($84A056)
CarrierPairSend:
    rep.b #$20
    lda.b !actor_x+1
    pha
    lda.b !actor_y+1
    pha
    lda.w !RANGE_X
    sta.b !actor_x+1
    sec
    lda.w !RANGE_Y
    sbc.w #$0028
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$38
    sta.b !actor_06
    jsl.l ActorCreateIn
    inc.b !actor_stream
    inc.b !actor_stream
    rep.b #$10
    ldx.w !NEW_RECORD
    stx.b !PAIR_OTHER
    lda.b !actor_07
    inc a
    sta.w !actor_07,x
    lda.b #$44
    sta.b !actor_spawn_at
    jsl.l ActorSpawnPlace
    lda.b #$0C
    sta.w !actor_state,x
    lda.b !actor_facing
    sta.w !actor_facing,x
    sta.w !actor_flip,x
    stz.w !actor_45,x
    stz.w !actor_46,x
    rep.b #$20
    tdc
    sta.w !actor_4c,x
    sep.b #$20
    inc.b !actor_41
    lda.b #$3C
    sta.b !PAIR_WAIT
    sep.b #$10
    ldy.b !actor_stream
    lda.b #$53
    sta.b !actor_06
    jsl.l ActorCreateIn
    rep.b #$10
    ldx.w !NEW_RECORD
    lda.b #$44
    sta.b !actor_spawn_at
    jsl.l ActorSpawnPlace
    rep.b #$21
    lda.w !actor_x+1,x
    adc.w #$FFFC
    sta.w !actor_x+1,x
    clc
    lda.w !actor_y+1,x
    adc.w #$000C
    sta.w !actor_y+1,x
    sep.b #$20
    lda.b #$0C
    sta.w !actor_state,x
    lda.b !actor_facing
    sta.w !actor_facing,x
    sta.w !actor_flip,x
    rep.b #$20
    tdc
    sta.w !actor_4c,x
    lda.b !PAIR_OTHER
    sta.w !PAIR_OTHER,x
    txy
    tax
    tya
    sta.w !PAIR_OTHER,x
    pla
    sta.b !actor_y+1
    pla
    sta.b !actor_x+1
    sep.b #$30
    inc.b !PAIRS_LIVE
    clc
    lda.w !actor_free_top
    adc.b #$04
    sta.w !actor_free_top
    rts
