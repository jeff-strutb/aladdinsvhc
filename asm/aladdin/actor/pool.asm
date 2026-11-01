; pool.asm: actor records taken from the free list.
;
; The records an actor can be created in are kept on a stack of their
; addresses (actor_free_list, actor_free_top the offset of its top word).
; C: src/aladdin/actor/pool.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 81:AB3F ActorAlloc
; WHAT IT DOES: Takes a free actor record, if there is one.
; HOW: The free list's top offset, negative when none is free; else the
; offset steps down a word and the address at the old top is the record.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: none free: Y = the offset; N set; X 8-bit and unchanged.
; Taken: X 16-bit; X = the record's address; Y = the old top offset; N, Z
; from X (N clear). A, C, V unchanged.
%at($81AB3F)
ActorAlloc:
    ldy.w !actor_free_top
    bmi .none
    rep.b #$10
    dec.w !actor_free_top
    dec.w !actor_free_top
    ldx.w !actor_free_list,y
.none:
    rtl
