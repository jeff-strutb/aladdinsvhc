; kind2a.asm: actor kind $2A: its start, which paces (or, no script, leaps
; at Aladdin), and its second state, downed: knocked away, a short
; flight up and a fall. Its wait is Kind2AWait (actor_waits_small.asm).
; C: src/aladdin/actor/kind2a.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND2A_DOWNS = $47             ; counted each time it is downed
!KIND2A_HITS_2 = $48            ; counted when downed by a shot (Kind2AWait)
!KIND2A_RIGHT_2 = $4C           ; counted when downed facing right (Kind2AWait)
!KIND2A_FRAMES = $43            ; frames left of its flight
!KIND2A_BOB_S = $49             ; nonzero: it bobs with its frames (Kind2AWait)
!KIND2A_UP_S = $4B              ; Kind2AWait's bob direction
!KIND2A_FRAME_S = $4A           ; the frame it last bobbed on (Kind2AWait)
!KIND2A_SPEED = $45             ; its pacing speed (word)
!KIND2A_LEG = $44               ; frames of each pace after the first
!KIND2A_BELOW = $41             ; the leaper: how far below it he must be (word)
!KIND2A_LEAP = $40              ; the leaper: frames left of its leap
!KIND2A_SPEEDS = $D0A4          ; in bank $80: pacing speeds by variant (words)
!KIND2A_LEGS = $D0B0            ; in bank $80: frames of the first pace by variant
!KIND2A_REACH = $D0A2           ; in bank $80: the leaper's KIND2A_BELOW by $07
!COPY_TEST = $701331            ; save RAM, which the retail board lacks: a write there reads back on a copier

; @routine 83:A1E5 Kind2AStart
; @actor
; WHAT IT DOES: Actor kind $2A: with bit 4 of $07 it paces across at the
; speed of its variant, slowing to a stop, turning and speeding up again,
; bobbing with its frames. Without it (no script has that) it waits until
; Aladdin is below and near, rises toward him, then leaps across at him,
; or (by $07) drops on him and leaps from there, again and again.
; HOW: KIND2A_RIGHT cleared, ACTOR_09 bits $90 set, AnimStart 0,
; ActorMotionPreset with Y = 6, KIND2A_DOWNS, KIND2A_HITS, KIND2A_BOB and
; KIND2A_UP cleared. Bit 4: by the low digit of $07 its speed (KIND2A_SPEEDS)
; in its speed x and KIND2A_SPEED, KIND2A_FRAMES = KIND2A_LEGS by it and
; KIND2A_LEG twice that; AnimStart 2, facing right, KIND2A_BOB up,
; KIND2A_FRAME = $FF, accel x $14. Then for ever: KIND2A_FRAMES frames of
; ActorMoveX; ActorMoveX and ActorDecelerateX each frame until its speed
; x is 0; COP 8; turned; COP 8; ActorMoveX and ActorAccelerateX until the
; speed reaches KIND2A_SPEED; KIND2A_FRAMES = KIND2A_LEG. Without bit 4:
; KIND2A_BELOW = KIND2A_REACH by $07, $07 halved, facing him; each frame
; until ActorPlayerBelow says he is below and ActorNearPlayer with Y =
; $0C; AnimStart 1, COP 5. $07 now zero: each frame ActorStepY (and
; ActorAccelerateY while its speed y is below $400) until its y plus
; KIND2A_BELOW reaches his; else ActorFall each frame until its y plus
; $18 reaches his, accel y $80, then ActorStepY and ActorDecelerateY each
; frame until it stops, AnimStart 2, COP $20. The leap: AnimStart 2,
; accel y $18 (or as left), accel x $20, KIND2A_LEAP = $38, facing him;
; each frame ActorAccelerateX below $240, ActorDecelerateY, ActorMove
; until KIND2A_LEAP runs out; accel y 6, then each frame
; ActorAccelerateY, ActorMoveX, ActorStepYBack for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $2A in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83A1E5)
Kind2AStart:
    stz.b !KIND2A_RIGHT_2
    lda.b #$90
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    ldy.b #$06
    jsl.l ActorMotionPreset
    stz.b !KIND2A_DOWNS
    stz.b !KIND2A_HITS_2
    stz.b !KIND2A_BOB_S
    stz.b !KIND2A_UP_S
    lda.b !actor_07
    bit.b #$10
    beq .leaper
    jmp.w .lA2FA
.leaper:
    ldx.b !actor_07
    lda.w !KIND2A_REACH,x
    sta.b !KIND2A_BELOW
    stz.b !KIND2A_BELOW+1
    txa
    lsr a
    sta.b !actor_07
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
.watch:
    jsl.l ActorPlayerBelow
    beq .notyet
    ldy.b #$0C
    jsl.l ActorNearPlayer
    bcs .notyet
    lda.b #$01
    jsl.l AnimStart
    lda.b #$05
    cop.b #$00
    lda.b !actor_07
    beq .rise0
    jmp.w .drop
.rise0:
    bra .rise
.notyet:
    brk.b #$00
    bra .watch
.rise:
    jsl.l ActorStepY
    rep.b #$20
    lda.b !actor_y+1
    adc.b !KIND2A_BELOW
    cmp.w !player+!actor_y+1
    bcc .climb
    sep.b #$20
    lda.b #$02
    jsl.l AnimStart
    lda.b #$18
    sta.b !actor_accel_y
.leap:
    lda.b #$20
    sta.b !actor_accel_x
    lda.b #$38
    sta.b !KIND2A_LEAP
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    eor.b #$01
    sta.b !actor_flip
    bra .leapwait
.climb:
    lda.b !actor_speed_y
    cmp.w #$0400
    sep.b #$20
    bcs .climbwait
    jsl.l ActorAccelerateY
.climbwait:
    brk.b #$00
    bra .rise
.leapstep:
    rep.b #$20
    lda.b !actor_speed_x
    cmp.w #$0240
    sep.b #$20
    bcs .fast
    jsl.l ActorAccelerateX
.fast:
    jsl.l ActorDecelerateY
    jsl.l ActorMove
    dec.b !KIND2A_LEAP
    bne .leapwait
    lda.b #$06
    sta.b !actor_accel_y
    bra .glide
.leapwait:
    brk.b #$00
    bra .leapstep
.glide:
    jsl.l ActorAccelerateY
    jsl.l ActorMoveX
    jsl.l ActorStepYBack
    brk.b #$00
    bra .glide
.drop:
    lda.b #$18
    sta.b !actor_accel_y
.fall:
    jsl.l ActorFall
    rep.b #$20
    lda.b !actor_y+1
    adc.w #$0018
    cmp.w !player+!actor_y+1
    sep.b #$20
    bcc .fallwait
    lda.b #$80
    sta.b !actor_accel_y
    bra .settle
.fallwait:
    brk.b #$00
    bra .fall
.settle:
    jsl.l ActorStepY
    jsl.l ActorDecelerateY
    lda.b !actor_speed_y
    bne .settlewait
    lda.b !actor_speed_y+1
    bne .settlewait
    lda.b #$02
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    jmp.w .leap
.settlewait:
    brk.b #$00
    bra .settle
.lA2FA:
    and.b #$0F
    tay
    asl a
    tax
    lda.w !KIND2A_SPEEDS,x
    sta.b !actor_speed_x
    sta.b !KIND2A_SPEED
    lda.w !KIND2A_SPEEDS+1,x
    sta.b !actor_speed_x+1
    sta.b !KIND2A_SPEED+1
    stz.b !actor_speed_x+2
    lda.w !KIND2A_LEGS,y
    sta.b !KIND2A_FRAMES
    asl a
    sta.b !KIND2A_LEG
    lda.b #$02
    jsl.l AnimStart
    stz.b !actor_facing
    lda.b #$01
    sta.b !actor_flip
    inc.b !KIND2A_BOB_S
    lda.b #$FF
    sta.b !KIND2A_FRAME_S
    lda.b #$14
    sta.b !actor_accel_x
.lA32D:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !KIND2A_FRAMES
    bne .lA32D
.lA337:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorDecelerateX
    lda.b !actor_speed_x
    bne .lA337
    lda.b !actor_speed_x+1
    bne .lA337
    lda.b #$08
    cop.b #$00
    lda.b !actor_facing
    sta.b !actor_flip
    eor.b #$01
    sta.b !actor_facing
    lda.b #$08
    cop.b #$00
.lA359:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorAccelerateX
    rep.b #$20
    lda.b !actor_speed_x
    cmp.b !KIND2A_SPEED
    sep.b #$20
    bcc .lA359
    lda.b !KIND2A_SPEED
    sta.b !actor_speed_x
    lda.b !KIND2A_SPEED+1
    sta.b !actor_speed_x+1
    lda.b !KIND2A_LEG
    sta.b !KIND2A_FRAMES
    bra .lA32D

; @routine 83:A37B Kind2ASecond
; @actor
; WHAT IT DOES: Actor kind $2A in its second state (downed): it is knocked
; away from Aladdin, flies up a little, then falls for good (its wait
; ends it out of range). On a cartridge whose save RAM can be written
; (a copy, not the retail board) each frame of the fall also pushes the
; player 16 pixels right.
; HOW: AnimStart 4; ActorMotionPreset with Y = $28; ACTOR_09 bit 7 set;
; KIND2A_DOWNS up. Facing negative (no script has it): facing and flip =
; its bit 0, speed x high byte up, on at the fly. Else KIND2A_RIGHT zero:
; SoundQueueAdd $0F, y plus 9. KIND2A_HITS zero: facing away from the
; player's flip byte, COP 9. ActorMotionPreset with Y = $28; 16 frames
; (BRK) of ActorFly; AnimStart 3; ActorMotionReset with A = $10; then each
; frame (BRK) AnimStep, ActorAccelerateY unless the speed y high byte is
; 3, ActorStepY, and the byte at COPY_TEST counted up: read back equal
; (save RAM), the player's x plus 16.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $2A in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($83A37B)
Kind2ASecond:
    lda.b #$04
    jsl.l AnimStart
    ldy.b #$28
    jsl.l ActorMotionPreset
    lda.b #$80
    tsb.b !actor_09
    inc.b !KIND2A_DOWNS
    lda.b !actor_facing
    bpl .right
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    inc.b !actor_speed_x+1
    bra .fly
.right:
    lda.b !KIND2A_RIGHT_2
    bne .hits
    lda.b #$0F
    jsl.l SoundQueueAdd
    rep.b #$21
    lda.b !actor_y+1
    adc.w #$0009
    sta.b !actor_y+1
    sep.b #$20
.hits:
    lda.b !KIND2A_HITS_2
    bne .preset
    lda.w !player+!actor_flip
    sta.b !actor_flip
    eor.b #$01
    sta.b !actor_facing
    lda.b #$09
    cop.b #$00
.preset:
    ldy.b #$28
    jsl.l ActorMotionPreset
.fly:
    lda.b #$10
    sta.b !KIND2A_FRAMES
.up:
    brk.b #$00
    jsl.l ActorFly
    dec.b !KIND2A_FRAMES
    bne .up
    lda.b #$03
    jsl.l AnimStart
    lda.b #$10
    jsl.l ActorMotionReset
.fall:
    brk.b #$00
    jsl.l AnimStep
    lda.b !actor_speed_y+1
    cmp.b #$03
    beq .step
    jsl.l ActorAccelerateY
.step:
    jsl.l ActorStepY
    lda.l !COPY_TEST
    inc a
    sta.l !COPY_TEST
    cmp.l !COPY_TEST
    bne .on
    rep.b #$21
    lda.w !player+!actor_x+1
    adc.w #$0010
    sta.w !player+!actor_x+1
    sep.b #$20
.on:
    bra .fall
