; level_screen_setups.asm: levels' own screen set-ups, by level from the
; table at $81:8802: palette cycles, the layers on the main and sub
; screens, colour maths, and an HDMA wave on layer 2. Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/level/level_screen_setups.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram_video.inc"

arch 65816

!TM_SHADOW = $399  ; the layers on the main screen (TM, for the NMI)
!TS_SHADOW = $39A  ; the layers on the sub screen (TS)
!V_083F = $83F  ; the level's layer set-up mode
!V_1C7D = $1C7D
!HDMA_TABLE_AT_HIGH = $351  ; the page of the HDMA table HDMA_TABLE_AT names
!V_7FF441 = $7FF441
!V_7FF457 = $7FF457
!V_7FF481 = $7FF481
!V_7FF495 = $7FF495

; @routine 81:8A88 Level08ScreenSetup
; WHAT IT DOES: Level 8's screen set-up: six palette cycles, layers 1-3
; and sprites on the main screen with sprites alone on the sub screen,
; colour maths adding the sub screen to layers 1 and 2, the backdrop and
; sprites, at half.
; HOW: PaletteCycleStart for (slot, cycle) ($54, $2D), (0, 8), ($0E, 9),
; ($1C, $0E), ($2A, $0F), ($38, $10); TM_SHADOW $17, TS_SHADOW $10; CGWSEL
; $03, CGADSUB $53 (shadows).
; Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
; at $81:8802 (81:87FF). Exit: by RTS (to the caller of the routine at
; $81:87EF); A = $53.
%at($818A88)
Level08ScreenSetup:
    ldx.b #$54
    lda.b #$2D
    jsr.w PaletteCycleStart
    ldx.b #$00
    lda.b #$08
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$09
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$0E
    jsr.w PaletteCycleStart
    ldx.b #$2A
    lda.b #$0F
    jsr.w PaletteCycleStart
    ldx.b #$38
    lda.b #$10
    jsr.w PaletteCycleStart
    lda.b #$17
    sta.w !TM_SHADOW
    lda.b #$10
    sta.w !TS_SHADOW
    lda.b #$03
    sta.w !cgwsel_shadow
    lda.b #$53
    sta.w !cgadsub_shadow
    rts


; @routine 81:8B1C Level0BScreenSetup
; WHAT IT DOES: Level $0B's screen set-up: eight palette cycles, layers
; 1-3 and sprites on the main screen, layers 1 and 3 on the sub screen,
; colour maths on all but layer 3, at half; layer set-up mode 2.
; HOW: PaletteCycleStart for (slot, cycle) (0, 0), ($0E, 1), ($1C, 3),
; ($2A, $0B), ($54, $18), ($64, $1B), ($74, $1E), ($84, $21); TM_SHADOW
; $17, TS_SHADOW $15; CGWSEL $03, CGADSUB $73 (shadows); V_083F = 2.
; Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
; at $81:8802 (81:87FF). Exit: by RTS (to the caller of the routine at
; $81:87EF); A = 2.
%at($818B1C)
Level0BScreenSetup:
    ldx.b #$00
    lda.b #$00
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$01
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$03
    jsr.w PaletteCycleStart
    ldx.b #$2A
    lda.b #$0B
    jsr.w PaletteCycleStart
    ldx.b #$54
    lda.b #$18
    jsr.w PaletteCycleStart
    ldx.b #$64
    lda.b #$1B
    jsr.w PaletteCycleStart
    ldx.b #$74
    lda.b #$1E
    jsr.w PaletteCycleStart
    ldx.b #$84
    lda.b #$21
    jsr.w PaletteCycleStart
    lda.b #$17
    sta.w !TM_SHADOW
    lda.b #$15
    sta.w !TS_SHADOW
    lda.b #$03
    sta.w !cgwsel_shadow
    lda.b #$73
    sta.w !cgadsub_shadow
    lda.b #$02
    sta.w !V_083F
    rts


; @routine 81:88C5 Level02ScreenSetup
; WHAT IT DOES: Level 2's (and level $15's) screen set-up: layer set-up
; mode $0E, layer 2 following $40, and HDMA channel 4 writing layer 2's
; horizontal scroll from the table at $7E:F400.
; HOW: V_083F = $0E, BG2_FOLLOWS = $40, V_7FF441 = 8, V_7FF457 = 6, V_1C7D
; = $0C; channel 4: mode 2 (two writes) to BG2HOFS ($0F), the table at
; $7E:F400 (also in HDMA_TABLE_AT), indirect bank 0; HDMAEN shadow = $10
; (channel 4 alone).
; Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
; at $81:8802 (81:87FF), or called by JSR (81:8D42, level $15's entry).
; Exit: A = $10; X, Y unchanged.
%at($8188C5)
Level02ScreenSetup:
    lda.b #$0E
    sta.w !V_083F
    lda.b #$40
    sta.w !bg2_follows
    lda.b #$08
    sta.l !V_7FF441
    lda.b #$06
    sta.l !V_7FF457
    lda.b #$0C
    sta.w !V_1C7D
.l88E0:
    lda.b #$02
    sta.w !DMAP4
    lda.b #$0F
    sta.w !BBAD4
    lda.b #$00
    sta.w !A1T4L
    sta.w !hdma_table_at
    lda.b #$F4
    sta.w !A1T4H
    sta.w !HDMA_TABLE_AT_HIGH
    lda.b #$7E
    sta.w !A1B4
    stz.w !DASB4
    lda.b #$10
    sta.w !hdmaen_shadow
    rts


; @routine 81:888B Level00ScreenSetup
; WHAT IT DOES: Level 0's (and levels $14 and $1D's) screen set-up: layer
; set-up mode $0A, layer 2 following $40, five palette cycles, and HDMA
; channel 4 writing layer 2's horizontal scroll from the table at $7E:F400
; (as Level02ScreenSetup).
; HOW: V_083F = $0A, BG2_FOLLOWS = $40; PaletteCycleStart ($54, 0);
; V_7FF481 and V_7FF495 = $18; PaletteCycleStart ($54, 0), ($64, 3), ($74,
; 6), ($84, 9); then into Level02ScreenSetup's HDMA channel 4 set-up.
; Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
; at $81:8802 (81:87FF), or called by JSR (81:8D6C). Exit: as
; Level02ScreenSetup.
%at($81888B)
Level00ScreenSetup:
    lda.b #$0A
    sta.w !V_083F
    lda.b #$40
    sta.w !bg2_follows
    ldx.b #$54
    lda.b #$00
    jsr.w PaletteCycleStart
    lda.b #$18
    sta.l !V_7FF481
    sta.l !V_7FF495
    ldx.b #$54
    lda.b #$00
    jsr.w PaletteCycleStart
    ldx.b #$64
    lda.b #$03
    jsr.w PaletteCycleStart
    ldx.b #$74
    lda.b #$06
    jsr.w PaletteCycleStart
    ldx.b #$84
    lda.b #$09
    jsr.w PaletteCycleStart
    bra Level02ScreenSetup_l88E0


; @routine 81:8B6E Level0CScreenSetup
; WHAT IT DOES: Level $0C's screen set-up: the eight palette cycles of
; level $0B, nothing else.
; HOW: PaletteCycleStart for (slot, cycle) (0, 0), ($0E, 1), ($1C, 3),
; ($2A, $0B), ($54, $18), ($64, $1B), ($74, $1E), ($84, $21).
; Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
; at $81:8802 (81:87FF). Exit: by RTS; as the last PaletteCycleStart left
; them.
%at($818B6E)
Level0CScreenSetup:
    ldx.b #$00
    lda.b #$00
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$01
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$03
    jsr.w PaletteCycleStart
    ldx.b #$2A
    lda.b #$0B
    jsr.w PaletteCycleStart
    ldx.b #$54
    lda.b #$18
    jsr.w PaletteCycleStart
    ldx.b #$64
    lda.b #$1B
    jsr.w PaletteCycleStart
    ldx.b #$74
    lda.b #$1E
    jsr.w PaletteCycleStart
    ldx.b #$84
    lda.b #$21
    jsr.w PaletteCycleStart
    rts
