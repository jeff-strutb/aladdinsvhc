; level_flags.asm: eight flags at $7E:F260 that a level sets up as it starts, from a mask
; the level's $035D picks. Drafted by cdraft.py; named and described by
; hand.
; C: src/aladdin/level/level_flags.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!LEVEL_FLAGS = $7EF260          ; eight flags (bytes); 1: set up set; bit 7 set by actors later
!v_035d = $035D                 ; the second byte from the level's entry at $81:B933
; In bank $80: per $035D a mask of the flags to set; and the bit of each
; flag (1, 2, 4 ... $80).
!LEVEL_FLAG_MASKS = $D27E
!FLAG_BITS = $BCA1

; @routine 83:C2F0 LevelFlagFromMask
; WHAT IT DOES: Sets flag X of the level's eight to 1 when its bit is set
; in the mask for the level's $035D, else to 0.
; HOW: The mask (by $035D) ANDed with the flag's bit; nonzero becomes 1;
; stored to the flag.
; Entry: M 8-bit, X 8-bit; X = the flag (0-7); DB = $80; called by JSR (by
; 83:C2E7 for all eight, from the level's set-up at 81:8BEF, and by
; actors). Exit: M 8-bit, X 8-bit; A low = the flag set; Y = $035D. No
; level the game plays has a mask bit set: setting a flag to 1 is reached
; only by unit_level_flags.box.
%at($83C2F0)
LevelFlagFromMask:
    ldy.w !v_035d
    lda.w !LEVEL_FLAG_MASKS,y
    and.w !FLAG_BITS,x
    beq .lC2FD
    lda.b #$01
.lC2FD:
    sta.l !LEVEL_FLAGS,x
    rts
