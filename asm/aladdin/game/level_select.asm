; level_select.asm: the level select (a development mode): L and R held
; with a direction step the level part and stage, and the level text shows
; them.
; C: src/aladdin/game/level_select.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/rom.inc"

arch 65816

; Places not yet named keep their address as their name.
!routine_81972d = $81972D  ; not yet filed
!t_ce2e = $CE2E  ; data bank
!v_035d = $035D
!v_037c = $037C
!v_7edc1a = $7EDC1A
!v_7edc1c = $7EDC1C
!v_7edc1e = $7EDC1E
!v_7edc20 = $7EDC20
!v_7edc22 = $7EDC22

; @routine 80:A0B1 LevelNumberText
; WHAT IT DOES: Writes the level's part and stage into the text layer (or
; a fixed message for part 6, and for part 5 stage 3), and has the NMI
; upload it.
; HOW: Part 6: message $5E; part 5 stage 3: message $60 (by $81:972D).
; Else the two digits ($2031 + each) at $7E:DC1C and $7E:DC20, a '.'
; between, spaces round; text_tilemap_upload up.
; Entry: M 8-bit; DB = $80; called by JSR. Exit: M 8-bit.
%at($80A0B1)
LevelNumberText:
    lda.w !level_035c
    cmp.b #$06
    beq .lA0FB
    rep.b #$20
    lda.w !level_035c
    cmp.w #$0305
    beq .lA0F2
    and.w #$000F
    clc
    adc.w #$2031
    sta.l !v_7edc1c
    lda.w !v_035d
    and.w #$000F
    adc.w #$2031
    sta.l !v_7edc20
    lda.w #$202E
    sta.l !v_7edc1e
    lda.w #$2020
    sta.l !v_7edc1a
    sta.l !v_7edc22
    sep.b #$20
    inc.w !text_tilemap_upload
    rts
.lA0F2:
    sep.b #$20
    lda.b #$60
    jsl.l !routine_81972d
    rts
.lA0FB:
    lda.b #$5E
    jsl.l !routine_81972d
    rts


; @routine 80:A05C LevelSelect
; WHAT IT DOES: With L and R held, steps the level part (UP, DOWN) or its
; stage (RIGHT, LEFT) within its limits and shows the new level: C clear;
; C set when L and R are not held.
; HOW: Pad 1 held L and R ($0377 bits 4 and 5) and a direction newly
; pressed: UP: part up to 6, stage 0; DOWN: part down to 0, stage 0;
; RIGHT: stage up to the part's last ($80:CE2E); LEFT: stage down to 0;
; LevelNumberText after a stage change.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: C as above.
%at($80A05C)
LevelSelect:
    lda.w !joy_held
    and.b #$30
    beq .lA0AF
    lda.w !v_037c
    and.b #$0F
    beq .lA0AF
    bit.b #$08
    beq .lA07B
    ldx.w !level_035c
    cpx.b #$06
    beq .lA0AD
    inc.w !level_035c
    stz.w !v_035d
.lA07B:
    bit.b #$04
    beq .lA08A
    ldx.w !level_035c
    beq .lA0AD
    dec.w !level_035c
    stz.w !v_035d
.lA08A:
    bit.b #$01
    beq .lA09E
    ldx.w !level_035c
    lda.w !v_035d
    cmp.w !t_ce2e,x
    beq .lA0AD
    inc.w !v_035d
    bra .lA0AA
.lA09E:
    bit.b #$02
    beq .lA0AA
    ldx.w !v_035d
    beq .lA0AD
    dec.w !v_035d
.lA0AA:
    jsr.w LevelNumberText
.lA0AD:
    clc
    rts
.lA0AF:
    sec
    rts
