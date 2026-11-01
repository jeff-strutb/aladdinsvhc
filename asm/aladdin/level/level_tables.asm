; level_tables.asm: values the level takes from per-level tables.
; C: src/aladdin/level/level_tables.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; Per level: two bytes for level_035c (word per level).
!level_035c_table = $81B933
; Per level, the offset of its trigger-spot list from level_spot_table;
; the list is a count, then 6-byte spots.
!level_spot_table = $81EC1E

; @routine 81:8716 LevelSpotsSet
; WHAT IT DOES: Finds the level's trigger spots: how many, and where their
; list is.
; HOW: The level's word in the spot table is the list's offset; its first
; byte is the count.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M and X 8-bit;
; A low byte = the count, high byte = the level word * 2's; X = the low
; byte of the list's offset; Y = the level * 2 (low byte); N, Z from the
; count; C = bit 15 of the level word; V unchanged.
%at($818716)
LevelSpotsSet:
    rep.b #$30
    lda.w !level
    asl a
    tay
    ldx.w !level_spot_table,y
    sep.b #$20
    lda.w !level_spot_table,x
    sta.w !level_spot_count
    stx.w !level_spots
    sep.b #$10
    rts

; @routine 81:878B Level035CSet
; WHAT IT DOES: Sets $035C-$035D from the level's entry in the table at
; $81:B933.
; HOW: The level * 2 indexes the table; its two bytes are copied.
; Entry: M 8-bit, X 8-bit; DB = $81. Exit: A low byte = the second byte;
; N, Z from it; X = the level * 2; C = the level's bit 7; Y, V unchanged.
%at($81878B)
Level035CSet:
    lda.w !level
    asl a
    tax
    lda.w !level_035c_table,x
    sta.w !level_035c
    lda.w !level_035c_table+1,x
    sta.w !level_035c+1
    rtl
