; reset.asm: clearing the game's state before a new screen or game.
; C: src/aladdin/system/reset.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; Work RAM the game clears whole for a new game: $03F9-$1CCD, and the
; 1.5 KB at $7E:F400.
!game_ram = $03F9
!GAME_RAM_LAST = $18D4
!game_ram_high = $7EF400
!GAME_RAM_HIGH_LAST = $05FF
; The four bytes from $03F5 the level code counts with.
!level_counters = $03F5
; Two tables of records the level code keeps: 6 of 14 bytes from $15F1, 10
; of 16 bytes from $1645.
!table_15f1 = $15F1
!table_1645 = $1645

; @routine 81:9626 TaskSlotsFree1To6
; WHAT IT DOES: Ends every task but those in slots 0 and 7.
; HOW: Clears the state byte of slots 1 to 6.
; Entry: DB a bank whose $0000-$1FFF is work RAM; M 8-bit. Exit: every
; register and flag unchanged.
%at($819626)
TaskSlotsFree1To6:
    stz.w !task_state+($18*1)
    stz.w !task_state+($18*2)
    stz.w !task_state+($18*3)
    stz.w !task_state+($18*4)
    stz.w !task_state+($18*5)
    stz.w !task_state+($18*6)
    rtl

; @routine 81:9647 LevelCountersClear
; WHAT IT DOES: Clears the four level counters at $03F5.
; HOW: Four byte stores, last to first.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: X = $FF; N set, Z clear (DEX); A, Y, C, V unchanged.
%at($819647)
LevelCountersClear:
    rep.b #$10
    ldx.w #3
.byte:
    stz.w !level_counters,x
    dex
    bpl .byte
    sep.b #$10
    rtl

; @routine 81:9655 GameRamClear
; WHAT IT DOES: Clears the game's state for a new game and ends every task
; but slot 0's.
; HOW: Clears $03F9-$1CCD and $7E:F400-$F9FF a byte at a time, last first,
; then the state byte of slots 7 down to 1.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: X 8-bit = 0; A low byte = 0; N clear, Z set, C set; V clear; Y
; unchanged.
GameRamClear:
    rep.b #$10
    ldx.w #!GAME_RAM_LAST
.low:
    stz.w !game_ram,x
    dex
    bpl .low
    ldx.w #!GAME_RAM_HIGH_LAST
    lda.b #0
.high:
    sta.l !game_ram_high,x
    dex
    bpl .high
    sep.b #$10
    ldx.b #$18*7
.slot:
    stz.w !task_state,x
    sec
    txa
    sbc.b #$18
    tax
    bne .slot
    rtl

; @routine 81:967B LevelTablesClear
; WHAT IT DOES: Empties the level code's record tables at $15F1 and $1645.
; HOW: Table15F1Clear, then clears the first byte of each of the ten
; 16-byte records at $1645.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A low byte = X = $A0; Y = 0; Z set, N clear; C, V from the last
; ADC (clear).
LevelTablesClear:
    jsl.l Table15F1Clear
    ldy.b #10
    ldx.b #0
.record:
    stz.w !table_1645,x
    txa
    clc
    adc.b #16
    tax
    dey
    bne .record
    rtl

; @routine 81:968F Table15F1Clear
; WHAT IT DOES: Empties the level code's six 14-byte records at $15F1.
; HOW: Clears each record's first byte.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM.
; Exit: A low byte = X = $54; Y = 0; Z set, N clear; C, V from the last
; ADC (clear).
Table15F1Clear:
    ldy.b #6
    ldx.b #0
.record:
    stz.w !table_15f1,x
    txa
    clc
    adc.b #14
    tax
    dey
    bne .record
    rtl
