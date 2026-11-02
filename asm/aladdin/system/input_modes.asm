; input_modes.asm: where a frame's buttons come from: the pads, the demo's
; script (the title's attract play), or the pads with a recorder that
; writes them into cartridge RAM (a development mode retail never enters).
; C: src/aladdin/system/input_modes.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_pad.inc"
incsrc "../include/wram_state.inc"

arch 65816

; InputRead's demo mode (play mode 2), entered only from InputRead.
%at($81BD53)
InputReadDemo:
    phx
    phy
    lda.w !joy_held+1
    pha
    jsr.w JoypadRead
    stz.w !joy_held
    stz.w !joy_pressed
    stz.w !joy_held+2
    stz.w !joy_pressed+2
    stz.w !joy_held+3
    stz.w !joy_pressed+3
    pla
    eor.b #$FF
    sta.w !joy_pressed+1
    lda.w !joy_held+1
    and.b #$10
    ora.w !demo_buttons
    sta.w !joy_held+1
    and.w !joy_pressed+1
    sta.w !joy_pressed+1
    dec.w !demo_input_frames
    bne .controls
    rep #$10
    ldx.w !demo_input_at
    inc.w !demo_input_at
    inc.w !demo_input_at
    lda.l !demo_script,x
    and.b #$EF
    sta.w !demo_buttons
    lda.l !demo_script+1,x
    sta.w !demo_input_frames
    sep #$10
.controls:
    jsr.w ControlsRead
    ply
    plx
    rts

; @routine 81:BDB1 InputRead
; WHAT IT DOES: Takes this frame's buttons and turns them into the game's
; actions: from the pads, or in the demo from its script (only the real
; pad's START kept, so a player can end the demo), or from the pads while
; the recorder writes them down.
; HOW: Play mode 2 (the demo): reads the pads, clears every held and
; pressed byte, makes pad 1's high byte the real START plus the script's
; buttons and its pressed byte the ones not held last frame, and steps the
; script when its frame count runs out. Otherwise reads the pads, and in
; mode 1 calls InputRecord. Both end in ControlsRead.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $4212-$421B are the CPU's registers; called by JSR.
; Exit: as ControlsRead; X and Y unchanged; M 8-bit, X 8-bit.
%at($81BDB1)
InputRead:
    lda.w !play_mode
    cmp.b #$02
    beq InputReadDemo
    phx
    phy
    jsr.w JoypadRead
    lda.w !play_mode
    dec a
    bne .controls
    jsr.w InputRecord
.controls:
    jsr.w ControlsRead
    ply
    plx
    rts

; @routine 81:BE5A InputRecord
; WHAT IT DOES: The development recorder: writes pad 1's buttons into
; cartridge RAM as runs (the buttons and how many frames they were held),
; until its buffer is full; then holds the game until pad 2's DOWN and
; starts over. This board has no cartridge RAM: the writes go nowhere and
; each read is the bus's last byte ($70).
; HOW: By its step, through its table: step 0 clears the buffer's first
; 256 bytes, its offset and pad 1's held buttons, and goes on to step 1;
; step 1 compares the buttons with the run at the offset: the same, the
; run's frame count goes up; else, or when the count would wrap past $FF,
; it starts the next run, 2 bytes on (its buttons, its count up by one),
; and at offset $FE keeps the offset, waits for pad 2's DOWN (JoypadRead
; in a loop) and sets the step back to 0. The wrap ($81:BE97) needs a
; read of $FF at $70:0001 up: on this board the read is $70, so it never
; runs, and no script covers it.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM;
; called by JSR (InputRead, play mode 1).
; Exit: M 8-bit, X 8-bit; A, X, Y, flags as its last step left them.
%at($81BE5A)
InputRecord:
    lda.w !record_step
    asl a
    tax
    jmp.w (.steps,x)
.steps:
    dw .start, .record
.start:
    ldx.b #$00
    txa
.clear:
    sta.l !record_buf,x
    dex
    bne .clear
    stz.w !record_pos
    stz.w !record_pos+1
    stz.w !joy_held
    stz.w !joy_held+1
    inc.w !record_step
.record:
    ldx.w !record_pos
    lda.w !joy_held+1
    cmp.l !record_buf,x
    bne .next
    tay
    lda.l !record_buf+1,x
    inc a
    sta.l !record_buf+1,x
    bne .done
    dec a
    sta.l !record_buf+1,x
    tya
.next:
    sta.l !record_buf+2,x
    lda.l !record_buf+3,x
    inc a
    sta.l !record_buf+3,x
    inx
    inx
    cpx.b #$FE
    beq .full
    stx.w !record_pos
.done:
    rts
.full:
    stx.w !record_end
.wait:
    jsr.w JoypadRead
    lda.w !joy_held+3
    bit.b #$04
    beq .wait
    stz.w !record_step
    rts
