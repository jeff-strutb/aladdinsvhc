; actor_84_misc.asm: bank $84 actor helpers: a child spawned with a hit
; box of $A2, an actor freed far out of range telling its parent, a chain
; of followers, a clamp above the floor, the player's fall test, and a
; grab test.
; C: src/aladdin/actor/actor_84_misc.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

; Places not yet named keep their address as their name.
!dp_0b = $0B  ; direct page
!dp_0d = $0D  ; direct page
!dp_0f = $0F  ; direct page
!dp_41 = $41  ; direct page
!dp_42 = $42  ; direct page
!t_c29a = $C29A  ; data bank
!t_d7c4 = $D7C4  ; data bank
!v_0007 = $0007
!v_000b = $000B
!v_002c = $002C
!v_0037 = $0037
!v_0042 = $0042
!v_004c = $004C
!v_004d = $004D
!v_08e7 = $08E7
!v_08ea = $08EA
!v_08f0 = $08F0
!v_141b = $141B
!v_145a = $145A
!v_1b16 = $1B16
!v_1b19 = $1B19
!v_1b2b = $1B2B
!v_7edfc0 = $7EDFC0

; @routine 84:8145 ActorSpawnA2
; WHAT IT DOES: Starts a child with hit box $A2 at the spawner's spawn
; point $82, telling it its parent and a $07, and counts it in the
; spawner's $42.
; HOW: A kept at $002C; ActorSpawn; none free: back. The child's $07 = the
; spawner's $07 less 3 plus A when that $07 is 3 or more, else the
; spawner's $44; the child's $4C word = the spawner's record (D); the
; spawner's $42 up.
; Entry: M 8-bit, X 8-bit; A = the byte for the child's $07 sum; D = the
; spawner; DB = $81; called by JSR. Exit: M 8-bit, X 8-bit; spawned: X =
; the child's low byte; else as ActorSpawn.
%at($848145)
ActorSpawnA2:
    sta.w !v_002c
    lda.b #$82
    sta.b !actor_spawn_at
    lda.b #$A2
    jsl.l ActorSpawn
    bmi .l8172
    sec
    lda.b !actor_07
    sbc.b #$03
    bcc .l8161
    clc
    adc.w !v_002c
    bra .l8163
.l8161:
    lda.b !actor_44
.l8163:
    sta.w !v_0007,x
    tdc
    sta.w !v_004c,x
    xba
    sta.w !v_004d,x
    inc.b !actor_42
    sep.b #$10
.l8172:
    rts


; @routine 84:832B ActorAnimFreeFar
; WHAT IT DOES: Plays the actor's animation, lets it hurt or be stomped by
; the player, and ends it when out of range by box $58, then counting it
; off its parent's $42.
; HOW: AnimStep, ActorStompOrHurtLong, ActorFreeOutOfRangeLong with X =
; $58; ended (C set): the parent's record (the actor's $4C word) has its
; $42 stepped down.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSL. Exit: M
; 8-bit, X 8-bit.
%at($84832B)
ActorAnimFreeFar:
    jsl.l AnimStep
    jsl.l ActorStompOrHurtLong
    ldx.b #$58
    jsl.l ActorFreeOutOfRangeLong
    bcc .l8348
    rep.b #$10
    ldx.b !actor_4c
    lda.w !v_0042,x
    dec a
    sta.w !v_0042,x
    sep.b #$10
.l8348:
    rtl


; @routine 84:8D04 ActorsFollowSteps
; WHAT IT DOES: Moves each actor of the list from the head at $1B16 to
; where the one before it is, then on along its own direction by the step
; $41 + $1B19 (at least 1).
; HOW: With 16-bit registers, from the head: each next record (the word at
; $37 of the one before, 0 ends) becomes D; its x and y (the bytes at
; $0B-$10) copied from the record before; its direction ($42 and $1FF) in
; Y and the step in A (0 or negative taken as 1); ActorMoveAlong. D kept
; on the stack.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M 8-bit, X
; 8-bit; D as at entry.
%at($848D04)
ActorsFollowSteps:
    phd
    rep.b #$30
    ldx.w !v_1b16
.l8D0A:
    lda.w !v_0037,x
    beq .l8D3D
    tcd
    lda.w !v_000b,x
    sta.b !dp_0b
    lda.w !scratch+1,x
    sta.b !dp_0d
    lda.w !scratch+3,x
    sta.b !dp_0f
    lda.b !dp_42
    and.w #$01FF
    tay
    sep.b #$20
    clc
    lda.b !dp_41
    adc.w !v_1b19
    beq .l8D31
    bpl .l8D33
.l8D31:
    lda.b #$01
.l8D33:
    jsl.l ActorMoveAlong
    rep.b #$30
    tdc
    tax
    bra .l8D0A
.l8D3D:
    sep.b #$30
    pld
    rts


; @routine 84:8F3C ActorClampAboveFloor
; WHAT IT DOES: Keeps the actor's y plus offset A above the floor line at
; its x: C set (and y moved up to the floor) when it reached it.
; HOW: The floor: the word for the actor's x from the table at $7E:DFC0
; ((x - $200) / 4, in words), sign-extended from 9 bits, negated, plus
; $B0. The actor's y + A: negative, C clear; below the floor, y = the
; floor less the actor's offset at $D7C4 (by Y), C set; else C clear.
; Entry: M 16-bit, X 8-bit; A = the offset; Y = the offset table's index;
; D = the actor; called by JSR. Exit: M 16-bit; C as above.
%at($848F3C)
ActorClampAboveFloor:
    sta.w !scratch
    ldy.b #$00
    sec
    lda.b !actor_x+1
    sbc.w #$0200
    lsr a
    lsr a
    and.w #$003E
    tax
    clc
    lda.l !v_7edfc0,x
    and.w #$01FF
    cmp.w #$0100
    bcc .l8F5D
    ora.w #$FF00
.l8F5D:
    eor.w #$FFFF
    inc a
    clc
    adc.w #$00B0
    sta.w !scratch+2
    clc
    lda.b !actor_y+1
    adc.w !scratch
    bmi .l8F7F
    cmp.w !scratch+2
    bcc .l8F80
    lda.w !scratch+2
    sbc.w !t_d7c4,y
    sta.b !actor_y+1
    sec
    rts
.l8F7F:
    clc
.l8F80:
    rts


; @routine 84:8F81 PlayerFallCheck
; WHAT IT DOES: When $1B2B is clear and the player is at or below y $B8,
; puts him into state $8C facing as he is (the fall).
; HOW: $1B2B nonzero: nothing; the player's y negative: nothing; at or
; past $B8: PlayerEnter8CFacingLong.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
; Exit: M 8-bit; as PlayerEnter8CFacingLong when it ran.
%at($848F81)
PlayerFallCheck:
    lda.w !v_1b2b
    bne .l8F98
    rep.b #$20
    lda.w !v_08ea
    bmi .l8F99
    cmp.w #$00B8
    sep.b #$20
    bcc .l8F98
    jsl.l PlayerEnter8CFacingLong
.l8F98:
    rts
.l8F99:
    sep.b #$20
    rts


; @routine 82:88CF ActorGrabTest
; WHAT IT DOES: Tells whether the actor can grab the player: C clear when
; he may be grabbed, touches it, is below it and is within its reach on
; the side he faces.
; HOW: $145A negative, $141B nonzero or $141C zero: C set.
; ActorHitsTestBoxIfActiveLong: no touch, C set. The actor's y at or below
; the player's: C set. Left of him: he must face right and its x + its hit
; box's width - 15 reach his x; right of him: he must face left and its x
; - width + 15 reach it; else C set.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80, the hit boxes' bank;
; called by JSL. Exit: M 8-bit, X 8-bit (C clear, or C set at $82:8931 or
; by the test).
%at($8288CF)
ActorGrabTest:
    lda.w !v_145a
    bmi .l8935
    lda.w !v_141b
    bne .l8935
    lda.w !v_141c
    beq .l8935
    jsl.l ActorHitsTestBoxIfActiveLong
    bcs .l8934
    rep.b #$30
    lda.b !actor_y+1
    cmp.w !v_08ea
    bcs .l8931
    lda.b !actor_06
    and.w #$00FF
    asl a
    asl a
    tay
    lda.b !actor_x+1
    sec
    sbc.w !v_08e7
    bcs .l8916
    lda.w !v_08f0
    and.w #$00FF
    beq .l8931
    lda.w !t_c29a,y
    sec
    sbc.w #$000F
    clc
    adc.b !actor_x+1
    cmp.w !v_08e7
    bcs .l8931
    bra .l892D
.l8916:
    lda.w !v_08f0
    and.w #$00FF
    bne .l8931
    lda.b !actor_x+1
    sec
    sbc.w !t_c29a,y
    clc
    adc.w #$000F
    cmp.w !v_08e7
    bcc .l8931
.l892D:
    sep.b #$30
    clc
    rtl
.l8931:
    sep.b #$31
    rtl
.l8934:
    rtl
.l8935:
    sec
    rtl
