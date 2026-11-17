; guard_start.asm: the start of bank $A5's actors of kind $08 (the guard
; and the scripted figures that share its kind): its record set up at the
; player's place, a count kept in cartridge RAM, and the start its $07
; names; the guard's own start (a short walk, then stopped).
; C: src/aladdin/actor/guard_start.c.held (held with the guard's pieces,
; GuardStop above all, which its start goes on into).

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!G_KIND_TIMES2 = $1476          ; the kind * 2 it was started with (from the actor loop's X)
!G_COUNT = $1475                ; the count in cartridge RAM, after its increment
!G_CART_COUNTS = $700000        ; cartridge RAM: a count by kind * 2 (the board has none: open bus)
!G_SHOWN = $49                  ; nonzero: started by $07 = 12 (the guard shown at once)
!G_STATE = $4D                  ; its state: the entry of GuardWait's table
!G_MOVING = $41
!G_LANDING = $42
!G_IN_AIR = $44
!G_GONE = $45
!G_BACK_OFF = $46
!G_CODE_SET = $146B
!G_146F = $146F

; @routine 83:9205 Kind08Enter
; @actor
; WHAT IT DOES: Where actors of kind $08 go on from the actor loop in its
; start state (the table at $83:8CAB): GuardStart.
; HOW: JML to GuardStart ($A5:E515).
; Entry: D = the actor's record; X = twice its kind; reached by the actor
; loop's jump (JMP (table,X)) by kind. Exit: never returns: as GuardStart.
%at($839205)
Kind08Enter:
    jml.l GuardStart

; @routine A5:E515 GuardStart
; @actor
; WHAT IT DOES: The start of bank $A5's actors of kind $08: shown and
; hittable, facing right, put where the player is (16 lower), a count in
; cartridge RAM by its kind stepped up, its fields cleared, then on by
; its $07: 0 and 12 the guard (a walk of 40 frames, then stopped), 2 and 3
; the scripted figure's ways, 7 to 10 the escort's spots, others their
; own starts.
; HOW: G_KIND_TIMES2 = X; ACTOR_08 bit 0, ACTOR_09 bits $90 set; $28
; $FF; facing and flip 0; $16 $CF8F, $29 $8909; its x = the player's, y =
; his + $10; 8-bit: the byte of G_CART_COUNTS by X, plus 1, to G_COUNT
; and back; G_SHOWN, $40, G_STATE, G_MOVING, G_IN_AIR, G_GONE, G_BACK_OFF,
; G_CODE_SET and G_146F cleared; JMP through the table at $A5:E56D by
; $07. $07 12 ($A5:E587): G_SHOWN 1, then as 0 ($A5:E58B): G_STATE 1,
; G_LANDING $28, ActorSpeedXFromA5 6, AnimStart 1; each frame (BRK)
; ActorMoveX and ActorSnapToGround until G_LANDING runs out; AnimStart 0,
; ActorSpeedXClear, and on into GuardStop.
; Entry: M 8-bit, X 16-bit or 8-bit (twice its kind, $10); D = the
; actor's record; DB = $A5; reached by Kind08Enter's JML. Exit: never
; returns.
%at($A5E515)
GuardStart:
    stx.w !G_KIND_TIMES2
    lda.b #$01
    tsb.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b #$FF
    sta.b $28
    stz.b !actor_facing
    stz.b !actor_flip
    rep.b #$20
    lda.w #$CF8F
    sta.b $16
    lda.w #$8909
    sta.b $29
    lda.w !player+!actor_x+1
    sta.b !actor_x+1
    clc
    lda.w !player+!actor_y+1
    adc.w #$0010
    sta.b !actor_y+1
    sep.b #$30
    lda.l !G_CART_COUNTS,x
    inc a
    sta.w !G_COUNT
    sta.l !G_CART_COUNTS,x
    stz.b !G_SHOWN
    stz.b !actor_40
    stz.b !G_STATE
    stz.b !G_MOVING
    stz.b !G_IN_AIR
    stz.b !G_GONE
    stz.b !G_BACK_OFF
    stz.w !G_CODE_SET
    stz.w !G_146F
    lda.b !actor_07
    asl a
    tax
    jmp.w (.starts,x)
.starts:
    dw .guard, $EB31, PlayerSceneLeap, PlayerSceneHops, $ED0F, $EE29, $EEE1
    dw EscortStaySpot18, EscortWaitSpot0, EscortWaitSpot0C, EscortWaitSpot10, $F00E, .shown
.shown:
    lda.b #$01
    sta.b !G_SHOWN
.guard:
    lda.b #$01
    sta.b !G_STATE
    lda.b #$28
    sta.b !G_LANDING
    ldy.b #$06
    jsr.w ActorSpeedXFromA5
    lda.b #$01
    jsl.l AnimStart
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    dec.b !G_LANDING
    bne .walk
    lda.b #$00
    jsl.l AnimStart
    jsl.l ActorSpeedXClear
assert pc() == $A5E5B6
