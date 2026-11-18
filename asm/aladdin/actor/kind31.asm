; kind31.asm: actor kind $31 (its start, its ways of waiting, its walk,
; its second state) and the tiles it breaks.
; C: src/aladdin/actor/kind31.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

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
    tsb.b $08
    lda.b #$08
    trb.b $08
    lda.b $09
    ora.b #$90
    sta.b $09
    rep.b #$20
    lda.w #$D18B
    sta.b $16
    sep.b #$20
    stz.b $42
    stz.b $14
    stz.b $15
    lda.w $035C
    cmp.b #$02
    bcs .lAC27
    lda.b $07
    and.b #$07
    asl a
    tax
    jmp.w (.tAC17,x)
.tAC17:
    dw .lAC46, .lAC6F, .lACCC, .lACD2, .lACD8, .lACD8, .lACD8, Kind31Drop
.lAC27:
    ldy.b #$3F
    jsl.l ActorPaletteFromRom
    lda.b $07
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
    tsb.b $08
    ldx.b #$00
    jsr.w ActorRiseWhenInReach
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    inc.b $42
    jsr.w ActorFacePlayer
    jmp.w Kind31Walk
.lAC6F:
    lda.b #$01
    sta.b $40
.lAC73:
    brk.b #$00
    rep.b #$20
    clc
    lda.w $08E7
    adc.w #$0060
    sec
    sbc.b $0C
    cmp.w #$0020
    sep.b #$20
    bcs .lAC73
.lAC88:
    brk.b #$00
    dec.b $40
    bne .lAC88
.lAC8E:
    lda.b $09
    bit.b #$40
    beq .lAC9A
    lda.b #$1D
    jsl.l SoundQueueAdd
.lAC9A:
    lda.b #$06
    jsl.l AnimStart
    lda.b #$08
    tsb.b $08
    lda.b #$00
    sta.b $21
    lda.b #$44
    sta.b $4D
    jsr.w ActorSpawnLookWait
    ldx.b #$14
    jsr.w ActorHopFromTable
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    inc.b $42
    jsr.w ActorFacePlayer
    jmp.w Kind31Walk
.lACCC:
    lda.b #$20
    sta.b $40
    bra .lAC73
.lACD2:
    lda.b #$40
    sta.b $40
    bra .lAC73
.lACD8:
    brk.b #$00
    rep.b #$20
    clc
    lda.b $0C
    adc.w #$0040
    sec
    sbc.w $08E7
    cmp.w #$0020
    sep.b #$20
    bcs .lACD8
    bra .lAC8E
.lACEF:
    lda.b #$70
    sta.b $45
    stz.b $46
.lACF5:
    brk.b #$00
    rep.b #$20
    clc
    lda.b $45
    adc.w $08E7
    sec
    sbc.b $0C
    cmp.w #$0020
    sep.b #$20
    bcs .lACF5
.lAD09:
    ldx.b #$00
    jsl.l RandomFromList
    sta.b $40
.lAD11:
    brk.b #$00
    dec.b $40
    bne .lAD11
    jsr.w Kind31BreakTiles
    lda.b #$2E
    jsl.l SoundQueueAdd
    lda.b #$03
    jsl.l AnimStart
    lda.b #$08
    tsb.b $08
    ldx.b #$14
    jsr.w ActorHopFromTable
    lda.b #$02
    jsl.l AnimStart
    lda.b #$08
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    inc.b $42
    jsr.w ActorFacePlayer
    bra Kind31Walk
.lAD46:
    lda.b #$30
    sta.b $45
    stz.b $46
    bra .lACF5
.lAD4E:
    lda.b #$00
    sta.b $45
    stz.b $46
    bra .lACF5
.lAD56:
    lda.b #$C0
    sta.b $45
    stz.b $46
    bra .lACF5
.lAD5E:
    brk.b #$00
    rep.b #$20
    clc
    lda.b $0C
    adc.w #$0020
    sec
    sbc.w $08E7
    cmp.w #$0020
    sep.b #$20
    bcc .lAD09
    bra .lAD5E
.lAD75:
    brk.b #$00
    rep.b #$20
    clc
    lda.b $0C
    adc.w #$0020
    sec
    sbc.w $08E7
    cmp.w #$0020
    sep.b #$20
    bcs .lAD75
    lda.b #$01
    sta.b $40
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
    stz.b $42
    lda.b #$03
    jsl.l AnimStart
    lda.b #$08
    tsb.b $08
    ldy.b #$06
    jsl.l ActorSpeedXSet
    ldy.b #$0F
    jsl.l ActorSpeedYSet
    lda.b #$28
    sta.b $20
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
    lda.b $07
    and.b #$18
    lsr a
    lsr a
    sta.b $07
    lda.b #$01
    sta.b $40
.lADC4:
    ldx.b $07
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
    lda.b $14
    eor.b #$01
    sta.b $14
    sta.b $15
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
    dec.b $40
    bne .lADCF
    lda.b #$20
    sta.b $40
    stz.w $000C
    rep.b #$20
    lda.b $0C
    sec
    sbc.w $08E7
    clc
    adc.w #$0060
    cmp.w #$00C0
    sep.b #$20
    bcs .lADCF
    rep.b #$20
    lda.b $0F
    sec
    sbc.w $08EA
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
    lda.b $1F
    bmi .lAE5A
    lda.b #$2F
    jsl.l SoundQueueAdd
    lda.b #$04
    jsl.l AnimStart
    lda.b #$80
    sta.b $42
    lda.b #$08
    cop.b #$00
    rep.b #$20
    lda.b $0C
    sta.b $44
    lda.b $0F
    sta.b $46
    lda.w $08E7
    sta.b $48
    lda.w $08EA
    sta.b $4A
    ldx.b #$0A
    stx.b $4C
    jsr.w ActorGoToPoint
    rep.b #$30
    lda.b $44
    sta.b $48
    sta.w $0010
    lda.b $46
    sta.b $4A
    sta.w $0012
    jsl.l ActorAimAt
    sta.b $45
    eor.b #$10
    sta.b $14
    ldx.b #$0A
    jsl.l ActorSpeedSetVector
    lda.b $45
    sta.b $14
    lda.b #$30
    sta.b $40
.lAEBB:
    brk.b #$00
    ldx.b #$0C
    jsl.l ActorSpeedAddVector
    jsl.l ActorMoveRight
    jsl.l ActorMoveY
    dec.b $40
    bne .lAEBB
    jsr.w ActorGoToPoint
    ldx.b #$08
    ldx.b #$00
    jsr.w ActorRiseWhenInReach
    lda.b #$00
    jsl.l AnimStart
    lda.b #$01
    sta.b $42
    lda.b $15
    sta.b $14
    stz.b $07
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
    sta.b $00
    stz.b $11
    lda.b #$0C
    sta.b $40
    lda.b #$02
    jsl.l AnimStart
.lAF32:
    brk.b #$00
    jsl.l AnimStep
    dec.b $40
    bne .lAF32
    lda.b #$07
    sta.b $07
    lda.b #$00
    sta.b $21
    lda.b $06
    jsl.l ActorSpawnSameLook
    lda.b #$01
    sta.b $14
    sta.b $15
    lda.b #$80
    tsb.b $09
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
    lda.b $0C
    adc.w #$FFF8
    tax
    clc
    lda.b $0F
    adc.w #$FFF8
    tay
    lda.w #$7FB8
    jsl.l TileChangeAdd
    clc
    lda.b $0C
    adc.w #$0008
    tax
    clc
    lda.b $0F
    adc.w #$FFF8
    tay
    lda.w #$7FC0
    jsl.l TileChangeAdd
    sep.b #$30
    ldx.b $34
    lda.b #$80
    sta.l $7EC600,x
    rts

