; kind2d.asm: actor kind $2D, a thrower: it watches for Aladdin within its
; reach ahead of it and throws copies of its kind at him (each with $07
; bit 7 set), which fly by their motion preset and hurt him in their zone;
; and the ROM check its start makes first.
; C: src/aladdin/actor/kind2d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!K2D_FRAMES = $40               ; the watch: frames left; the thrown copy: frames of a pose
!K2D_ARC = $41                  ; the throw's arc (the copy's $07, bit 7 set); the copy: its pose
!K2D_POSE = $43                 ; the throw's pose: 4 (level), 3 (Aladdin above)
!K2D_COUNT = $45                ; the throw's frames left
!K2D_FIRST = $47                ; the throw's frames, by $07
!K2D_AT = $48                   ; the frame of the throw the copy leaves at, by $07
!K2D_AGAIN = $49                ; set once a $07 = 2 thrower has thrown twice
!K2D_4F = $4F                   ; 0: a thrower; 1: a thrown copy
!K2D_29 = $29                   ; word: $8911
!K2D_REACH_X = $D122            ; (bank $83) by facing * 4: x offset (word), then reach (word)
!K2D_REACH = $D124
!K2D_FRAMES_BY = $D130          ; by $07: K2D_FIRST
!K2D_AT_BY = $D133              ; by $07: K2D_AT
!K2D_NEXT_ARC = $D136           ; by the arc: the second throw's arc
!K2D_POSES = $D12A              ; by the copy's $07: its first pose
!SPAWN_COUNTS = $7EFF00         ; by hit box: copies spawned
!CHECK_A = $83989E              ; two code bytes the ROM check reads: a NOP ($EA) at either
!CHECK_B = $8398AC              ; means the ROM was changed
!NOP = $EA

; @routine 83:A82F Kind2DStart
; @actor
; WHAT IT DOES: Actor kind $2D's start: first a check that two bytes of
; the actor loop's code are not NOPs (a changed ROM: then its hit box is
; a random one, code the retail game never runs); a thrown copy ($07
; bit 7) goes to Kind2DThrown; a thrower shows itself, faces Aladdin and
; rests (Kind2DRest).
; HOW: CHECK_A or CHECK_B $EA: RandomNext, its A to $06 ($83:A83F).
; $07 bit 7: Kind2DThrown (JMP). Else K2D_4F 0, ACTOR_08 bit 1 set,
; K2D_29 $8911, AnimStart 0, ActorPictureQueue, BRK, ACTOR_09 bits $D0
; set, facing and flip by ActorSideOfPlayer; Kind2DRest (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $2D in state $0C (the table at $83:8CAB)
; and for kind $2C with state bit 7 set (the table at $83:8DFD). Exit:
; never returns.
%at($83A82F)
Kind2DStart:
    lda.l !CHECK_A
    cmp.b #!NOP
    beq .changed
    lda.l !CHECK_B
    cmp.b #!NOP
    bne .checked
.changed:
    jsl.l RandomNext
    sta.b !actor_06
.checked:
    lda.b !actor_07
    bpl .thrower
    jmp.w Kind2DThrown
.thrower:
    stz.b !K2D_4F
    lda.b #$02
    tsb.b !actor_08
    lda.b #$11
    sta.b !K2D_29
    lda.b #$89
    sta.b !K2D_29+1
    lda.b #$00
    jsl.l AnimStart
    jsl.l ActorPictureQueue
    brk.b #$00
    lda.b #$D0
    tsb.b !actor_09
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    sta.b !actor_flip
    jmp.w Kind2DRest

; @routine 83:A875 Kind2DWatch
; @actor
; WHAT IT DOES: The thrower's watch: for up to 128 frames, each frame,
; when Aladdin is near its height (from 64 above it to 96 below) and
; within its reach ahead: pose 2, the arc by how far he is (3 more when he
; is above it), the pose 3 or 4, a wait (1 frame, or 32 for $07 = 0),
; then the throw. At frame 96 it turns to face him in pose 1. The time
; up, it rests (Kind2DRest).
; HOW: K2D_FRAMES $80. Each frame (BRK): (its y + $40 - Aladdin's y) <
; $A0 and (its x + K2D_REACH_X - his x) < K2D_REACH, by facing * 4:
; AnimStart 2; K2D_ARC (word) = (|its x - his x| - 16) / 32; its y - 16
; below his: K2D_POSE 3 and K2D_ARC + 3, else 4; COP 1 ($20 with $07 0);
; the throw ($83:A90C, in Kind2DRest's code). Else at K2D_FRAMES $20
; facing and flip by ActorSideOfPlayer and AnimStart 1; K2D_FRAMES down;
; at 0 Kind2DRest.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; a jump from Kind2DRest and Kind2DAfterThrow. Exit: never returns.
%at($83A875)
Kind2DWatch:
    lda.b #$80
    sta.b !K2D_FRAMES
.frame:
    brk.b #$00
    rep.b #$21
    lda.b !actor_y+1
    adc.w #$0040
    sec
    sbc.w !player+!actor_y+1
    cmp.w #$00A0
    sep.b #$20
    bcs .not_near
    lda.b !actor_facing
    asl a
    asl a
    tax
    rep.b #$21
    lda.b !actor_x+1
    adc.w !K2D_REACH_X,x
    sec
    sbc.w !player+!actor_x+1
    cmp.w !K2D_REACH,x
    sep.b #$20
    bcs .not_near
    lda.b #$02
    jsl.l AnimStart
    rep.b #$20
    lda.b !actor_x+1
    sec
    sbc.w !player+!actor_x+1
    bpl .distance
    eor.w #$FFFF
    inc a
.distance:
    sec
    sbc.w #$0010
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta.b !K2D_ARC
    lda.b !actor_y+1
    sec
    sbc.w #$0010
    cmp.w !player+!actor_y+1
    sep.b #$20
    ldy.b #$04
    bcs .pose
    lda.b #$03
    clc
    adc.b !K2D_ARC
    sta.b !K2D_ARC
    dey
.pose:
    sty.b !K2D_POSE
    lda.b #$01
    ldy.b !actor_07
    bne .wait
    lda.b #$20
.wait:
    cop.b #$00
    bra Kind2DRest_throw
.not_near:
    lda.b !K2D_FRAMES
    cmp.b #$20
    bne .count
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$01
    jsl.l AnimStart
.count:
    dec.b !K2D_FRAMES
    beq Kind2DRest
    jmp.w .frame

; @routine 83:A903 Kind2DRest
; @actor
; WHAT IT DOES: The thrower at rest (pose 7), then on watching; its code
; also holds the throw ($83:A90C): the pose 3 or 4, and on the frame its
; $07 says a copy of its kind thrown (sound $11; the copy's $07 the arc
; with bit 7 set, its look the thrower's, its $4F 1, counted in
; SPAWN_COUNTS); a $07 = 2 thrower throws twice, the second on the arc
; K2D_NEXT_ARC gives; then Kind2DAfterThrow.
; HOW: AnimStart 7; Kind2DWatch (JMP). The throw: K2D_AGAIN 0; K2D_FIRST
; and K2D_AT by $07; AnimStart K2D_POSE; ACTOR_SPAWN_AT $16 or $18 (pose
; 4 or 3... its bit 2); K2D_COUNT frames of BRK, at K2D_AT: SoundQueueAdd
; $11, $07 kept on the stack and set to K2D_ARC EOR $80, ActorSpawn with
; its hit box; spawned (N clear): the child's $2B-$2C and $4F, the count
; up; $07 back.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; a jump from Kind2DStart and Kind2DWatch. Exit: never returns.
%at($83A903)
Kind2DRest:
    lda.b #$07
    jsl.l AnimStart
    jmp.w Kind2DWatch
Kind2DRest_throw:
    stz.b !K2D_AGAIN
.again:
    ldx.b !actor_07
    lda.w !K2D_FRAMES_BY,x
    sta.b !K2D_FIRST
    lda.w !K2D_AT_BY,x
    sta.b !K2D_AT
    lda.b !K2D_POSE
    jsl.l AnimStart
    lda.b !K2D_POSE
    lsr a
    and.b #$02
    clc
    adc.b #$16
    sta.b !actor_spawn_at
    lda.b !K2D_FIRST
    sta.b !K2D_COUNT
.frame:
    brk.b #$00
    lda.b !K2D_COUNT
    cmp.b !K2D_AT
    bne .count
    lda.b #$11
    jsl.l SoundQueueAdd
    lda.b !actor_07
    pha
    lda.b !K2D_ARC
    eor.b #$80
    sta.b !actor_07
    lda.b !actor_06
    jsl.l ActorSpawn
    bmi .none
    lda.b !actor_look
    sta.w !actor_look,x
    lda.b !actor_look+1
    sta.w !actor_look+1,x
    lda.b #$01
    sta.w !K2D_4F,x
    sep.b #$10
    ldx.b !actor_06
    lda.l !SPAWN_COUNTS,x
    inc a
    sta.l !SPAWN_COUNTS,x
.none:
    sep.b #$10
    pla
    sta.b !actor_07
.count:
    dec.b !K2D_COUNT
    bne .frame
    lda.b !actor_07
    cmp.b #$02
    bne Kind2DAfterThrow
    lda.b !K2D_AGAIN
    bne Kind2DAfterThrow
    ldx.b !K2D_ARC
    lda.w !K2D_NEXT_ARC,x
    sta.b !K2D_ARC
    inc.b !K2D_AGAIN
    jmp.w .again

; @routine 83:A988 Kind2DAfterThrow
; @actor
; WHAT IT DOES: After a throw: pose 7 for 72 frames, then on watching.
; HOW: AnimStart 7; COP $48; Kind2DWatch (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached
; from the throw ($83:A976, $83:A97A). Exit: never returns.
%at($83A988)
Kind2DAfterThrow:
    lda.b #$07
    jsl.l AnimStart
    lda.b #$48
    cop.b #$00
    jmp.w Kind2DWatch

; @routine 83:A995 Kind2DThrown
; @actor
; WHAT IT DOES: A thrown copy of kind $2D: it flies by the motion preset
; its arc names, stepping through its poses every 13 frames up to pose
; $0E, hurting Aladdin in its zone, until it is out of range. In levels
; 0, 3 and $0B it is drawn with sprite priority $10.
; HOW: ACTOR_08 bit 0 set; ACTOR_09 |= $10 in levels 0, 3, $0B (0
; otherwise); $07 AND $0F; ActorMotionPreset $17 + $07; K2D_ARC (its
; pose) = K2D_POSES by $07, AnimStart; K2D_FRAMES $0D. Each frame (BRK):
; below pose $0E, K2D_FRAMES down, at 0 again $0D and the next pose
; (AnimStart); ActorFly; ActorZoneHurtsPlayer with zone (pose - 8) * 4;
; ActorEndOutOfRange with box 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; a jump from Kind2DStart ($07 bit 7 set). Exit: never returns; ends
; when out of range.
%at($83A995)
Kind2DThrown:
    lda.b #$01
    tsb.b !actor_08
    lda.b #$00
    ldx.w !level
    beq .front
    cpx.b #$03
    beq .front
    cpx.b #$0B
    bne .priority
.front:
    lda.b #$10
.priority:
    tsb.b !actor_09
    lda.b !actor_07
    and.b #$0F
    sta.b !actor_07
    clc
    adc.b #$17
    tay
    jsl.l ActorMotionPreset
    ldy.b !actor_07
    lda.w !K2D_POSES,y
    sta.b !K2D_ARC
    jsl.l AnimStart
    lda.b #$0D
    sta.b !K2D_FRAMES
.frame:
    brk.b #$00
    lda.b !K2D_ARC
    cmp.b #$0E
    beq .fly
    dec.b !K2D_FRAMES
    bne .fly
    lda.b #$0D
    sta.b !K2D_FRAMES
    inc.b !K2D_ARC
    lda.b !K2D_ARC
    jsl.l AnimStart
.fly:
    jsl.l ActorFly
    lda.b !K2D_ARC
    sec
    sbc.b #$08
    asl a
    asl a
    tay
    jsr.w ActorZoneHurtsPlayer
    ldx.b #$00
    jsr.w ActorEndOutOfRange
    bra .frame
