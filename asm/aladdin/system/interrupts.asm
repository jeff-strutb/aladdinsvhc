; interrupts.asm: the interrupt vectors' code and the handlers of the NMI
; (each frame's vertical blank: the uploads and the PPU's registers from
; their shadows, the waiting tasks counted down) and of the IRQ (the raster
; interrupt that turns the display on below the status bar).
; C: src/aladdin/system/interrupts.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"

arch 65816

!v_0008 = $0008                 ; cleared at each NMI; purpose not yet established
!v_032a = $032A                 ; counted up at each NMI (a second frame count)
!v_0839 = $0839                 ; nonzero: the NMI leaves the HDMA tables' addresses as they are
!v_082a = $082A                 ; nonzero: the NMI skips the layer and VRAM-list uploads
!v_1c7d = $1C7D                 ; nonzero: the IRQ line is 8 (else from the V counter)
!bg1sc_shadow = $03A0           ; BG1SC's shadow
!bg2sc_shadow = $03A1           ; BG2SC's shadow
!bg3sc_shadow = $03A2           ; BG3SC's shadow
!bg12nba_shadow = $03A4         ; BG12NBA's shadow
!bg34nba_shadow = $03A5         ; BG34NBA's shadow
!wh0_shadow = $03A6             ; WH0-WH3's shadows: the windows' edges
!wh1_shadow = $03A7
!wh2_shadow = $03A8
!wh3_shadow = $03A9
!w12sel_shadow = $03AA          ; W12SEL's shadow
!w34sel_shadow = $03AB          ; W34SEL's shadow
!wbglog_shadow = $03AD          ; WBGLOG's shadow
!wobjlog_shadow = $03AE         ; WOBJLOG's shadow
!mosaic_shadow = $03B1          ; MOSAIC's shadow
!tm_shadow = $0399              ; TM's shadow: the main screen's layers
!ts_shadow = $039A              ; TS's shadow: the sub screen's layers
!bgmode_shadow = $039D          ; BGMODE's shadow
!uploads_this_frame = $0403     ; uploads queued this frame (the NMI clears it)

; @routine 80:FFA4 NmiVector
; @interrupt
; WHAT IT DOES: Where the NMI vector leads (at $00:FFA4): on to the NMI's
; handler in the fast bank.
; HOW: JML NmiHandler.
; Entry: an NMI's frame on the stack (P, then the PC and bank it returns
; to); run at $00:FFA4. Exit: as NmiHandler.
%at($80FFA4)
NmiVector:
    jml.l NmiHandler

; @routine 80:FFA8 IrqVector
; @interrupt
; WHAT IT DOES: Where the IRQ vector leads (at $00:FFA8): on to the IRQ's
; handler.
; HOW: JML IrqHandler.
; Entry: an IRQ's frame on the stack; run at $00:FFA8. Exit: as IrqHandler.
%at($80FFA8)
IrqVector:
    jml.l IrqHandler

; @routine 80:FFAC BrkVector
; @interrupt
; WHAT IT DOES: Where the BRK vector leads (at $00:FFAC): an actor yields
; for a frame (ActorYieldEntry).
; HOW: JML ActorYieldEntry.
; Entry: a BRK's frame on the stack (the actor's resume address past the
; BRK's signature byte, P); D = the actor's record; run at $00:FFAC.
; Exit: as ActorYieldEntry.
%at($80FFAC)
BrkVector:
    jml.l ActorYieldEntry

; @routine 80:FFB0 CopVector
; @interrupt
; WHAT IT DOES: Where the COP vector leads (at $00:FFB0): an actor waits A
; frames (ActorWaitEntry).
; HOW: JML ActorWaitEntry.
; Entry: a COP's frame on the stack; D = the actor's record; A low byte =
; the frames; run at $00:FFB0. Exit: as ActorWaitEntry.
%at($80FFB0)
CopVector:
    jml.l ActorWaitEntry

; @routine 80:8111 NmiHandler
; @interrupt
; WHAT IT DOES: The vertical blank's work, each frame: the screen blanked
; while it uploads the frame's queued graphics, sprites, palettes and
; scroll values, the PPU's registers set from their shadows, the HDMA and
; raster IRQ set up for the next frame, the tasks that wait counted down
; (one whose count runs out is made ready), the sound command sent; then
; the display and the NMI back on.
; HOW: All registers saved (16-bit), D = 0, DB = $80; FRAME_COUNT and
; V_032A up; HDMA off, NmiDisable, forced blank, RDNMI read;
; ScrollRegistersWrite, WramMovesRun; HDMAEN from its shadow unless an IRQ
; is on (NMITIMEN_SHADOW bits 4-5); unless V_0839, the HDMA channels 4-7
; on in the shadow get their tables' addresses; with OAM_0857 set, the
; sprite table by DMA, VramQueueFlush and (V_082A clear) LayersCopyToVram;
; the VRAM lists, tile blocks, words, text buffer and its parts, uploads
; by number (these skipped when V_082A); character sets, stepped uploads,
; blocks B and A, the palette; the windows, layer bases, window logic,
; colour math, fixed colour, screens, mosaic and tile bases from their
; shadows; with an IRQ on, its H position $98 and its line (8 when V_1C7D,
; else from the V counter latched, plus 3 when 5 or more and not
; negative), the HDMA tables restarted (A2A = A1T, NTRL 1) for channels
; 1-7; BGMODE from its shadow; Mode7MatrixWrite; INIDISP from its shadow
; unless an IRQ is on (the IRQ turns it on); UPLOADS_THIS_FRAME 0; each task
; slot 7 to 0 waiting (state 1) counts its wait down, at 0 state 4 (ready);
; SoundCommandSend, NmiEnable; the registers back; RTI.
; Entry: an NMI's frame on the stack; any registers. Exit: by RTI with
; every register as at the NMI.
%at($808111)
NmiHandler:
    rep.b #$30
    pha
    phb
    phd
    phx
    phy
    cld
    lda.w #$0000
    tcd
    sep.b #$30
    stz.w !v_0008
    lda.b #$80
    pha
    plb
    inc.w !frame_count
    stz.w !HDMAEN
    jsl.l NmiDisable
    lda.b #$80
    sta.w !INIDISP
    inc.w !v_032a
    lda.w !RDNMI
    jsr.w ScrollRegistersWrite
    jsr.w WramMovesRun
    lda.w !nmitimen_shadow
    and.b #$30
    bne .l814E
    lda.w !hdmaen_shadow
    sta.w !HDMAEN
.l814E:
    lda.w !v_0839
    bne .l8182
    lda.w !hdmaen_shadow
    and.b #$F0
    beq .l8182
    rep.b #$10
    asl a
    bcc .l8165
    ldx.w !hdma_table_at+6
    stx.w !A1T7L
.l8165:
    asl a
    bcc .l816E
    ldx.w !hdma_table_at+4
    stx.w !A1T6L
.l816E:
    asl a
    bcc .l8177
    ldx.w !hdma_table_at+2
    stx.w !A1T5L
.l8177:
    asl a
    bcc .l8180
    ldx.w !hdma_table_at
    stx.w !A1T4L
.l8180:
    sep.b #$10
.l8182:
    lda.w !oam_0857
    beq .l81C0
    stz.w !OAMADDL
    stz.w !OAMADDH
    lda.b #$00
    sta.w !DMAP0
    lda.b #$04
    sta.w !BBAD0
    lda.b #$80
    sta.w !A1T0L
    lda.b #$FA
    sta.w !A1T0H
    lda.b #$7E
    sta.w !A1B0
    lda.b #$20
    sta.w !DAS0L
    lda.b #$02
    sta.w !DAS0H
    lda.b #$01
    sta.w !MDMAEN
    jsr.w VramQueueFlush
    lda.w !v_082a
    bne .l81D2
    jsr.w LayersCopyToVram
.l81C0:
    jsr.w VramListsUpload
    jsr.w TileBlockQueueFlush
    jsr.w VramWordsUpload
    jsr.w TextBufferUpload
    jsr.w TextBufferPartsUpload
    jsr.w VramUploadByNumber
.l81D2:
    jsr.w UploadCharsets
    jsr.w UploadStepped
    jsr.w UploadBlockB
    jsr.w UploadBlockA
    jsr.w PaletteUploadFrom
    lda.w !wh0_shadow
    sta.w !WH0
    lda.w !wh1_shadow
    sta.w !WH1
    lda.w !wh2_shadow
    sta.w !WH2
    lda.w !wh3_shadow
    sta.w !WH3
    lda.w !bg1sc_shadow
    sta.w !BG1SC
    lda.w !bg2sc_shadow
    sta.w !BG2SC
    lda.w !bg3sc_shadow
    sta.w !BG3SC
    lda.w !w12sel_shadow
    sta.w !W12SEL
    lda.w !w34sel_shadow
    sta.w !W34SEL
    lda.w !wobjsel_shadow
    sta.w !WOBJSEL
    lda.w !wbglog_shadow
    sta.w !WBGLOG
    lda.w !wobjlog_shadow
    sta.w !WOBJLOG
    lda.w !cgwsel_shadow
    sta.w !CGWSEL
    lda.w !cgadsub_shadow
    sta.w !CGADSUB
    lda.w !coldata_red
    and.b #$1F
    ora.b #$20
    sta.w !COLDATA
    lda.w !coldata_green
    and.b #$1F
    ora.b #$40
    sta.w !COLDATA
    lda.w !coldata_blue
    and.b #$1F
    ora.b #$80
    sta.w !COLDATA
    lda.w !tm_shadow
    ldx.w !ts_shadow
    sta.w !TM
    stx.w !TS
    lda.w !tmw_shadow
    sta.w !TMW
    lda.w !tsw_shadow
    sta.w !TSW
    lda.w !mosaic_shadow
    sta.w !MOSAIC
    lda.w !bg12nba_shadow
    sta.w !BG12NBA
    lda.w !bg34nba_shadow
    sta.w !BG34NBA
    lda.w !nmitimen_shadow
    and.b #$30
    beq .l82F0
    lda.b #$98
    sta.w !HTIMEL
    stz.w !HTIMEH
    lda.w !v_1c7d
    bne .l82A5
    lda.b #$08
    ldy.w !SLHV
    ldy.w !STAT78
    ldy.w !OPVCT
    bmi .l82A5
    cpy.b #$05
    bcc .l82A5
    tya
    adc.b #$03
.l82A5:
    sta.w !VTIMEL
    stz.w !VTIMEH
    rep.b #$20
    lda.w !A1T1L
    sta.w !A2A1L
    lda.w !A1T2L
    sta.w !A2A2L
    lda.w !A1T3L
    sta.w !A2A3L
    lda.w !A1T4L
    sta.w !A2A4L
    lda.w !A1T5L
    sta.w !A2A5L
    lda.w !A1T6L
    sta.w !A2A6L
    lda.w !A1T7L
    sta.w !A2A7L
    sep.b #$20
    lda.b #$01
    sta.w !NTRL1
    sta.w !NTRL2
    sta.w !NTRL3
    sta.w !NTRL4
    sta.w !NTRL5
    sta.w !NTRL6
    sta.w !NTRL7
.l82F0:
    lda.w !bgmode_shadow
    sta.w !BGMODE
    jsr.w Mode7MatrixWrite
    lda.w !nmitimen_shadow
    and.b #$30
    bne .l8306
    lda.w !inidisp_shadow
    sta.w !INIDISP
.l8306:
    stz.w !uploads_this_frame
    ldx.b #$C0
.l830B:
    lda.w !task_state-24,x
    cmp.b #$01
    bne .l831C
    dec.w !task_wait-24,x
    bne .l831C
    lda.b #$04
    sta.w !task_state-24,x
.l831C:
    sec
    txa
    sbc.b #$18
    tax
    bne .l830B
    jsr.w SoundCommandSend
    jsl.l NmiEnable
    rep.b #$30
    ply
    plx
    pld
    plb
    pla
    rti
; @routine 80:8332 IrqHandler
; @interrupt
; WHAT IT DOES: The raster interrupt: at the line below the status bar it
; waits for the horizontal blank and turns the display on: HDMA, the
; brightness, the NMI back to the vertical blank only, and the screens.
; HOW: A and DB saved (M 8-bit), DB = $80; TIMEUP read (the IRQ
; acknowledged); HVBJOY until bit 6 (horizontal blank); HDMAEN and INIDISP
; from their shadows; NMITIMEN = $81; TM and TS from their shadows; DB and
; A back; RTI.
; Entry: an IRQ's frame on the stack. Exit: by RTI with A, DB and P as at
; the IRQ, M 8-bit within.
%at($808332)
IrqHandler:
    sep.b #$20
    pha
    phb
    lda.b #$80
    pha
    plb
    lda.w !TIMEUP
.l833D:
    bit.w !HVBJOY
    bvc .l833D
    lda.w !hdmaen_shadow
    sta.w !HDMAEN
    lda.w !inidisp_shadow
    sta.w !INIDISP
    lda.b #$81
    sta.w !NMITIMEN
    lda.w !tm_shadow
    sta.w !TM
    lda.w !ts_shadow
    sta.w !TS
    plb
    pla
    rti