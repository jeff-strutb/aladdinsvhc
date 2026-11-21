; kind8b.asm: actor kind $8B, a cloud that rises as kind $5D's does, then bobs, takes hold of Aladdin
; when he is within reach, lifts him and lets him go; ended once its maker has gone. Its wait is
; Kind8BWaitWork.
; C: src/aladdin/actor/kind8b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K8B_PHASE = $13                ; Kind8BGrab's phase: 0 it waits for Aladdin, 1 it keeps him, 2 it waits to again
!K8B_GRABBED = $40              ; set once it has taken hold of Aladdin
!K8B_TIMER = $41                ; frames before it ends after that
!K8B_COUNT = $46                ; phase 2's frames
!K8B_ENDS = $47                 ; set when it is to end: Kind8BWaitWork ends the actor
!K8B_STEP = $48                 ; Kind8BStep's step, 0-8
!K8B_Y_LIMIT = $49              ; the y (a word) Kind8BStep's steps move to
!K8B_HELD = $1427               ; nonzero: the player is kept on the lifting actor
!K8B_LET_GO = $1458             ; nonzero: what holds him lets him go
!K8B_HELD_RESUME = $D6F8        ; the player's code resumed while he is held
!player_resume = $08DE          ; where the player's code resumes (word)
!player_x = $08E7               ; the player's x pixels (word)
!player_y = $08EA               ; and y pixels (word)

; @routine 84:9813 Kind8BStart
; @actor
; WHAT IT DOES: Actor kind $8B (placed once in levels $08 and $18, at x
; 16383: made by other code): a cloud that rises as kind $5D's does
; (Kind5DRise), then, kept by its wait (Kind8BWaitWork), bobs and takes
; hold of Aladdin when he is within reach, lifts and lets him go; it
; ends once its maker has gone.
; HOW: Kind5DRise; $08 = 2, $09 = $80 (its kind's wait runs), the
; palette byte ($2C) = 6; AnimStart $10; K8B_STEP = 1; K8B_Y_LIMIT (the
; word $49) = its y pixels ($0F). Each frame (BRK) Kind5DParentGoneLong:
; gone, to the end; else K8B_GRABBED ($40) clear: on; set: K8B_TIMER
; ($41) = $FF and each frame (BRK) the same test, K8B_TIMER down to 0.
; The end: $08 bit 4 set, a COP wait of $57 frames, K8B_ENDS ($47) up
; (the wait routine then ends the actor), then BRK for ever.
; Entry: M 8-bit, X 8-bit; D = the actor's record; reached by the JML of
; kind $8B's stub (Kind8BEnter, $83:932A) from the actor loop in the
; start state.
; Exit: never returns: ended by Kind8BWaitWork.
%at($849813)
Kind8BStart:
    jsl.l Kind5DRise
    lda.b #$02
    sta.b !actor_08
    lda.b #$80
    sta.b !actor_09
    lda.b #$06
    sta.b !actor_look+1
    lda.b #$10
    jsl.l AnimStart
    lda.b #$01
    sta.b !K8B_STEP
    lda.b !actor_y+1
    sta.b !K8B_Y_LIMIT
    lda.b !actor_y+2
    sta.b !K8B_Y_LIMIT+1
.until_taken:
    brk.b #$00
    jsl.l Kind5DParentGoneLong
    bne .the_end
    lda.b !K8B_GRABBED
    beq .until_taken
    lda.b #$FF
    sta.b !K8B_TIMER
.until_gone:
    brk.b #$00
    jsl.l Kind5DParentGoneLong
    bne .the_end
    dec.b !K8B_TIMER
    bne .until_gone
.the_end:
    lda.b !actor_08
    ora.b #$10
    sta.b !actor_08
    lda.b #$57
    cop.b #$00
    inc.b !K8B_ENDS
.stays:
    brk.b #$00
    bra .stays

; @routine 84:9861 Kind8BWaitWork
; WHAT IT DOES: What actors of kind $8B do each frame they wait:
; Kind8BStep and Kind8BGrab, their animation steps, and once K8B_ENDS is
; set the actor ends.
; HOW: JSR Kind8BStep; JSR Kind8BGrab; AnimStep; K8B_ENDS ($47) zero:
; RTL. Else the five bytes its caller (the actor loop, by the stub at
; $83:932E) pushed pulled, and JML ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kind's wait stub
; (ActorKind8BWait, $83:932E) through the actor loop's wait table.
; Exit: K8B_ENDS clear: RTL, the actor goes on. Set: never returns, the
; actor ended.
%at($849861)
Kind8BWaitWork:
    jsr.w Kind8BStep
    jsr.w Kind8BGrab
    jsl.l AnimStep
    lda.b !K8B_ENDS
    beq .stays
    pla
    pla
    pla
    pla
    pla
    jml.l ActorEndPlaced
.stays:
    rtl

; @routine 84:987A Kind8BStep
; WHAT IT DOES: One step of Kind8BWaitWork's motion: by K8B_STEP,
; nothing (0); its y speed set to $5C (1) and then, each frame, it moves
; against the speed until its y is above K8B_Y_LIMIT (2); the speed set
; to $5D (3) and moving with it until its y is back at K8B_Y_LIMIT (4,
; then step 1); to $5E (5) and moving against it until its y speed is
; negative (6); K8B_Y_LIMIT 8 above its y and the speed set to $5F (7)
; and moving with it until the speed is no longer negative (8, then step
; 4, its acceleration $0A).
; HOW: JMP through the nine words after it by K8B_STEP * 2. Steps 1, 3,
; 5, 7: ActorMotionPreset with Y = $5C, $5D, $5E, $5F; K8B_STEP up; and
; on into step 2, 4, 6, 8: ActorFlyAgainst (2, 6) or ActorFly (4, 8)
; moves it by its speed, then step 2: y (16 bits) against K8B_Y_LIMIT,
; K8B_STEP up unless it is at least; step 4: y against K8B_Y_LIMIT,
; below it: K8B_STEP = 1; step 6: the speed's top byte ($1F) negative:
; K8B_STEP up; step 8: the speed's top byte not negative: K8B_STEP = 4
; and the acceleration ($20) = $0A. Step 7 first sets K8B_Y_LIMIT to y
; less 8.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
; Kind8BWaitWork. Exit: M and X 8-bit.
%at($84987A)
Kind8BStep:
    lda.b !K8B_STEP
    asl a
    tax
    jmp.w (.steps,x)
.steps:
    dw .none, .up, .up_on, .down, .down_on, .lift, .lift_on, .drop, .drop_on
.up:
    ldy.b #$5C
    jsl.l ActorMotionPreset
    inc.b !K8B_STEP
.up_on:
    jsl.l ActorFlyAgainst
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !K8B_Y_LIMIT
    sep.b #$20
    bcs .none
    inc.b !K8B_STEP
.none:
    rts
.down:
    ldy.b #$5D
    jsl.l ActorMotionPreset
    inc.b !K8B_STEP
.down_on:
    jsl.l ActorFly
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !K8B_Y_LIMIT
    sep.b #$20
    bcc .down_done
    lda.b #$01
    sta.b !K8B_STEP
.down_done:
    rts
.lift:
    ldy.b #$5E
    jsl.l ActorMotionPreset
    inc.b !K8B_STEP
.lift_on:
    jsl.l ActorFlyAgainst
    lda.b !actor_speed_y+2
    bpl .lift_done
    inc.b !K8B_STEP
.lift_done:
    rts
.drop:
    rep.b #$20
    sec
    lda.b !actor_y+1
    sbc.w #$0008
    sta.b !K8B_Y_LIMIT
    sep.b #$20
    ldy.b #$5F
    jsl.l ActorMotionPreset
    inc.b !K8B_STEP
.drop_on:
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .drop_done
    lda.b #$04
    sta.b !K8B_STEP
    lda.b #$0A
    sta.b !actor_accel_y
.drop_done:
    rts

; @routine 84:98FF Kind8BGrab
; WHAT IT DOES: The other step of Kind8BWaitWork, by K8B_PHASE: 0, it
; takes hold of Aladdin when he is within its reach (and no actor holds
; him); 1, it keeps him at its place; 2, a wait of $10 frames before it
; can again.
; HOW: JMP through the three words after it by K8B_PHASE * 2. Phase 0
; ($84:990C): $1427 nonzero, or ActorGrabbableByPlayerLong with carry
; set: nothing. Else the player's resume address ($08DE) = $D6F8, the
; saved position ($145D, $145F) = its x and y pixels, $1427 up,
; K8B_GRABBED = 1, AnimStart $15, K8B_STEP = 5, K8B_PHASE up; with
; K8B_ENDS nonzero, $1458 (let go) up. Phase 1 ($84:9946): $1427
; nonzero: the player's x ($08E7) = its x and his y ($08EA) = its y + 7,
; and with K8B_ENDS zero RTS there, else $1458 up; then (and at once
; with $1427 zero) K8B_PHASE up, K8B_COUNT = $10, AnimStart $16. Phase 2
; ($84:9972): K8B_COUNT down; at 0, K8B_PHASE = 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR from
; Kind8BWaitWork. Exit: M and X 8-bit.
%at($8498FF)
Kind8BGrab:
    lda.b !K8B_PHASE
    asl a
    tax
    jmp.w (.phases,x)
.phases:
    dw .reach, .keep, .wait
.reach:
    lda.w !K8B_HELD
    bne .reach_done
    jsl.l ActorGrabbableByPlayerLong
    bcs .reach_done
    rep.b #$20
    lda.w #!K8B_HELD_RESUME
    sta.w !player_resume
    lda.b !actor_x+1
    sta.w !player_pos_saved
    lda.b !actor_y+1
    sta.w !player_pos_saved+2
    sep.b #$20
    inc.w !K8B_HELD
    lda.b #$01
    sta.b !K8B_GRABBED
    lda.b #$15
    jsl.l AnimStart
    lda.b #$05
    sta.b !K8B_STEP
    inc.b !K8B_PHASE
    lda.b !K8B_ENDS
    beq .reach_done
    inc.w !K8B_LET_GO
.reach_done:
    rts
.keep:
    lda.w !K8B_HELD
    beq .let_go
    rep.b #$20
    lda.b !actor_x+1
    sta.w !player_x
    clc
    lda.b !actor_y+1
    adc.w #$0007
    sta.w !player_y
    sep.b #$20
    lda.b !K8B_ENDS
    bne .ends
    rts
.ends:
    inc.w !K8B_LET_GO
.let_go:
    inc.b !K8B_PHASE
    lda.b #$10
    sta.b !K8B_COUNT
    lda.b #$16
    jsl.l AnimStart
    rts
.wait:
    dec.b !K8B_COUNT
    bne .wait_done
    stz.b !K8B_PHASE
.wait_done:
    rts
