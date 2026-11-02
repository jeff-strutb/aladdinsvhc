; level_tables.asm: values the level takes from per-level tables.
; C: src/aladdin/level/level_tables.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

; Per level: two bytes for level_035c (word per level).
!level_035c_table = $81B933
; Per level, the offset of its trigger-spot list from level_spot_table;
; the list is a count, then 6-byte spots.
!level_spot_table = $81EC1E

; The level's start: per level a word, the offset of its starts (x, y:
; words; x bit 15 = facing left); the camera's (x, y) the same at
; level_camera; the companion's $07 + 1; the song (bit 7: a sound
; command); the restart point's x (negative: none).
!level_starts = $ED9D
!level_camera = $EF15
!level_companion = $B7BB
!level_songs = $B8EF
!level_restart_x = $F015
!level_restart = $0360           ; the restart point the level starts at (0 or 1)
!restart_held = $1423            ; nonzero: no restart point is set
!V_083F = $083F
!range_x = $1A44                 ; the point actors are kept in range of (words)
!range_y = $1A46
!V_1A40 = $1A40
!V_1A42 = $1A42
!companion = $097B               ; the companion actor's record
!sound_command_f0 = $81F9F0      ; sends sound command $F0 (not yet filed)
!PLAYER_FACING_LEFT = $0101      ; the player's facing and flip, left
; The exit: per level, nonzero when the camera passing the spot ends it
; (and the exit walk is the companion's); per sequence, the next one.
!level_exit_by_scroll = $B96B
!next_sequence = $B988
!V_141B = $141B
!V_1454 = $1454
!V_0347 = $0347
!EXIT_WALK = $CBF9               ; the player's code for the exit walk
!EXIT_WALK_COMPANION = $E730

; @routine 81:84B2 LevelStartPlace
; WHAT IT DOES: Puts the player and the camera at the level's start (its
; restart point $0360): the player's position and facing, both layers'
; scroll and the point actors are kept in range of.
; HOW: $1738 cleared; the level's start list (a word at $80:ED9D + level
; * 2, plus $0360 * 4) gives the player's x (bit 15: facing left, $08EF =
; $0101) and y; the camera list ($80:EF15) gives layer 0's scroll x and
; y, also its last and drawn positions; layer 1 takes them halved (x
; whole on level $10 or with $083F set; y whole on levels 0, 3, $0D,
; $14, $1A); the range point = the scroll + $80 each way; $1A40 = 0,
; $1A42 = $7F00.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
; A = $7F00 (16 bits); X, Y as the tables left them (low bytes);
; scratch $0C = $0360 * 4.
%at($8184B2)
LevelStartPlace:
    ldx.b #$00
.clear:
    stz.w !layer0,x
    dex
    bpl .clear
    lda.w !level
    asl a
    tay
    lda.w !level_restart
    asl a
    asl a
    sta.w !scratch
    stz.w !scratch+1
    rep.b #$30
    clc
    lda.w !level_starts,y
    adc.w !scratch
    tax
    lda.w !level_starts,x
    pha
    and.w #$7FFF
    sta.w !player+!actor_x+1
    stz.w !player+!actor_facing
    pla
    bpl .right
    lda.w #!PLAYER_FACING_LEFT
    sta.w !player+!actor_facing
.right:
    lda.w !level_starts+2,x
    sta.w !player+!actor_y+1
    clc
    lda.w !level_camera,y
    adc.w !scratch
    tax
    lda.w 0,x
    sta.w !layer0+!LAYER_SCROLL_X
    sta.w !scroll_x_before
    sta.w !layer0+!LAYER_DRAWN_X
    ldy.w !level
    cpy.w #$0010
    beq .x_whole
    ldy.w !V_083F
    bne .x_whole
    lsr a
.x_whole:
    sta.w !layer1+!LAYER_SCROLL_X
    sta.w !layer1+!LAYER_DRAWN_X
    lda.w 2,x
    sta.w !layer0+!LAYER_SCROLL_Y
    sta.w !scroll_y_before
    sta.w !layer0+!LAYER_DRAWN_Y
    ldy.w !level
    beq .y_whole
    cpy.w #$0003
    beq .y_whole
    cpy.w #$000D
    beq .y_whole
    cpy.w #$0014
    beq .y_whole
    cpy.w #$001A
    beq .y_whole
    lsr a
.y_whole:
    sta.w !layer1+!LAYER_SCROLL_Y
    sta.w !layer1+!LAYER_DRAWN_Y
    clc
    lda.w !layer0+!LAYER_SCROLL_X
    adc.w #$0080
    sta.w !range_x
    clc
    lda.w !layer0+!LAYER_SCROLL_Y
    adc.w #$0080
    sta.w !range_y
    stz.w !V_1A40
    lda.w #$7F00
    sta.w !V_1A42
    sep.b #$30
    rts

; @routine 81:8623 LevelCompanionStart
; WHAT IT DOES: On a level with a companion actor (the table at $80:B7BB),
; starts it at the player's place, and loads its palettes.
; HOW: The level's byte, less one, is the companion's $07; the record at
; $097B is made new ($0C) with hit box $72, look $0240 and the player's
; position; palette set $3A, and on level $12 set $48 too (PaletteLoad).
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: as PaletteLoad,
; or with no companion A low byte = 0, Z set.
%at($818623)
LevelCompanionStart:
    ldx.w !level
    lda.w !level_companion,x
    beq .done
    dec a
    sta.w !companion+!actor_07
    lda.b #$0C
    sta.w !companion+!actor_state
    lda.b #$72
    sta.w !companion+!actor_06
    rep.b #$20
    lda.w #$0240
    sta.w !companion+!actor_look
    lda.w !player+!actor_x+1
    sta.w !companion+!actor_x+1
    lda.w !player+!actor_y+1
    sta.w !companion+!actor_y+1
    sep.b #$20
    lda.b #$3A
    jsl.l PaletteLoad
    lda.w !level
    sec
    cmp.b #$12
    bne .done
    lda.b #$48
    jsl.l PaletteLoad
.done:
    rts

; @routine 81:86DE LevelSongPlay
; WHAT IT DOES: Plays the level's song, or sends its sound command when
; the level's entry is one.
; HOW: The level's byte in the table at $80:B8EF: below $80, SongPlay;
; else $81:F9F0 (command $F0).
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: as SongPlay
; or $81:F9F0.
%at($8186DE)
LevelSongPlay:
    ldx.w !level
    lda.w !level_songs,x
    bmi .command
    jsl.l SongPlay
    rts
.command:
    jsl.l !sound_command_f0
    rts

; @routine 81:86F0 RestartPointCheck
; WHAT IT DOES: Sets the level's restart point once the camera has
; passed it: from there a lost life starts the level at its second start.
; HOW: With no restart point yet ($0360 = 0) and $1423 clear, the level's
; word in the table at $80:F015 (negative: none) below layer 0's scroll x
; sets $0360 = 1.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
; A = the last word read (16 bits); X as the test left it.
RestartPointCheck:
    rep.b #$20
    ldx.w !level_restart
    bne .done
    lda.w !restart_held
    bne .done
    lda.w !level
    and.w #$00FF
    asl a
    tax
    lda.w !level_restart_x,x
    bmi .done
    cmp.w !layer0+!LAYER_SCROLL_X
    bcs .done
    ldx.b #$01
    stx.w !level_restart
.done:
    sep.b #$30
    rts

; @routine 81:8716 LevelSpotsSet
; WHAT IT DOES: Finds the level's trigger spots: how many, and where their
; list is.
; HOW: The level's word in the spot table is the list's offset; its first
; byte is the count.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSR. Exit: M and X 8-bit;
; A low byte = the count, high byte = the level word * 2's; X = the low
; byte of the list's offset; Y = the level * 2 (low byte); N, Z from the
; count; C = bit 15 of the level word; V unchanged.
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

; @routine 81:872E LevelExitCheck
; WHAT IT DOES: Ends the level when the player reaches its exit spot (on
; some levels, when the camera passes it): sets the next stage and starts
; the player's exit walk.
; HOW: Not while $141C, $1423 or $141B is set, nor with no spots. Levels
; marked in the table at $80:B96B: the first spot's x below layer 0's
; scroll x. Others: each spot in turn, the player within 8 pixels of its
; x, then of the first spot's y. Reached ($81:879D on): the spot's byte
; at +6 (low 7 bits) is the stage ($035D), 0 moving the sequence on
; through the table at $80:B988; the player's code = $E730 (marked
; levels) or $CBF9, its facing from the byte's bit 7; $0347 bit 7 set;
; $1423 and $1454 up by one; the restart point cleared.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: M and X 8-bit;
; A, X, Y as the tests left them.
LevelExitCheck:
    lda.w !v_141c
    ora.w !restart_held
    ora.w !V_141B
    bne .done
    lda.w !level_spot_count
    beq .done
    tay
    ldx.w !level
    lda.w !level_exit_by_scroll,x
    bne .scroll
    rep.b #$30
    ldx.w !level_spots
.spot:
    sec
    lda.w !level_spot_table+1,x
    sbc.w !player+!actor_x+1
    clc
    adc.w #$0008
    cmp.w #$0010
    bcs .next
    ldx.w !level_spots
    sec
    lda.w !level_spot_table+3,x
    sbc.w !player+!actor_y+1
    clc
    adc.w #$0008
    cmp.w #$0010
    bcc LevelExitReached
.next:
    clc
    txa
    adc.w #$0006
    tax
    dey
    bne .spot
.out:
    sep.b #$30
.done:
    rts
.scroll:
    rep.b #$30
    ldx.w !level_spots
    sec
    lda.w !level_spot_table+1,x
    sbc.w !layer0+!LAYER_SCROLL_X
    bcs .out
    bra LevelExitReached

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

; LevelExitCheck's exit reached (past Level035CSet).
%at($81879D)
LevelExitReached:
    sep.b #$20
    ldx.w !level_spots
    lda.w !level_spot_table+5,x
    lda.w !level_spot_table+6,x
    pha
    and.b #$7F
    sta.w !level_035c+1
    bne .stage
    ldx.w !level_035c
    lda.w !next_sequence,x
    sta.w !level_035c
.stage:
    ldx.w #$0000
    pla
    asl a
    bcc .facing
    inx
.facing:
    ldy.w !level
    lda.w !level_exit_by_scroll,y
    beq .walk
    ldy.w #!EXIT_WALK_COMPANION
    bra .code
.walk:
    ldy.w #!EXIT_WALK
.code:
    sty.w !player+!actor_03
    lda.b #$80
    tsb.w !V_0347
    sep.b #$30
    stx.w !player+!actor_flip
    stx.w !player+!actor_facing
    inc.w !restart_held
    inc.w !V_1454
    stz.w !level_restart
    rts
