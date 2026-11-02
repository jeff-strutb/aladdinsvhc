; level_flags.asm: eight flags at $7E:F260 that a level sets up as it starts, from a mask
; the level's $035D picks. Drafted by cdraft.py; named and described by
; hand.
; C: src/aladdin/level/level_flags.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!LEVEL_FLAGS = $7EF260          ; eight flags (bytes); 1: set up set; bit 7 set by actors later
!v_035d = $035D                 ; the second byte from the level's entry at $81:B933
; In bank $80: per $035D a mask of the flags to set; and the bit of each
; flag (1, 2, 4 ... $80).
!LEVEL_FLAG_MASKS = $D27E
!FLAG_BITS = $BCA1
; In bank $80: per level from $0E, the offset of its list of flagged
; spots: a count, then 4 bytes a spot (x, y).
!FLAG_SPOT_LISTS = $D24A

; @routine 83:C2A9 LevelFlagSpotFind
; WHAT IT DOES: Finds a point ($000C, $000E, each to 8 pixels) in the
; level's list of flagged spots: C clear and X its flag when found, C set
; when not.
; HOW: Both words rounded down to 8; the level less $0E picks the list
; (from $80:D24A): its count, then 4 bytes a spot (x, y); compared in
; turn, 16-bit A.
; Entry: M 8-bit, X 8-bit; $000C, $000E = the point; DB = $80; levels $0E
; up; called by JSR (83:C181). Exit: M 8-bit; found: C clear, X = the
; count of spots from it to the list's end less 1; not found: C set
; (reached only by unit_point_lookup_miss.box).
%at($83C2A9)
LevelFlagSpotFind:
    lda.b #$07
    trb.w !scratch
    trb.w !scratch+2
    sec
    lda.w !level
    sbc.b #$0E
    asl a
    tax
    rep.b #$20
    ldy.w !FLAG_SPOT_LISTS,x
    ldx.w !FLAG_SPOT_LISTS,y
.lC2C1:
    lda.w !FLAG_SPOT_LISTS+1,y
    and.w #$FFF8
    cmp.w !scratch
    bne .lC2D7
    lda.w !FLAG_SPOT_LISTS+3,y
    and.w #$FFF8
    cmp.w !scratch+2
    beq .lC2E2
.lC2D7:
    iny
    iny
    iny
    iny
    dex
    bne .lC2C1
    sep.b #$20
    sec
    rts
.lC2E2:
    sep.b #$20
    dex
    clc
    rts

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
