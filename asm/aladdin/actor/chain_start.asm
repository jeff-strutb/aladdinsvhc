; chain_start.asm: the starts of the nine-piece chain's pieces (kinds $74
; and $75), and the pattern its eighth piece runs for the whole chain: in
; rounds, a sweep across at random speeds, or the head's animations with
; the pieces' lists of children, by random picks per round.
; C: src/aladdin/actor/chain_start.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!CHAIN_GO = $1B18               ; nonzero once the chain is built: the pieces start
!CHAIN_ROUNDS = $1B1A           ; counted up as the pattern starts
!CHAIN_PICK = $1B1B             ; this phase's random pick (0 between phases)
!CHAIN_PHASE = $1B1C            ; 0: the sweep; 1: the head's turn
!CHAIN_ROUND = $1B1D            ; 0-3, a step each two phases
!CHAIN_ROUND_OR = $1B1E         ; ORed with the round for the pick lists
!HEAD_ANIM = $1B20              ; the head's animation (kind73.asm's K73_ANIM)
!HEAD_CUE = $1B22               ; set when the head is done with its own code
!CHAIN_UNTOUCHABLE = $1B2C      ; nonzero: no touch tests
!HEAD_TURNS = $1B33             ; the head's own-code turns, four then the long one
!FIRST_SWEEPS = $1B34           ; sweeps left that take no random pick
!HEAD_POSE = $1B35              ; the head's touch pose (kind73.asm's K73_POSE)
!CHAIN_HEAD = $1B14             ; the head's record (word)
!PIECE_NUMBER = $40             ; the piece's place in the chain (8: the one that runs the pattern)
!SWEEP_FRAMES = $44             ; frames left of a sweep, or of a pause
!SWEEP_SPEED = $1D              ; the sweep's speed entry; negative: still
!SWEEP_STEPS = $D7B2            ; (data bank) by SWEEP_SPEED: the x step (word)
!HEAD_OWN_CODE = $8696          ; where the head's code goes on for its own turn
!PIECE_PALETTE = $7FE7E8        ; a kind $75 piece's look, from here (word), kept beside it

; @routine 84:87C2 Kind74Start
; @actor
; WHAT IT DOES: Actor kind $74, one of the five rear pieces of the
; nine-piece chain: once the chain is built it starts and animates by its
; number, and the eighth piece runs the chain's pattern for good: rounds
; of a sweep across at random speeds and the head's turn (its long turn
; with the pieces' children, or the head's own code at $84:8696), by
; random picks from lists by the round; the others stay.
; HOW: Each frame (BRK) until CHAIN_GO; ActorStartBy40, ActorAnimBy40;
; then (at $84:87EC, in Kind75Start's code) BRK; PIECE_NUMBER 8:
; CHAIN_PHASE and CHAIN_ROUND 0, FIRST_SWEEPS 2, ActorListStart $89A0
; with $10, BRK until ActorListFindIdle, CHAIN_UNTOUCHABLE 0,
; ActorListStartAD, CHAIN_ROUNDS up; then through the table at $84:8845
; by CHAIN_PHASE: the sweep (SWEEP_SPEED from RandomFromList, $FF frames
; of x + SWEEP_STEPS, held from $80 to $F0) or the head's turn (the
; table at $84:88D1 by a pick: the long turn, or HEAD_OWN_CODE written
; as the head's resume address and BRK until HEAD_CUE, four times before
; the long turn); the phase flipped, each two the round up (0-3). Not
; the eighth: ActorYieldForever.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds the tables (bank $84); reached by
; the actor loop's jump for kind $74 in state $0C (the table at $83:8CAB,
; through the stub at $83:9267). Exit: never returns.
%at($8487C2)
Kind74Start:
    brk.b #$00
    lda.w !CHAIN_GO
    beq Kind74Start
    jsr.w ActorStartBy40
    jsr.w ActorAnimBy40
    bra Kind75Start_on

; @routine 84:87D1 Kind75Start
; @actor
; WHAT IT DOES: Actor kind $75, one of the three front pieces of the
; nine-piece chain: as Kind74Start, with its look taken from
; PIECE_PALETTE (and kept beside it) once it starts; its code holds the
; pattern the eighth piece runs ($84:87EC).
; HOW: Each frame (BRK) until CHAIN_GO; ActorStartBy40, ActorAnimBy40;
; ACTOR_LOOK and PIECE_PALETTE + 2 = PIECE_PALETTE (16-bit); then on as
; Kind74Start.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds the tables (bank $84); reached by
; the actor loop's jump for kind $75 in state $0C (the table at $83:8CAB,
; through the stub at $83:9274). Exit: never returns.
%at($8487D1)
Kind75Start:
    brk.b #$00
    lda.w !CHAIN_GO
    beq Kind75Start
    jsr.w ActorStartBy40
    jsr.w ActorAnimBy40
    rep.b #$20
    lda.l !PIECE_PALETTE
    sta.b !actor_look
    sta.l !PIECE_PALETTE+2
    sep.b #$20
Kind75Start_on:
    brk.b #$00
    lda.b !PIECE_NUMBER
    cmp.b #$08
    beq .pattern
    jmp.w ActorYieldForever
.pattern:
    stz.w !CHAIN_PHASE
    stz.w !CHAIN_ROUND
    lda.b #$02
    sta.w !FIRST_SWEEPS
    ldx.b #$A0
    ldy.b #$89
    lda.b #$10
    jsr.w ActorListStart
.until_idle:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .until_idle
    stz.w !CHAIN_UNTOUCHABLE
    jsr.w ActorListStartAD
    inc.w !CHAIN_ROUNDS
    stz.w !CHAIN_PICK
.phase:
    lda.w !CHAIN_PHASE
    asl a
    tax
    jmp.w (.phases,x)
.next:
    inc.w !CHAIN_PHASE
    stz.w !CHAIN_PICK
    lda.w !CHAIN_PHASE
    and.b #$01
    sta.w !CHAIN_PHASE
    bne .same_round
    inc.w !CHAIN_ROUND
    lda.w !CHAIN_ROUND
    cmp.b #$04
    bne .same_round
    stz.w !CHAIN_ROUND
.same_round:
    bra .phase
.phases:
    dw .sweep, .head_turn
.sweep:
    stz.w !HEAD_ANIM
    lda.w !FIRST_SWEEPS
    beq .pick_speed_list
    dec.w !FIRST_SWEEPS
    bra .speed
.pick_speed_list:
    lda.w !CHAIN_ROUND
    ora.w !CHAIN_ROUND_OR
    clc
    adc.b #$0C
    asl a
    tax
    jsl.l RandomFromList
    sta.w !CHAIN_PICK
.speed:
    clc
    lda.w !CHAIN_ROUND
    adc.b #$21
    asl a
    tax
    jsl.l RandomFromList
    sta.b !SWEEP_SPEED
    lda.b #$FF
    sta.b !SWEEP_FRAMES
.sweep_frame:
    brk.b #$00
    lda.w !player+!actor_x+1
    cmp.b !actor_x+1
    bcc .moving
    stz.b !SWEEP_SPEED
.moving:
    ldx.b !SWEEP_SPEED
    bmi .sweep_count
    rep.b #$20
    clc
    lda.w !SWEEP_STEPS,x
    adc.b !actor_x
    sta.b !actor_x
    sep.b #$20
    xba
    tax
    lda.b !SWEEP_SPEED
    bne .left_edge
    cpx.b #$F0
    bcc .left_edge
    lda.b #$F0
    sta.b !actor_x+1
    stz.b !actor_x
    lda.b #$02
    sta.b !SWEEP_SPEED
    bra .left_edge
.left_edge:
    cpx.b #$80
    bcs .sweep_count
    lda.b #$80
    sta.b !actor_x+1
    stz.b !actor_x
    stz.b !SWEEP_SPEED
.sweep_count:
    dec.b !SWEEP_FRAMES
    bne .sweep_frame
    jmp.w .next
.head_turn:
    lda.w !CHAIN_ROUND
    ora.w !CHAIN_ROUND_OR
    clc
    adc.b #$14
    asl a
    tax
    jsl.l RandomFromList
    tax
    jmp.w (.turns,x)
.turns:
    dw .long_turn, .own_turn, .long_turn
.long_turn:
    stz.w !HEAD_TURNS
    lda.b #$01
    sta.w !HEAD_ANIM
    ldx.b #$D5
    ldy.b #$89
    lda.b #$01
    jsr.w ActorListStart
    lda.b #$01
    sta.b !actor_18
.count_down:
    brk.b #$00
    jsr.w StepCountDown
    bne .count_down
.idle_1:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .idle_1
    lda.b #$08
    sta.b !SWEEP_FRAMES
.pause_1:
    brk.b #$00
    dec.b !SWEEP_FRAMES
    bne .pause_1
    lda.b #$06
    sta.w !HEAD_ANIM
    ldx.b #$2E
    ldy.b #$8A
    lda.b #$10
    jsr.w ActorListStart
    lda.b #$49
    jsl.l SoundQueueAdd
    lda.b #$04
    sta.w !HEAD_POSE
.count_up:
    brk.b #$00
    jsr.w StepCountUp
    bne .count_up
.idle_2:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .idle_2
    lda.b #$06
    sta.w !HEAD_ANIM
    lda.b #$3F
    sta.b !SWEEP_FRAMES
.pause_2:
    brk.b #$00
    dec.b !SWEEP_FRAMES
    bne .pause_2
    lda.b #$00
    sta.w !HEAD_ANIM
    stz.w !HEAD_POSE
    ldx.b #$D1
    ldy.b #$89
    lda.b #$02
    jsr.w ActorListStart
.idle_3:
    brk.b #$00
    jsr.w ActorListFindIdle
    bcc .idle_3
    jsr.w ActorListStartAD
    jmp.w .next
.fourth:
    jmp.w .long_turn
.own_turn:
    inc.w !HEAD_TURNS
    lda.w !HEAD_TURNS
    cmp.b #$04
    beq .fourth
    stz.w !HEAD_CUE
    rep.b #$30
    ldx.w !CHAIN_HEAD
    lda.w #!HEAD_OWN_CODE
    sta.w !actor_03,x
    sta.w !actor_35,x
    sep.b #$30
.until_cue:
    brk.b #$00
    lda.w !HEAD_CUE
    beq .until_cue
    jmp.w .next
