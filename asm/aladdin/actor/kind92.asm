; kind92.asm: actor kind $92, a marker that puts the level's escort actor at its own place and clears
; the escort's mode, then ends. Its wait is Kind91FreeOff (kind_steps.asm).
; C: src/aladdin/actor/kind92.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K92_ESCORT_RESUME = $E826      ; where the escort's code resumes (its word at +3)
!K92_ESCORT_47 = $47            ; the escort's byte its marker's $07 is kept in
!K92_ESCORT_BUSY = $45          ; the escort's: nonzero while it is busy
!K92_ESCORT_MODE = $4D          ; the escort's mode

; @routine A5:F5BA Kind92Start
; @actor
; WHAT IT DOES: Actor kind $92, a marker that puts the level's escort
; actor at its own place, to resume at $E826, with its $07 in the
; escort's $47, and clears the escort's mode (and, for $07 0, its busy
; flag); then it ends. (Placed in levels $02, $08 and $18.)
; HOW: ACTOR_09 bit 7 set (its kind's wait runs, Kind91FreeOff). The
; escort's x pixels ($0937) and y pixels ($093A) = its own, its resume
; address ($092E, a word) = $E826, its $47 ($0972) = its $07. JMP
; through the four words after it by $07 * 2: 0, the escort's mode
; ($0978) and busy flag ($0970) cleared; 1, 2, 3, the mode cleared; each
; then JML ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML of
; kind $92's stub (Kind92Enter, $83:937C) from the actor loop in the
; start state.
; Exit: never returns: ends in ActorEndPlaced.
%at($A5F5BA)
Kind92Start:
    lda.b #$80
    tsb.b !actor_09
    rep.b #$20
    lda.b !actor_x+1
    sta.w !actor_escort+!actor_x+1
    lda.b !actor_y+1
    sta.w !actor_escort+!actor_y+1
    lda.w #!K92_ESCORT_RESUME
    sta.w !actor_escort+!actor_03
    sep.b #$20
    lda.b !actor_07
    sta.w !actor_escort+!K92_ESCORT_47
    asl a
    tax
    jmp.w (.ways,x)
.ways:
    dw .way0, .way1, .way2, .way3
.way0:
    stz.w !actor_escort+!K92_ESCORT_MODE
    stz.w !actor_escort+!K92_ESCORT_BUSY
    jml.l ActorEndPlaced
.way1:
    stz.w !actor_escort+!K92_ESCORT_MODE
    jml.l ActorEndPlaced
.way2:
    stz.w !actor_escort+!K92_ESCORT_MODE
    jml.l ActorEndPlaced
.way3:
    stz.w !actor_escort+!K92_ESCORT_MODE
    jml.l ActorEndPlaced
