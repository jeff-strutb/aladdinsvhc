; kind48_5a.asm: two still actors. Kind $48 (placed in no level, spawned
; by no code) shows pose 0 for good; in its second state it ends. Kind $5A
; (the Genie's screen, level $2F) shows the pose the scene asks for in
; $00B5. Their waits are Kind48Wait and Kind5AWait (kind_waits_rest.asm).
; C: src/aladdin/actor/kind48_5a.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!K5A_29 = $29                   ; set to K5A_29_START at its start; purpose not yet established (word)
!K5A_29_START = $8915
!K5A_POSE_CUE = $00B5           ; the Genie's screen: 0 pose 0, 1 pose 1, more pose 2

; @routine 83:CE14 Kind48Start
; @actor
; WHAT IT DOES: Actor kind $48: pose 0, still, for good (its wait
; animates it). Placed in no level and spawned by no code: a record
; restarted as the kind runs it (cov_kind48_5a).
; HOW: $09 = $80; AnimStart 0; a BRK each frame.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $48 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83CE14)
Kind48Start:
    lda.b #$80
    sta.b !actor_09
    lda.b #$00
    jsl.l AnimStart
.frame:
    brk.b #$00
    bra .frame

; @routine 83:CE22 Kind48Second
; @actor
; WHAT IT DOES: Kind $48 in a state with bit 7 set (its second): it ends
; for good. It cannot be hit ($09 bit 6 clear), so no play gives it that
; state: a record put in state $8C runs it (cov_kind48_5a).
; HOW: JMP ActorEndPlaced.
; Entry: D = the actor's record; reached by the actor loop's jump for
; kind $48 in a state with bit 7 set (the table at $83:8DFD).
; Exit: never returns.
%at($83CE22)
Kind48Second:
    jmp.w ActorEndPlaced
%ends_at($83CE25)

; @routine 83:DEBE Kind5AStart
; @actor
; WHAT IT DOES: Actor kind $5A of the Genie's screen: each frame the pose
; its scene asks for in $00B5 (0: pose 0, 1: pose 1, 2 up: pose 2).
; HOW: $08 bit 1, $09 bits 7, 5 and 4 set; redraw; $29 $8915; AnimStart
; 0. Each turn: $00B5 0: BRK; 1: AnimStart 1, BRK; else AnimStart 2, BRK.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $5A in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83DEBE)
Kind5AStart:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$B0
    tsb.b !actor_09
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    rep.b #$20
    lda.w #!K5A_29_START
    sta.b !K5A_29
    sep.b #$20
    lda.b #$00
    jsl.l AnimStart
.turn:
    lda.w !K5A_POSE_CUE
    beq .frame
    dec a
    bne .two
    lda.b #$01
    jsl.l AnimStart
.frame:
    brk.b #$00
    bra .turn
.two:
    lda.b #$02
    jsl.l AnimStart
    bra .frame
%ends_at($83DEF3)
