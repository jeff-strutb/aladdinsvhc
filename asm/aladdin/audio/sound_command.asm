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
incsrc "../include/wram_video.inc"

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
; Exit: as the C states.
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

; Levels $14-$1E queue no sound commands.
!LEVEL_QUIET_FIRST = $0014
!LEVEL_QUIET_END = $001F

; @routine 81:FA00 SoundQueueAdd
; WHAT IT DOES: Adds sound command A to the queue the NMI sends from,
; unless it is 0 or the level is one of $14-$1E, which queue none.
; HOW: The caller's Z says A is 0. With 16-bit X the level word is
; compared and the write index read as a word; the index then steps,
; wrapping at 32.
; Entry: M 8-bit, X 8-bit; A = the command, Z from it; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL. Exit: as the C states.
%at($81FA00)
SoundQueueAdd:
    rep.b #$10
    beq .done
    ldx.w !level
    cpx.w #!LEVEL_QUIET_FIRST
    bcs .late
.queue:
    ldx.w !sound_queue_write
    sta.w !sound_queue,x
    txa
    inc a
    and.b #31
    sta.w !sound_queue_write
    bra .done
.late:
    cpx.w #!LEVEL_QUIET_END
    bcs .queue
.done:
    sep.b #$10
    rtl

!SOUND_SPECIAL = $03F8          ; the last $F2 or $F6 command queued
!SOUND_CMD_F2 = $F2
!SOUND_CMD_F6 = $F6

; @routine 81:F9D2 SoundQueueF6With1
; WHAT IT DOES: Queues sound command $F6 with its parameter 1, keeping
; $F6 as the last special command.
; HOW: The parameter waits in A's high byte; SoundQueueAdd for each.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
; called by JSL. Exit: as the C states.
%at($81F9D2)
SoundQueueF6With1:
    lda.b #$01
    xba
    lda.b #!SOUND_CMD_F6
    sta.w !SOUND_SPECIAL
    jsl.l SoundQueueAdd
    xba
    bra SoundQueueAdd

; @routine 81:F9F7 SoundQueueF2
; WHAT IT DOES: Queues sound command $F2, keeping it as the last special
; command.
; HOW: Then on into SoundQueueAdd.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
; called by JSL. Exit: as the C states.
%at($81F9F7)
SoundQueueF2:
    lda.b #!SOUND_CMD_F2
    sta.w !SOUND_SPECIAL
    bra SoundQueueAdd
