; nmi_uploads.asm: uploads the NMI makes on request flags the retail game
; never sets: character sets from $7F:D000 or $7F:D400, a stepped row
; upload from $7F:B000, two fixed blocks, and a palette upload from a
; table.
; C: src/aladdin/video/nmi_uploads.c.

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
!routine_808584 = $808584  ; not yet filed
!t_8558 = $8558  ; data bank
!t_855a = $855A  ; data bank
!t_b608 = $B608  ; data bank
!t_b609 = $B609  ; data bank
!t_b60a = $B60A  ; data bank
!t_b6a2 = $B6A2  ; data bank
!t_b6a3 = $B6A3  ; data bank
!t_b6a4 = $B6A4  ; data bank
!t_b6a5 = $B6A5  ; data bank
!t_b6a7 = $B6A7  ; data bank
!t_b6ef = $B6EF  ; data bank
!t_b6f0 = $B6F0  ; data bank
!t_b6f1 = $B6F1  ; data bank
!t_b6f2 = $B6F2  ; data bank
!t_b6f4 = $B6F4  ; data bank
!t_b6f5 = $B6F5  ; data bank
!v_082a = $082A
!v_082b = $082B
!v_082c = $082C
!v_082d = $082D
!v_082e = $082E
!v_0845 = $0845

; @routine 80:851B UploadCharsets
; WHAT IT DOES: When $082B asks, copies one of two character sets from
; $7F:D000 or $7F:D400 to VRAM from 0, 32 rows of $20 bytes.
; HOW: $082B (3 or 6) indexes the source table at $80:8558 (word, bank);
; cleared; DMA channel 0, mode 0 to VMDATAL, 32 transfers of $20 bytes,
; VRAM address stepping $80.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; Y = 0 after a copy.
%at($80851B)
UploadCharsets:
    ldy.w !v_082b
    beq .l855A
    stz.w !v_082b
    stz.w !VMAIN
    stz.w !DMAP0
    lda.b #$18
    sta.w !BBAD0
    rep.b #$20
    lda.w !t_8558,y
    sta.w !A1T0L
    ldx.w !t_855a,y
    stx.w !A1B0
    ldy.b #$20
    ldx.b #$01
    lda.w #$0000
.l8543:
    sta.w !VMADDL
    pha
    lda.w #$0020
    sta.w !DAS0L
    stx.w !MDMAEN
    pla
    clc
    adc.w #$0080
    dey
    bne .l8543
    sep.b #$20
.l855A:
    rts


; @routine 80:8577 UploadStepped
; WHAT IT DOES: When $082A asks, runs step $082E of a stepped upload
; through the table at $80:8584.
; HOW: $082E * 2 indexes the jump table (0: the row number cleared, the
; step on; 1: a row of the buffer at $7F:B000 written to VRAM).
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: as
; the step.
%at($808577)
UploadStepped:
    ldy.w !v_082a
    beq UploadStepped+23
    lda.w !v_082e
    asl a
    tax
    jmp.w (!routine_808584,x)


; @routine 80:869A UploadBlockA
; WHAT IT DOES: When $082C asks, DMAs one of the blocks listed at $80:B6A2
; to VRAM.
; HOW: $082C is the entry's offset (7 bytes: source address and bank,
; size, VRAM address); cleared; DMA channel 0, mode 1 to VMDATA, VMAIN
; $80.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; X = the entry.
%at($80869A)
UploadBlockA:
    ldx.w !v_082c
    beq .l86D8
    stz.w !v_082c
    lda.b #$80
    sta.w !VMAIN
    lda.b #$01
    sta.w !DMAP0
    lda.b #$18
    sta.w !BBAD0
    lda.w !t_b6a2,x
    sta.w !A1T0L
    lda.w !t_b6a3,x
    sta.w !A1T0H
    lda.w !t_b6a4,x
    sta.w !A1B0
    rep.b #$20
    lda.w !t_b6a5,x
    sta.w !DAS0L
    lda.w !t_b6a7,x
    sta.w !VMADDL
    sep.b #$20
    lda.b #$01
    sta.w !MDMAEN
.l86D8:
    rts


; @routine 80:86D9 UploadBlockB
; WHAT IT DOES: When $082D asks, reads back one of the VRAM blocks listed
; at $80:B6EF into work RAM.
; HOW: $082D is the entry's offset (7 bytes: destination address and bank,
; size, VRAM address); cleared; DMA channel 0, mode $81 from RDVRAML,
; VMAIN $80, after the dummy read of RDVRAMH.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; X = the entry.
%at($8086D9)
UploadBlockB:
    ldx.w !v_082d
    beq .l8720
    stz.w !v_082d
    lda.b #$80
    sta.w !VMAIN
    lda.b #$81
    sta.w !DMAP0
    lda.b #$39
    sta.w !BBAD0
    lda.w !t_b6ef,x
    sta.w !A1T0L
    lda.w !t_b6f0,x
    sta.w !A1T0H
    lda.w !t_b6f1,x
    sta.w !A1B0
    rep.b #$20
    lda.w !t_b6f2,x
    sta.w !DAS0L
    sep.b #$20
    lda.w !t_b6f4,x
    sta.w !VMADDL
    lda.w !t_b6f5,x
    sta.w !VMADDH
    lda.w !RDVRAMH
    lda.b #$01
    sta.w !MDMAEN
.l8720:
    rts


; @routine 80:874F PaletteUploadFrom
; WHAT IT DOES: When palette_upload asks, DMAs the palettes to CGRAM from
; the source $0845 picks in the table at $80:B608: 512 bytes, or 256 for
; source 6.
; HOW: palette_upload cleared, CGADD 0; DMA channel 0, mode 0 to CGDATA,
; the source from the table (3 bytes an entry), the size by the entry.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR from the NMI. Exit: M
; 8-bit; X = $0845.
%at($80874F)
PaletteUploadFrom:
    lda.w !palette_upload
    beq .l8792
    stz.w !CGADD
    stz.w !palette_upload
    lda.b #$00
    sta.w !DMAP0
    lda.b #$22
    sta.w !BBAD0
    ldx.w !v_0845
    lda.w !t_b608,x
    sta.w !A1T0L
    lda.w !t_b609,x
    sta.w !A1T0H
    lda.w !t_b60a,x
    sta.w !A1B0
    cpx.b #$06
    bne .l8783
    lda.b #$00
    ldy.b #$01
    bra .l8787
.l8783:
    lda.b #$00
    ldy.b #$02
.l8787:
    sta.w !DAS0L
    sty.w !DAS0H
    lda.b #$01
    sta.w !MDMAEN
.l8792:
    rts
