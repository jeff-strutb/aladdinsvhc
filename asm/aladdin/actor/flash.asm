; flash.asm: an actor flashing while the countdown at $1B29 runs: its
; sprites in palette 3 on odd counts, or palette 2 turned white.
; C: src/aladdin/actor/flash.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!flash_count = $1B29            ; counted down while the actor flashes; odd: flash
!kind_look_high = $7FE701       ; per kind (kind * 2): the high byte of its sprite attributes
!FLASH_PALETTE = 2              ; the palette PaletteFlash whitens

; @routine 84:8E3C ActorLookFlash
; WHAT IT DOES: On odd counts of the flash countdown shows the actor's
; sprites in palette 3; on even counts gives them back their kind's.
; HOW: Bit 0 of $1B29: set, the palette bits of the sprite attributes'
; high byte ($2C) set to 3; clear, that byte from its kind's at
; $7F:E701 (kind * 2).
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: X = the kind * 2; A low byte = the new
; byte.
%at($848E3C)
ActorLookFlash:
    lda.b !actor_06
    asl a
    tax
    lda.w !flash_count
    lsr a
    bcc .kind
    lda.b !actor_look+1
    and.b #$F1
    ora.b #$06
    sta.b !actor_look+1
    rts
.kind:
    lda.l !kind_look_high,x
    sta.b !actor_look+1
    rts


; @routine 84:8E56 PaletteFlash
; WHAT IT DOES: On odd counts of the flash countdown turns palette 2
; white; on even counts gives it back its kept colours.
; HOW: Bit 0 of $1B29: set, PaletteWhite 2; clear, PaletteRestoreReversed
; 2.
; Entry: M 8-bit, X 8-bit; DB = $00-$3F or $80-$BF; called by JSR. Exit:
; as PaletteWhite or PaletteRestoreReversed.
%at($848E56)
PaletteFlash:
    lda.w !flash_count
    lsr a
    bcc .restore
    lda.b #!FLASH_PALETTE
    jsl.l PaletteWhite
    rts
.restore:
    lda.b #!FLASH_PALETTE
    jsl.l PaletteRestoreReversed
    rts
