; kind95.asm: actor kind $95, figures of the story's scenes: by its $07
; one of fifteen parts, each a short piece of motion (a fall, a fly-past, a
; bounce, a hop in place) with its poses and sounds, some starting the
; story's scene task.
; C: src/aladdin/actor/kind95.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K95_FRAMES = $40               ; frames left of a step (a word in part 14)
!K95_STEP = $41                 ; part 8: its step; the hops: how many are left
!K95_HOP = $4C                  ; the hops: which set, then its index by 2
!K95_BACK = $4E                 ; the hops: their caller's return address, kept while they wait
!STORY_PAGE = $00CF             ; the story's page: nonzero while a page is up
!K95_SCENE_TASK = $81F332       ; the opening story's scene task (not yet filed)
!PART8_FRAMES = $D8F6           ; part 8: each step's frames, by K95_STEP
!HOP_POSE = $D8B6               ; the hops, by set: the pose
!HOP_DELAY = $D8BA              ; the frames before the first
!HOP_COUNT = $D8BE              ; how many
!HOP_UP = $D8C2                 ; by set * 2: the rise's frames, its speed entry
!HOP_DOWN = $D8CA               ; by set * 2: the fall's frames, its speed entry

; @routine 84:9BE3 Kind95Start
; @actor
; WHAT IT DOES: Actor kind $95, a figure of the story's scenes: by its
; $07 (low four bits) one of fifteen parts: a figure that speeds off
; through four poses (0, or 14 after a long wait off screen); one that
; bounces along and falls (2, 1 facing left); one that flies up (4, 3
; facing left, which then starts the story's scene task with scene 6);
; hops in place, then still (5-7); hops, the scene task with scene 1, then
; a flight in six steps (8); a still pose (9-11); a sound after 16 frames
; (12); a sound every $12C frames (13). Unless its $07 bit 4 is set it
; takes palette 5.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set, facing and flip 0;
; $07 bit 4 clear: ACTOR_LOOK's palette bits ($2C AND $0E) = $0A. Then
; JMP through PARTS by $07 AND $0F. Waits by COP, frames by BRK with
; K95_FRAMES counting; motion by ActorSpeedUpMove, ActorMoveY, ActorFly
; and ActorStepYBack with speeds from the bank's entries
; (ActorSpeedXFrom84, ActorSpeedYFrom84) or ActorMotionPreset; the hops
; are Kind95Hops; the scene task is TaskCreate (slot 4, scene, $81:F332,
; the opening story's) once STORY_PAGE is 0; part 8's steps go on through
; PART8_STEPS by K95_STEP. The parts that end do so by ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $95 in state $0C (the table at $83:8DAB,
; through the stub at $83:93A3). Exit: never returns.
%at($849BE3)
Kind95Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b !actor_07
    bit.b #$10
    bne .l9C01
    lda.b #$0E
    trb.b !actor_look+1
    lda.b #$0A
    tsb.b !actor_look+1
.l9C01:
    lda.b !actor_07
    and.b #$0F
    asl a
    tax
    jmp.w (.parts,x)
.parts:
    dw .part0, .part1, .part2, .part3, .part4, .part5, .part6, .part7
    dw .part8, .part9, .part10, .part11, .part12, .part13, .part14
.part14:
    rep.b #$10
    lda.b #$80
    trb.b !actor_09
    ldx.w #$0468
    stx.b !actor_x+1
    ldx.w #$0116
    stx.b !actor_y+1
    ldx.w #$06F0
.l9C3B:
    stx.b !K95_FRAMES
    brk.b #$00
    ldx.b !K95_FRAMES
    dex
    bne .l9C3B
    lda.b #$80
    tsb.b !actor_09
    sep.b #$10
.part0:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    lda.b #$43
    jsl.l SoundQueueAdd
    lda.b #$40
    sta.b !actor_accel_x
    ldy.b #$0C
    jsr.w ActorSpeedXFrom84
    ldy.b #$0F
    jsr.w ActorSpeedYFrom84
    lda.b #$01
.l9C6A:
    sta.b !K95_STEP
    jsl.l AnimStart
    lda.b #$10
    sta.b !K95_FRAMES
.l9C74:
    brk.b #$00
    jsl.l ActorSpeedUpMove
    dec.b !K95_FRAMES
    bne .l9C74
    lda.b !K95_STEP
    inc a
    cmp.b #$05
    bcc .l9C6A
    jml.l ActorEndPlaced
.part1:
    inc.b !actor_facing
.part2:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$28
    cop.b #$00
    lda.b #$D8
    sta.b !K95_FRAMES
    ldy.b #$1E
    jsr.w ActorSpeedYFrom84
    ldy.b #$21
    jsr.w ActorSpeedXFrom84
    lda.b #$09
    sta.b !actor_accel_x
.l9CA7:
    brk.b #$00
    jsl.l ActorSpeedUpMove
    lda.b !actor_speed_x+2
    bmi .l9CD5
    rep.b #$20
    lda.b !actor_speed_x
    cmp.w #$00C0
    bcc .l9CD5
    lda.b !actor_speed_y
    eor.w #$FFFF
    inc a
    sta.b !actor_speed_y
    sep.b #$20
    lda.b !actor_speed_y+2
    eor.b #$FF
    sta.b !actor_speed_y+2
    ldy.b #$21
    jsr.w ActorSpeedXFrom84
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
.l9CD5:
    sep.b #$20
    dec.b !K95_FRAMES
    bne .l9CA7
    ldy.b #$15
    jsr.w ActorSpeedYFrom84
    lda.b #$58
    sta.b !K95_FRAMES
.l9CE4:
    brk.b #$00
    jsl.l ActorMoveY
    dec.b !K95_FRAMES
    bne .l9CE4
    jml.l ActorEndPlaced
.part3:
    inc.b !actor_facing
.part4:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$88
    cop.b #$00
    ldy.b #$1B
    jsr.w ActorSpeedYFrom84
    ldy.b #$12
    jsr.w ActorSpeedXFrom84
    lda.b #$20
    sta.b !actor_accel_y
    lda.b #$30
    sta.b !K95_FRAMES
.l9D10:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .l9D25
    rep.b #$20
    lda.w #$0200
    cmp.b !actor_speed_y
    bcs .l9D25
    sta.b !actor_speed_y
.l9D25:
    sep.b #$20
    dec.b !K95_FRAMES
    bne .l9D10
    lda.b !actor_07
    cmp.b #$03
    bne .l9D41
.l9D31:
    brk.b #$00
    lda.w !STORY_PAGE
    bne .l9D31
    jsl.l TaskCreate
    db 4*24, 6                ; slot 4, scene 6
    dl !K95_SCENE_TASK
.l9D41:
    jml.l ActorEndPlaced
.part5:
    ldx.b #$00
    jsr.w Kind95Hops
.l9D4A:
    brk.b #$00
    bra .l9D4A
.part6:
    ldx.b #$01
    jsr.w Kind95Hops
.l9D53:
    brk.b #$00
    bra .l9D53
.part7:
    ldx.b #$02
    jsr.w Kind95Hops
.l9D5C:
    brk.b #$00
    bra .l9D5C
.part8:
    ldx.b #$03
    jsr.w Kind95Hops
    lda.b #$F0
    cop.b #$00
    jsl.l TaskCreate
    db 4*24, 1                ; slot 4, scene 1
    dl !K95_SCENE_TASK
.l9D72:
    brk.b #$00
    lda.w !STORY_PAGE
    bne .l9D72
    ldy.b #$61
    jsl.l ActorMotionPreset
    lda.b #$1D
    sta.b !K95_FRAMES
    inc.b !actor_facing
.l9D85:
    brk.b #$00
    jsl.l ActorFly
    dec.b !K95_FRAMES
    bne .l9D85
    rep.b #$20
    lda.w #$0090
    sta.b !actor_x+1
    lda.w #$0090
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$80
    cop.b #$00
    stz.b !actor_facing
    ldy.b #$66
    jsl.l ActorMotionPreset
    stz.b !K95_STEP
    lda.b #$09
.l9DAD:
    jsl.l AnimStart
    ldx.b !K95_STEP
    lda.w !PART8_FRAMES,x
    sta.b !K95_FRAMES
.l9DB8:
    brk.b #$00
    jsl.l ActorFly
    lda.b #$E0
    cmp.b !actor_speed_y
    bcs .l9DC6
    sta.b !actor_speed_y
.l9DC6:
    dec.b !K95_FRAMES
    bne .l9DB8
    lda.b !K95_STEP
    asl a
    tax
    jmp.w (.steps,x)
.steps:
    dw .step0, .step1, .step2, .step3, .step4, .step5
.step0:
    inc.b !K95_STEP
    lda.b #$0A
    bra .l9DAD
.step1:
    inc.b !K95_STEP
    lda.b #$10
    sta.b !actor_speed_x
    lda.b #$0B
    bra .l9DAD
.step2:
    inc.b !K95_STEP
    lda.b #$20
    sta.b !actor_speed_x
    lda.b #$0C
    bra .l9DAD
.step3:
    inc.b !K95_STEP
    lda.b #$03
    tsb.b !actor_08
    lda.b #$60
    sta.b !K95_FRAMES
.l9E01:
    brk.b #$00
    jsl.l ActorStepYBack
    dec.b !K95_FRAMES
    bne .l9E01
    lda.b #$60
    cop.b #$00
    lda.b #$0D
    jsl.l AnimStart
    rep.b #$20
    lda.w #$00A0
    sta.b !actor_x+1
    lda.w #$006C
    sta.b !actor_y+1
    sep.b #$20
    inc.b !actor_facing
    lda.b #$0D
    bra .l9DAD
.step4:
    inc.b !K95_STEP
    lda.b !actor_08
    and.b #$FC
    eor.b #$02
    sta.b !actor_08
    lda.b #$0E
    jmp.w .l9DAD
.step5:
    jml.l ActorEndPlaced
.part9:
    lda.b #$08
    jsl.l AnimStart
.l9E42:
    brk.b #$00
    bra .l9E42
.part10:
    lda.b #$05
    jsl.l AnimStart
.l9E4C:
    brk.b #$00
    bra .l9E4C
.part11:
    lda.b #$06
    jsl.l AnimStart
.l9E56:
    brk.b #$00
    bra .l9E56
.part12:
    lda.b #$10
    cop.b #$00
    lda.b #$05
    jsl.l SoundQueueAdd
    jml.l ActorEndPlaced
.part13:
    lda.b #$2D
    cop.b #$00
    lda.b #$3E
    jsl.l SoundQueueAdd
    lda.b #$FF
    cop.b #$00
    bra .part13

; @routine 84:9E78 Kind95Hops
; @actor
; WHAT IT DOES: Kind $95's hops in place: set X's pose, a wait, then its
; count of hops, each a rise and a fall for the set's frames at the set's
; speeds; then back to the caller.
; HOW: Its return address pulled into K95_BACK (so its waits leave the
; actor's stack as the loop expects); K95_HOP = X; AnimStart HOP_POSE by
; X; K95_STEP = HOP_COUNT by X; COP HOP_DELAY by X; K95_HOP doubled. Each
; hop: K95_FRAMES from HOP_UP, Y the byte after, ActorSpeedYFrom84, each
; frame (BRK) ActorMoveY until it counts down; the same from HOP_DOWN;
; K95_STEP down by one. K95_BACK pushed back, RTS.
; Entry: M 8-bit, X 8-bit; X = the set (0-3); D = the actor's record; DB
; = $84; called by JSR from Kind95Start's parts 5-8. Exit: M 8-bit, X
; 8-bit; A = K95_BACK's low byte; X = K95_HOP; Y and C as the last
; ActorMoveY leaves them; N and Z from the last count.
%at($849E78)
Kind95Hops:
    rep.b #$20
    pla
    sta.b !K95_BACK
    sep.b #$20
    stx.b !K95_HOP
    lda.w !HOP_POSE,x
    jsl.l AnimStart
    ldx.b !K95_HOP
    lda.w !HOP_COUNT,x
    sta.b !K95_STEP
    lda.w !HOP_DELAY,x
    cop.b #$00
    asl.b !K95_HOP
.hop:
    ldx.b !K95_HOP
    lda.w !HOP_UP,x
    sta.b !K95_FRAMES
    ldy.w !HOP_UP+1,x
    jsr.w ActorSpeedYFrom84
.rise:
    brk.b #$00
    jsl.l ActorMoveY
    dec.b !K95_FRAMES
    bne .rise
    ldx.b !K95_HOP
    lda.w !HOP_DOWN,x
    sta.b !K95_FRAMES
    ldy.w !HOP_DOWN+1,x
    jsr.w ActorSpeedYFrom84
.fall:
    brk.b #$00
    jsl.l ActorMoveY
    dec.b !K95_FRAMES
    bne .fall
    dec.b !K95_STEP
    bne .hop
    rep.b #$20
    lda.b !K95_BACK
    pha
    sep.b #$20
    rts
