; kind27.asm: actor kind $27: a figure that crosses the view and closes
; the level, a still pose, or one that waits for the first to be touched
; and then flies about; both leave a trail. Written from the bytes where
; no script runs them; named and described by hand.
; C: src/aladdin/actor/kind27.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K27_FRAMES = $40                ; frames left (the still pose: its frames)
!K27_STEP = $41                  ; bit 3: the palette fade's step; the waiter: rounds left
!K27_ROUNDS = $41
!K27_OPEN = $42                  ; nonzero: Kind27Wait lets it be touched
!K27_V_036C = $036C              ; counted up as one of bit 3's figures is touched
!K27_VIEW_X = $1773              ; the view's left edge (pixels, word)
!K27_VIEW_Y = $1777              ; its top (pixels, word)
!K27_CLOCK = $032D               ; the frame clock's high byte
!K27_CHECK = $702000             ; the copier check's byte: work RAM on a copier, none on the cartridge
!K27_CLEARED = $08EF             ; + 1 to + $2C: the player's record bytes a copier loses
!CHILD_07 = $0007                ; a child's fields (absolute, by its record)
!CHILD_40 = $0040
!CHILD_LOOK = $002B

; @routine 83:9E3A Kind27Start
; @actor
; WHAT IT DOES: Actor kind $27, by its $07: with bit 3 a figure that
; comes in at the view's top left and flies down, circles, crosses,
; leaves a figure (hit box $85), and then the level's palettes fade out
; and the screen darkens before it ends; with bit 4 a still pose ($0D)
; for its $40 frames, then the end; else Kind27Waiter. Each moving frame
; it leaves a trail (Kind27Trail).
; HOW: ACTOR_09 bits $90 set (TSB); ACTOR_LOOK $0200; facing and flip 1;
; K27_OPEN 0. Bit 3: V_036C 0; facing and flip 0; K27_OPEN up; x pixels
; = VIEW_X - 16, y pixels = VIEW_Y + 32; AnimStart $0C; ActorSpeedXSet
; with entry $2A; ACTOR_ACCEL_Y $38; ActorSpeedYClear; 24 frames (BRK
; first) of Kind27Trail and ActorFly, its vertical speed held to $0300
; at most (then ACTOR_ACCEL_Y 0); the direction ($14) 0, then 15 down to
; 1, 3 frames each of Kind27Trail and ActorMoveByVector (X = $1A);
; direction 0, 12 frames of Kind27Trail and ActorMove; ACTOR_SPAWN_AT 0,
; ActorCreateChild8 with A = $85; COP $20; LevelTablesClear; PaletteSave
; (A = 0, Y = $10); 31 frames (BRK first) of PaletteFadeStep by K27_STEP
; (0 up), Y = $10; TaskCreate (slot 2, parameter 4, BrightnessDownTask);
; ActorEndPlaced. Bit 4: ACTOR_08 bit 0 set (TSB); AnimStart $0D; COP
; with its $40; ActorEndPlaced. Neither: on at Kind27Waiter. No script
; runs bit 3: written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81 (the speed
; table's bank); reached by the actor loop's jump for kind $27 in state
; $0C (the table at $83:8CAB). Exit: never returns.
%at($839E3A)
Kind27Start:
    lda.b #$90
    tsb.b !actor_09
    rep.b #$20
    lda.w #$0200
    sta.b !actor_look
    sep.b #$20
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    stz.b !K27_OPEN
    lda.b !actor_07
    bit.b #$08
    bne .crosser
    jmp.w .other
.crosser:
    stz.w !K27_V_036C
    stz.b !actor_facing
    stz.b !actor_flip
    inc.b !K27_OPEN
    rep.b #$20
    clc
    lda.w !K27_VIEW_X
    adc.w #$FFF0
    sta.b !actor_x+1
    clc
    lda.w !K27_VIEW_Y
    adc.w #$0020
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$0C
    jsl.l AnimStart
    ldy.b #$2A
    jsl.l ActorSpeedXSet
    lda.b #$38
    sta.b !actor_accel_y
    jsl.l ActorSpeedYClear
    lda.b #$18
    sta.b !K27_FRAMES
.down:
    brk.b #$00
    jsr.w Kind27Trail
    jsl.l ActorFly
    rep.b #$20
    lda.w #$0300
    cmp.b !actor_speed_y
    bcs .held
    sta.b !actor_speed_y
    ldx.b #$00
    stx.b !actor_accel_y
.held:
    sep.b #$20
    dec.b !K27_FRAMES
    bne .down
    lda.b #$00
.circle:
    sta.b !actor_facing
    lda.b #$03
    sta.b !K27_FRAMES
.circleFrame:
    brk.b #$00
    jsr.w Kind27Trail
    ldx.b #$1A
    jsl.l ActorMoveByVector
    dec.b !K27_FRAMES
    bne .circleFrame
    lda.b !actor_facing
    dec a
    and.b #$0F
    bne .circle
    stz.b !actor_facing
    lda.b #$0C
    sta.b !K27_FRAMES
.cross:
    brk.b #$00
    jsr.w Kind27Trail
    jsl.l ActorMove
    dec.b !K27_FRAMES
    bne .cross
    stz.b !actor_spawn_at
    lda.b #$85
    jsl.l ActorCreateChild8
    lda.b #$20
    cop.b #$00
    jsl.l LevelTablesClear
    lda.b #$00
    ldy.b #$10
    jsl.l PaletteSave
    lda.b #$1F
    sta.b !K27_FRAMES
    stz.b !K27_STEP
.fade:
    brk.b #$00
    lda.b !K27_STEP
    ldy.b #$10
    jsl.l PaletteFadeStep
    inc.b !K27_STEP
    dec.b !K27_FRAMES
    bne .fade
    jsl.l TaskCreate
    db 2*24, 4                  ; slot 2, parameter 4
    dl BrightnessDownTask
    jml.l ActorEndPlaced
.other:
    bit.b #$10
    beq Kind27Waiter
    lda.b #$01
    tsb.b !actor_08
    lda.b #$0D
    jsl.l AnimStart
    lda.b !K27_FRAMES
    cop.b #$00
    jml.l ActorEndPlaced

; @routine 83:9F2F Kind27Waiter
; @actor
; WHAT IT DOES: Kind $27 with neither bit 3 nor bit 4 of $07: waits
; until one of bit 3's figures has been touched (V_036C), then rises 48
; frames, circles in five steps, and flies off against the walls,
; leaving a trail, again and again three times before it stays. A copier
; check comes first: when the count at $70:2000 stays written (work RAM
; there, which the cartridge does not have), bytes $08F0-$091B of the
; player's record are cleared.
; HOW: Each frame: ACTOR_08 bit 1 set (TSB), BRK, its bit 3 cleared
; (TRB), until V_036C is nonzero; V_036C 0. The byte at K27_CHECK up and
; compared; equal: X = $2C down to 1, 0 to $08EF + X. ACTOR_08 bit 3 set
; (TSB); AnimStart $0C; ActorSpeedYSet with entry $51; 48 frames (BRK
; first) of Kind27Trail and ActorMoveY; K27_OPEN up; K27_ROUNDS 3. A
; round: the direction 4 down to 0, 8 frames each of Kind27Trail and
; ActorMoveByVector (X = $16); direction 0; ActorMotionPreset with Y =
; $81; ACTOR_ACCEL_X $18; frames (BRK first) of Kind27Trail,
; ActorAccelerateX and ActorFlyAgainst while its vertical speed's top
; byte is negative; then frames of Kind27Trail and ActorFlyAgainst, for
; good when K27_ROUNDS is 0, else with ActorDecelerateX until its
; horizontal speed's two high bytes add to 0; K27_ROUNDS down, the next
; round. No script runs it: written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; reached by
; a branch from Kind27Start. Exit: never returns.
Kind27Waiter:
    lda.b #$02
    tsb.b !actor_08
    brk.b #$00
    lda.b #$08
    trb.b !actor_08
    lda.w !K27_V_036C
    beq Kind27Waiter
    stz.w !K27_V_036C
    lda.l !K27_CHECK
    inc a
    sta.l !K27_CHECK
    cmp.l !K27_CHECK
    bne .checked
    ldx.b #$2C
    lda.b #$00
.clear:
    sta.w !K27_CLEARED,x
    dex
    bne .clear
.checked:
    lda.b #$08
    tsb.b !actor_08
    lda.b #$0C
    jsl.l AnimStart
    ldy.b #$51
    jsl.l ActorSpeedYSet
    lda.b #$30
    sta.b !K27_FRAMES
.rise:
    brk.b #$00
    jsr.w Kind27Trail
    jsl.l ActorMoveY
    dec.b !K27_FRAMES
    bne .rise
    inc.b !K27_OPEN
    lda.b #$03
    sta.b !K27_ROUNDS
.round:
    lda.b #$04
.circle:
    sta.b !actor_facing
    lda.b #$08
    sta.b !K27_FRAMES
.circleFrame:
    brk.b #$00
    jsr.w Kind27Trail
    ldx.b #$16
    jsl.l ActorMoveByVector
    dec.b !K27_FRAMES
    bne .circleFrame
    lda.b !actor_facing
    dec a
    and.b #$07
    cmp.b #$07
    bne .circle
    stz.b !actor_facing
    ldy.b #$81
    jsl.l ActorMotionPreset
    lda.b #$18
    sta.b !actor_accel_x
.off:
    brk.b #$00
    jsr.w Kind27Trail
    jsl.l ActorAccelerateX
    jsl.l ActorFlyAgainst
    lda.b !actor_speed_y+2
    bpl .off
.glide:
    brk.b #$00
    jsr.w Kind27Trail
    jsl.l ActorFlyAgainst
    lda.b !K27_ROUNDS
    beq .glide
    jsl.l ActorDecelerateX
    clc
    lda.b !actor_speed_x
    adc.b !actor_speed_x+1
    bne .glide
    dec.b !K27_ROUNDS
    bra .round

; @routine 83:9FDA Kind27Trail
; WHAT IT DOES: Kind $27's trail: every 16 frames of the clock a new
; figure of its hit box at its spawn point 0, sort $10, for 32 frames,
; in its own look; every other time also a sound $37 when it can be hit.
; HOW: K27_CLOCK low 4 bits nonzero: RTS. Bit 4 clear and ACTOR_09 bit 6
; set: SoundQueueAdd $37. ACTOR_SPAWN_AT 0; ActorSpawn with its hit box;
; spawned: the child's $07 = $10, $40 = $20, ACTOR_LOOK = its own; X
; 8-bit. No script runs it: written from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $81; called by
; JSR from Kind27Start and Kind27Waiter. Exit: M 8-bit, X 8-bit; not
; now: A low = the clock byte, Z clear; none free: as ActorSpawn (N
; set); spawned: A low = its look's high byte.
Kind27Trail:
    lda.w !K27_CLOCK
    bit.b #$0F
    bne .done
    bit.b #$10
    bne .spawn
    lda.b !actor_09
    bit.b #$40
    beq .spawn
    lda.b #$37
    jsl.l SoundQueueAdd
.spawn:
    stz.b !actor_spawn_at
    lda.b !actor_06
    jsl.l ActorSpawn
    bmi .done
    lda.b #$10
    sta.w !CHILD_07,x
    lda.b #$20
    sta.w !CHILD_40,x
    lda.b !actor_look
    sta.w !CHILD_LOOK,x
    lda.b !actor_look+1
    sta.w !CHILD_LOOK+1,x
    sep.b #$10
.done:
    rts
