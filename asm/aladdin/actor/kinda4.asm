; kinda4.asm: actor kind $A4, figures of a scene with the level-end screen's
; cue ($00AE, as kind $A8's) and its step ($00B5): by its $07 one blinks
; while the cue is set, one drops and shakes the scene's y ($00B0), one
; rises, flies and leaves a child, and two only pose once and end. Its
; wait is KindA4WaitWork (kind_waits_84.asm). The kind that spawns it,
; $59 (83:DD10, spawns at 83:DDB1-DEB1), is placed on no saved stage and
; made by no spawn found in the ROM, so scripts make kind $A4 itself
; (ActorCreate) and set its $07.
; C: src/aladdin/actor/kinda4.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KA4_COUNT = $40                ; way 2: frames of its flight
!KA4_LEFT = $41                 ; frames left of a step; way 1's shake: the table entry
!CUE_AE = $00AE                 ; the scene's cue (as kind $A8's): nonzero, it shows
!SCENE_STEP = $00B5             ; the scene's step: negative or 2 lets ways 1 and 2 go on
!SCENE_Y = $00B0                ; the scene's y that way 1 shakes (word)
!KA4_SHAKES = $D972             ; (data bank $80) way 1's shake, entry 8 down to 0 (words)
!KA4_WAY4 = $10                 ; a $07 of $10 or more takes way 4 (its low 4 bits kept)

; @routine 84:AA24 KindA4Start
; @actor
; WHAT IT DOES: Actor kind $A4, a figure of a scene, by its $07 (0, 2, 4, 6;
; $10 or more for the last): 0 shows while CUE_AE is set, steady for 127
; frames, then blinking every 32, for good; 2 waits for SCENE_STEP,
; moves 32 frames with its pull reversed and shakes SCENE_Y, then ends;
; 4 is put at y $018A (x low byte $80) with a preset motion, and once
; SCENE_STEP comes poses, flies for 17 frames and leaves a child of its
; kind ($07 $10), then ends; 6 and $10 up pose once and end.
; HOW: $08 bits 0 (TSB) and $09 bits $90 set; the palette byte ($2C) 6.
; $07 $10 or more: $07 AND $0F, X = 8; else X = $07. JMP through the table
; below by X.
; 0: AnimStart 0. Again: $08 bit 3 cleared; each frame (BRK) until
; CUE_AE; KA4_LEFT $7F frames (BRK), back to again if CUE_AE clears;
; then for good: $20 frames (BRK, the same test), $08 bit 3 flipped.
; 2: each frame (BRK) until SCENE_STEP is negative or 2; COP $43;
; SoundQueueAdd $41; AnimStart 1; ActorSpeedYSetGravity with A = $FF,
; Y = $8A; $20 frames (BRK) of ActorFallAgainst; COP $0A; 5 frames (BRK
; first): SCENE_Y + $10 (REP #$21: carry clear); then KA4_LEFT 8 down to
; 0, each frame (BRK first): SCENE_Y + KA4_SHAKES entry KA4_LEFT, and
; SoundQueueAdd $3A at entry 5; SoundQueueAdd $3A; ActorEndPlaced.
; 4: y high byte 1, x low byte $80, y low byte $8A, $08 = 1;
; ActorMotionPreset $74; spawn place 0; each frame (BRK) until
; SCENE_STEP is negative or 2; COP $98; y high byte 0; AnimStart 2;
; SoundQueueAdd $43; COP $14; AnimStart 3; y low byte - 4; SoundQueueAdd
; $42; KA4_COUNT $11 frames of ActorFly (then BRK); $07 = $10;
; ActorSpawnSameLook with A = its $06; ActorEndPlaced.
; 6: look's low byte ($2B) 0; AnimStart 5; COP $0E; ActorEndPlaced.
; $10 up: SoundQueueAdd $2D; AnimStart 4; x low byte $34, y low byte
; $16; COP $30; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $A4 in state $0C (the table at $83:8DAB,
; through the stub at $83:942C). Exit: never returns.
%at($84AA24)
KindA4Start:
    lda.b #$00
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$06
    sta.b !actor_look+1
    lda.b !actor_07
    cmp.b #!KA4_WAY4
    bcc .by_07
    and.b #$0F
    sta.b !actor_07
    ldx.b #$08
    bra .way
.by_07:
    ldx.b !actor_07
.way:
    jmp.w (.ways,x)
.ways:
    dw .pulse
    dw .drop
    dw .rise
    dw .pose
    dw .pose_once
.pulse:
    lda.b #$00
    jsl.l AnimStart
.off:
    lda.b #$08
    trb.b !actor_08
.cue_wait:
    brk.b #$00
    lda.w !CUE_AE
    bne .steady
    bra .cue_wait
.steady:
    lda.b #$7F
    sta.b !KA4_LEFT
.steady_frame:
    brk.b #$00
    lda.w !CUE_AE
    beq .off
    dec.b !KA4_LEFT
    bne .steady_frame
.blink:
    lda.b #$20
    sta.b !KA4_LEFT
.blink_frame:
    brk.b #$00
    lda.w !CUE_AE
    beq .off
    dec.b !KA4_LEFT
    bne .blink_frame
    lda.b !actor_08
    eor.b #$08
    sta.b !actor_08
    bra .blink
.drop:
    brk.b #$00
    lda.w !SCENE_STEP
    bmi .dropping
    cmp.b #$02
    bne .drop
.dropping:
    lda.b #$43
    cop.b #$00
    lda.b #$41
    jsl.l SoundQueueAdd
    lda.b #$01
    jsl.l AnimStart
    lda.b #$FF
    ldy.b #$8A
    jsl.l ActorSpeedYSetGravity
    lda.b #$20
    sta.b !KA4_LEFT
.falling:
    brk.b #$00
    jsl.l ActorFallAgainst
    dec.b !KA4_LEFT
    bne .falling
    lda.b #$0A
    cop.b #$00
    lda.b #$05
    sta.b !KA4_LEFT
.push:
    brk.b #$00
    rep.b #$21
    lda.w !SCENE_Y
    adc.w #$0010
    sta.w !SCENE_Y
    sep.b #$20
    dec.b !KA4_LEFT
    bne .push
    lda.b #$08
    sta.b !KA4_LEFT
.shake:
    brk.b #$00
    lda.b !KA4_LEFT
    asl a
    tax
    rep.b #$21
    lda.w !SCENE_Y
    adc.w !KA4_SHAKES,x
    sta.w !SCENE_Y
    sep.b #$20
    lda.b !KA4_LEFT
    cmp.b #$05
    bne .shake_next
    lda.b #$3A
    jsl.l SoundQueueAdd
.shake_next:
    dec.b !KA4_LEFT
    bpl .shake
    lda.b #$3A
    jsl.l SoundQueueAdd
    jml.l ActorEndPlaced
.rise:
    lda.b #$01
    sta.b !actor_y+2
    lda.b #$80
    sta.b !actor_x+1
    lda.b #$8A
    sta.b !actor_y+1
    lda.b #$01
    sta.b !actor_08
    ldy.b #$74
    jsl.l ActorMotionPreset
    stz.b !actor_spawn_at
.rise_wait:
    brk.b #$00
    lda.w !SCENE_STEP
    bmi .rising
    cmp.b #$02
    bne .rise_wait
.rising:
    lda.b #$98
    cop.b #$00
    stz.b !actor_y+2
    lda.b #$02
    jsl.l AnimStart
    lda.b #$43
    jsl.l SoundQueueAdd
    lda.b #$14
    cop.b #$00
    lda.b #$03
    jsl.l AnimStart
    lda.b !actor_y+1
    sec
    sbc.b #$04
    sta.b !actor_y+1
    lda.b #$42
    jsl.l SoundQueueAdd
    lda.b #$11
    sta.b !KA4_COUNT
.flying:
    jsl.l ActorFly
    brk.b #$00
    dec.b !KA4_COUNT
    bne .flying
    lda.b #!KA4_WAY4
    sta.b !actor_07
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    jml.l ActorEndPlaced
.pose:
    stz.b !actor_look
    lda.b #$05
    jsl.l AnimStart
    lda.b #$0E
    cop.b #$00
    jml.l ActorEndPlaced
.pose_once:
    lda.b #$2D
    jsl.l SoundQueueAdd
    lda.b #$04
    jsl.l AnimStart
    lda.b #$34
    sta.b !actor_x+1
    lda.b #$16
    sta.b !actor_y+1
    lda.b #$30
    cop.b #$00
    jml.l ActorEndPlaced
%ends_at($84AB97)
