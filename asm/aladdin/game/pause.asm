; pause.asm: the pause: Start during play freezes every task but the one
; that runs it, shows PAUSED and pauses the music, until Start again.
; C: src/aladdin/game/pause.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"

arch 65816

!PAUSE_COOLDOWN = $000B         ; frames before Start can pause again
!V_0382 = $0382                 ; a pad byte: bit 4 (Start) pauses and unpauses
!PAD_START = $10
!PAUSE_BLOCK = $0404            ; nonzero: no pause (with play_mode and V_03FA)
!PAUSED = $0405                 ; counted up while paused
!V_03F9 = $03F9                 ; 1 while paused
!V_03FA = $03FA
!TEXT_PAUSED = $16              ; text layouts: PAUSED; with bit 7, blanked
!TEXT_PAUSED_BLANK = !TEXT_PAUSED|$80
!LEVEL_NO_PAUSE_SOUND = $12     ; the level that pauses without sound $0E
!SOUND_PAUSE = $0E
!SOUND_MUSIC_PAUSE = $F3
!SOUND_MUSIC_GO_ON = $F4
!TASK_SLOT = $18                ; bytes a task slot

; PauseCheck's way out when it does not pause: the cooldown counted down.
; It sits before the routine's entry.
%at($81838E)
PauseCooldown:
    lda.w !PAUSE_COOLDOWN
    beq .done
    dec a
    sta.w !PAUSE_COOLDOWN
.done:
    rts

; @routine 81:8398 PauseCheck
; WHAT IT DOES: Pauses the game when Start is pressed during play (full
; brightness, no demo, nothing blocking): every other task frozen, PAUSED
; shown and the music paused, until Start again; then a short cooldown.
; HOW: PAUSE_COOLDOWN nonzero, the screen not at full brightness,
; PAUSE_BLOCK, PLAY_MODE or V_03FA nonzero, or V_0382 bit 4 clear: the
; cooldown counted down (when nonzero) and back (the code at 81:838E).
; Else: TEXT_UPLOAD_SIZE $0380, PAUSED up, PAUSED shown, V_03F9 = 1,
; sound $0E (not on level $12) and $F3; task slots 7 down to 1 have their
; state pushed and set to 2; each frame (TaskWait 1) until V_0382 bit 4;
; PAUSED blanked, V_03F9 and PAUSED cleared, cooldown $10, sound $F4; the
; states pulled back into slots 1 up to 7; one frame more; TEXT_UPLOAD_SIZE
; $0160.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSR from the play loop's task (81:82DB), whose stack holds the states
; while it waits. Exit: M and X 8-bit; not paused: A low = the cooldown, N
; and Z from it; paused: A low = $60, X = $C0, N, Z and C from the CPX.
%at($818398)
PauseCheck:
    lda.w !PAUSE_COOLDOWN
    bne PauseCooldown
    lda.w !inidisp_shadow
    and.b #$0F
    cmp.b #$0F
    bne PauseCooldown
    lda.w !PAUSE_BLOCK
    ora.w !play_mode
    ora.w !V_03FA
    bne PauseCooldown
    lda.w !V_0382
    bit.b #!PAD_START
    beq PauseCooldown
    rep.b #$20
    lda.w #$0380
    sta.w !text_upload_size
    sep.b #$20
    inc.w !PAUSED
    lda.b #!TEXT_PAUSED
    jsl.l TextLayoutShow
    lda.b #$01
    sta.w !V_03F9
    lda.w !level
    cmp.b #!LEVEL_NO_PAUSE_SOUND
    beq .music
    lda.b #!SOUND_PAUSE
    jsl.l SoundQueueAdd
.music:
    lda.b #!SOUND_MUSIC_PAUSE
    jsl.l SoundQueueAdd
    ldx.b #7*!TASK_SLOT
.freeze:
    lda.w !task_state,x
    pha
    lda.b #$02
    sta.w !task_state,x
    sec
    txa
    sbc.b #!TASK_SLOT
    tax
    bne .freeze
.paused:
    lda.b #$01
    jsl.l TaskWait
    lda.w !V_0382
    bit.b #!PAD_START
    beq .paused
    lda.b #!TEXT_PAUSED_BLANK
    jsl.l TextLayoutShow
    stz.w !V_03F9
    stz.w !PAUSED
    lda.b #$10
    sta.w !PAUSE_COOLDOWN
    lda.b #!SOUND_MUSIC_GO_ON
    jsl.l SoundQueueAdd
    ldx.b #!TASK_SLOT
.thaw:
    pla
    sta.w !task_state,x
    clc
    txa
    adc.b #!TASK_SLOT
    tax
    cpx.b #8*!TASK_SLOT
    bne .thaw
    lda.b #$01
    jsl.l TaskWait
    rep.b #$20
    lda.w #$0160
    sta.w !text_upload_size
    sep.b #$20
    rts
