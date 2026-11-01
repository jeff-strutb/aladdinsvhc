; spawn_at.asm: a new actor of a kind at a place the caller puts in two
; work RAM words, started at its next turn.
; C: src/aladdin/actor/spawn_at.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!SPAWN_X = $002C                ; where the actor goes: x, then y (pixels, words)
!SPAWN_Y = $002E
!SPAWN_07 = $0030               ; the byte it gets at $07, kept during the call
!ACTOR_NEW = $0C                ; the state of a record that starts at its next turn

; Not yet filed: a free record of the first pool for kind A (its $06): X
; the record (16 bits), C clear; none: C set.
!ACTOR_ALLOC = $82AA2B

; @routine 80:A4D8 ActorSpawnAtLong
; WHAT IT DOES: ActorSpawnAt for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorSpawnAt; called by JSL. Exit: as the C states.
%at($80A4D8)
ActorSpawnAtLong:
    jsr.w ActorSpawnAt
    rtl

; @routine 80:A4DC ActorSpawnAt
; WHAT IT DOES: Starts a new actor of kind A's high byte at the place in
; SPAWN_X and SPAWN_Y, with A's low byte at its $07; nothing when no
; record is free.
; HOW: The low byte kept at $0030; the record taken by the routine at
; $82:AA2B (which sets the kind); its state $0C (new), its $07, and the
; whole pixels of its x and y from the two words.
; Entry: M 8-bit, X 8-bit; A = kind (high byte) and the byte for $07
; (low); DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
ActorSpawnAt:
    sta.w !SPAWN_07
    xba
    jsl.l !ACTOR_ALLOC
    bcs .none
    lda.b #!ACTOR_NEW
    sta.w !actor_state,x
    lda.w !SPAWN_07
    sta.w !actor_07,x
    rep.b #$20
    lda.w !SPAWN_X
    sta.w !actor_x+1,x
    lda.w !SPAWN_Y
    sta.w !actor_y+1,x
    clc
.none:
    rts
