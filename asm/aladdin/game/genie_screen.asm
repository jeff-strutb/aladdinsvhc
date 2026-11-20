; genie_screen.asm: the Genie screen (level $2F).
; C: src/aladdin/game/genie_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!V_000D = $000D
!V_000E = $000E
!V_000F = $000F
!V_0010 = $0010
!V_0087 = $0087
!V_00A5 = $00A5
!V_00A6 = $00A6
!V_00A7 = $00A7
!V_00A8 = $00A8
!V_00A9 = $00A9
!V_00AA = $00AA
!V_00AB = $00AB
!V_00AC = $00AC
!V_00AD = $00AD
!V_00AE = $00AE
!V_00AF = $00AF
!V_00B0 = $00B0
!V_00B2 = $00B2
!V_00B3 = $00B3
!V_00B4 = $00B4
!V_00B5 = $00B5
!V_0331 = $0331
!V_035B = $035B
!V_035E = $035E
!V_0364 = $0364
!V_0365 = $0365
!V_0367 = $0367
!V_037C = $037C
!V_038D = $038D
!V_038F = $038F
!V_0391 = $0391
!V_0393 = $0393
!V_0394 = $0394
!V_0395 = $0395
!V_0397 = $0397
!V_0399 = $0399
!V_039A = $039A
!V_03A4 = $03A4
!V_03A6 = $03A6
!V_03A7 = $03A7
!V_03AA = $03AA
!V_03AB = $03AB
!V_1773 = $1773
!V_1777 = $1777
!V_1A1B = $1A1B
!V_1A1D = $1A1D
!V_DA64 = $DA64
!V_DA6B = $DA6B
!V_DA73 = $DA73
!V_DA77 = $DA77
!V_DA79 = $DA79
!V_DA7B = $DA7B
!V_DA7D = $DA7D
!V_DA7F = $DA7F
!V_DA83 = $DA83
!V_DA87 = $DA87
!V_DA8B = $DA8B
!V_DA8F = $DA8F
!V_DA96 = $DA96

; @routine 80:A63B GenieScreen
; WHAT IT DOES: The Genie screen (level $2F), the bonus after a stage where
; a figure of kind $27 was touched: the screen is built, the Genie's lamp
; zooms in, the wheel spins until a button stops it, slows and stops on a
; slot, and the slot's prize is given (health, a continue, or gems); then
; GenieScreenEnd.
; HOW: DB $80. TaskSlotsFree1To6; $033F, PLAY_MODE, $AE, $B5, $AF, $AC 0;
; screen off and cleared (NmiDisable, ScreenBlank, ScrollStateClear,
; OamBufferClear, DisplayDefaults, TextLayerClear); LEVEL $2F,
; LevelRamSetupLong; the view 0, the screen's y ($00B0) $FFB0;
; LevelMapBuildLong, LayersFillLong; UnpackSet $17 and $2E with VramCopy
; X = $3F, $46, $4D, $54 and $23; PaletteLoad $5C, $39; NmiEnable; $0399,
; $039A 0, $03A4 5; a frame (TaskWait 1); Mode7ScreenSetup $10;
; CircleWipeStep $50 at $80, $80; $200 bytes $7E:5000 to $7F:F500 (MVN);
; HdmaTableFill $FF00, $0100, $28; TaskCreate (slot $18, parameter 4,
; $81:9E8E); $03B8 0; $0395 $80, $0397 $D0; SongPlay $29.
; The lamp ($0399/$039A 1): GenieFrame, GenieZoomReset; $1A1D 1, $1A1C
; $80, the speed $A5-$A7 $000100, its step $A8 $40, $A9 0; each frame
; (GenieFrame) GenieCountdownStep until $1A1D is
; negative (sound $3A, the count cleared), with the zoom stepped
; (GenieZoomStep) by $AA in x or y ($0394 picks); $A9 counts the passes, to
; 2; then two more (to 4) moving $038D down and $0393 up. Then $0399,
; $039A $11, $0397 $80, TaskCreate twice (slots $60 and $48, $82:8000).
; The wheel: $038B, $038C 1, $03A6 0, $03A7 $FF, $03AA $33, $03AB, $03AC
; 0; a frame; $75 and $A5-$AA 0; $AE up; each frame until a button
; ($037B & $C0 | $037C, bits $D0); then each frame the speed $A5 up $30
; to $088A (FrameStepLong, Mode7Rotate by $A8, GenieSlotStep) until a
; button with the speed at $088A; then each frame the speed down a step
; every 4th frame ($032D) below $0130, with GeniePointerCheck,
; GenieSlotStep, its sound $3B on a slot and Random choices, until the
; speed runs out. $20 frames; $AC $FE; the slot from $A8 against the
; seven bounds at $DA96; by it (the table after the JMP): a lamp ($B4
; $2C, GeniePrizeSignal2; a health box more, to $0A, and GenieHealthFill,
; else 3 gems), a continue ($B4 $2C, GeniePrizeSignal80, $035B up, $30
; frames), 1 gem ($B4 $33), health filled or 1 gem ($B4 $20), 2 gems
; ($B4 $33); then GenieScreenEnd (the health fill: from its sound).
; @unreachable 80:AA49-AA4D The slot count run below 0: the DEC at 80:AA44
; follows only GenieSlotEnd's C clear (80:AA23), and with $00A9 0 that
; compare is CMP $DA6B,X against $00, which sets C, so $00A9 is 1-7 there
; ($00A9 is only ever 0-7: STZ, INC to 4 at 80:A844/A8A4, GenieSlotStep's
; INC wrapping 7 to 0, this DEC); the DEC leaves 0-6 and the BPL is always
; taken. No frame boundary lies between GenieSlotEnd's return and that DEC
; (80:AA20-AA44 calls no GenieFrame or TaskWait), so no poke can land
; there (master, 2026-11-19).
; Entry: M 8-bit, X 8-bit; a task's entry, by TaskExec from
; PlayerSceneFigure (81:CC4A). Exit: never returns (TaskExec).
%at($80A63B)
GenieScreen:
    lda.b #$80
    pha
    plb
    jsl.l TaskSlotsFree1To6
    stz.w !v_033f
    stz.w !V_0331
    stz.w !V_00AE
    stz.w !V_00B5
    stz.w !V_00AF
    stz.w !V_00AC
    sep.b #$20
    jsl.l NmiDisable
    jsl.l ScreenBlank
    jsl.l ScrollStateClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    jsl.l TextLayerClear
    lda.b #$2F
    sta.w !V_035E
    jsl.l LevelRamSetupLong
    rep.b #$20
    stz.w !V_1773
    stz.w !V_1777
    lda.w #$FFB0
    sta.w !V_00B0
    sep.b #$20
    jsl.l LevelMapBuildLong
    jsl.l LayersFillLong
    lda.b #$17
    jsl.l UnpackSet
    ldx.b #$3F
    jsl.l VramCopy
    ldx.b #$46
    jsl.l VramCopy
    ldx.b #$4D
    jsl.l VramCopy
    ldx.b #$54
    jsl.l VramCopy
    lda.b #$2E
    jsl.l UnpackSet
    ldx.b #$23
    jsl.l VramCopy
    lda.b #$5C
    jsl.l PaletteLoad
    lda.b #$39
    jsl.l PaletteLoad
    jsl.l NmiEnable
    stz.w !V_0399
    stz.w !V_039A
    lda.b #$05
    sta.w !V_03A4
    lda.b #$01
    jsl.l TaskWait
    lda.b #$10
    jsl.l Mode7ScreenSetup
    rep.b #$30
    lda.w #$0050
    ldx.w #$0080
    ldy.w #$0080
    jsl.l CircleWipeStep
    phb
    ldx.w #$5000
    ldy.w #$F500
    lda.w #$01FF
    mvn $7E,$7F
    plb
    lda.w #$FF00
    ldx.w #$0100
    ldy.w #$0028
    jsl.l HdmaTableFill
    sep.b #$30
    jsl.l TaskCreate
    db $18, $04, $8E, $9E, $81
    stz.w !inidisp_shadow
    rep.b #$20
    lda.w #$0080
    sta.w !V_0395
    lda.w #$00D0
    sta.w !V_0397
    sep.b #$20
    lda.b #$29
    jsl.l SongPlay
    lda.b #$01
    sta.w !V_0399
    sta.w !V_039A
    jsr.w GenieFrame
    jsr.w GenieZoomReset
    lda.b #$01
    sta.w !V_1A1D
    lda.b #$80
    sta.w !layer_offset_y
    stz.w !V_00A5
    lda.b #$01
    sta.w !V_00A6
    stz.w !V_00A7
    lda.b #$40
    sta.w !V_00A8
    stz.w !V_00A9
.lA75A:
    jsr.w GenieFrame
    jsr.w GenieCountdownStep
    lda.w !V_1A1D
    bpl .lA75A
    stz.w !V_1A1B
    stz.w !layer_offset_y
    stz.w !V_1A1D
    lda.b #$3A
    jsl.l SoundQueueAdd
.lA774:
    ldx.w !V_00A9
    lda.w !V_DA73,x
    sta.w !V_00AA
    lda.w !V_DA79,x
    sta.w !V_00AB
.lA783:
    jsr.w GenieFrame
    lda.w !V_00AB
    beq .lA794
    dec a
    bne .lA791
    jsr.w GenieZoomReset
.lA791:
    dec.w !V_00AB
.lA794:
    lda.w !V_00AA
    tay
    lsr a
    lsr a
    sty.w !scratch
    sta.w !V_000E
    ldx.w !V_0394
    bne .lA7AB
    sty.w !V_000E
    sta.w !scratch
.lA7AB:
    stz.w !V_000D
    stz.w !V_000F
    ldx.w !V_00A9
    lda.w !V_DA7B,x
    sta.w !V_0010
    jsr.w GenieZoomStep
    bpl .lA783
    stz.w !V_00AA
    lda.w !V_DA7F,x
    sta.w !V_00A5
    lda.w !V_DA83,x
    sta.w !V_00A6
    lda.w !V_DA87,x
    sta.w !V_00A7
    lda.w !V_DA8B,x
    sta.w !V_00A8
.lA7DA:
    jsr.w GenieFrame
    lda.w !V_00AA
    tay
    asl a
    asl a
    sty.w !scratch
    sta.w !V_000E
    ldx.w !V_0394
    bne .lA813
    sty.w !V_000E
    sta.w !scratch
    stz.w !V_000D
    stz.w !V_000F
    jsr.w GenieCountdownStep
    lda.w !V_1A1D
    bpl .lA813
    lda.b #$3A
    jsl.l SoundQueueAdd
    stz.w !V_1A1B
    stz.w !layer_offset_y
    stz.w !V_1A1D
    bra .lA844
.lA813:
    ldx.w !V_00A9
    lda.w !V_00AA
    cmp.w !V_DA77,x
    bcs .lA7DA
    rep.b #$21
    lda.w !V_038D
    adc.w !scratch
    sta.w !V_038D
    lda.w !V_0393
    sec
    sbc.w !V_000E
    sta.w !V_0393
    sep.b #$20
    ldx.w !V_00A9
    lda.w !V_00AA
    clc
    adc.w !V_DA7D,x
    sta.w !V_00AA
    bra .lA7DA
.lA844:
    inc.w !V_00A9
    lda.w !V_00A9
    cmp.b #$02
    beq .lA851
    jmp.w .lA774
.lA851:
    ldx.w !V_00A9
    lda.w !V_DA7F,x
    sta.w !V_00A5
    lda.w !V_DA83,x
    sta.w !V_00A6
    lda.w !V_DA87,x
    sta.w !V_00A7
    lda.w !V_DA8B,x
    sta.w !V_00A8
.lA86C:
    jsr.w GenieFrame
    lda.w !V_038D
    beq .lA877
    dec.w !V_038D
.lA877:
    lda.w !V_0393
    beq .lA88D
    inc.w !V_0393
    inc.w !V_0393
    lda.w !V_0393
    bmi .lA88D
    stz.w !V_0393
    inc.w !V_0394
.lA88D:
    jsr.w GenieCountdownStep
    lda.w !V_1A1D
    bpl .lA86C
    lda.b #$3A
    jsl.l SoundQueueAdd
    stz.w !V_1A1B
    stz.w !layer_offset_y
    stz.w !V_1A1D
    inc.w !V_00A9
    lda.w !V_00A9
    cmp.b #$04
    bne .lA851
    lda.b #$11
    sta.w !V_0399
    sta.w !V_039A
    lda.b #$80
    sta.w !V_0397
    jsl.l TaskCreate
    db $60, $0E, $00, $80, $82
    jsl.l TaskCreate
    db $48, $04, $00, $80, $82
    lda.b #$01
    sta.w !tmw_shadow
    sta.w !tsw_shadow
    stz.w !V_03A6
    lda.b #$FF
    sta.w !V_03A7
    lda.b #$33
    sta.w !V_03AA
    stz.w !V_03AB
    stz.w !wobjsel_shadow
    lda.b #$01
    jsl.l TaskWait
    stz.w !task_param
    stz.w !V_00A7
    stz.w !V_00A8
    stz.w !V_00A5
    stz.w !V_00A6
    stz.w !V_00A9
    stz.w !V_00AA
    inc.w !V_00AE
.lA906:
    jsr.w GenieFrame
    lda.w !joy_pressed
    and.b #$C0
    ora.w !V_037C
    bit.b #$D0
    beq .lA906
    stz.w !V_00AE
.lA918:
    lda.b #$01
    jsl.l TaskWait
    rep.b #$20
    lda.w !V_00A5
    cmp.w #$088A
    sep.b #$20
    bcc .lA93B
    lda.b #$01
    sta.w !V_00AE
    lda.w !joy_pressed
    and.b #$C0
    ora.w !V_037C
    bit.b #$D0
    bne .lA971
.lA93B:
    jsl.l FrameStepLong
    rep.b #$21
    lda.w !V_00A5
    cmp.w #$088A
    bcc .lA951
    lda.w #$088A
    sta.w !V_00A5
    bra .lA95A
.lA951:
    lda.w !V_00A5
    adc.w #$0030
    sta.w !V_00A5
.lA95A:
    lda.w !V_00A5
    adc.w !V_00A7
    sta.w !V_00A7
    sep.b #$20
    lda.w !V_00A8
    jsl.l Mode7Rotate
    jsr.w GenieSlotStep
    bra .lA918
.lA971:
    lda.b #$FF
    sta.w !V_00AC
.lA976:
    stz.w !V_00AE
    jsr.w GenieFrame
    lda.l $00032D
    and.b #$03
    bne .lA998
    rep.b #$20
    lda.w !V_00A5
    cmp.w #$0130
    bcs .lA996
    dec.w !V_00A5
    bpl .lA996
    jmp.w .lAA2E
.lA996:
    sep.b #$20
.lA998:
    stz.w !V_00AD
    jsr.w GeniePointerCheck
    jsr.w GenieSlotStep
    bcc .lA9D8
    rep.b #$20
    lda.w !V_00A5
    cmp.w #$0110
    bcs .lA9C6
    lda.w !V_00A5
    cmp.w #$0070
    bcc .lA9FD
    cmp.w #$0080
    bcc .lA9EE
    lda.w !V_00A5
    sec
    sbc.w #$0006
    sta.w !V_00A5
    bra .lA9CC
.lA9C6:
    sbc.w #$002E
    sta.w !V_00A5
.lA9CC:
    bmi .lAA2E
.lA9CE:
    sep.b #$20
    lda.b #$3B
    jsl.l SoundQueueAdd
    rep.b #$20
.lA9D8:
    rep.b #$21
    lda.w !V_00A5
    adc.w !V_00A7
    sta.w !V_00A7
    sep.b #$20
    lda.w !V_00A8
    jsl.l Mode7Rotate
    bra .lA976
.lA9EE:
    sep.b #$20
    lda.w !V_00AA
    bne .lA9FD
    jsl.l RandomNext
    and.b #$03
    bne .lA9CE
.lA9FD:
    sep.b #$20
    lda.b #$01
    sta.w !V_00AD
    sta.w !V_00AF
.lAA07:
    jsr.w GenieFrame
    rep.b #$20
    dec.w !V_00A5
    bmi .lAA2E
    lda.w !V_00A7
    sec
    sbc.w !V_00A5
    sta.w !V_00A7
    sep.b #$20
    jsr.w GeniePointerCheck
    jsr.w GenieSlotEnd
    bcc .lAA3F
    lda.w !V_00A8
    jsl.l Mode7Rotate
    bra .lAA07
.lAA2E:
    sep.b #$20
    lda.b #$20
    sta.w !V_00B2
.lAA35:
    jsr.w GenieFrame
    dec.w !V_00B2
    bne .lAA35
    bra .lAA4E
.lAA3F:
    lda.b #$01
    sta.w !V_00AA
    dec.w !V_00A9
    bpl .lA9D8
    stz.w !V_00A9
    bra .lA9D8
.lAA4E:
    lda.b #$FE
    sta.w !V_00AC
    lda.w !V_00A8
    ldy.b #$00
.lAA58:
    cmp.w !V_DA96,y
    bcc .lAA64
    iny
    cpy.b #$07
    bne .lAA58
    ldy.b #$00
.lAA64:
    tya
    asl a
    tax
    jmp.w (.lAA6A,x)
.lAA6A:
    dw .lAA78, .lAA98, .lAABA, .lAAC9, .lAA98, .lAADE, .lAA98
.lAA78:
    lda.b #$2C
    sta.w !V_00B4
    jsr.w GeniePrizeSignal2
    lda.w !V_0365
    inc a
    cmp.b #$0B
    bcc .lAA8F
    lda.b #$03
    jsr.w GenieGemsAdd
    bra .lAA95
.lAA8F:
    sta.w !V_0365
    jsr.w GenieHealthFill
.lAA95:
    jmp.w GenieScreenEnd
.lAA98:
    lda.b #$2C
    sta.w !V_00B4
    jsr.w GeniePrizeSignal80
    inc.w !V_035B
    lda.b #$30
    sta.w !V_00B2
.lAAA8:
    jsr.w GenieFrame
    dec.w !V_00B2
    bne .lAAA8
    jmp.w GenieScreenEnd
.lAAB3:
    lda.b #$01
    jsr.w GenieGemsAdd
    bra GenieScreenEnd
.lAABA:
    lda.b #$33
    sta.w !V_00B4
    jsr.w GeniePrizeSignal2
    lda.b #$01
    jsr.w GenieGemsAdd
    bra GenieScreenEnd
.lAAC9:
    lda.b #$20
    sta.w !V_00B4
    jsr.w GeniePrizeSignal2
    lda.w !V_0365
    cmp.w !V_0367
    beq .lAAB3
    jsr.w GenieHealthFill
    bra GenieScreenEnd_lAB4D
.lAADE:
    lda.b #$33
    sta.w !V_00B4
    jsr.w GeniePrizeSignal2
    lda.b #$02
    jsr.w GenieGemsAdd
    bra GenieScreenEnd

; @routine 80:AAED GenieHealthFill
; WHAT IT DOES: Fills Aladdin's health to its top, a box every 10 frames,
; then waits 64 frames.
; HOW: 10 frames (GenieFrame, $B2 counting), then while $0367 is below
; $0365: $0367 up and again; then 64 frames.
; Entry: M 8-bit, X 8-bit; called by JSR from GenieScreen. Exit: M 8-bit,
; X 8-bit; $B2 0.
%at($80AAED)
GenieHealthFill:
    lda.b #$0A
    sta.w !V_00B2
.lAAF2:
    jsr.w GenieFrame
    dec.w !V_00B2
    bne .lAAF2
    lda.w !V_0367
    cmp.w !V_0365
    bcs .lAB07
    inc.w !V_0367
    bra GenieHealthFill
.lAB07:
    lda.b #$40
    sta.w !V_00B2
.lAB0C:
    jsr.w GenieFrame
    dec.w !V_00B2
    bne .lAB0C
    rts

; @routine 80:AB15 GenieGemsAdd
; WHAT IT DOES: Gives A gems, one every 16 frames, up to 100.
; HOW: $B3 = A; each time 16 frames (GenieFrame), then $0364 at $64: done;
; else $0364 up, $B3 down, again until 0.
; Entry: M 8-bit, X 8-bit; A = the gems; called by JSR from GenieScreen.
; Exit: M 8-bit, X 8-bit.
%at($80AB15)
GenieGemsAdd:
    sta.w !V_00B3
.lAB18:
    lda.b #$10
    sta.w !V_00B2
.lAB1D:
    jsr.w GenieFrame
    dec.w !V_00B2
    bne .lAB1D
    lda.w !V_0364
    cmp.b #$64
    beq .lAB34
    inc.w !V_0364
    dec.w !V_00B3
    bne .lAB18
.lAB34:
    rts

; @routine 80:AB35 GenieScreenEnd
; WHAT IT DOES: The Genie screen's end: a pause, the prize's cue, its song,
; then the next stage is set going.
; HOW: $58 frames; $B5 1 for a frame, then 0; (from here GenieScreen's
; health fill joins) SoundQueueAdd $F0; two frames; SongPlay $B4; $A0
; frames; TaskCreate (slot $18, parameter 4, $81:9EB9); $81:F9D2; each
; frame (TaskWait 1) until $0087 is 0; TaskSlotsFree1To6; TaskExec
; $81:8181.
; Entry: M 8-bit, X 8-bit; reached by a jump from GenieScreen's prizes.
; Exit: never returns (TaskExec).
%at($80AB35)
GenieScreenEnd:
    lda.b #$58
    sta.w !V_00B2
.lAB3A:
    jsr.w GenieFrame
    dec.w !V_00B2
    bne .lAB3A
    lda.b #$01
    sta.w !V_00B5
    jsr.w GenieFrame
    stz.w !V_00B5
.lAB4D:
    lda.b #$F0
    jsl.l SoundQueueAdd
    jsr.w GenieFrame
    jsr.w GenieFrame
    lda.w !V_00B4
    jsl.l SongPlay
    lda.b #$A0
    sta.w !V_00A9
.lAB65:
    jsr.w GenieFrame
    dec.w !V_00A9
    bne .lAB65
    jsl.l TaskCreate
    db $18, $04, $B9, $9E, $81
    jsl.l SoundQueueF6With1
.lAB7A:
    lda.b #$01
    jsl.l TaskWait
    lda.w !V_0087
    bne .lAB7A
    jsl.l TaskSlotsFree1To6
    jsl.l TaskExec
    db $81, $81, $81

; @routine 80:AB90 GenieFrame
; WHAT IT DOES: One frame of the Genie screen: the task waits a frame, then
; one frame of play.
; HOW: TaskWait 1; FrameStepLong.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: as FrameStepLong.
%at($80AB90)
GenieFrame:
    lda.b #$01
    jsl.l TaskWait
    jsl.l FrameStepLong
    rts

; @routine 80:AB9B GeniePrizeSignal80
; WHAT IT DOES: The screen's step ($00B5) $80 for a frame (the figures'
; second cue), then $EA frames.
; HOW: $B5 $80; GenieFrame; on at GeniePrizeSignal2's end: $B5 0, $EA
; frames.
; Entry: M 8-bit, X 8-bit; called by JSR from GenieScreen. Exit: M 8-bit,
; X 8-bit; $B2 0.
%at($80AB9B)
GeniePrizeSignal80:
    lda.b #$80
    sta.w !V_00B5
    jsr.w GenieFrame
    bra GeniePrizeSignal2_lABAD

; @routine 80:ABA5 GeniePrizeSignal2
; WHAT IT DOES: The screen's step ($00B5) 2 for a frame (the prize's figure
; cue), then $EA frames.
; HOW: $B5 2; GenieFrame; $B5 0; $B2 $EA, GenieFrame each until 0.
; Entry: M 8-bit, X 8-bit; called by JSR from GenieScreen. Exit: M 8-bit,
; X 8-bit; $B2 0.
%at($80ABA5)
GeniePrizeSignal2:
    lda.b #$02
    sta.w !V_00B5
    jsr.w GenieFrame
.lABAD:
    stz.w !V_00B5
    lda.b #$EA
    sta.w !V_00B2
.lABB5:
    jsr.w GenieFrame
    dec.w !V_00B2
    bne .lABB5
    rts

; @routine 80:ABBE GenieSlotStep
; WHAT IT DOES: Tells whether the wheel's angle ($A8) has passed the next
; slot's bound: C set, and the slot count ($A9) up (wrapping at 7).
; HOW: X = $A9; 0: only with $A8 not negative go on; 7: $A9 and X 0; then
; $A8 against $DA64,X (bank $80): at or past it, $A9 up and C set; else C
; clear.
; Entry: M 8-bit, X 8-bit; DB $80; called by JSR from GenieScreen. Exit:
; C as above; X = the old count (0 after a wrap).
%at($80ABBE)
GenieSlotStep:
    ldx.w !V_00A9
    beq .lABDD
    cpx.b #$07
    bne .lABCE
    stz.w !V_00A9
    ldx.b #$00
    bra .lABDD
.lABCE:
    lda.w !V_00A8
    cmp.w !V_DA64,x
    bcc .lABDB
    inc.w !V_00A9
    sec
    rts
.lABDB:
    clc
    rts
.lABDD:
    lda.w !V_00A8
    bmi .lABDB
    bra .lABCE

; @routine 80:ABE4 GenieSlotEnd
; WHAT IT DOES: Compares the wheel's angle ($A8) with the end of the slot
; the count ($A9) names: C set at or past it.
; HOW: X = $A9; 1 with $A8 negative: C set; else CMP $DA6B,X.
; Entry: M 8-bit, X 8-bit; DB $80; called by JSR from GenieScreen. Exit:
; C and Z from the compare (or C set); X = $A9.
%at($80ABE4)
GenieSlotEnd:
    ldx.w !V_00A9
    cpx.b #$01
    beq .lABF2
.lABEB:
    lda.w !V_00A8
    cmp.w !V_DA6B,x
    rts
.lABF2:
    lda.w !V_00A8
    bmi .lABEB
    sec
    rts

; @routine 80:ABF9 GenieZoomStep
; WHAT IT DOES: A step of the lamp's zoom: the mode-7 scale words move by
; $000C and $000E, and $AA by $0010.
; HOW: 16-bit, carry clear: $0393 += $000C, $038D -= $000E; 8-bit with C
; set: $AA -= $0010.
; Entry: M 8-bit; called by JSR from GenieScreen. Exit: M 8-bit; A = the
; new $AA; N, Z, C from its subtraction.
%at($80ABF9)
GenieZoomStep:
    rep.b #$21
    lda.w !scratch
    adc.w !V_0393
    sta.w !V_0393
    lda.w !V_038D
    sec
    sbc.w !V_000E
    sta.w !V_038D
    sep.b #$21
    lda.w !V_00AA
    sbc.w !V_0010
    sta.w !V_00AA
    rts

; @routine 80:AC1A GenieCountdownStep
; WHAT IT DOES: The 24-bit count at $1A1B less the speed ($A5-$A7), and the
; speed up by $A8.
; HOW: SEC; byte by byte $1A1B-$1A1D -= $A5-$A7; then $A5-$A7 += $A8
; (the carry on through).
; Entry: M 8-bit; called by JSR from GenieScreen. Exit: M 8-bit; A = the
; new $A7.
%at($80AC1A)
GenieCountdownStep:
    lda.w !V_1A1B
    sec
    sbc.w !V_00A5
    sta.w !V_1A1B
    lda.w !layer_offset_y
    sbc.w !V_00A6
    sta.w !layer_offset_y
    lda.w !V_1A1D
    sbc.w !V_00A7
    sta.w !V_1A1D
    lda.w !V_00A8
    clc
    adc.w !V_00A5
    sta.w !V_00A5
    lda.b #$00
    adc.w !V_00A6
    sta.w !V_00A6
    lda.b #$00
    adc.w !V_00A7
    sta.w !V_00A7
    rts

; @routine 80:AC51 GenieZoomReset
; WHAT IT DOES: The lamp's mode-7 scale back to 1 and its offsets 0.
; HOW: 16-bit: $038D, $0393 $0100; $038F, $0391 0.
; Entry: M 8-bit; called by JSR. Exit: M 8-bit; A = $0100.
%at($80AC51)
GenieZoomReset:
    rep.b #$20
    lda.w #$0100
    sta.w !V_038D
    sta.w !V_0393
    stz.w !V_038F
    stz.w !V_0391
    sep.b #$20
    rts

; @routine 80:AC65 GeniePointerCheck
; WHAT IT DOES: Tells the pointer whether the wheel's angle ($A8) is inside
; one of the seven slots' windows: $AC up when it is; else, if it was set,
; $AC $FF.
; HOW: X from 6 down to 0: $A8 at or past $DA64,X and below $DA8F,X:
; $AC up, done. None: $AC nonzero, $AC $FF.
; Entry: M 8-bit, X 8-bit; DB $80; called by JSR from GenieScreen. Exit:
; M 8-bit, X 8-bit.
%at($80AC65)
GeniePointerCheck:
    ldx.b #$06
.lAC67:
    lda.w !V_00A8
    cmp.w !V_DA64,x
    bcc .lAC79
    cmp.w !V_DA8F,x
    bcs .lAC79
    inc.w !V_00AC
    bra .lAC86
.lAC79:
    dex
    bpl .lAC67
    lda.w !V_00AC
    beq .lAC86
    lda.b #$FF
    sta.w !V_00AC
.lAC86:
    rts

