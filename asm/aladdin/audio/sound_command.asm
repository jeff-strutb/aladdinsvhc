; sound_command.asm: sending queued sound commands to the SPC700.
;
; The game queues command bytes in a 32-entry ring. Once a frame one
; command goes to the driver through APU port 0, but only after the driver
; has echoed the previous command's sequence number on port 2, so no
; command is sent before the driver has taken the last one.
; C: src/aladdin/audio/sound_command.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; Commands $F5 and $F6 take a parameter, the next byte in the ring.
!SOUND_CMD_PARAM_A = $F5
!SOUND_CMD_PARAM_B = $F6

; @routine 80:B463 SoundCommandSend
; WHAT IT DOES: Sends the next queued sound command to the SPC700, when
; sending is not held, the queue is not empty and the driver has taken the
; last command.
; HOW: The driver has taken the last command when APU port 2 reads back
; the sequence number. Then the sending flag and the sequence number step
; up, and the command byte at the read index goes to port 0. Commands $F5
; and $F6 carry a parameter: the read index steps past the command, and
; the parameter goes to port 3 (with port 2 cleared) before the command
; goes to port 0. The read index then steps past the last byte sent,
; wrapping at 32. The sending flag is cleared on every way out.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $2140-$2143 are the APU ports ($00-$3F or $80-$BF); D any.
; Exit: as the C states, by the way out taken.
%at($80B463)
SoundCommandSend:
    lda.w !sound_hold
    bne .done
    ldx.w !sound_queue_read
    cpx.w !sound_queue_write
    beq .done
    lda.w !sound_seq
    cmp.w !APUIO2
    bne .done
    inc.w !sound_sending
    inc.w !sound_seq
    lda.w !sound_queue,x
    cmp.b #!SOUND_CMD_PARAM_A
    beq .with_param
    cmp.b #!SOUND_CMD_PARAM_B
    bne .send
.with_param:
    pha
    txa
    inc a
    and.b #31
    sta.w !sound_queue_read
    tax
    lda.w !sound_queue,x
    stz.w !APUIO2
    sta.w !APUIO3
    pla
.send:
    sta.w !APUIO0
    txa
    inc a
    and.b #31
    sta.w !sound_queue_read
.done:
    stz.w !sound_sending
    rts
