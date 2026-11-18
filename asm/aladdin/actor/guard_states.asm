; guard_states.asm: the bank $A5 guard's (actor kind $08) wait: each frame
; its place kept for others, its animation and picture, then the step of
; the state it is in: calm or chasing, facing the view, idle, holding back
; while the player is to one side, or running off to the right.
; C: src/aladdin/actor/guard_states.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_state.inc"

arch 65816

!GUARD_STATE = $4D              ; its state: the entry of GuardWait's table
!GUARD_HELD = $41               ; nonzero: it does not chase (its code set is cleared of all but bit 7)
!GUARD_GONE = $45               ; counted up when it runs off for good
!GUARD_CODE_SET = $146B         ; which code it was last sent to: bit 7 set, it chases at once
!GUARD_X_KEPT = $1471           ; its x, kept for others (word)
!GUARD_Y_KEPT = $1473           ; its y, kept for others (word)
!RANGE_X = $1A44                ; the x of the point actors are kept in range of (word)
!player_x = !player+!actor_x+1  ; the player's x, whole pixels (word)
!CODE_CHASE = $E812             ; its code for a chase
!CODE_WALK = $E807              ; its code once the player has no health left

; @routine A5:F020 GuardWait
; WHAT IT DOES: What the guard runs each frame it waits: its place kept
; for others, its animation and picture sent; then by its state: 0
; nothing; 1 calm or chasing; 2 facing the view; 3 nothing; 4 and 5
; holding back while the player is to its left or right; 6 running off
; to the right.
; HOW: GUARD_X_KEPT, GUARD_Y_KEPT = its x, y; AnimStep, ActorPictureSend;
; JMP through the table at $A5:F03D by GUARD_STATE: 0 RTL ($A5:F04B); 1
; ($A5:F04C): GUARD_HELD set: GUARD_CODE_SET bits 0-6 cleared; else
; GUARD_CODE_SET bit 7 set: code word CODE_CHASE; else no health: code
; word CODE_WALK; else GuardChase; 2 GuardFaceView; 3 GuardIdle; 4
; GuardHoldLeft; 5 GuardHoldRight; 6 GuardRunOff. States 4, 5 and 6 and 3
; no script meets: written from their bytes.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the actor loop's stub for
; kind $08 ($83:9209). Exit: M 8-bit, X 8-bit (state 1); else as the
; state's step leaves them.
%at($A5F020)
GuardWait:
    rep.b #$20
    lda.b !actor_x+1
    sta.w !GUARD_X_KEPT
    lda.b !actor_y+1
    sta.w !GUARD_Y_KEPT
    sep.b #$20
    jsl.l AnimStep
    jsl.l ActorPictureSend
    lda.b !GUARD_STATE
    asl a
    tax
    jmp.w (.states,x)
.states:
    dw .none, .calm, GuardFaceView, GuardIdle, GuardHoldLeft, GuardHoldRight, GuardRunOff
.none:
    rtl
.calm:
    lda.b !GUARD_HELD
    beq .chase
    lda.b #$7F
    trb.w !GUARD_CODE_SET
    rtl
.chase:
    lda.w !GUARD_CODE_SET
    bpl .health
    rep.b #$20
    lda.w #!CODE_CHASE
    sta.b !actor_03
    bra .done
.health:
    lda.w !health
    bne .pace
    rep.b #$20
    lda.w #!CODE_WALK
    sta.b !actor_03
    bra .done
.pace:
    rep.b #$20
    jsr.w GuardChase
.done:
    sep.b #$30
    rtl

; @routine A5:F07A GuardHoldLeft
; WHAT IT DOES: The guard's state 4: while the player is to its left, it
; stops if it faces left and, unless held, keeps pace with him; once he
; is level with it or to its right, it goes calm (state 1) at once.
; HOW: 16-bit: the player's x at or past its own: GUARD_STATE 1 and on as
; GuardWait's state 1 ($A5:F04C). Else its facing and flip (word)
; nonzero: speed 0 (STZ of three words from ACTOR_SPEED_X); GUARD_HELD
; and the byte after it (word) zero: GuardChase from its close step
; ($A5:F172).
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
%at($A5F07A)
GuardHoldLeft:
    rep.b #$20
    lda.w !player_x
    cmp.b !actor_x+1
    bcs .level
    lda.b !actor_facing
    beq .held
    stz.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
.held:
    lda.b !GUARD_HELD
    bne .out
    jsr.w GuardChaseClose
.out:
    sep.b #$20
    rtl
.level:
    sep.b #$20
    lda.b #$01
    sta.b !GUARD_STATE
    bra GuardWait_calm

; @routine A5:F09F GuardHoldRight
; WHAT IT DOES: The guard's state 5: as GuardHoldLeft with the sides
; swapped: while the player is to its right, it stops if it faces right
; and, unless held, keeps pace with him; once he is level with it or to
; its left, it goes calm (state 1) at once.
; HOW: 16-bit: its x at or past the player's: GUARD_STATE 1 and on as
; GuardWait's state 1. Else its facing and flip (word) zero: speed 0;
; GUARD_HELD (word) zero: GuardChase from its close step ($A5:F172).
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
%at($A5F09F)
GuardHoldRight:
    rep.b #$20
    lda.b !actor_x+1
    cmp.w !player_x
    bcs .level
    lda.b !actor_facing
    bne .held
    stz.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
.held:
    lda.b !GUARD_HELD
    bne .out
    jsr.w GuardChaseClose
.out:
    sep.b #$20
    rtl
.level:
    sep.b #$20
    lda.b #$01
    sta.b !GUARD_STATE
    bra GuardWait_calm

; @routine A5:F0C4 GuardRunOff
; WHAT IT DOES: The guard's state 6: unless held, it runs off to the
; right at speed, and once it is more than 192 pixels right of the
; player it is gone: counted, state 0, and moved far off.
; HOW: GUARD_HELD set: GUARD_CODE_SET bits 0-6 cleared. Else 16-bit: the
; player's x + $C0 below its x: GUARD_GONE up, GUARD_STATE 0, the high
; bytes of its x and y $3F. Else GuardChase from its speed-up step
; ($A5:F127, which leaves M 8-bit); ACTOR_SPEED_X's middle byte 5;
; facing and flip 0.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
%at($A5F0C4)
GuardRunOff:
    lda.b !GUARD_HELD
    bne .held
    rep.b #$20
    clc
    lda.w !player_x
    adc.w #$00C0
    cmp.b !actor_x+1
    bcc .gone
    jsr.w GuardChaseSpeedUp
    lda.b #$05
    sta.b !actor_speed_x+1
    stz.b !actor_facing
    stz.b !actor_flip
    rtl
.gone:
    sep.b #$20
    inc.b !GUARD_GONE
    stz.b !GUARD_STATE
    lda.b #$3F
    sta.b !actor_x+2
    sta.b !actor_y+2
    rtl
.held:
    lda.b #$7F
    trb.w !GUARD_CODE_SET
    rtl
assert pc() <= $A5F0F4

; @routine A5:F2E5 GuardFaceView
; WHAT IT DOES: The guard's state 2: it faces toward the point actors are
; kept in range of: left when it is more than 96 pixels right of it,
; right when it is more than 96 pixels left of it.
; HOW: 16-bit: RANGE_X + $60 less its x: a borrow: facing and flip 1;
; else 8-bit, $C0 or more: facing and flip 0.
; Entry: M 8-bit, X 8-bit; D = the guard's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by GuardWait's JMP. Exit: M 8-bit.
%at($A5F2E5)
GuardFaceView:
    rep.b #$20
    clc
    lda.w !RANGE_X
    adc.w #$0060
    sec
    sbc.b !actor_x+1
    bcc .left
    cmp.w #$00C0
    sep.b #$20
    bcc .out
    stz.b !actor_facing
    stz.b !actor_flip
.out:
    rtl
.left:
    sep.b #$20
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    rtl

; @routine A5:F308 GuardIdle
; WHAT IT DOES: The guard's state 3: nothing (no script meets it).
; HOW: RTL.
; Entry: any; reached by GuardWait's JMP. Exit: as entered.
%at($A5F308)
GuardIdle:
    rtl
assert pc() <= $A5F309
