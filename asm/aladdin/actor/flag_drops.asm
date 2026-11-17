; flag_drops.asm: the cases of ActorFlagOnce (actor/actor_steps.asm): an
; actor that drops children once, its flag in V_0359 marking that it has.
; Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/flag_drops.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

!FLAG_V_0359 = $0359             ; eight flags, set by the actors that use these
!FLAG_MASK = $41                 ; the actor's flag's mask
!FLAG_DROP_SORTS_A = $D31E       ; in bank $80: by chance (0-31), a child's sort, 0 none
!FLAG_DROP_SORTS_B = $D33E       ; in bank $80: by chance (0-31), a child's sort
!FLAG_DROP_LEFT = $40            ; FlagDropThree: children left, less one
!FLAG_DROP_BOX = $30             ; the children's hit box
!CHILD_21 = $0021                ; a child's $21 (absolute, by its record)

; @routine 83:CF31 FlagDropMaybe
; WHAT IT DOES: ActorFlagOnce's case 0: by chance drops a child (hit box
; $30) of one of two sorts, and marks the actor's flag when it does or
; when none falls.
; HOW: RandomNext; RANDOM's low 5 bits index FLAG_DROP_SORTS_A (bank $80):
; nonzero, on into FlagDropChild with it; 0, on at ActorFlagOnce's mark
; ($83:CF40): the flag set.
; Entry: M 8-bit, X 8-bit; D = the actor's record; $41 = its flag's mask;
; DB = $80; reached by a jump from ActorFlagOnce. Exit: as FlagDropChild,
; or as the mark leaves them (A low = the mask).
%at($83CF31)
FlagDropMaybe:
    jsl.l RandomNext
    lda.w !random
    and.b #$1F
    tax
    lda.w !FLAG_DROP_SORTS_A,x
    bne FlagDropChild
; (ActorFlagOnce's BRA to its mark, at $83:CF40, follows)

; @routine 83:CF42 FlagDropAlways
; WHAT IT DOES: ActorFlagOnce's case 1: drops a child (hit box $30) of a
; sort picked by chance, and marks the actor's flag.
; HOW: RandomNext; RANDOM's low 5 bits index FLAG_DROP_SORTS_B (bank $80);
; on into FlagDropChild with it. No script runs it: written from its
; bytes.
; Entry: as FlagDropMaybe. Exit: as FlagDropChild.
%at($83CF42)
FlagDropAlways:
    jsl.l RandomNext
    lda.w !random
    and.b #$1F
    tax
    lda.w !FLAG_DROP_SORTS_B,x
    bra FlagDropChild

; @routine 83:CF51 FlagDropFour
; WHAT IT DOES: ActorFlagOnce's case 2: drops a child of sort 4 (hit box
; $30), and marks the actor's flag.
; HOW: A = 4, on into FlagDropChild. No script runs it: written from its
; bytes.
; Entry: as FlagDropMaybe. Exit: as FlagDropChild.
FlagDropFour:
    lda.b #$04

; @routine 83:CF53 FlagDropChild
; WHAT IT DOES: Drops a child of sort A (its $07) with hit box $30 at the
; actor's spawn point $3C; when there was a record for it, marks the
; actor's flag so it drops no more.
; HOW: $07 = A, ACTOR_SPAWN_AT = $3C, ActorCreateChild8 with A = $30;
; created (C clear): on at ActorFlagOnce's mark ($83:CF61), the flag set
; in V_0359 (TSB); else RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; A = the sort; $41 =
; its flag's mask; DB = $80; reached from FlagDropMaybe, FlagDropAlways
; and FlagDropFour. Exit: created: A low = the mask, Z from its test with
; V_0359; else as ActorCreateChild8 leaves them (C set).
FlagDropChild:
    sta.b !actor_07
    lda.b #$3C
    sta.b !actor_spawn_at
    lda.b #!FLAG_DROP_BOX
    jsl.l ActorCreateChild8
    bcs ActorFlagOnce_mark+5
; (ActorFlagOnce's mark, at $83:CF61, follows: LDA, TSB, RTS)

; @routine 83:CF67 FlagDropThree
; WHAT IT DOES: ActorFlagOnce's case 3: drops three children (hit box $30,
; sort 7) at the actor's spawn point $92, numbered 2, 1 and 0 in their
; $21, and marks the actor's flag; when the records run out it stops
; without the mark.
; HOW: FLAG_DROP_LEFT = 2, $07 = 7, ACTOR_SPAWN_AT = $92; each: ActorSpawn
; with A = $30, none free (N): RTS; the child's $21 = FLAG_DROP_LEFT, X
; 8-bit, FLAG_DROP_LEFT down while not negative. Then the flag set in
; V_0359 (TSB).
; Entry: as FlagDropMaybe. Exit: M 8-bit, X 8-bit; all three: A low = the
; mask, Z from its test with V_0359; else as ActorSpawn leaves them.
%at($83CF67)
FlagDropThree:
    lda.b #$02
    sta.b !FLAG_DROP_LEFT
    lda.b #$07
    sta.b !actor_07
    lda.b #$92
    sta.b !actor_spawn_at
.child:
    lda.b #!FLAG_DROP_BOX
    jsl.l ActorSpawn
    bmi .done
    lda.b !FLAG_DROP_LEFT
    sta.w !CHILD_21,x
    sep.b #$10
    dec.b !FLAG_DROP_LEFT
    bpl .child
    lda.b !FLAG_MASK
    tsb.w !FLAG_V_0359
.done:
    rts
