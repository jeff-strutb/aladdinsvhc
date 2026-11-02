; player_box.asm: the box actors are tested against, set from the player's
; box table: its offsets from the player and its half-sizes. Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/player/player_box.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

; The test box (see actor/hit_box.asm): its centre's offsets from the
; player (x, y), its half-width and half-height (words).
!BOX_OFFSET_X = $148C
!BOX_OFFSET_Y = $148E
!BOX_HALF_W = $1488
!BOX_HALF_H = $148A
; In bank $80: 10 bytes a box: x offset, y offset, half-width,
; half-height, and the word the player's $16 gets.
!PLAYER_BOXES = $C7EB
!PLAYER_16 = $16                ; a word of the player's record each box sets

; @routine 81:E909 PlayerBoxUpdate
; WHAT IT DOES: Sets the test box to the player's box wanted, when it is
; not the one set already.
; HOW: PLAYER_BOX_WANTED compared with PLAYER_BOX: the same, returns at
; PlayerBoxSet's RTS; else on into PlayerBoxSet.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR. Exit: the same: A low = the box, Z set; else as PlayerBoxSet.
%at($81E909)
PlayerBoxUpdate:
    lda.w !player_box_wanted
    cmp.w !player_box
    beq PlayerBoxSet_done


; @routine 81:E911 PlayerBoxSet
; WHAT IT DOES: Sets the test box to the player's box A: its offsets from
; the player, its half-sizes, and the player's $16.
; HOW: A kept in PLAYER_BOX; times 10 ($002C the double) indexes the box
; table; the x offset negated when the player's $15 is nonzero; five words
; copied.
; Entry: M 8-bit, X 8-bit; A = the box; D = the player's record; DB = $80.
; Exit: M 8-bit, X 8-bit; X = the box times 10; Y = the player's $15; A =
; the $16 word.
%at($81E911)
PlayerBoxSet:
    sta.w !player_box
    asl a
    sta.w !scratch+32
    asl a
    asl a
    clc
    adc.w !scratch+32
    tax
    rep.b #$20
    ldy.b !actor_flip
    bne .lE92D
    lda.w !PLAYER_BOXES,x
    sta.w !BOX_OFFSET_X
    bra .lE937
.lE92D:
    lda.w !PLAYER_BOXES,x
    eor.w #$FFFF
    inc a
    sta.w !BOX_OFFSET_X
.lE937:
    lda.w !PLAYER_BOXES+2,x
    sta.w !BOX_OFFSET_Y
    lda.w !PLAYER_BOXES+4,x
    sta.w !BOX_HALF_W
    lda.w !PLAYER_BOXES+6,x
    sta.w !BOX_HALF_H
    lda.w !PLAYER_BOXES+8,x
    sta.b !PLAYER_16
    sep.b #$20
.done:
    rts
