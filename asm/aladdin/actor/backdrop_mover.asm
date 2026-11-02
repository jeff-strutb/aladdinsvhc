; backdrop_mover.asm: helpers of the actor at $84:9044 that moves the
; backdrop layer up and down against the play field, and holds its sounds
; once the camera is past x $08E8.
; C: src/aladdin/actor/backdrop_mover.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!backdrop_speeds = $D7CA        ; data bank $80: per facing, the step added to backdrop_offset_y (4 bytes)
!MOVER_SOUNDS_END_X = $08E8     ; past this scroll x the actor sends no sounds
!scroll_x = !layer0+!LAYER_SCROLL_X

; @routine 84:9083 BackdropOffsetStep
; WHAT IT DOES: Moves the backdrop layer against the play field by one
; frame's step for the actor's facing, and counts the actor's frames.
; HOW: The facing * 4 indexes the steps at $80:D7CA; its 32-bit value is
; added to backdrop_offset_y ($1A2B); the actor's word at $40 steps down.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR. Exit:
; M 8-bit; Z set when $40 reached 0; X = the facing * 4.
%at($849083)
BackdropOffsetStep:
    rep.b #$20
    lda.b !actor_facing
    asl a
    asl a
    tax
    clc
    lda.w !backdrop_offset_y
    adc.w !backdrop_speeds,x
    sta.w !backdrop_offset_y
    lda.w !backdrop_offset_y+2
    adc.w !backdrop_speeds+2,x
    sta.w !backdrop_offset_y+2
    dec.b !actor_40
    sep.b #$20
    rts


; @routine 84:90B7 CameraPastX08E8
; WHAT IT DOES: Says whether the play field's scroll x has reached $08E8:
; C set when it has.
; HOW: A 16-bit compare of the scroll x with $08E8.
; Entry: M 8-bit; DB a bank whose $0000-$1FFF is work RAM; called by JSR.
; Exit: M 8-bit; C, N, Z from the compare; A = the scroll x.
%at($8490B7)
CameraPastX08E8:
    rep.b #$20
    lda.w !scroll_x
    cmp.w #!MOVER_SOUNDS_END_X
    sep.b #$20
    rts


!mover_sound_sent = $43         ; direct page (the actor): set once sound $F1 is queued past the end
!MOVER_END_SOUND = $F1          ; the sound command queued once past the end

; @routine 84:90A2 MoverEndSound
; WHAT IT DOES: Once the camera is past x $08E8, queues sound command $F1
; one time; before that keeps the actor's flag clear so it can be queued.
; HOW: CameraPastX08E8; not past: $43 cleared. Past with $43 clear:
; SoundQueueAdd $F1, $43 up.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM; called by JSL. Exit: M 8-bit; as SoundQueueAdd when it
; queued.
%at($8490A2)
MoverEndSound:
    jsr.w CameraPastX08E8
    bcc .before
    lda.b !mover_sound_sent
    bne .sent
    lda.b #!MOVER_END_SOUND
    jsl.l SoundQueueAdd
    inc.b !mover_sound_sent
.sent:
    rtl
.before:
    stz.b !mover_sound_sent
    rtl


; @routine 84:90C2 MoverSound
; WHAT IT DOES: Queues sound command A while the camera is not yet past x
; $08E8.
; HOW: A kept in X; CameraPastX08E8; not past: SoundQueueAdd with it.
; Entry: M 8-bit, X 8-bit; A = the command; DB a bank whose $0000-$1FFF
; is work RAM; called by JSR. Exit: X = the command; as SoundQueueAdd
; when it queued.
%at($8490C2)
MoverSound:
    tax
    jsr.w CameraPastX08E8
    bcs .past
    txa
    jsl.l SoundQueueAdd
.past:
    rts
