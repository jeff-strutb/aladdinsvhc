; kind98.asm: actor kind $98 (stage a5 s2), the flier that bobs along the
; top of the screen, drops kind $9A, spawns kind $23, takes six hits and
; throws kind $A5 after each; its wait is Kind98WaitWork. The stub at
; $83:93C2 (kind_wait.asm) calls that.
; C: src/aladdin/actor/kind98.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!ACTOR_PROBES = $16             ; its probe list's address (word)
!K98_29 = $29                   ; set to $891B as it starts (word)
!K98_FRAMES = $40               ; frames left of a wait
!K98_STEPS = $42                ; frames left of a move; throws left (the $07 of each kind $A5)
!K98_RIGHT = $44                ; 0: it flies right across next, else left
!K98_HIT = $46                  ; nonzero while hit: its wait does not hurt or turn shots
!K98_TURNS = $48                ; nonzero: the next turn at the left edge spawns kind $23
!K98_HITS = $11                 ; hits left (6 as it starts); counted down by the hit code
!K98_PROBES = $D90A             ; (data bank $80) its probes
!K98_DONE = $1B14               ; cleared as it starts, counted up when it is beaten
!SCROLL_X_HELD = $1A10          ; nonzero: no follow across (word)
!SCROLL_Y_HELD = $1A12          ; the same up and down (word)
!CUE_BD = $00BD                 ; the scene's cue: set when it is beaten
!K98_TOP = $38                  ; the y it bobs at (pixels)
!K98_RANDOM_LIST = $3E          ; RandomFromList's list: its way, 0 or 2
!K98_DROP_KIND = $9A
!K98_SPAWN_KIND = $23
!K98_THROW_KIND = $A5

; @routine 84:A112 Kind98Start
; @actor
; WHAT IT DOES: Actor kind $98: holds the screen still, waits $80 frames,
; then over and over bobs at the top for $60 frames and by a random pick
; either flies across (dropping a kind $9A on the way) or, when it last
; turned after a hit, spawns a kind $23; then bobs $20 frames, turns to
; face the middle of the screen and flies back up to its height, and goes
; the other way next.
; HOW: K98_HITS 6; K98_DONE and K98_TURNS 0; both scroll holds 1; COP $80;
; $08 bits 0-1 and $09 bits $90 set; 16-bit probes K98_PROBES and K98_29
; $891B; its x pixels $80, y pixels K98_TOP; K98_RIGHT 1, K98_HIT and
; facing 0; SoundQueueAdd $46; AnimStart 0; COP $20; AnimStart 2; on into
; Kind98Cycle (84:A167), whose ways go on in Kind98Flight,
; Kind98StartSpawn and Kind98StartEnd.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop for kind $98 in state $0C (the stub at $83:93BE). Exit:
; never returns.
%at($84A112)
Kind98Start:
    lda.b #$06
    sta.b !K98_HITS
    stz.w !K98_DONE
    stz.b !K98_TURNS
    lda.b #$01
    sta.w !SCROLL_X_HELD
    sta.w !SCROLL_Y_HELD
    lda.b #$80
    cop.b #$00
    lda.b !actor_08
    ora.b #$03
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #!K98_PROBES
    sta.b !ACTOR_PROBES
    lda.w #$891B
    sta.b !K98_29
    sep.b #$20
    lda.b #$80
    sta.b !actor_x+1
    lda.b #!K98_TOP
    sta.b !actor_y+1
    lda.b #$01
    sta.b !K98_RIGHT
    stz.b !K98_HIT
    stz.b !actor_facing
    lda.b #$46
    jsl.l SoundQueueAdd
    lda.b #$00
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$02
    jsl.l AnimStart

; @routine 84:A167 Kind98Cycle
; @actor
; WHAT IT DOES: Kind $98's cycle: back at the top, it bobs $60 frames and
; by a random pick flies across (dropping a kind $9A) or spawns a kind $23.
; HOW: y K98_TOP (fraction 0), $49 0, ActorSpeedYClear, gravity 6,
; ActorSpeedXClear; $60 frames (BRK) ActorBobY; RandomFromList with X =
; K98_RANDOM_LIST, JMP (K98_WAYS,X): way 2 is Kind98StartSpawn; way 0:
; facing K98_RIGHT, speed across $0180; $20 frames ActorMoveX and
; ActorBobY; AnimStart 3, SoundQueueAdd $2F; $10 frames; ActorSpawnFacing
; kind $9A; $20 frames; AnimStart 2; on into Kind98Flight.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached from
; Kind98Start's end and by Kind98StartEnd's jump (84:A2A3). Exit: never
; returns.
%at($84A167)
Kind98Cycle:
    lda.b #!K98_TOP
    sta.b !actor_y+1
    stz.b !actor_y
    stz.b !actor_49
    jsl.l ActorSpeedYClear
    lda.b #$06
    sta.b !actor_accel_y
.clear_x:
    jsl.l ActorSpeedXClear
    lda.b #$60
    sta.b !K98_FRAMES
.bob:
    brk.b #$00
    jsl.l ActorBobY
    dec.b !K98_FRAMES
    bne .bob
    ldx.b #!K98_RANDOM_LIST
    jsl.l RandomFromList
    tax
    jmp.w (.ways,x)
.ways:
    dw .across, Kind98StartSpawn
.across:
    lda.b !K98_RIGHT
    sta.b !actor_facing
    stz.b !actor_speed_x+2
    lda.b #$01
    sta.b !actor_speed_x+1
    lda.b #$80
    sta.b !actor_speed_x
    lda.b #$20
    sta.b !K98_STEPS
.across_1:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorBobY
    dec.b !K98_STEPS
    bne .across_1
    lda.b #$03
    jsl.l AnimStart
    lda.b #$2F
    jsl.l SoundQueueAdd
    lda.b #$10
    sta.b !K98_STEPS
.across_2:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorBobY
    dec.b !K98_STEPS
    bne .across_2
    lda.b #!K98_DROP_KIND
    jsl.l ActorSpawnFacing
    lda.b #$20
    sta.b !K98_STEPS
.across_3:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorBobY
    dec.b !K98_STEPS
    bne .across_3
    lda.b #$02
    jsl.l AnimStart

; @routine 84:A1F3 Kind98Flight
; @actor
; WHAT IT DOES: Kind $98's flight across: each frame it moves and bobs
; until it reaches the far side, then turns (Kind98StartEnd), or spawns a
; kind $23 first (Kind98StartSpawn) when it last turned after a hit.
; HOW: Each frame (BRK first) ActorMoveX and ActorBobY until, K98_RIGHT
; set, its x + $60 is below $20 (K98_RIGHT 0, JMP to Kind98StartEnd), or,
; K98_RIGHT 0, at or past $E0 (K98_RIGHT up; K98_TURNS zero: JMP to
; Kind98StartEnd; else on at Kind98StartSpawn). The JMP at 84:A20F, after
; the first JMP, is never reached.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached from
; Kind98Cycle's way 0 and by Kind98Hit's jump (84:A36A). Exit: never
; returns.
%at($84A1F3)
Kind98Flight:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorBobY
    lda.b !K98_RIGHT
    beq .rightward
    lda.b !actor_x+1
    clc
    adc.b #$60
    cmp.b #$20
    bcs Kind98Flight
    stz.b !K98_RIGHT
    jmp.w Kind98StartEnd
    jmp.w Kind98Cycle_clear_x   ; never reached
.rightward:
    lda.b !actor_x+1
    clc
    adc.b #$60
    cmp.b #$E0
    bcc Kind98Flight
    inc.b !K98_RIGHT
    lda.b !K98_TURNS
    bne Kind98StartSpawn
    jmp.w Kind98StartEnd

; @routine 84:A224 Kind98StartSpawn
; @actor
; WHAT IT DOES: Kind $98's second way: it spawns a kind $23, then turns.
; HOW: K98_TURNS 0; SoundQueueAdd $47; AnimStart 4; $10 frames (BRK)
; ActorBobY; ActorSpawnFacing Y = 0 kind $23; $1A frames; AnimStart 2; JMP
; to Kind98StartEnd.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; Kind98Cycle's JMP (K98_WAYS,X) and Kind98Flight's branch. Exit: never
; returns.
%at($84A224)
Kind98StartSpawn:
    stz.b !K98_TURNS
    lda.b #$47
    jsl.l SoundQueueAdd
    lda.b #$04
    jsl.l AnimStart
    lda.b #$10
    sta.b !K98_FRAMES
.bob_1:
    brk.b #$00
    jsl.l ActorBobY
    dec.b !K98_FRAMES
    bne .bob_1
    ldy.b #$00
    lda.b #!K98_SPAWN_KIND
    jsl.l ActorSpawnFacing
    lda.b #$1A
    sta.b !K98_FRAMES
.bob_2:
    brk.b #$00
    jsl.l ActorBobY
    dec.b !K98_FRAMES
    bne .bob_2
    lda.b #$02
    jsl.l AnimStart
    jmp.w Kind98StartEnd

; @routine 84:A25F Kind98StartEnd
; @actor
; WHAT IT DOES: Kind $98's turn: it bobs, faces the middle of the screen,
; flies back up to its height and goes the other way next.
; HOW: $20 frames (BRK) ActorBobY; SoundQueueAdd $50; facing 0, up if x +
; $60 - $80 leaves no borrow; ActorMotionPreset Y = $70; AnimStart 2; each
; frame (BRK first) ActorFlyAgainst until its y is at most K98_TOP (y
; K98_TOP, fraction 0); K98_RIGHT ^= 1; JMP to Kind98Cycle.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the JMPs at 84:A20C, 84:A221 and 84:A25C. Exit: never returns.
%at($84A25F)
Kind98StartEnd:
    lda.b #$20
    sta.b !K98_STEPS
.bob:
    brk.b #$00
    jsl.l ActorBobY
    dec.b !K98_STEPS
    bne .bob
    lda.b #$50
    jsl.l SoundQueueAdd
    stz.b !actor_facing
    lda.b !actor_x+1
    clc
    adc.b #$60
    sec
    sbc.b #$80
    bcc .faced
    inc.b !actor_facing
.faced:
    ldy.b #$70
    jsl.l ActorMotionPreset
    lda.b #$02
    jsl.l AnimStart
.rise:
    brk.b #$00
    jsl.l ActorFlyAgainst
    lda.b #!K98_TOP
    cmp.b !actor_y+1
    bcc .rise
    sta.b !actor_y+1
    stz.b !actor_y
    lda.b !K98_RIGHT
    eor.b #$01
    sta.b !K98_RIGHT
    jmp.w Kind98Cycle

; @routine 84:A2A6 Kind98Hit
; @actor
; WHAT IT DOES: Kind $98 hit: sound $0F and its hit pose for 8 frames.
; With no hits left it is beaten: the scene's cue, K98_DONE up, its last
; pose with sound $46, $30 frames and it ends. Else it falls to the
; ground, shows its hurt pose $30 frames, rises back to the top, bobs $10
; frames, throws three kind $A5 ($10 and $30 frames about each) and flies
; across again, to the right or left as before, spawning kind $23 at the
; next turn.
; HOW: K98_HIT up; $09 bit 7 set (TSB); SoundQueueAdd $0F; AnimStart 5;
; COP 8. K98_HITS zero: CUE_BD 1, K98_DONE up, AnimStart 1, SoundQueueAdd
; $46, COP $30, JML ActorEndPlaced. Else ActorSpeedXClear;
; ActorSpeedYSetGravity Y = $8D, A = K98_TOP; each frame (BRK) ActorFly
; and ActorOutOfGround until that raises it; AnimStart 6; COP $30; $08 bit
; 4 set (TSB); AnimStart 2; ActorSpeedYSet Y = $90; each frame ActorMoveY
; until its y is at most K98_TOP; y K98_TOP, $49 0, ActorSpeedYClear,
; gravity 6, $08 bit 4 cleared, K98_HIT 0; $10 frames ActorBobY. K98_STEPS
; 3: each time AnimStart 7, $10 frames ActorBobY, Kind98Throw, $30
; frames. AnimStart 2; state $0C; facing K98_RIGHT, speed across $0180;
; K98_TURNS up; JMP to the flight (84:A1F3) in Kind98Start.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop for kind $98 hit (the stub at $83:93C7). Exit: never
; returns.
Kind98Hit:
    inc.b !K98_HIT
    lda.b #$80
    tsb.b !actor_09
    lda.b #$0F
    jsl.l SoundQueueAdd
    lda.b #$05
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b !K98_HITS
    bne .fall
    jmp.w .beaten
.fall:
    jsl.l ActorSpeedXClear
    ldy.b #$8D
    lda.b #!K98_TOP
    jsl.l ActorSpeedYSetGravity
.falling:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfGround
    beq .falling
    lda.b #$06
    jsl.l AnimStart
    lda.b #$30
    cop.b #$00
    lda.b #$10
    tsb.b !actor_08
    lda.b #$02
    jsl.l AnimStart
    ldy.b #$90
    jsl.l ActorSpeedYSet
.rising:
    brk.b #$00
    jsl.l ActorMoveY
    lda.b #!K98_TOP
    cmp.b !actor_y+1
    bcc .rising
    sta.b !actor_y+1
    stz.b !actor_49
    jsl.l ActorSpeedYClear
    lda.b #$06
    sta.b !actor_accel_y
    lda.b !actor_08
    and.b #$EF
    sta.b !actor_08
    stz.b !K98_HIT
    lda.b #$10
    sta.b !K98_FRAMES
.bob:
    brk.b #$00
    jsl.l ActorBobY
    dec.b !K98_FRAMES
    bne .bob
    lda.b #$03
    sta.b !K98_STEPS
.throws:
    lda.b #$07
    jsl.l AnimStart
    lda.b #$10
    sta.b !K98_FRAMES
.before:
    brk.b #$00
    jsl.l ActorBobY
    dec.b !K98_FRAMES
    bne .before
    jsr.w Kind98Throw
    lda.b #$30
    sta.b !K98_FRAMES
.after:
    brk.b #$00
    jsl.l ActorBobY
    dec.b !K98_FRAMES
    bne .after
    dec.b !K98_STEPS
    bne .throws
    lda.b #$02
    jsl.l AnimStart
    lda.b #$0C
    sta.b !actor_state
    lda.b !K98_RIGHT
    sta.b !actor_facing
    stz.b !actor_speed_x+2
    lda.b #$01
    sta.b !actor_speed_x+1
    lda.b #$80
    sta.b !actor_speed_x
    inc.b !K98_TURNS
    jmp.w Kind98Flight
.beaten:
    lda.b #$01
    sta.w !CUE_BD
    inc.w !K98_DONE
    lda.b #$01
    jsl.l AnimStart
    lda.b #$46
    jsl.l SoundQueueAdd
    lda.b #$30
    cop.b #$00
    jml.l ActorEndPlaced

; @routine 84:A389 Kind98Throw
; WHAT IT DOES: Kind $98 throws a kind $A5, its $07 the throws left, with
; sound $3E.
; HOW: SoundQueueAdd $3E; $07 = K98_STEPS; spawn place 0;
; ActorCreateChild8 with A = $A5.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR from Kind98Hit. Exit: M and X 8-bit; as ActorCreateChild8 leaves
; A, X, Y and the flags.
Kind98Throw:
    lda.b #$3E
    jsl.l SoundQueueAdd
    lda.b !K98_STEPS
    sta.b !actor_07
    stz.b !actor_spawn_at
    lda.b #!K98_THROW_KIND
    jsl.l ActorCreateChild8
    rts

; @routine 84:A39C Kind98WaitWork
; WHAT IT DOES: What kind $98 does each frame it waits: its animation
; steps and its picture is sent; it turns to Aladdin; unless hit, Aladdin
; coming down on it or touching it, and his shots turning on it, are
; dealt with.
; HOW: AnimStep; ActorPictureSend; ActorSideOfPlayer into its flip ($15);
; K98_HIT zero: ActorStompOrHurtLong, ActorTurnsShotBackLong.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSL
; from the kind's stub ($83:93C2) through the actor loop's wait table.
; Exit: RTL; as the last routine called leaves A, X, Y and the flags.
Kind98WaitWork:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    jsl.l ActorSideOfPlayer
    sta.b !actor_flip
    lda.b !K98_HIT
    bne .done
    jsl.l ActorStompOrHurtLong
    jsl.l ActorTurnsShotBackLong
.done:
    rtl
%ends_at($84A3B7)
