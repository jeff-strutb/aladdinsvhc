; kind23_25.asm: actor kinds $23 and $25. Kind $23 makes four children of
; its kind across the view, each of which shows itself, waits and makes a
; kind $6D, then ends; kind $25 plays sound $3E every 291 frames while
; $1423 is 0. Their waits are Kind23Wait (kind_waits_rest.asm) and the
; loop's default.
; C: src/aladdin/actor/kind23_25.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K23_PART = $07                 ; 0: the maker; 1-4: a child, its place in the tables
!K23_XS = $D06E                 ; (data bank $80) the children's x (words), by part
!K23_NEXT_07 = $D076            ; (data bank $80) the $07 each child gives its kind $6D (bytes), by part
!K23_LOOK = $2B                 ; sprite attributes (word)
!V_1423 = $1423                 ; nonzero: kind $25 keeps quiet

; @routine 83:9CEB Kind23Start
; @actor
; WHAT IT DOES: Actor kind $23's start: with PART 0 it makes four children
; of its kind (parts 4 down to 1) and ends; a child takes its x from
; K23_XS, y $40, look $0600, shows animation $19 for $1A frames, makes a
; kind $6D with the $07 of K23_NEXT_07, and ends.
; HOW: PART 0: spawn point 0; PART = 4; ActorSpawnFacing kind $23 with Y =
; PART while PART counts down; Kind23Second (BRA). Else X = (PART - 1) * 2:
; x pixels = K23_XS word, look $0600, y pixels low $40, ACTOR_08 OR 0,
; ACTOR_09 bits $90; AnimStart $19; COP $1A; PART = K23_NEXT_07 by PART - 1;
; spawn point 0; ActorCreateChild8 kind $6D; Kind23Second (BRA).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
; actor loop's jump for kind $23 (the table at $83:8CAB). Exit: never
; returns.
%at($839CEB)
Kind23Start:
    lda.b !K23_PART
    bne .child
    stz.b !actor_spawn_at
    lda.b #$04
    sta.b !K23_PART
.make:
    lda.b !K23_PART
    tay
    lda.b #$23
    jsl.l ActorSpawnFacing
    dec.b !K23_PART
    bne .make
    bra Kind23Second
.child:
    lda.b !K23_PART
    dec a
    asl a
    tax
    rep.b #$20
    lda.w !K23_XS,x
    sta.b !actor_x+1
    lda.w #$0600
    sta.b !K23_LOOK
    sep.b #$20
    lda.b #$40
    sta.b !actor_y+1
    lda.b !actor_08
    ora.b #$00
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$19
    jsl.l AnimStart
    lda.b #$1A
    cop.b #$00
    ldx.b !K23_PART
    dex
    lda.w !K23_NEXT_07,x
    sta.b !K23_PART
    stz.b !actor_spawn_at
    lda.b #$6D
    jsl.l ActorCreateChild8
    bra Kind23Second

; @routine 83:9D43 Kind23Second
; @actor
; WHAT IT DOES: Kind $23's end, and where its actors go on from the actor
; loop in a state with bit 7 set (the table at $83:8DFD): ActorEndPlaced.
; HOW: JMP ActorEndPlaced.
; Entry: D = the actor's record; reached by Kind23Start's branches and the
; actor loop's jump. Exit: never returns.
Kind23Second:
    jmp.w ActorEndPlaced
%ends_at($839D46)

; @routine 83:9D4B Kind25Start
; @actor
; WHAT IT DOES: Actor kind $25's start: for good, every 291 frames (255,
; then 36) it plays sound $3E unless $1423 is nonzero.
; HOW: ACTOR_08 0; COP $FF; COP $24; $1423 nonzero: again; else sound $3E
; and again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the actor
; loop's jump for kind $25 (the table at $83:8CAB). Exit: never returns.
%at($839D4B)
Kind25Start:
    stz.b !actor_08
.wait:
    lda.b #$FF
    cop.b #$00
    lda.b #$24
    cop.b #$00
    lda.w !V_1423
    bne .wait
    lda.b #$3E
    jsl.l SoundQueueAdd
    bra .wait
%ends_at($839D62)
