; continue_screen.asm: the screen between a lost life and the next try,
; where the continues left are shown and one can be spent.
; C: src/aladdin/game/continue_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"

arch 65816

!CHOICE_CONTINUE = $0076        ; set when Continue is chosen
!CHOICE_END = $0077             ; set when End is chosen (or no continues are left)
!WAIT_LEFT = $007E              ; frames the wait with no continues still runs
!SLOT1_STATE = !task_state+24   ; slot 1's state: the fades run there
!PAD1_PRESSED_HIGH = !joy_pressed+1 ; bit 4: Start newly pressed
!V_035D = $035D                 ; cleared with the level on End; purpose not yet established
!V_0360 = $0360                 ; cleared here; purpose not yet established
!V_0369 = $0369                 ; cleared here; purpose not yet established
!V_036F = $036F                 ; 1 when play goes on from a continue
!V_0373 = $0373                 ; cleared on Continue; purpose not yet established
!V_0374 = $0374
!V_0375 = $0375
!V_03A0 = $03A0                 ; copied to V_03A1 here; purpose not yet established
!V_03A1 = $03A1
!V_142E = $142E                 ; counted up while the screen shows; purpose not yet established

; @routine 80:B2E3 ContinueScreenTask
; WHAT IT DOES: The continue screen, after Aladdin's last life is lost: the
; screen built and faded in with the continues left shown; with none left,
; a wait (up to $F0 frames, or until Start); else the choice (Continue or
; End) by the actor on the screen, a continue spent on Continue. Then the
; fade-out, and the game goes on (GameStartTask) or the title comes back
; (TitleStartTask, the level cleared).
; HOW: Song $2B; NmiDisable, ScreenBlank; level $25 set up; V_0369 and
; V_0360 cleared; Aladdin's record new (state $0C) with $07 = 6 (continues
; left) or $0F (none); V_142E counted up; both tilemaps ($4000, $4800)
; filled; graphics sets 1, 8 (copy $5B, or $77 as a second copy when none
; are left), $62, $F5, $0E, $31; palettes $60, $4A; V_03A1 = V_03A0;
; LevelGraphicsLoadAllLong; NmiEnable; WramC800Clear; text layouts $5A and
; $3E around the fade-in task; ContinuesShow. None left: CHOICE_END = 1,
; then 64 frames and up to $F0 more (Start ends them). Else: the choice
; cleared, layout $0A, a frame at a time until CHOICE_CONTINUE or
; CHOICE_END is set; Continue: CONTINUES down one and shown. Then 40
; frames; sound $F6 with 1; the fade-out task; frames until slot 1 is
; free; V_142E cleared. CHOICE_END: LEVEL_035C, V_035D, LEVEL, V_0360 and
; V_036F cleared, TaskExec TitleStartTask. Else V_036F = 1, V_0373-V_0375
; cleared, TaskExec GameStartTask.
; Entry: a task's start (TaskExec); M 8-bit, X 8-bit; DB a bank whose
; $0000-$1FFF is work RAM. Exit: never returns.
%at($80B2E3)
ContinueScreenTask:
    lda.b #$2B
    jsl.l SongPlay
    jsl.l NmiDisable
    jsl.l ScreenBlank
    lda.b #$25
    sta.w !level
    jsl.l LevelRamSetupLong
    stz.w !V_0369
    stz.w !V_0360
    lda.b #$06
    ldx.w !continues
    bne .lB309
    lda.b #$0F
.lB309:
    sta.w !player+!actor_07
    lda.b #$0C
    sta.w !player+!actor_state
    inc.w !V_142E
    rep.b #$10
    ldx.w #$4000
    jsl.l VramFillMap
    ldx.w #$4800
    jsl.l VramFillMap
    sep.b #$10
    lda.b #$01
    jsl.l UnpackSet
    ldx.b #$00
    jsl.l VramCopy
    lda.b #$08
    jsl.l UnpackSet
    ldx.w !continues
    bne .lB345
    ldx.b #$77
    jsl.l VramCopy2
    bra .lB34B
.lB345:
    ldx.b #$5B
    jsl.l VramCopy
.lB34B:
    ldx.b #$62
    jsl.l VramCopy
    ldx.b #$F5
    jsl.l VramCopy
    lda.b #$0E
    jsl.l UnpackSet
    ldx.b #$7E
    jsl.l VramCopy
    lda.b #$31
    jsl.l UnpackSet
    ldx.b #$23
    jsl.l VramCopy
    lda.b #$60
    jsl.l PaletteLoad
    lda.b #$4A
    jsl.l PaletteLoad
    lda.w !V_03A0
    sta.w !V_03A1
    jsl.l LevelGraphicsLoadAllLong
    jsl.l NmiEnable
    jsr.w WramC800Clear
    lda.b #$5A
    jsl.l TextLayoutShow
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl BrightnessUpTask
    lda.b #$3E
    jsl.l TextLayoutShow
    jsr.w ContinuesShow
    ldx.w !continues
    bne .lB3CE
    lda.b #$01
    sta.w !CHOICE_END
    stz.w !CHOICE_CONTINUE
    ldx.b #$40
    jsr.w FrameEndTimes
    lda.b #$F0
    sta.w !WAIT_LEFT
.lB3BB:
    ldx.b #$01
    jsr.w FrameEndTimes
    lda.w !PAD1_PRESSED_HIGH
    and.b #$10
    bne .lB3CC
    dec.w !WAIT_LEFT
    bne .lB3BB
.lB3CC:
    bra .lB3F2
.lB3CE:
    stz.w !CHOICE_CONTINUE
    stz.w !CHOICE_END
    lda.b #$0A
    jsl.l TextLayoutShow
.lB3DA:
    ldx.b #$01
    jsr.w FrameEndTimes
    lda.w !CHOICE_CONTINUE
    ora.w !CHOICE_END
    beq .lB3DA
    lda.w !CHOICE_CONTINUE
    beq .lB3F2
    dec.w !continues
    jsr.w ContinuesShow
.lB3F2:
    ldx.b #$28
    jsr.w FrameEndTimes
    jsl.l SoundQueueF6With1
    jsl.l TaskCreate
    db 1*24, 4                  ; slot 1, parameter 4
    dl BrightnessDownTask
.lB404:
    ldx.b #$01
    jsr.w FrameEndTimes
    lda.w !SLOT1_STATE
    bne .lB404
    stz.w !V_142E
    ldx.w !CHOICE_END
    beq .lB42C
    stz.w !level_035c
    stz.w !V_035D
    stz.w !level
    stz.w !V_0360
    stz.w !V_036F
    jsl.l TaskExec
    dl TitleStartTask
.lB42C:
    lda.b #$01
    sta.w !V_036F
    stz.w !V_0373
    stz.w !V_0374
    stz.w !V_0375
    jsl.l TaskExec
    dl GameStartTask

; Where the continues digit sits in the text buffer: row 26, column 29.
!CONTINUES_DIGIT = !text_tilemap+1722
!TILE_DIGIT_0 = $30

; @routine 80:B441 ContinuesShow
; WHAT IT DOES: Shows the number of continues left on the continue screen.
; HOW: The digit's tile is $30 plus the count (decimal when D is set, as
; the CPU adds); it goes into the text buffer and the buffer's upload is
; asked for.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C
; states.
%at($80B441)
ContinuesShow:
    clc
    lda.w !continues
    adc.b #!TILE_DIGIT_0
    sta.l !CONTINUES_DIGIT
    inc.w !text_tilemap_upload
    rts

; @routine 80:B44F WramC800Clear
; WHAT IT DOES: Clears the 4 KB buffer at $7E:C800 before a level starts.
; HOW: Zeroes $7E:C800-$7E:D7FF a byte at a time with 16-bit X; P comes
; back from the stack.
; Entry: M 8-bit. Exit: as the C states.
WramC800Clear:
    php
    rep.b #$10
    ldx.w #0
    lda.b #0
.clear:
    sta.l !tile_behaviour,x
    inx
    cpx.w #4096
    bcc .clear
    plp
    rts
