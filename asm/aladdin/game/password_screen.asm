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
