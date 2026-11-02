; play_time.asm: the time the game has been played, counted each frame in
; decimal digits.
; C: src/aladdin/game/play_time.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!play_frames = $147C              ; frames into the second (0-59)
!play_seconds = $147D             ; seconds, counted without carry
!play_time = $147E                ; seconds units, tens; minutes units, tens; hours units, tens

; @routine 81:85CE PlayTimeTick
; WHAT IT DOES: Counts one more frame of play time.
; HOW: Frames to 60 make a second; the seconds' units carry at 10, tens at
; 6, minutes' units at 10, tens at 6, hours' units at 10 into the tens.
; The seconds count at $147D steps too, without carrying.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
; Exit: A low byte = the last digit compared; N, Z, C from its compare; X,
; Y, V unchanged.
%at($8185CE)
PlayTimeTick:
    inc.w !play_frames
    lda.w !play_frames
    cmp.b #$3C
    bcc .l8622
    stz.w !play_frames
    inc.w !play_seconds
    inc.w !play_time
    lda.w !play_time
    cmp.b #$0A
    bcc .l8622
    stz.w !play_time
    inc.w !play_time+1
    lda.w !play_time+1
    cmp.b #$06
    bcc .l8622
    stz.w !play_time+1
    inc.w !play_time+2
    lda.w !play_time+2
    cmp.b #$0A
    bcc .l8622
    stz.w !play_time+2
    inc.w !play_time+3
    lda.w !play_time+3
    cmp.b #$06
    bcc .l8622
    stz.w !play_time+3
    inc.w !play_time+4
    lda.w !play_time+4
    cmp.b #$0A
    bcc .l8622
    stz.w !play_time+4
    inc.w !play_time+5
.l8622:
    rts
