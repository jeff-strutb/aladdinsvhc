; kind34.asm: actor kind $34, a short-lived figure that animates in
; place or flies off, by its $07, then ends. Drafted by cdraft.py; named
; and described by hand.
; C: src/aladdin/actor/kind34.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND34_COUNT = !kind_count+$34  ; kind $34's count of live actors, up as one starts
!KIND34_PARTS = $BBB9            ; in bank $83: the parts' code, by $07 (words)
!K34_FRAMES = $40                ; frames left
!K34_SPEED_UP = $09              ; ActorSpeedYSet's entry for part 1

; @routine 83:BB97 Kind34Start
; @actor
; WHAT IT DOES: Actor kind $34, a short-lived figure: counts itself in
; KIND34_COUNT (its kind's count of live actors), starts animation 6,
; and by its $07 plays one of three parts for 21 frames before it ends:
; part 0 animates in place, part 1 flies off upward, part 2 ends at
; once.
; HOW: KIND34_COUNT up; ACTOR_09 = $10; the palette byte ($2C) = 4;
; AnimStart 6; K34_FRAMES = $15; then JMP through KIND34_PARTS by $07
; times 2: Kind34Part0, Kind34Part1, Kind34Part2.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $34
; in state $0C (the table at $83:8CAB). Exit: never returns.
%at($83BB97)
Kind34Start:
    lda.l !KIND34_COUNT
    inc a
    sta.l !KIND34_COUNT
    lda.b #$10
    sta.b !actor_09
    lda.b #$04
    sta.b !actor_look+1
    lda.b #$06
    jsl.l AnimStart
    lda.b #$15
    sta.b !K34_FRAMES
    lda.b !actor_07
    asl a
    tax
    jmp.w (!KIND34_PARTS,x)

; @routine 83:BBBF Kind34Part0
; @actor
; WHAT IT DOES: Kind $34's part 0: animates in place for K34_FRAMES
; frames, then ends.
; HOW: Each frame (BRK first) AnimStep, K34_FRAMES down; at 0,
; ActorEndPlaced. No script runs it: written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by a jump from Kind34Start. Exit: never
; returns.
%at($83BBBF)
Kind34Part0:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K34_FRAMES
    bne Kind34Part0
    jml.l ActorEndPlaced

; @routine 83:BBCD Kind34Part1
; @actor
; WHAT IT DOES: Kind $34's part 1: flies off upward, animating, for
; K34_FRAMES frames, then ends.
; HOW: ActorSpeedXClear; ActorSpeedYSet with entry 9; ACTOR_ACCEL_Y = 2;
; each frame (BRK first) AnimStep, ActorFly, K34_FRAMES down; at 0,
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81 (the speed
; table's bank); reached by a jump from Kind34Start. Exit: never returns.
Kind34Part1:
    jsl.l ActorSpeedXClear
    ldy.b #!K34_SPEED_UP
    jsl.l ActorSpeedYSet
    lda.b #$02
    sta.b !actor_accel_y
.frame:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFly
    dec.b !K34_FRAMES
    bne .frame
    jml.l ActorEndPlaced

; @routine 83:BBED Kind34Part2
; @actor
; WHAT IT DOES: Kind $34's part 2: ends at once.
; HOW: JML ActorEndPlaced. No script runs it: written from its bytes.
; Entry: D = the actor's record; reached by a jump from Kind34Start.
; Exit: never returns.
Kind34Part2:
    jml.l ActorEndPlaced
