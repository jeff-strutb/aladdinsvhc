; ending.asm: the ending: its zoom scene's task, the stages task that
; shows the levels played back, and the text pages with their extras.
; C: src/aladdin/game/ending.c.

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
!dp_06 = $06  ; direct page
!dp_07 = $07  ; direct page
!dp_08 = $08  ; direct page
!dp_09 = $09  ; direct page
!dp_0a = $0A  ; direct page
!routine_8181a3 = $8181A3  ; not yet filed
!routine_81830d = $81830D  ; not yet filed
!routine_81972d = $81972D  ; not yet filed
!routine_819e8e = $819E8E  ; not yet filed
!routine_819eb9 = $819EB9  ; not yet filed
!routine_82b458 = $82B458  ; not yet filed
!routine_82b4af = $82B4AF  ; not yet filed
!routine_82b4d5 = $82B4D5  ; not yet filed
!routine_82b501 = $82B501  ; not yet filed
!routine_82b538 = $82B538  ; not yet filed
!routine_838bda = $838BDA  ; not yet filed
!t_e0ae = $E0AE  ; data bank
!t_e433 = $E433  ; data bank
!v_0000 = $0000
!v_0005 = $0005
!v_0007 = $0007
!v_0076 = $0076
!v_0077 = $0077
!v_0078 = $0078
!v_0079 = $0079
!v_007a = $007A
!v_007c = $007C
!v_007d = $007D
!v_007e = $007E
!v_0080 = $0080
!v_0081 = $0081
!v_0082 = $0082
!v_0087 = $0087
!v_00ee = $00EE
!v_00ef = $00EF
!v_00f0 = $00F0
!v_00f4 = $00F4
!v_00f5 = $00F5
!v_00f9 = $00F9
!v_0347 = $0347
!v_0376 = $0376
!v_038d = $038D
!v_038f = $038F
!v_0391 = $0391
!v_0393 = $0393
!v_0395 = $0395
!v_0397 = $0397
!v_0399 = $0399
!v_039a = $039A
!v_03a6 = $03A6
!v_03a7 = $03A7
!v_03a8 = $03A8
!v_03a9 = $03A9
!v_03aa = $03AA
!v_03ab = $03AB
!v_0825 = $0825
!v_0826 = $0826
!v_0827 = $0827
!v_0828 = $0828
!v_0829 = $0829
!v_083d = $083D
!v_142e = $142E
!v_1773 = $1773
!v_1a44 = $1A44
!v_1c9e = $1C9E

; @routine 80:B1D8 SceneSpinZoomTask
; WHAT IT DOES: Spins and zooms a Mode 7 scene: the angle steps on by a
; speed that grows every 16 frames and the zoom grows every 24, the zoom
; taken as its negative past $7F.
; HOW: D = the task's slot page. The centre ($80, $50); Mode 7 scaled
; (nmi_ppu_mode 1); angle 0, speed 4, zoom 8, counts 16 and 16. Each
; frame: the counts step (speed up every 16, zoom up every 24); the zoom
; (negated when negative) into the four scales; the matrix shadows shifted
; left 4 (C = -B); TaskWait 1; Mode7Rotate by the angle, then the angle
; plus the speed.
; Entry: M 8-bit, X 8-bit; Y = the task's slot; a task's entry. Exit:
; never returns.
%at($80B1D8)
SceneSpinZoomTask:
    rep.b #$20
    clc
    tya
    adc.w #$006F
    tcd
    lda.w #$0080
    sta.w !v_0395
    lda.w #$0050
    sta.w !v_0397
    sep.b #$20
    lda.b #$01
    sta.w !v_0829
    stz.b !dp_06
    lda.b #$04
    sta.b !dp_07
    lda.b #$08
    sta.b !dp_08
    lda.b #$10
    sta.b !dp_09
    lda.b #$10
    sta.b !dp_0a
.lB205:
    dec.b !dp_09
    bne .lB20F
    lda.b #$10
    sta.b !dp_09
    inc.b !dp_07
.lB20F:
    dec.b !dp_0a
    bne .lB219
    lda.b #$18
    sta.b !dp_0a
    inc.b !dp_08
.lB219:
    lda.b !dp_08
    bpl .lB220
    eor.b #$FF
    inc a
.lB220:
    sta.w !v_0827
    sta.w !v_0828
    sta.w !v_0825
    sta.w !v_0826
    rep.b #$20
    lda.w !v_038d
    asl a
    asl a
    asl a
    asl a
    sta.w !v_038d
    sta.w !v_0393
    lda.w !v_038f
    asl a
    asl a
    asl a
    asl a
    sta.w !v_038f
    eor.w #$FFFF
    inc a
    sta.w !v_0391
    sep.b #$20
    lda.b #$01
    jsl.l TaskWait
    lda.b !dp_06
    jsl.l Mode7Rotate
    clc
    lda.b !dp_06
    adc.b !dp_07
    sta.b !dp_06
    bra .lB205


; @routine 82:B1CE TextPageExtra
; WHAT IT DOES: After a page of the ending's text, adds what that page
; shows besides the text: an actor and lines for page 12 of the timed
; text, and for page 6 of the untimed one.
; HOW: $F4 nonzero (timed) and page ($EE) 12, or $F4 zero and page 6: an
; actor (kind 5, at $0376's x, $838BDA) and messages $76 and $78, then $7C
; when the item total ($0376) is 70, else $7A (by $81:972D).
; Entry: M 8-bit, X 8-bit; D = 0; DB = $82; called by JSR. Exit: M 8-bit.
%at($82B1CE)
TextPageExtra:
    lda.b !v_00f4
    beq .lB1D9
    lda.b !v_00ee
    cmp.b #$0C
    beq .lB1E0
    rts
.lB1D9:
    lda.b !v_00ee
    cmp.b #$06
    beq .lB1F8
    rts
.lB1E0:
    ldx.w !v_0376
    lda.b #$05
    ldy.b #$22
    jsl.l !routine_838bda
    lda.b #$6C
    jsl.l !routine_81972d
    lda.b #$6E
    jsl.l !routine_81972d
    rts
.lB1F8:
    ldx.w !v_0376
    lda.b #$05
    ldy.b #$30
    jsl.l !routine_838bda
    lda.b #$76
    jsl.l !routine_81972d
    lda.b #$78
    jsl.l !routine_81972d
    lda.w !v_0376
    cmp.b #$46
    beq .lB21D
    lda.b #$7A
    jsl.l !routine_81972d
    rts
.lB21D:
    lda.b #$7C
    jsl.l !routine_81972d
    rts


; @routine 82:B0AD EndingTextTask
; WHAT IT DOES: Runs the ending's text: 13 pages, each drawn, its bands
; revealed, shown for 660 frames and concealed; after the last, long waits
; and the text masked for good.
; HOW: Page 0, timed ($F4 = 14); HDMA channel 5 on the bands' window bytes
; (TextBandsCommit), the screen set up. Per page: TextPageDraw from the
; page tables in bank $A5, TextPageExtra, TaskWait, TextBandsReveal; the
; count $EF from $294; at 0 TextBandsConceal and the next page. After page
; 13: four waits of 255 frames, TextBandsConceal, then a frame's wait
; forever.
; Entry: M 8-bit, X 8-bit; D = 0; a task's entry. Exit: never returns.
%at($82B0AD)
EndingTextTask:
    lda.b #$0E
    sta.b !v_00f4
    stz.b !v_00ee
    lda.b #$00
    sta.b !v_00ef
    lda.b #$02
    sta.b !v_00f0
    lda.b #$61
    jsl.l PaletteLoad
    lda.b #$5F
    jsl.l PaletteLoad
    stz.b !v_00f9
    lda.b #$40
    sta.w !DMAP5
    lda.b #$24
    sta.w !BBAD5
    lda.b #$EF
    sta.w !A1T5L
    lda.b #$CE
    sta.w !A1T5H
    lda.b #$80
    sta.w !A1B5
    stz.w !DASB5
    ldx.b #$07
.lB0E7:
    stz.w !text_bands,x
    dex
    bpl .lB0E7
    jsr.w TextBandsCommit
    lda.b #$20
    tsb.w !hdmaen_shadow
    lda.b #$00
    sta.w !v_03a8
    lda.b #$FF
    sta.w !v_03a9
    stz.w !v_0005
    jsl.l KeptOptionsSeal
    lda.b !v_00f4
    beq .lB115
.lB10A:
    rep.b #$20
    dec.b !v_00ef
    sep.b #$20
    bne .lB187
    jsr.w TextBandsConceal
.lB115:
    jsl.l TextLayerClear
    ldx.b #$07
.lB11B:
    lda.b #$08
    sta.w !text_bands,x
    sta.w !v_1c9e,x
    dex
    bpl .lB11B
    jsr.w TextBandsCommit
    lda.b !v_00ee
    asl a
    adc.b !v_00f4
    tax
    phb
    lda.b #$A5
    pha
    plb
    rep.b #$10
    ldy.w !t_e433,x
    sty.b !v_00f5
    ldy.w !t_e0ae,x
    jsr.w TextPageDraw
    sep.b #$10
    plb
    jsr.w TextPageExtra
    lda.b #$01
    jsl.l TaskWait
    jsr.w TextBandsReveal
    lda.b !v_00f4
    beq .lB17F
    lda.b #$94
    sta.b !v_00ef
    lda.b #$02
    sta.b !v_00f0
    inc.b !v_00ee
    lda.b !v_00ee
    cmp.b #$0D
    bne .lB187
    lda.b #$FF
    jsl.l TaskWait
    lda.b #$FF
    jsl.l TaskWait
    lda.b #$FF
    jsl.l TaskWait
    lda.b #$FF
    jsl.l TaskWait
    jsr.w TextBandsConceal
.lB17F:
    lda.b #$01
    jsl.l TaskWait
    bra .lB17F
.lB187:
    lda.b #$01
    jsl.l TaskWait
    jmp.w .lB10A


; @routine 82:B29C EndingStagesTask
; WHAT IT DOES: Plays back the levels at the ending: stage $0C alone with
; fewer than 50 items ($0376), else the stages from 4 to $0B, each run as
; a demo for its countdown with a window over it, the last scrolled across
; to x $400.
; HOW: $0082 set when the total is under 50; the stage ($033F) 4 or $0C;
; the song $26. Per stage: DemoStartLong, the new game state, the play
; counts, the level started; with 50 or more the windows and colour math
; set; the screen's task started; StageCountdownSet, then a frame of the
; demo per count; stage $0B scrolls on 3 pixels a frame to x $400 and ends
; the run; else the next stage. Fewer than 50 runs the single stage on at
; $82:B435.
; Entry: M 8-bit, X 8-bit; a task's entry. Exit: never returns.
%at($82B29C)
EndingStagesTask:
    stz.w !v_0082
    lda.w !v_0376
    cmp.b #$32
    bcs .lB2A9
    inc.w !v_0082
.lB2A9:
    stz.w !v_0077
    ldx.b #$04
    lda.w !v_0082
    beq .lB2B5
    ldx.b #$0C
.lB2B5:
    stx.w !v_033f
    stz.w !v_0076
    lda.b #$26
    jsl.l SongPlay
.lB2C1:
    jsl.l DemoStartLong
    nop
    nop
    nop
    jsl.l NewGameStateLong
    jsl.l PlayStartCountsLong
    jsl.l !routine_8181a3
    inc.w !v_142e
    lda.w !v_0082
    bne .lB32F
    lda.b #$38
    sta.w !v_03a6
    stz.w !v_0078
    sta.w !v_0079
    stz.w !v_007a
    lda.b #$C8
    sta.w !v_03a7
    stz.w !v_007c
    sta.w !v_007d
    stz.w !v_007e
    lda.b #$33
    sta.w !v_03aa
    lda.b #$0C
    sta.w !v_03ab
    lda.w !v_0399
    sta.w !tmw_shadow
    lda.w !v_039a
    sta.w !tsw_shadow
    lda.b #$23
    sta.w !wobjsel_shadow
    lda.w !level
    cmp.b #$1A
    beq .lB32F
    lda.b #$A3
    sta.w !cgadsub_shadow
    lda.b #$1F
    sta.w !coldata_red
    sta.w !coldata_green
    sta.w !coldata_blue
    lda.b #$20
    sta.w !cgwsel_shadow
.lB32F:
    lda.b #$80
    sta.w !inidisp_shadow
    lda.b #$FF
    sta.w !v_0347
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl !routine_819e8e
    lda.w !v_0082
    beq .lB34A
    jmp.w .lB435
.lB34A:
    stz.w !v_0080
    stz.w !v_0081
    jsr.w StageCountdownSet
.lB353:
    sep.b #$20
    jsr.w !routine_82b501
    rep.b #$20
    inc.w !v_0080
    dec.w !v_0343
    sep.b #$20
    bne .lB353
    lda.b #$3F
    jsl.l TaskWait
    lda.w !v_033f
    cmp.b #$0B
    beq .lB396
    jsr.w !routine_82b458
    lda.w !v_0077
    eor.b #$01
    sta.w !v_0077
    jsl.l TaskCreate
    db 1*24, 4                ; slot 1, parameter 4
    dl !routine_819eb9
.lB385:
    lda.b #$01
    jsl.l TaskWait
    lda.w !v_0087
    bne .lB385
    inc.w !v_033f
    jmp.w .lB2C1
.lB396:
    lda.b #$01
    jsl.l TaskWait
    rep.b #$20
    clc
    lda.w !v_1773
    adc.w #$0003
    sta.w !v_1773
    lda.w !v_1a44
    adc.w #$0003
    sta.w !v_1a44
    sep.b #$20
    jsr.w !routine_82b538
    rep.b #$20
    lda.w !v_1773
    cmp.w #$0400
    sep.b #$20
    bcc .lB396
    jsl.l ActorRecordsInit
    jsl.l OamBufferClear
    lda.b #$40
    jsl.l TaskWait
    lda.b #$0C
    sta.w !v_083d
.lB3D5:
    lda.b #$01
    jsl.l TaskWait
    ldx.b #$08
    jsr.w !routine_82b4af
    ldx.b #$0C
    jsr.w !routine_82b4d5
    lda.w !v_03a6
    bne .lB3D5
    lda.w !v_03a7
    cmp.b #$FF
    bne .lB3D5
    lda.b #$30
    sta.w !cgwsel_shadow
    stz.w !cgadsub_shadow
    stz.w !v_03aa
    stz.w !v_03ab
    sta.w !wobjsel_shadow
    stz.w !coldata_red
    stz.w !coldata_green
    stz.w !coldata_blue
    lda.b #$20
    jsl.l TaskWait
    lda.b #$A7
    jsl.l ActorCreate
    lda.b #$0C
    sta.w !v_0000,x
    sep.b #$10
    lda.b #$95
    jsl.l ActorCreate
    lda.b #$0C
    sta.w !v_0000,x
    lda.b #$1E
    sta.w !v_0007,x
    sep.b #$10
.lB430:
    jsr.w !routine_82b501
    bra .lB430
.lB435:
    stz.w !v_0080
    stz.w !v_0081
    jsr.w StageCountdownSet
    jsl.l TaskCreate
    db 5*24, 0                ; slot 5, parameter 0
    dl EndingTextTask
.lB447:
    jsl.l !routine_81830d
.lB44B:
    lda.b #$01
    jsl.l TaskWait
    ldy.w !vram_queue_pending
    bne .lB44B
    bra .lB447
