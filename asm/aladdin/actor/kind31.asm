; kind31.asm: actor kind $31 (its start, its ways of waiting, its walk,
; its second state) and the tiles it breaks.
; C: src/aladdin/actor/kind31.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!PLAYER_X = !player+!actor_x+1  ; Aladdin's x (word)
!PLAYER_Y = !player+!actor_y+1  ; his y (word)
!PLACEMENT_FLAGS = $7EC600      ; by placement: $80 once its tiles are broken
!K31_PROBES = $16               ; its probe list's address (word)
!K31_11 = $11                   ; cleared by its second state
!K31_HOME_X = $44               ; its own point before it flies to Aladdin (words)
!K31_HOME_Y = $46
!K31_GOTO_X = $48               ; ActorGoToPoint's point (words)
!K31_GOTO_Y = $4A
!K31_GOTO_SPEED = $4C           ; ActorGoToPoint's direction set offset
!K31_AIM_X = !scratch+4         ; ActorAimAt's point (words)
!K31_AIM_Y = !scratch+6

; @routine 83:ABEA Kind31Start
; @actor
; WHAT IT DOES: Actor kind $31: by its $07 (bits 0-2) one of eight ways
; of waiting for Aladdin, then on into its walk (Kind31Walk); from level 2
; on its palette from the ROM and another table of ways.
; HOW: $08 bit 1 set, bit 3 clear; $09 |= $90; $16 = $D18B; $42, $14,
; $15 0. Level below 2: JMP ($AC17,X) by $07 AND 7, times 2; else
; ActorPaletteFromRom (Y $3F) and JMP ($AC36,X). The ways: rise when in
; reach; wait till he is $41-$60 left of it (then 1, $20 or $40 frames),
; or $21-$40 right, or within $20 of its x plus $45 ($70, $30, 0, $C0),
; or within its next $20; then a spawn of kind $4D's look
; (ActorSpawnLookWait) and a hop (ActorHopFromTable), or a random wait,
; its tiles broken (Kind31BreakTiles) and a hop; way 7: Kind31Drop.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
; actor loop's jump for kind $31 in state $0C. Exit: never returns.
%at($83ABEA)
Kind31Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$08
    trb.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D18B
    sta.b !K31_PROBES
    sep.b #$20
    stz.b !actor_42
    stz.b !actor_facing
    stz.b !actor_flip
    lda.w !level_035c
    cmp.b #$02
    bcs .lAC27
    lda.b !actor_07
    and.b #$07
    asl a
    tax
    jmp.w (.tAC17,x)
.tAC17:
    dw .lAC46, .lAC6F, .lACCC, .lACD2, .lACD8, .lACD8, .lACD8, Kind31Drop
.lAC27:
    ldy.b #$3F
    jsl.l ActorPaletteFromRom
    lda.b !actor_07
    and.b #$07
    asl a
    tax
    jmp.w (.tAC36,x)
.tAC36:
    dw .lAC46, .lACEF, .lAD46, .lAD4E, .lAD5E, .lAD75, .lAD56, Kind31Drop
.lAC46:
    brk.b #$00
    lda.b #$00
    jsl.l AnimStart
    lda.b #$08
    tsb.b !actor_08
    ldx.b #$00
    jsr.w ActorRiseWhenInReach
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    inc.b !actor_42
    jsr.w ActorFacePlayer
    jmp.w Kind31Walk
.lAC6F:
    lda.b #$01
    sta.b !actor_40
.lAC73:
    brk.b #$00
    rep.b #$20
    clc
    lda.w !PLAYER_X
    adc.w #$0060
    sec
    sbc.b !actor_x+1
    cmp.w #$0020
    sep.b #$20
    bcs .lAC73
.lAC88:
    brk.b #$00
    dec.b !actor_40
    bne .lAC88
.lAC8E:
    lda.b !actor_09
    bit.b #$40
    beq .lAC9A
    lda.b #$1D
    jsl.l SoundQueueAdd
.lAC9A:
    lda.b #$06
    jsl.l AnimStart
    lda.b #$08
    tsb.b !actor_08
    lda.b #$00
    sta.b !actor_spawn_at
    lda.b #$44
    sta.b !actor_4d
    jsr.w ActorSpawnLookWait
    ldx.b #$14
    jsr.w ActorHopFromTable
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    inc.b !actor_42
    jsr.w ActorFacePlayer
    jmp.w Kind31Walk
.lACCC:
    lda.b #$20
    sta.b !actor_40
    bra .lAC73
.lACD2:
    lda.b #$40
    sta.b !actor_40
    bra .lAC73
.lACD8:
    brk.b #$00
    rep.b #$20
    clc
    lda.b !actor_x+1
    adc.w #$0040
    sec
    sbc.w !PLAYER_X
    cmp.w #$0020
    sep.b #$20
    bcs .lACD8
    bra .lAC8E
.lACEF:
    lda.b #$70
    sta.b !actor_45
    stz.b !K31_HOME_Y
.lACF5:
    brk.b #$00
    rep.b #$20
    clc
    lda.b !actor_45
    adc.w !PLAYER_X
    sec
    sbc.b !actor_x+1
    cmp.w #$0020
    sep.b #$20
    bcs .lACF5
.lAD09:
    ldx.b #$00
    jsl.l RandomFromList
    sta.b !actor_40
.lAD11:
    brk.b #$00
    dec.b !actor_40
    bne .lAD11
    jsr.w Kind31BreakTiles
    lda.b #$2E
    jsl.l SoundQueueAdd
    lda.b #$03
    jsl.l AnimStart
    lda.b #$08
    tsb.b !actor_08
    ldx.b #$14
    jsr.w ActorHopFromTable
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    inc.b !actor_42
    jsr.w ActorFacePlayer
    bra Kind31Walk
.lAD46:
    lda.b #$30
    sta.b !actor_45
    stz.b !K31_HOME_Y
    bra .lACF5
.lAD4E:
    lda.b #$00
    sta.b !actor_45
    stz.b !K31_HOME_Y
    bra .lACF5
.lAD56:
    lda.b #$C0
    sta.b !actor_45
    stz.b !K31_HOME_Y
    bra .lACF5
.lAD5E:
    brk.b #$00
    rep.b #$20
    clc
    lda.b !actor_x+1
    adc.w #$0020
    sec
    sbc.w !PLAYER_X
    cmp.w #$0020
    sep.b #$20
    bcc .lAD09
    bra .lAD5E
.lAD75:
    brk.b #$00
    rep.b #$20
    clc
    lda.b !actor_x+1
    adc.w #$0020
    sec
    sbc.w !PLAYER_X
    cmp.w #$0020
    sep.b #$20
    bcs .lAD75
    lda.b #$01
    sta.b !actor_40
    bra .lAD11

; @routine 83:AD90 Kind31Drop
; @actor
; WHAT IT DOES: Kind $31's way 7 (and its second state's end): falls and
; drifts for good.
; HOW: $42 0; AnimStart 3; $08 bit 3 set; ActorSpeedXSet 6, ActorSpeedYSet
; $0F; $20 = $28; each frame (BRK first) ActorMoveX, ActorFall.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; Kind31Start's table or Kind31Second's jump. Exit: never returns.
%at($83AD90)
Kind31Drop:
    stz.b !actor_42
    lda.b #$03
    jsl.l AnimStart
    lda.b #$08
    tsb.b !actor_08
    ldy.b #$06
    jsl.l ActorSpeedXSet
    ldy.b #$0F
    jsl.l ActorSpeedYSet
    lda.b #$28
    sta.b !actor_accel_y
.lADAC:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorFall
    bra .lADAC

; @routine 83:ADB8 Kind31Walk
; @actor
; WHAT IT DOES: Kind $31's walk once it has met Aladdin: by $07 (bits 3-4
; at first) it walks, turning at edges and hopping up walls, or waits
; near him, leaps, lands, flies to where he was and back, and rises.
; HOW: $07 = ($07 AND $18) / 4; $40 = 1; JMP ($ADC9,X) by $07. 0: speed
; $15; each frame (BRK first) ActorMoveX; a wall (ActorOutOfWallProbe4): the
; wall ahead probed (ActorProbeAheadInGround X $10, Y $38), in ground: turn;
; else AnimStart 1, COP $10, ActorHopFromTable $0F, AnimStart 2, COP 8,
; AnimStart 0, again; off the ground (ActorSnapToGround C clear): turn
; ($14 and $15 flipped). 2, 4: every $20 frames, Aladdin within $60 across
; and $40 up or down: AnimStart 1, a leap (ActorSpeedYSetGravity $38, 3)
; until it falls, sound $2F, AnimStart 4, $42 $80, COP 8, to his point
; (ActorGoToPoint), back along its aim $30 frames, back (ActorGoToPoint),
; ActorRiseWhenInReach 0, AnimStart 0, $42 1, $07 0, again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; jumps from Kind31Start. Exit: never returns.
%at($83ADB8)
Kind31Walk:
    lda.b !actor_07
    and.b #$18
    lsr a
    lsr a
    sta.b !actor_07
    lda.b #$01
    sta.b !actor_40
.lADC4:
    ldx.b !actor_07
    jmp.w (.tADC9,x)
.tADC9:
    dw .lADCF, .lAE1B, .lAE1B
.lADCF:
    ldy.b #$15
    jsl.l ActorSpeedXSet
.lADD5:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorOutOfWallProbe4
    bcs .lADF1
    jsl.l ActorSnapToGround
    bcs .lADC4
.lADE7:
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    bra .lADD5
.lADF1:
    ldx.b #$10
    ldy.b #$38
    jsr.w ActorProbeAheadInGround
    bcs .lADE7
    lda.b #$01
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    ldx.b #$0F
    jsr.w ActorHopFromTable
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
.lAE19:
    bra .lADCF
.lAE1B:
    dec.b !actor_40
    bne .lADCF
    lda.b #$20
    sta.b !actor_40
    stz.w !scratch
    rep.b #$20
    lda.b !actor_x+1
    sec
    sbc.w !PLAYER_X
    clc
    adc.w #$0060
    cmp.w #$00C0
    sep.b #$20
    bcs .lADCF
    rep.b #$20
    lda.b !actor_y+1
    sec
    sbc.w !PLAYER_Y
    clc
    adc.w #$0040
    cmp.w #$0080
    sep.b #$20
    bcs .lAE19
    lda.b #$01
    jsl.l AnimStart
    lda.b #$38
    ldy.b #$03
    jsl.l ActorSpeedYSetGravity
.lAE5A:
    brk.b #$00
    jsl.l ActorFall
    lda.b !actor_speed_y+2
    bmi .lAE5A
    lda.b #$2F
    jsl.l SoundQueueAdd
    lda.b #$04
    jsl.l AnimStart
    lda.b #$80
    sta.b !actor_42
    lda.b #$08
    cop.b #$00
    rep.b #$20
    lda.b !actor_x+1
    sta.b !K31_HOME_X
    lda.b !actor_y+1
    sta.b !K31_HOME_Y
    lda.w !PLAYER_X
    sta.b !K31_GOTO_X
    lda.w !PLAYER_Y
    sta.b !K31_GOTO_Y
    ldx.b #$0A
    stx.b !K31_GOTO_SPEED
    jsr.w ActorGoToPoint
    rep.b #$30
    lda.b !K31_HOME_X
    sta.b !K31_GOTO_X
    sta.w !K31_AIM_X
    lda.b !K31_HOME_Y
    sta.b !K31_GOTO_Y
    sta.w !K31_AIM_Y
    jsl.l ActorAimAt
    sta.b !actor_45
    eor.b #$10
    sta.b !actor_facing
    ldx.b #$0A
    jsl.l ActorSpeedSetVector
    lda.b !actor_45
    sta.b !actor_facing
    lda.b #$30
    sta.b !actor_40
.lAEBB:
    brk.b #$00
    ldx.b #$0C
    jsl.l ActorSpeedAddVector
    jsl.l ActorMoveRight
    jsl.l ActorMoveY
    dec.b !actor_40
    bne .lAEBB
    jsr.w ActorGoToPoint
    ldx.b #$08
    ldx.b #$00
    jsr.w ActorRiseWhenInReach
    lda.b #$00
    jsl.l AnimStart
    lda.b #$01
    sta.b !actor_42
    lda.b !actor_flip
    sta.b !actor_facing
    stz.b !actor_07
    jmp.w .lADC4

; @routine 83:AF1C Kind31Second
; @actor
; WHAT IT DOES: Kind $31's state $80: a sound, $0C frames of its
; animation, a spawn of its own look, then on to Kind31Drop.
; HOW: SoundQueueAdd $0F; state $0C, $11 0; $40 = $0C; AnimStart 2; $0C
; frames (BRK first) of AnimStep; $07 = 7, $21 = 0; ActorSpawnSameLook of
; its kind; $14, $15 = 1; $09 bit 7 set; JMP Kind31Drop.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $31 in state $80. Exit: never returns.
%at($83AF1C)
Kind31Second:
    lda.b #$0F
    jsl.l SoundQueueAdd
    lda.b #$0C
    sta.b !actor_state
    stz.b !K31_11
    lda.b #$0C
    sta.b !actor_40
    lda.b #$02
    jsl.l AnimStart
.lAF32:
    brk.b #$00
    jsl.l AnimStep
    dec.b !actor_40
    bne .lAF32
    lda.b #$07
    sta.b !actor_07
    lda.b #$00
    sta.b !actor_spawn_at
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$80
    tsb.b !actor_09
    jmp.w Kind31Drop

; @routine 83:AF57 Kind31BreakTiles
; WHAT IT DOES: Changes the two map tiles above the actor ($7FB8 left,
; $7FC0 right) and marks its placement ($7E:C600 by $34) $80.
; HOW: TileChangeAdd at (x-8, y-8) and (x+8, y-8), 16-bit; then 8-bit.
; Entry: D = the actor's record; called by JSR ($83:AD17). Exit: M and X
; 8-bit; A = $80; X = its $34.
%at($83AF57)
Kind31BreakTiles:
    rep.b #$30
    clc
    lda.b !actor_x+1
    adc.w #$FFF8
    tax
    clc
    lda.b !actor_y+1
    adc.w #$FFF8
    tay
    lda.w #$7FB8
    jsl.l TileChangeAdd
    clc
    lda.b !actor_x+1
    adc.w #$0008
    tax
    clc
    lda.b !actor_y+1
    adc.w #$FFF8
    tay
    lda.w #$7FC0
    jsl.l TileChangeAdd
    sep.b #$30
    ldx.b !actor_34
    lda.b #$80
    sta.l !PLACEMENT_FLAGS,x
    rts

