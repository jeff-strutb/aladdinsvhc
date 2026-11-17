; kind35.asm: actor kind $35, a gem: touched, it counts on the HUD, plays
; its sound and leaves sparkles. Its wait is ActorWaitAnimEnd10
; (wait_step.asm).
; C: src/aladdin/actor/kind35.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"

arch 65816

!GEMS = $036B                   ; the HUD's second counter (gems): two decimal digits
!GEM_SETS_FOUND = $0373         ; a bit per set gem found (word)
!GEM_SETS = $0375               ; set gems found
!V_0364 = $0364                 ; counted up to 100 by a tenth gem at full health marks
!SET_BITS = $D1B6               ; in bank $80: each set gem's bit, by the low digit of $07 (words)
!TEST_BOX_X_35 = $1484          ; the touch test box's x and y (words)
!TEST_BOX_Y_35 = $1486
!PLACED_FLAGS_35 = $7EC600      ; a byte per placement: bit 7 set, never placed again
!KIND35_TIMES = $40             ; sparkles left to leave; a sparkle's frames

; @routine 83:BBF2 Kind35Start
; @actor
; WHAT IT DOES: Actor kind $35, a gem: when Aladdin (with health) touches
; it, it counts on the HUD's gem counter (three for a set gem, by bit 4
; of $07, kept as found in GEM_SETS_FOUND), every tenth refills his
; health or adds a mark (no script has that), plays its sound, is never
; placed again, and leaves three sparkles (kind $27) where he touched it.
; HOW: ACTOR_08 = 2, ACTOR_09 bits $90 set, flip 0, its look $04E0 at
; $2B (level $035C 6) or $0440. A set gem (bit 4) not found yet and not
; stage 2: AnimStart $1D; else $07 cleared, AnimStart $1C. Each frame
; (BRK) until ActorHitsTestBoxLong clears C with health nonzero. A set
; gem: (not stage 2) GEM_SETS up and its bit in GEM_SETS_FOUND (by $07,
; from SET_BITS), GEMS up by 2. GEMS up; its low digit at $0A: less $0A
; plus $10, and at $A0 in the high digit (no script) the low digit kept
; alone, health marks below $0A: one more and health = them; at $0A:
; health = $0A, V_0364 up to 100; sound 9. Else sound $31 ($32 with level
; $035C 6). PLACED_FLAGS by its $34 = $80; ACTOR_08 bit 3 and $21
; cleared; three times: its x and y = the test box's, ActorSpawn kind $27
; (a child: $07 = $10, its look, $40 = $10), COP 7. Then ActorEndPlaced
; (by the JML at $83:BD1E).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $35 in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83BBF2)
Kind35Start:
    lda.b #$02
    sta.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    stz.b !actor_flip
    rep.b #$20
    lda.w #$04E0
    ldx.w !level_035c
    cpx.b #$06
    beq .lBC0B
    lda.w #$0440
.lBC0B:
    sta.b !actor_look
    sep.b #$20
    lda.b !actor_07
    bit.b #$10
    beq .lBC31
    lda.w !v_033f
    cmp.b #$02
    beq .lBC2D
    lda.b !actor_07
    and.b #$0F
    tay
    rep.b #$20
    lda.w !SET_BITS,y
    and.w !GEM_SETS_FOUND
    sep.b #$20
    bne .lBC31
.lBC2D:
    lda.b #$1D
    bra .lBC35
.lBC31:
    stz.b !actor_07
    lda.b #$1C
.lBC35:
    jsl.l AnimStart
.lBC39:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcs .lBC39
    lda.w !health
    beq .lBC39
    lda.b !actor_07
    and.b #$10
    beq .lBC6B
    lda.w !v_033f
    cmp.b #$02
    beq .lBC65
    inc.w !GEM_SETS
    lda.b !actor_07
    and.b #$0F
    tay
    rep.b #$20
    lda.w !SET_BITS,y
    tsb.w !GEM_SETS_FOUND
    sep.b #$20
.lBC65:
    inc.w !GEMS
    inc.w !GEMS
.lBC6B:
    inc.w !GEMS
    lda.w !GEMS
    and.b #$0F
    cmp.b #$0A
    bmi .lBCB4
    lda.w !GEMS
    sec
    sbc.b #$0A
    clc
    adc.b #$10
    sta.w !GEMS
    and.b #$F0
    cmp.b #$A0
    bne .lBCB4
    lda.w !health_marks
    cmp.b #$0A
    bne .mark
    lda.w !V_0364
    cmp.b #$64
    beq .full
    inc.w !V_0364
.full:
    lda.b #$0A
    sta.w !health
    bra .digit
.mark:
    inc a
    sta.w !health
    sta.w !health_marks
.digit:
    lda.w !GEMS
    and.b #$0F
    sta.w !GEMS
    lda.b #$09
    bra .lBCBF
.lBCB4:
    lda.b #$31
    ldx.w !level_035c
    cpx.b #$06
    bne .lBCBF
    lda.b #$32
.lBCBF:
    jsl.l SoundQueueAdd
    ldx.b !actor_34
    lda.b #$80
    sta.l !PLACED_FLAGS_35,x
    lda.b #$08
    trb.b !actor_08
    stz.b !actor_spawn_at
    lda.b #$03
    sta.b !KIND35_TIMES
.lBCD5:
    rep.b #$20
    lda.w !TEST_BOX_X_35
    sta.b !actor_x+1
    lda.w !TEST_BOX_Y_35
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$27
    jsl.l ActorSpawn
    bmi .lBCFF
    lda.b #$10
    sta.w !actor_07,x
    lda.b !actor_look
    sta.w !actor_look,x
    lda.b !actor_look+1
    sta.w !actor_look+1,x
    lda.b #$10
    sta.w !KIND35_TIMES,x
.lBCFF:
    sep.b #$10
    lda.b #$07
    cop.b #$00
    dec.b !KIND35_TIMES
    bne .lBCD5
    bra .lBD1E

%at($83BD1E)
.lBD1E:
    jml.l ActorEndPlaced
