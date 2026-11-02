; wram_pad.inc: work RAM of the joypads, as wram.inc names it.

includeonce

!pad1_held = $037F              ; joypad 1 buttons held (word): high byte B Y Select Start Up Down Left Right
!record_step = $0334            ; recorder step: 0 start, 1 recording
!record_pos = $0335             ; recorder offset in its buffer (word)
!record_end = $0339             ; offset where the full buffer stopped
!demo_buttons = $033D           ; demo's pad 1 high byte (START never set)
!record_buf = $700000           ; recorder buffer, cartridge RAM space (none on this board)
!demo_script = $84F700          ; demo script: pad 1 high byte, frame count per step
