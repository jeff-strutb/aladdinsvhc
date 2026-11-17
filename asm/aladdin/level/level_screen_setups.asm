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
.colours:
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

; @routine 81:8AC7 Level09ScreenSetup
; WHAT IT DOES: Level 9's screen set-up: four palette cycles, then the
; layers and colour maths of level 8.
; HOW: PaletteCycleStart for (slot, cycle) (0, $19), ($0E, $1A), ($1C,
; $26), ($2A, $27); then into Level08ScreenSetup's layers and colour maths.
; Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
; at $81:8802 (81:87FF). Exit: by RTS; A = $53.
%at($818AC7)
Level09ScreenSetup:
    ldx.b #$00
    lda.b #$19
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$1A
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$26
    jsr.w PaletteCycleStart
    ldx.b #$2A
    lda.b #$27
    jsr.w PaletteCycleStart
    bra Level08ScreenSetup_colours

!scroll_kind = $083D             ; the level's scroll handler (video/scroll_update.asm), times 2
!v_1a09 = $1A09                  ; added to !v_1a08 for the camera's window
!V_145B = $145B
!V_1C75 = $1C75                  ; word
!scroll_y_held = $1A12           ; nonzero: no camera follow up and down
!RESTART_POINT = $0360           ; the level's restart point the player starts from
!V_7FF043 = $7FF043
!LEVEL06_BANDS = $80B991         ; level 6's band count, then each band's lines (bank DB, $80)
!LEVEL06_COLOURS = $80B99E       ; level 6's COLDATA HDMA table

; @routine 81:8943 Level04ScreenSetup
; WHAT IT DOES: Level 4's screen set-up: its camera trigger task and one
; palette cycle.
; HOW: TaskCreate slot 3, CameraTriggerTask; PaletteCycleStart (0, 2).
; Entry: M 8-bit, X 8-bit; reached by the jump through the table by level
; at $81:8802 (81:87FF). Exit: by RTS (to the caller of the routine at
; $81:87EF); as PaletteCycleStart leaves the registers.
%at($818943)
Level04ScreenSetup:
    jsl.l TaskCreate
    db 3*24, 0               ; slot 3, parameter 0
    dl CameraTriggerTask
    ldx.b #$00
    lda.b #$02
    jsr.w PaletteCycleStart
    rts

; @routine 81:8954 Level05ScreenSetup
; WHAT IT DOES: Level 5's screen set-up: the camera's window moved on, its
; end scene's task and a window effect, three palette cycles, layers 1-3
; and sprites on the main screen with sprites alone on the sub screen,
; colour maths adding the sub screen at half.
; HOW: V_1A09 $28; TaskCreate slot 3 Level05EndScene, slot 6 (parameter
; 20) WindowEffectTask; PaletteCycleStart (0, $0A), ($0E, $0C), ($1C,
; $25); TM_SHADOW $17, TS_SHADOW $10; CGWSEL $03, CGADSUB $53 (shadows).
; Entry: as Level04ScreenSetup. Exit: by RTS; A = $53.
%at($818954)
Level05ScreenSetup:
    lda.b #$28
    sta.w !v_1a09
    jsl.l TaskCreate
    db 3*24, 0               ; slot 3, parameter 0
    dl Level05EndScene
    jsl.l TaskCreate
    db 6*24, 20               ; slot 6, parameter 20
    dl WindowEffectTask
    ldx.b #$00
    lda.b #$0A
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$0C
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$25
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

; @routine 81:8995 Level06ScreenSetup
; WHAT IT DOES: Level 6's screen set-up: two tasks (the bands' speed-up
; and a colour band), eight palette cycles, the camera's scroll kind 2
; and parallax kind $0C, BG2 following the backdrop's y only, and two
; HDMA channels: 1 writing COLDATA from $80:B99E, 2 the backdrop bands'
; BG2 scroll from $7E:F400, whose line counts it copies in.
; HOW: TaskCreate slot 3 BandsSpeedUpTask, slot 6 ColourBandTask;
; PaletteCycleStart ($54, $0C), ($64, $0F), ($74, $12), ($84, $15), (0,
; 4), ($0E, 5), ($1C, 6), ($2A, 7); V_145B 3, SCROLL_KIND 2, V_083F $0C,
; BG2_FOLLOWS $40; channel 1: DMAP 0, BBAD $2C, A1T $80:B99E; channel 2:
; DMAP 2, BBAD $0F, A1T $7E:F400; LEVEL06_BANDS' count of line bytes to
; every third byte of BAND_HDMA; HDMAEN_SHADOW 6.
; Entry: as Level04ScreenSetup; DB = $80. Exit: by RTS; A = 6; X = 3 per
; band, Y = the bands (8-bit); SCRATCH 0.
%at($818995)
Level06ScreenSetup:
    jsl.l TaskCreate
    db 3*24, 0               ; slot 3, parameter 0
    dl BandsSpeedUpTask
    jsl.l TaskCreate
    db 6*24, 0               ; slot 6, parameter 0
    dl ColourBandTask
    ldx.b #$54
    lda.b #$0C
    jsr.w PaletteCycleStart
    ldx.b #$64
    lda.b #$0F
    jsr.w PaletteCycleStart
    ldx.b #$74
    lda.b #$12
    jsr.w PaletteCycleStart
    ldx.b #$84
    lda.b #$15
    jsr.w PaletteCycleStart
    ldx.b #$00
    lda.b #$04
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$05
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$06
    jsr.w PaletteCycleStart
    ldx.b #$2A
    lda.b #$07
    jsr.w PaletteCycleStart
    lda.b #$03
    sta.w !V_145B
    lda.b #$02
    sta.w !scroll_kind
    lda.b #$0C
    sta.w !V_083F
    lda.b #$40
    sta.w !bg2_follows
    lda.b #$00                     ; channel 1: COLDATA from !LEVEL06_COLOURS
    sta.w !DMAP1
    lda.b #$2C
    sta.w !BBAD1
    lda.b #!LEVEL06_COLOURS
    sta.w !A1T1L
    lda.b #!LEVEL06_COLOURS>>8
    sta.w !A1T1H
    lda.b #!LEVEL06_COLOURS>>16
    sta.w !A1B1
    stz.w !DASB1
    lda.b #$02                     ; channel 2: BG2's scroll from !band_hdma
    sta.w !DMAP2
    lda.b #$0F
    sta.w !BBAD2
    lda.b #!band_hdma
    sta.w !A1T2L
    lda.b #!band_hdma>>8
    sta.w !A1T2H
    lda.b #!band_hdma>>16
    sta.w !A1B2
    stz.w !DASB2
    ldx.b #0                       ; the bands' line counts into its table
    txy
    lda.w !LEVEL06_BANDS
    sta.w !scratch
.band:
    lda.w !LEVEL06_BANDS+1,y
    sta.l !band_hdma,x
    iny
    inx
    inx
    inx
    dec.w !scratch
    bne .band
    lda.b #$06
    sta.w !hdmaen_shadow
    rts

; @routine 81:8AE5 Level0AScreenSetup
; WHAT IT DOES: Level $0A's screen set-up: the camera's window moved on,
; the climb's top task, three palette cycles; V_1C75 $00BD; started from
; a restart point, $7F:F043 = $FE (no script starts it there).
; HOW: V_1A09 $50; TaskCreate slot 3 ClimbTopTask; PaletteCycleStart (0,
; $1D), ($0E, $1E), ($1C, $23); V_1C75 $BD, then 0; RESTART_POINT nonzero:
; V_7FF043 $FE.
; Entry: as Level04ScreenSetup. Exit: by RTS; A = RESTART_POINT, N and Z
; from it, or $FE.
%at($818AE5)
Level0AScreenSetup:
    lda.b #$50
    sta.w !v_1a09
    jsl.l TaskCreate
    db 3*24, 0               ; slot 3, parameter 0
    dl ClimbTopTask
    ldx.b #$00
    lda.b #$1D
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$1E
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$23
    jsr.w PaletteCycleStart
    lda.b #$BD
    sta.w !V_1C75
    stz.w !V_1C75+1
    lda.w !RESTART_POINT
    beq .start
    lda.b #$FE                     ; from a restart point (no script runs it)
    sta.l !V_7FF043
.start:
    rts

; @routine 81:8BA7 Level0DScreenSetup
; WHAT IT DOES: Level $0D's screen set-up: its scene's task, two palette
; cycles, parallax kind $14.
; HOW: TaskCreate slot 3 Level0DScene; PaletteCycleStart ($0E, 1), ($1C,
; 3); V_083F $14.
; Entry: as Level04ScreenSetup. Exit: by RTS; A = $14.
%at($818BA7)
Level0DScreenSetup:
    jsl.l TaskCreate
    db 3*24, 0               ; slot 3, parameter 0
    dl Level0DScene
    ldx.b #$0E
    lda.b #$01
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$03
    jsr.w PaletteCycleStart
    lda.b #$14
    sta.w !V_083F
    rts

; @routine 81:8BC4 Level0EScreenSetup
; WHAT IT DOES: Level $0E's screen set-up: the grey past task, three
; palette cycles, layers 1-3 and sprites on both screens, parallax kind
; 8, then the level's bits expanded.
; HOW: TaskCreate slot 6 GreyPastTask; PaletteCycleStart (0, $0D), ($0E,
; $1B), ($54, $15); TM_SHADOW and TS_SHADOW $17; V_083F 8;
; Bits035DExpand.
; Entry: as Level04ScreenSetup. Exit: by RTS; as Bits035DExpand leaves
; the registers.
%at($818BC4)
Level0EScreenSetup:
    jsl.l TaskCreate
    db 6*24, 0               ; slot 6, parameter 0
    dl GreyPastTask
    ldx.b #$00
    lda.b #$0D
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$1B
    jsr.w PaletteCycleStart
    ldx.b #$54
    lda.b #$15
    jsr.w PaletteCycleStart
    lda.b #$17
    sta.w !TM_SHADOW
    sta.w !TS_SHADOW
    lda.b #$08
    sta.w !V_083F
    jsl.l Bits035DExpand
    rts

; @routine 81:8CF3 Level12ScreenSetup
; WHAT IT DOES: Level $12's screen set-up: it scrolls on its own, scroll
; kind $0A (a set speed, the camera following up and down).
; HOW: A = $0A, on into AutoScrollScreenSetup.
; Entry: as Level04ScreenSetup. Exit: as AutoScrollScreenSetup.
%at($818CF3)
Level12ScreenSetup:
    lda.b #$0A

; @routine 81:8CF5 AutoScrollScreenSetup
; WHAT IT DOES: The screen set-up of the levels that scroll on their own:
; the scroll kind A, no camera up and down, no parallax, layers 1-3 and
; sprites on the main screen with sprites alone on the sub screen, colour
; maths adding it at half, a window effect and four palette cycles.
; HOW: SCROLL_KIND = A; V_145B 1, SCROLL_Y_HELD 1, V_083F 0; TM_SHADOW
; $17, TS_SHADOW $10; CGWSEL $03, CGADSUB $53; TaskCreate slot 6
; (parameter 18) WindowEffectTask; PaletteCycleStart (0, $15), ($0E,
; $16), ($1C, $17), ($2A, $18).
; Entry: M 8-bit, X 8-bit; A = the scroll kind. Exit: as
; PaletteCycleStart leaves the registers.
AutoScrollScreenSetup:
    sta.w !scroll_kind
    lda.b #$01
    sta.w !V_145B
    lda.b #$01
    sta.w !scroll_y_held
    lda.b #$00
    sta.w !V_083F
    lda.b #$17
    sta.w !TM_SHADOW
    lda.b #$10
    sta.w !TS_SHADOW
    lda.b #$03
    sta.w !cgwsel_shadow
    lda.b #$53
    sta.w !cgadsub_shadow
    jsl.l TaskCreate
    db 6*24, 18               ; slot 6, parameter 18
    dl WindowEffectTask
    ldx.b #$00
    lda.b #$15
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$16
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$17
    jsr.w PaletteCycleStart
    ldx.b #$2A
    lda.b #$18
    jsr.w PaletteCycleStart
    rts

; @routine 81:8D42 Level15ScreenSetup
; WHAT IT DOES: Level $15's screen set-up: level 2's.
; HOW: Level02ScreenSetup.
; Entry: as Level04ScreenSetup. Exit: as Level02ScreenSetup.
%at($818D42)
Level15ScreenSetup:
    jsr.w Level02ScreenSetup
    rts

; @routine 81:8D61 Level1CScreenSetup
; WHAT IT DOES: Level $1C's screen set-up: it scrolls on its own, scroll
; kind $0E (the faster set speed), and the windows mask layer 3.
; HOW: AutoScrollScreenSetup with A = $0E; TMW_SHADOW 4.
; Entry: as Level04ScreenSetup. Exit: by RTS; A = 4.
%at($818D61)
Level1CScreenSetup:
    lda.b #$0E
    jsr.w AutoScrollScreenSetup
    lda.b #$04
    sta.w !tmw_shadow
    rts

; @routine 81:8D6C Level14ScreenSetup
; WHAT IT DOES: Levels $14 and $1D's screen set-up: level 0's.
; HOW: Level00ScreenSetup.
; Entry: as Level04ScreenSetup. Exit: as Level00ScreenSetup.
%at($818D6C)
Level14ScreenSetup:
    jsr.w Level00ScreenSetup
    rts

!V_03A1 = $03A1
!L11_BAND_FILL = $7EDF80        ; level $11's table of 64 words $4000 (its HDMA scroll)
!LEVEL11_MODES = $80B9C3        ; level $11's HDMA tables in bank $80: BGMODE, BG3 y, TM
!LEVEL11_BG3Y = $80B9CA
!LEVEL11_LAYERS = $80B9D4
!LEVEL_SETUPS = $818802         ; the set-ups by level (words, this bank)

; @routine 81:8A4A Level07ScreenSetup
; WHAT IT DOES: Level 7's screen set-up: its red-out task, three palette
; cycles, scroll kind 6, layers 1-3 and sprites on the main screen, its
; graphics set $1A unpacked, parallax kind $12.
; HOW: TaskCreate slot 6 RedOutTask; PaletteCycleStart (0, $11), ($0E,
; $12), ($1C, $13); V_145B 1, SCROLL_KIND 6, TM_SHADOW $17;
; UnpackSet $1A; V_083F $12; V_03A1 $3B.
; Entry: as Level04ScreenSetup. Exit: by RTS; A = $3B.
%at($818A4A)
Level07ScreenSetup:
    jsl.l TaskCreate
    db 6*24, 0               ; slot 6, parameter 0
    dl RedOutTask
    ldx.b #$00
    lda.b #$11
    jsr.w PaletteCycleStart
    ldx.b #$0E
    lda.b #$12
    jsr.w PaletteCycleStart
    ldx.b #$1C
    lda.b #$13
    jsr.w PaletteCycleStart
    lda.b #$01
    sta.w !V_145B
    lda.b #$06
    sta.w !scroll_kind
    lda.b #$17
    sta.w !TM_SHADOW
    lda.b #$1A
    jsl.l UnpackSet
    lda.b #$12
    sta.w !V_083F
    lda.b #$3B
    sta.w !V_03A1
    rts

; @routine 81:8C62 Level11ScreenSetup
; WHAT IT DOES: Level $11's screen set-up: parallax kind 2, its graphics
; set $21 unpacked, a table of 64 words $4000 at BAND_FILL, three HDMA
; channels from bank $80's tables (BGMODE, BG3's y, the main screen's
; layers), layers 2 and 3 and sprites on the main screen, three palette
; cycles.
; HOW: V_083F 2; UnpackSet $21; SCROLL_LIMIT_X's high byte 1; BAND_FILL
; from its last word down: 0, $40; channel 1: DMAP 0, BBAD $05, A1T
; $80:B9C3; channel 2: DMAP 2, BBAD $12, A1T $80:B9CA; channel 3: DMAP 0,
; BBAD $2C, A1T $80:B9D4; TM_SHADOW $16; PaletteCycleStart ($64, $24),
; ($74, $27), ($84, $2A).
; Entry: as Level04ScreenSetup. Exit: by RTS; as PaletteCycleStart leaves
; the registers.
%at($818C62)
Level11ScreenSetup:
    lda.b #$02
    sta.w !V_083F
    lda.b #$21
    jsl.l UnpackSet
    lda.b #$01
    sta.w !scroll_limit_x+1
    ldx.b #126
.fill:
    lda.b #$00
    sta.l !L11_BAND_FILL,x
    lda.b #$40
    sta.l !L11_BAND_FILL+1,x
    dex
    dex
    bpl .fill
    lda.b #$00
    sta.w !DMAP1
    lda.b #$05
    sta.w !BBAD1
    lda.b #!LEVEL11_MODES
    sta.w !A1T1L
    lda.b #!LEVEL11_MODES>>8
    sta.w !A1T1H
    lda.b #!LEVEL11_MODES>>16
    sta.w !A1B1
    stz.w !DASB1
    lda.b #$02
    sta.w !DMAP2
    lda.b #$12
    sta.w !BBAD2
    lda.b #!LEVEL11_BG3Y
    sta.w !A1T2L
    lda.b #!LEVEL11_BG3Y>>8
    sta.w !A1T2H
    lda.b #!LEVEL11_BG3Y>>16
    sta.w !A1B2
    stz.w !DASB2
    lda.b #$00
    sta.w !DMAP3
    lda.b #$2C
    sta.w !BBAD3
    lda.b #!LEVEL11_LAYERS
    sta.w !A1T3L
    lda.b #!LEVEL11_LAYERS>>8
    sta.w !A1T3H
    lda.b #!LEVEL11_LAYERS>>16
    sta.w !A1B3
    stz.w !DASB3
    lda.b #$16
    sta.w !TM_SHADOW
    ldx.b #$64
    lda.b #$24
    jsr.w PaletteCycleStart
    ldx.b #$74
    lda.b #$27
    jsr.w PaletteCycleStart
    ldx.b #$84
    lda.b #$2A
    jsr.w PaletteCycleStart
    rts

; @routine 81:87EF LevelScreenSetup
; WHAT IT DOES: Starts the level's screen: V_145B and V_1C7D cleared,
; NMI, IRQ by V and auto joypad read on, then the level's own set-up.
; HOW: V_145B 0, NMITIMEN_SHADOW $31, V_1C7D 0; JMP through LEVEL_SETUPS
; by LEVEL (byte, times 2).
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
; called by JSR from the level's start (81:8216). Exit: as the level's
; set-up.
%at($8187EF)
LevelScreenSetup:
    stz.w !V_145B
    lda.b #$31
    sta.w !nmitimen_shadow
    stz.w !V_1C7D
    lda.w !level
    asl a
    tax
    jmp.w (!LEVEL_SETUPS,x)
