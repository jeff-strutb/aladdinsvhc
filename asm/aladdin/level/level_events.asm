; level_events.asm: levels' event tasks that wait for the camera or the
; player to get somewhere, then change the backdrop, the colours or the
; map.
; C: src/aladdin/level/level_events.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"

arch 65816

; The backdrop bands (bank $80): per band a word, the speed it starts
; with, its step and its most.
!band_speeds_start = $C5D6
!band_speed_steps = $C5C0
!band_speeds_most = $C5EC
!BANDS = 11
!palette_2 = !palette_buffer+64
!V_0080 = $0080
!V_0399 = $0399
!V_039A = $039A
!level_restart = $0360

; @routine 81:A0C3 BandsSpeedUpTask
; WHAT IT DOES: Level $06's task that speeds the backdrop bands up once
; the camera gets far into the level, and keeps them at it.
; HOW: The 11 bands' speeds from the table at $80:C5D6; then, each frame,
; until layer 0's scroll x reaches $0680, nothing; then every frame each
; speed grows by its step ($80:C5C0) up to its most ($80:C5EC).
; Entry: the task's start: M 8-bit, X 8-bit; DB = $80. Exit: none: it
; runs on until the level ends.
%at($81A0C3)
BandsSpeedUpTask:
    rep.b #$20
    lda.w #!BANDS
    sta.w !scratch
    ldx.b #0
    txy
.start:
    lda.w !band_speeds_start,y
    sta.l !band_speed,x
    iny
    iny
    inx
    inx
    inx
    inx
    dec.w !scratch
    bne .start
    sep.b #$20
    rep.b #$10
.wait:
    lda.b #1
    jsl.l TaskWait
    ldx.w !layer0+!LAYER_SCROLL_X
    cpx.w #$0680
    bcc .wait
    sep.b #$10
.frame:
    rep.b #$20
    lda.w #!BANDS
    sta.w !scratch
    ldx.b #0
    txy
.band:
    clc
    lda.l !band_speed,x
    adc.w !band_speed_steps,y
    cmp.w !band_speeds_most,y
    bcc .set
    lda.w !band_speeds_most,y
.set:
    sta.l !band_speed,x
    iny
    iny
    inx
    inx
    inx
    inx
    dec.w !scratch
    bne .band
    sep.b #$20
    lda.b #1
    jsl.l TaskWait
    bra .frame

; @routine 81:A128 GreyPastTask
; WHAT IT DOES: A level's task that greys the screen down a little once
; the player gets past x $0E00, then ends.
; HOW: Each frame until the player's x reaches $0E00, nothing; then colour
; math subtracts a grey of 4 (CGADSUB's shadow $82, CGWSEL's 0, WOBJSEL's
; $80, the fixed colour 4, 4, 4), the palette upload is asked for, and
; TaskExit.
; Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
; RAM. Exit: none: the task ends.
GreyPastTask:
    rep.b #$10
.wait:
    lda.b #1
    jsl.l TaskWait
    ldx.w !player+!actor_x+1
    cpx.w #$0E00
    bcc .wait
    lda.b #$82
    sta.w !cgadsub_shadow
    stz.w !cgwsel_shadow
    lda.b #$80
    sta.w !wobjsel_shadow
    lda.b #4
    sta.w !coldata_red
    lda.b #4
    sta.w !coldata_green
    lda.b #4
    sta.w !coldata_blue
    inc.w !palette_upload
    jml.l TaskExit

; @routine 81:A15B RedOutTask
; WHAT IT DOES: Level $07's task that, once the camera passes x $1A00,
; takes the red out of palette 2 and adds blue over 8 steps a quarter
; second apart, then ends.
; HOW: Each frame until layer 0's scroll x reaches $1A00, nothing; then
; colour math adds the fixed colour to BG1 (CGADSUB's shadow 1, CGWSEL's
; 0, WOBJSEL's $80, $0080 cleared); 7 times: 15 frames, PaletteRedDown and
; the fixed colour's blue up by one; then PaletteRedDown again and
; TaskExit.
; Entry: the task's start: M 8-bit; DB a bank whose $0000-$1FFF is work
; RAM. Exit: none: the task ends.
RedOutTask:
    rep.b #$10
.wait:
    lda.b #1
    jsl.l TaskWait
    ldx.w !layer0+!LAYER_SCROLL_X
    cpx.w #$1A00
    bcc .wait
    lda.b #$01
    sta.w !cgadsub_shadow
    stz.w !cgwsel_shadow
    stz.w !V_0080
    lda.b #$80
    sta.w !wobjsel_shadow
    ldy.w #7
    ldx.w #8
.step:
    lda.b #15
    jsl.l TaskWait
    jsr.w PaletteRedDown
    inc.w !coldata_blue
    dey
    bne .step
    jsr.w PaletteRedDown
    jml.l TaskExit

; @routine 81:A197 PaletteRedDown
; WHAT IT DOES: Takes one step of red from each colour of palette 2
; (floor 0) and asks for the upload, unless X is 0.
; HOW: With X nonzero (kept on the stack), for the 16 colours from the
; last: the low byte's red (low 5 bits) down by one, not below 0, its
; other bits kept (scratch $0C); then the palette upload is asked for.
; Entry: M 8-bit, X 16-bit; X = nonzero to act; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: X as at entry; with it
; nonzero, A low byte = colour 0's new low byte; scratch $0C its other
; bits.
PaletteRedDown:
    txa
    beq .done
    phx
    ldx.w #30
.colour:
    lda.l !palette_2,x
    pha
    and.b #$E0
    sta.w !scratch
    pla
    and.b #$1F
    dec a
    bpl .keep
    lda.b #0
.keep:
    ora.w !scratch
    sta.l !palette_2,x
    dex
    dex
    bpl .colour
    inc.w !palette_upload
    plx
.done:
    rts

; @routine 81:A30D ClimbTopTask
; WHAT IT DOES: Level $0A's (and $19's) task for the climb: puts the
; player's sprites in front while near the top, and once the player
; stands at the top changes the colour math and opens the way right.
; HOW: Each frame (until the restart point is set): the player's sprite
; priority bits ($08E4 bits 4-5) = $10 when its y is under 256, else 0;
; on when the camera is at the top (scroll y 0), the player's x low byte
; $80 or more and y low byte under $78. Then the scroll y limit's high
; byte cleared and, not on level $19, $0399 = $17, $039A = $10, CGWSEL's
; shadow 3, CGADSUB's $53; each frame until the scroll x's high byte is
; 1; then the block map's byte at $7F:F043 = $FE and TaskExit.
; Entry: the task's start: M 8-bit, X 8-bit; DB a bank whose
; $0000-$1FFF is work RAM. Exit: none: the task ends.
%at($81A30D)
ClimbTopTask:
    lda.b #1
    jsl.l TaskWait
    lda.w !level_restart
    bne .top
    lda.w !player+!actor_09
    and.b #$CF
    sta.w !player+!actor_09
    lda.b #$00
    ldx.w !player+!actor_y+2
    bne .behind
    lda.b #$10
.behind:
    ora.w !player+!actor_09
    sta.w !player+!actor_09
    lda.w !layer0+!LAYER_SCROLL_Y
    ora.w !layer0+!LAYER_SCROLL_Y+1
    bne ClimbTopTask
    lda.w !player+!actor_x+1
    cmp.b #$80
    bcc ClimbTopTask
    lda.w !player+!actor_y+1
    cmp.b #$78
    bcs ClimbTopTask
.top:
    stz.w !scroll_limit_y+1
    lda.w !level
    cmp.b #$19
    beq .right
    lda.b #$17
    sta.w !V_0399
    lda.b #$10
    sta.w !V_039A
    lda.b #$03
    sta.w !cgwsel_shadow
    lda.b #$53
    sta.w !cgadsub_shadow
.right:
    lda.b #1
    jsl.l TaskWait
    lda.w !layer0+!LAYER_SCROLL_X+1
    cmp.b #$01
    bne .right
    lda.b #$FE
    sta.l !block_map+67
    jml.l TaskExit
