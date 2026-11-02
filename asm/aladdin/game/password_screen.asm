; password_screen.asm: the password screen's four slots, each showing a
; picture: its tiles drawn in the slot's place, its palette in the
; slot's.
; C: src/aladdin/game/password_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!picture = !scratch+3           ; the picture to show
!slot = !scratch+2              ; the slot (0-3)
!picture_tiles = $FE22          ; data bank $80: per picture, its first tile's tilemap word
!slot_vram = $FE2E              ; data bank $80: per slot, the VRAM address of its tiles

; @routine 84:AF52 PasswordPictureDraw
; WHAT IT DOES: Shows picture $000F in password slot $000E: its tiles in
; the slot's place and its palette in the slot's.
; HOW: TileBlockQueue at the slot's VRAM address ($80:FE2E) with the
; picture's first tile ($80:FE22) in palette 4 + the slot; then
; PaletteFromRom, ROM palette = the picture, palette = the slot.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: as
; PaletteFromRom.
%at($84AF52)
PasswordPictureDraw:
    lda.w !picture
    asl a
    tay
    lda.w !slot
    asl a
    tax
    rep.b #$30
    lda.w !scratch+1
    and.w #$FF00
    asl a
    asl a
    ora.w #$1000
    ora.w !picture_tiles,y
    tay
    lda.w !slot_vram,x
    jsl.l TileBlockQueue
    lda.w !picture
    ldx.w !slot
    jsl.l PaletteFromRom
    rts


; More places not yet named.
!region_byte = $80FFD9  ; the cartridge header's region byte
!region_list = $FE54  ; data bank $80: the count, then the region bytes
!routine_818181 = $818181  ; not yet filed
!routine_81825b = $81825B  ; not yet filed
!routine_81830d = $81830D  ; not yet filed
!routine_8196b1 = $8196B1  ; not yet filed
!routine_81972d = $81972D  ; not yet filed
!routine_81f10d = $81F10D  ; not yet filed
!t_fdd1 = $FDD1  ; data bank
!t_fe36 = $FE36  ; data bank
!t_fe3c = $FE3C  ; data bank
!v_0005 = $0005
!v_005d = $005D
!v_0087 = $0087
!v_00a5 = $00A5
!v_00a6 = $00A6
!v_00aa = $00AA
!v_00ab = $00AB
!v_00ac = $00AC
!v_00ad = $00AD
!v_0347 = $0347
!v_035d = $035D
!v_0364 = $0364
!v_0365 = $0365
!v_0367 = $0367
!v_0369 = $0369
!v_036b = $036B
!v_036e = $036E
!v_0370 = $0370
!v_037c = $037C
!v_0399 = $0399
!v_039a = $039A
!v_1522 = $1522

; @routine 84:AF7F CopierCheck
; WHAT IT DOES: A check for cartridge RAM at $70:1522 (a copier's): when
; $38 and $82 both read back there, lives, health and $0365 are set to 1.
; This board has no such RAM (each read is the bus's last byte), so the
; punishment never runs and no script covers it.
; HOW: DB = $70 ($28 + $48); $38 written and compared, then $82; both
; equal: $0364, $0367 and $0365 = 1.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: DB as at entry; RAM found:
; Y = 0, A = 1.
%at($84AF7F)
CopierCheck:
    phb
    lda.b #$28
    clc
    adc.b #$48
    pha
    plb
    ldy.b #$00
    lda.b #$38
    sta.w !v_1522,y
    cmp.w !v_1522,y
    bne .lAFAC
    lda.b #$82
    sta.w !v_1522,y
    cmp.w !v_1522,y
    bne .lAFAC
    plb
    lda.b #$01
    sta.w !v_0364
    lda.b #$01
    sta.w !v_0367
    sta.w !v_0365
    rts
.lAFAC:
    plb
    rts


; @routine 84:AFAE RegionCheck
; WHAT IT DOES: Clears $005D, and on a PAL console (STAT78 bit 4) sets it
; when the cartridge's region byte ($80:FFD9) is one of those listed at
; $FE54. This board is NTSC: the PAL branch never runs and no script
; covers it.
; HOW: STAT78 and $10; set: the byte compared with the list from its last
; entry (its count at $FE54) down; found: $005D up.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSL. Exit: M 8-bit; A = the
; masked STAT78 or the region byte.
%at($84AFAE)
RegionCheck:
    stz.w !v_005d
    lda.w !STAT78
    and.b #$10
    beq .lAFC7
    lda.l !region_byte
    ldx.w !region_list
.lAFBF:
    cmp.w !region_list,x
    beq .lAFC8
    dex
    bne .lAFBF
.lAFC7:
    rtl
.lAFC8:
    inc.w !v_005d
    rtl


; @routine 84:AD50 PasswordScreen
; WHAT IT DOES: The password screen: four slots of pictures stepped with
; the pad, checked against the table of passwords; a match sets the level
; and starts it, anything else says so and goes back to the title.
; HOW: The screen set up (palettes, graphics, the level $2C map), each
; slot's picture from the start table ($80:FDD1) shown
; (PasswordPictureDraw), slot 3 current. Each frame: A, B, X, Y or START
; checks the slots against the 8 passwords at $80:FDD5 (4 bytes each): a
; match sets the level part ($80:FE36), $AC and a message; none, a message
; and 128 frames. UP and DOWN step the slot's picture (0-5, wrapping),
; LEFT and RIGHT the slot (0-3), each with sound $0C. Then the screen
; fades; a match starts its level (with $0009 set, its counts from
; $84:AF02 instead; level part 7 by TaskExec $81:BCF6 at $84:AF44), else
; CopierCheck and the title.
; Entry: M 8-bit, X 8-bit; D = 0; a task's entry (TaskExec from the
; title). Exit: never returns.
%at($84AD50)
PasswordScreen:
    jsl.l NmiDisable
    jsl.l !routine_8196b1
    jsl.l ScrollStateClear
    jsl.l OamBufferClear
    jsl.l DisplayDefaults
    lda.b #$0D
    jsl.l UnpackSet
    ldx.b #$70
    jsl.l VramCopy
    lda.b #$23
    jsl.l UnpackSet
    ldx.b #$85
    jsl.l VramCopy
    ldx.b #$5B
    jsl.l VramCopy2
    lda.b #$06
    jsl.l UnpackSet
    ldx.b #$77
    jsl.l VramCopy
    lda.b #$2C
    sta.w !level
    jsl.l !routine_81825b
    lda.b #$4D
    jsl.l PaletteLoad
    lda.b #$00
    asl a
    asl a
    sta.w !scratch+4
    stz.w !scratch+2
.lADA7:
    lda.w !scratch+2
    tax
    lda.w !scratch+4
    clc
    adc.w !scratch+2
    tay
    lda.w !t_fdd1,y
    sta.b !v_00a6,x
    sta.w !scratch+3
    jsr.w PasswordPictureDraw
    inc.w !scratch+2
    lda.w !scratch+2
    cmp.b #$04
    bne .lADA7
    lda.b #$03
    sta.b !v_00a5
    stz.b !v_00ac
    jsl.l NmiEnable
    lda.b #$01
    jsl.l TaskWait
    lda.b #$2D
    jsl.l SongPlay
    lda.b #$1F
    sta.w !v_0399
    sta.w !v_039a
    lda.b #$01
    sta.w !v_1a08
    jsl.l TaskCreate
    db 1*24, 2                ; slot 1, parameter 2
    dl ColourFadeInTask
.lADF4:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !v_0087
    bne .lADF4
.lAE03:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !joy_pressed
    and.b #$E0
    ora.w !v_037c
    bit.b #$D0
    beq .lAE61
    lda.b #$D5
    sta.b !v_00aa
    lda.b #$FD
    sta.b !v_00ab
    stz.w !scratch
.lAE24:
    ldy.b #$00
.lAE26:
    ldx.b !v_00a6,y
    txa
    cmp.b (!v_00aa),y
    beq .lAE45
    rep.b #$21
    lda.b !v_00aa
    adc.w #$0004
    sta.b !v_00aa
    sep.b #$20
    inc.w !scratch
    lda.w !scratch
    cmp.b #$08
    bne .lAE24
    jmp.w .lAEBB
.lAE45:
    iny
    cpy.b #$04
    bne .lAE26
    lda.w !scratch
    tax
    lda.w !t_fe36,x
    sta.w !v_0005
    sta.w !level_035c
    stz.w !v_035d
    jsl.l KeptOptionsSeal
    jmp.w .lAEB1
.lAE61:
    bit.b #$08
    beq .lAE73
    ldx.b !v_00a5
    inc.b !v_00a6,x
    lda.b !v_00a6,x
    cmp.b #$06
    bne .lAE81
    lda.b #$00
    bra .lAE81
.lAE73:
    bit.b #$04
    beq .lAE94
    ldx.b !v_00a5
    dec.b !v_00a6,x
    lda.b !v_00a6,x
    bpl .lAE81
    lda.b #$05
.lAE81:
    sta.b !v_00a6,x
    sta.w !scratch+3
    stx.w !scratch+2
    jsr.w PasswordPictureDraw
    lda.b #$0C
    jsl.l SoundQueueAdd
    bra .lAEAE
.lAE94:
    bit.b #$02
    beq .lAE9C
    inc.b !v_00a5
    bra .lAEA2
.lAE9C:
    bit.b #$01
    beq .lAEAE
    dec.b !v_00a5
.lAEA2:
    lda.b !v_00a5
    and.b #$03
    sta.b !v_00a5
    lda.b #$0C
    jsl.l SoundQueueAdd
.lAEAE:
    jmp.w .lAE03
.lAEB1:
    inc.b !v_00ac
    lda.b #$64
    jsl.l !routine_81972d
    bra .lAEC1
.lAEBB:
    lda.b #$62
    jsl.l !routine_81972d
.lAEC1:
    lda.b #$80
    sta.b !v_00ad
.lAEC5:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    dec.b !v_00ad
    bne .lAEC5
    jsl.l SoundQueueF6With1
    jsl.l TaskCreate
    db 1*24, 1                ; slot 1, parameter 1
    dl ColourFadeOutTask
.lAEE0:
    lda.b #$01
    jsl.l TaskWait
    jsl.l !routine_81830d
    lda.w !v_0087
    bne .lAEE0
    lda.b !v_00ac
    bne .lAEFD
    jsr.w CopierCheck
    jsl.l TaskExec
    dl TitleAfterFadeOut
.lAEFD:
    lda.w !cheat_0009
    beq .lAF16
    lda.b #$01
    sta.w !v_036e
    lda.b #$0A
    sta.w !health_marks
    sta.w !health
    lda.b #$99
    sta.w !v_0369
    bra .lAF22
.lAF16:
    ldx.w !level_035c
    lda.w !t_fe3c,x
    sta.w !health_marks
    sta.w !health
.lAF22:
    stz.w !v_036b
    lda.b #$03
    sta.w !v_0364
    sta.w !v_0370
    lda.b #$01
    sta.w !continues
    stz.w !v_0347
    lda.b #$80
    sta.w !inidisp_shadow
    jsr.w CopierCheck
    lda.w !level_035c
    cmp.b #$07
    bne .lAF4B
    jsl.l TaskExec
    dl !routine_818181
.lAF4B:
    jsl.l TaskExec
    dl !routine_81f10d
