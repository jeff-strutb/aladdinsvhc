; spawn_simple.asm: a child started at its spawner's spawn point with
; only a hit box (and the spawner's tile slot), or with a facing; and
; every record put back free.
; C: src/aladdin/actor/spawn_simple.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

; The records ActorsFreeAll frees, from the last down to the first of the
; pool.
!ACTOR_POOL_LAST = $13CB
!ACTOR_POOL_END = $0B0B

; @routine 81:A9E9 ActorSpawnShared
; WHAT IT DOES: Starts a child with hit box A at the spawner's spawn point,
; sharing the spawner's animated-tile slot, if a record is free.
; HOW: ActorAlloc; then ActorSpawnTyped and ActorTileSlotShare.
; Entry: M 8-bit, X 8-bit; A low byte = the hit box; D = the spawner; DB =
; $81; called by JSL. Exit: none free: as ActorAlloc (N set). Started: M
; 8-bit, X 8-bit; A low byte = 0, Z set; X = the child's low byte.
%at($81A9E9)
ActorSpawnShared:
    jsl.l ActorAlloc
    bmi .lA9F9
    jsr.w ActorSpawnTyped
    jsr.w ActorTileSlotShare
    sep.b #$30
    lda.b #$00
.lA9F9:
    rtl


; @routine 81:AA57 ActorSpawnFacingY
; WHAT IT DOES: Starts a child with hit box A at the spawner's spawn point,
; facing X, with Y as its $07, if a record is free.
; HOW: ActorAlloc; the child's hit box, state $0C (new), ActorSpawnPlace;
; its facing and flip from X, its $07 from Y.
; Entry: M 8-bit, X 8-bit; A low byte = the hit box; X = the facing; Y =
; the child's $07; D = the spawner; DB = $81; called by JSL. Exit: none
; free: as ActorAlloc (N set). Started: M 8-bit, X 16-bit, X = the child;
; Z set (SEP #$22); A low byte = Y at entry.
%at($81AA57)
ActorSpawnFacingY:
    stx.w !scratch+1
    sty.w !scratch+2
    jsl.l ActorAlloc
    bmi .lAA80
    sta.w !actor_06,x
    lda.b #$0C
    sta.w !actor_state,x
    jsl.l ActorSpawnPlace
    lda.w !scratch+1
    sta.w !actor_facing,x
    sta.w !actor_flip,x
    lda.w !scratch+2
    sta.w !actor_07,x
    sep.b #$22
.lAA80:
    rtl


; @routine 81:9D82 ActorsFreeAll
; WHAT IT DOES: Puts every record of the pool back on the free list.
; HOW: The free list emptied (top $FFFE); ActorFreePlacedAtYLong for each
; record from $13CB down to $0B5B; then the top set to $36 (28 entries).
; Entry: M 8-bit; DB = $00-$3F or $80-$BF; called by JSL. Exit: M 8-bit,
; X 8-bit; X = $36; Y = $0B (low byte of $0B0B).
%at($819D82)
ActorsFreeAll:
    rep.b #$10
    ldx.w #$FFFE
    stx.w !actor_free_top
    ldy.w #!ACTOR_POOL_LAST
.l9D8D:
    phy
    jsl.l ActorFreePlacedAtYLong
    rep.b #$20
    sec
    pla
    sbc.w #$0050
    tay
    sep.b #$20
    cpy.w #!ACTOR_POOL_END
    bne .l9D8D
    ldx.w #$0036
    stx.w !actor_free_top
    sep.b #$10
    rtl
