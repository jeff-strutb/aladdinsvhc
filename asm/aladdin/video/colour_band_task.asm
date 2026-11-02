; colour_band_task.asm: the colour band task: the fixed colour set line by
; line by HDMA channel 7, the lower bands pulsing dimmer and back. Drafted
; by cdraft.py; named and described by hand.
; C: src/aladdin/video/colour_band_task.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram_video.inc"

arch 65816

!BAND_TASK_STEP = $06  ; the task's own data (D = its slot): how much the lower bands are dimmed
!BAND_TASK_BUFFER = $07  ; the buffer ColourBandsWrite writes next (0 or 1); with $08 the bands left (word)
!COLOUR_BAND_HDMA_AT = $356  ; the buffer the HDMA reads: its offset, then $0357 its page

; @routine 81:A00C ColourBandTask
; WHAT IT DOES: A task that shows colour bands down the screen through the
; fixed colour (HDMA channel 7 on COLDATA), its lower bands pulsing for
; good: dimmed one step every 4 frames to 16 steps, held about half a
; second, brightened back the same way, held again.
; HOW: D = its slot (Y + $6F); step and buffer cleared. COLDATA red $0C;
; channel 7: mode 0 to COLDATA ($32), source bank $7E, indirect bank 0;
; the buffer page $F5; colour maths shadows: CGADSUB 2, CGWSEL 0, WOBJSEL
; $80. ColourBandsWrite, HDMA channel 7 on in the shadow, TaskWait 1. Then
; for good: the step up by one, ColourBandsWrite, TaskWait 4, until 16;
; TaskWait $1F; the step down by one, ColourBandsWrite, TaskWait 4, until
; 0; TaskWait $1F.
; Entry: M 8-bit, X 8-bit; Y = its slot's offset; DB = $80; started by the
; scheduler (a task entry). Exit: never returns.
%at($81A00C)
ColourBandTask:
    rep.b #$20
    clc
    tya
    adc.w #$006F
    tcd
    stz.b !BAND_TASK_BUFFER
    sep.b #$20
    lda.b #$0C
    sta.w !coldata_red
    lda.b #$00
    sta.w !DMAP7
    lda.b #$32
    sta.w !BBAD7
    lda.b #$7E
    sta.w !A1B7
    stz.w !DASB7
    lda.b #$F5
    sta.w !COLOUR_BAND_HDMA_AT+1
    lda.b #$02
    sta.w !cgadsub_shadow
    stz.w !cgwsel_shadow
    lda.b #$80
    sta.w !wobjsel_shadow
    jsr.w ColourBandsWrite
    lda.w !hdmaen_shadow
    ora.b #$80
    sta.w !hdmaen_shadow
    lda.b #$01
    jsl.l TaskWait
.lA052:
    clc
    lda.b !BAND_TASK_STEP
    adc.b #$01
    sta.b !BAND_TASK_STEP
    jsr.w ColourBandsWrite
    lda.b #$04
    jsl.l TaskWait
    lda.b !BAND_TASK_STEP
    cmp.b #$10
    bne .lA052
    lda.b #$1F
    jsl.l TaskWait
.lA06E:
    sec
    lda.b !BAND_TASK_STEP
    sbc.b #$01
    sta.b !BAND_TASK_STEP
    jsr.w ColourBandsWrite
    lda.b #$04
    jsl.l TaskWait
    lda.b !BAND_TASK_STEP
    bne .lA06E
    lda.b #$1F
    jsl.l TaskWait
    bra .lA052
