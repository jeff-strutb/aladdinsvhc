; kind20_carrier.asm: actor kind $20's guard with $07 bit 4, which stands
; with a pot (kind $42, kind42.asm) over its head and throws it at
; Aladdin when he comes near and it sees him; then it walks its beat as
; another guard, or charges. No level places such a guard (its placements'
; $07 are $21, $22, $29 and $80), so no play runs it.
; C: src/aladdin/actor/kind20_carrier.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!K20C_PLACE = $07               ; its placement's byte: bit 0 its facing, bit 1 the pot's way (to its $07)
!K20C_POT = $42                 ; the pot's record (word)
!K20C_SIGNAL = $4F              ; set by the pot: 1 it hit Aladdin, 2 it passed him
!K20C_COPY_LEFT = $000E         ; words of the records left to swap
!K20C_COPY_TMP = $0010          ; the pot's word being swapped
!KIND_POT = $42

; @routine 83:9AB9 Kind20CarrierWalks
; @actor
; WHAT IT DOES: The guard without its pot (none could be made, or it is
; gone) becomes a walking guard: facing Aladdin, $07 that facing with
; bits 1 and 4 ($12), its beat from here; on as Kind20Start's walk.
; HOW: 8-bit registers; ActorSideOfPlayer OR $12 into PLACE;
; Actor44Set40And80; JMP to Kind20StartWalk.
; Entry: D = the guard's record; DB = $80; reached by Kind20Carrier's
; branch (ActorCreate failed) and its JMP (the pot gone, or its throw
; done). Exit: never returns.
%at($839AB9)
Kind20CarrierWalks:
    sep.b #$30
    jsl.l ActorSideOfPlayer
    ora.b #$12
    sta.b !K20C_PLACE
    jsr.w Actor44Set40And80
    jmp.w Kind20StartWalk

; @routine 83:9AC9 Kind20Carrier
; @actor
; WHAT IT DOES: Kind $20's guard with PLACE bit 4: makes its pot (kind
; $42) over its head and waits for Aladdin; when he is near and it sees
; him, turns to him and throws (the pot flies when its animation shows
; frame $15); then, told by the pot that it hit him, laughs and walks
; (Kind20CarrierWalks); that it passed him, turns red and charges
; (Kind20Spotted); the pot gone first, walks.
; HOW: Facing and flip = A AND 1; AnimStart 6; spawn point $20; SIGNAL
; 0; ActorCreate kind $42 (C set: Kind20CarrierWalks). The pot: state
; $0C, $34 $FF, $07 = PLACE AND 2, facing and flip from the guard's,
; ActorSpawnPlace; its $40 = this record, POT = its record. When the pot's
; record comes first, the guard's own resume address is made
; $83:9B53 (PHK, PEA-like PHA, PHP: P and the address into $02-$05), the
; two records swapped word by word ($28 words) and on at Kind42Start as
; the pot (JMP), the guard to go on at Kind20CarrierWaits in the pot's
; old record; else each frame (BRK) Kind20CarrierWaits.
; Entry: M 8-bit, X 8-bit; A = PLACE; D = the guard's record; DB = $80;
; reached by Kind20Start's JMP. Exit: never returns.
Kind20Carrier:
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$06
    jsl.l AnimStart
    lda.b #$20
    sta.b !actor_spawn_at
    stz.b !K20C_SIGNAL
    lda.b #!KIND_POT
    jsl.l ActorCreate
    bcs Kind20CarrierWalks
    lda.b #$0C
    sta.w !actor_state,x
    lda.b #$FF
    sta.w !actor_34,x
    lda.b !K20C_PLACE
    and.b #$02
    sta.w !actor_07,x
    lda.b !actor_facing
    sta.w !actor_facing,x
    eor.b #$01
    sta.w !actor_flip,x
    jsl.l ActorSpawnPlace
    rep.b #$20
    tdc
    sta.w !scratch
    sta.w !actor_40,x
    stx.b !K20C_POT
    cpx.w !scratch
    bcs .after
    txa
    sta.w !actor_40,x
    lda.w !scratch
    sta.b !K20C_POT
    phk
    lda.w #Kind20CarrierWaits
    pha
    php
    pla
    sta.b !actor_state+2
    pla
    sta.b !actor_state+4
    lda.w #$0028
    sta.w !K20C_COPY_LEFT
    txy
    ldx.w #$0000
.swap:
    lda.w !actor_state,y
    sta.w !K20C_COPY_TMP
    lda.b !actor_state,x
    sta.w !actor_state,y
    lda.w !K20C_COPY_TMP
    sta.b !actor_state,x
    inx
    inx
    iny
    iny
    dec.w !K20C_COPY_LEFT
    bne .swap
    sep.b #$30
    jmp.w Kind42Start
.after:
    sep.b #$30
.wait:
    brk.b #$00

; @routine 83:9B53 Kind20CarrierWaits
; @actor
; WHAT IT DOES: The guard with its pot waits for Aladdin: near it (box
; $1C) and in its sight, it turns to him, its pot turned too, and
; throws: 16 frames, AnimStart 7, the pot placed at spawn point $3A, 31
; frames, AnimStart 0; then each frame it waits for the pot's word: none
; while the pot lives; the pot gone: Kind20CarrierWalks; SIGNAL 1: 32
; frames, AnimStart 8, 96 frames, Kind20CarrierWalks; SIGNAL 2:
; palette $20, AnimStart 9, 32 frames, palette 5, PLACE $10,
; Actor44Set40And80, Kind20Spotted.
; HOW: ActorNearPlayer (box $1C) and ActorSeesPlayerLine (C clear both),
; else BRK and again. ActorSideOfPlayer as facing and flip, the pot's
; the same and its flip the other, ActorSpawnPlace for the pot; COP $10;
; AnimStart 7; spawn point $3A; ActorSpawnPlace; COP $1F; AnimStart 0.
; Each frame (BRK first): the pot's state 0: JMP Kind20CarrierWalks;
; SIGNAL 0: again; 1: COP $20, AnimStart 8, COP $60, JMP
; Kind20CarrierWalks; else ActorPaletteFromRom Y = $20, AnimStart 9, COP
; $20, ActorPaletteFromRom Y = 5, PLACE $10, Actor44Set40And80, JMP
; Kind20Spotted.
; Entry: D = the guard's record; DB = $80; reached by the actor loop's RTI
; at the guard's BRK above, or (the records swapped) at the resume address
; its code wrote. Exit: never returns.
Kind20CarrierWaits:
    sep.b #$30
    ldy.b #$1C
    jsl.l ActorNearPlayer
    bcs Kind20Carrier_wait
    jsr.w ActorSeesPlayerLine
    bcs Kind20Carrier_wait
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    sta.b !actor_flip
    rep.b #$10
    ldx.b !K20C_POT
    lda.b !actor_facing
    sta.w !actor_facing,x
    eor.b #$01
    sta.w !actor_flip,x
    jsl.l ActorSpawnPlace
    sep.b #$10
    lda.b #$10
    cop.b #$00
    lda.b #$07
    jsl.l AnimStart
    lda.b #$3A
    sta.b !actor_spawn_at
    rep.b #$10
    ldx.b !K20C_POT
    jsl.l ActorSpawnPlace
    sep.b #$10
    lda.b #$1F
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
.watch:
    brk.b #$00
    rep.b #$10
    ldx.b !K20C_POT
    lda.w !actor_state,x
    sep.b #$10
    beq .walks
    lda.b !K20C_SIGNAL
    beq .watch
    dec a
    bne .missed
    lda.b #$20
    cop.b #$00
    lda.b #$08
    jsl.l AnimStart
    lda.b #$60
    cop.b #$00
.walks:
    jmp.w Kind20CarrierWalks
.missed:
    ldy.b #$20
    jsl.l ActorPaletteFromRom
    lda.b #$09
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    ldy.b #$05
    jsl.l ActorPaletteFromRom
    lda.b #$10
    sta.b !K20C_PLACE
    jsr.w Actor44Set40And80
    jmp.w Kind20Spotted
%ends_at($839BE5)
