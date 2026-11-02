; sound_test.asm: the options screen's sound test: its number stepped up
; or down, wrapping, and shown.
; C: src/aladdin/game/sound_test.c.

incsrc "../include/macros.inc"

arch 65816

!SOUND_TEST = $1B15             ; the sound test's number, less one (0-34)
!SOUND_TEST_LAST = $22          ; the last number (less one)

; @routine 83:8B7E SoundTestStep
; WHAT IT DOES: Steps the sound test's number up (C set) or down,
; wrapping within 1-35, queues sound command $F2 and shows the number.
; HOW: INC or DEC of the number less one: past $22 it becomes 0, below 0
; it becomes $22; SoundQueueF2, then on into SoundTestShow.
; Entry: M 8-bit, X 8-bit; C = the direction; DB a bank whose
; $0000-$1FFF is work RAM and whose $4200-$421F are the CPU registers.
; Exit: as the C states.
%at($838B7E)
SoundTestStep:
    bcs .up
    dec.w !SOUND_TEST
    bpl .show
    lda.b #!SOUND_TEST_LAST
    sta.w !SOUND_TEST
    bra .show
.up:
    inc.w !SOUND_TEST
    lda.w !SOUND_TEST
    cmp.b #!SOUND_TEST_LAST+1
    bcc .show
    stz.w !SOUND_TEST
.show:
    jsl.l SoundQueueF2
    ; on into SoundTestShow
