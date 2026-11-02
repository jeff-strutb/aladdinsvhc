; wram.inc: work RAM, as the S-CPU code names it (the same names as the C's
; wram.h, in lower case).

includeonce
; A variable whose purpose is not yet established keeps its address as its
; name until it is.

; Sound commands to the SPC700 (audio/sound_command.asm).
!sound_seq = $0002              ; sequence number of the last command sent; the driver echoes it on APU port 2
!sound_hold = $0371             ; nonzero: no command is sent this frame
!sound_sending = $0372          ; nonzero while a command is being sent
!sound_queue_read = $03BB       ; queue index of the next command to send (0-31)
!sound_queue_write = $03BD      ; queue index where the next command is added (0-31)
!sound_queue = $03BF            ; 32-entry ring of command bytes

; Tasks (system/task.asm): 8 slots of $18 bytes from $006F, indexed by a
; slot's byte offset (0, $18, ... $A8).
!task_state = $006F             ; 0 free, 1 waiting, 4 ready, 8 running, $0C new
!task_wait = $0070              ; frames left to wait
!task_saved_s = $0071           ; S where it stopped (word)
!task_stack_top = $0073         ; top of its stack (word)
!task_param = $0075             ; the byte its creator gave it
!task_current = $0327           ; the running task's slot
!task_slot_count = $0328        ; how many slots the scheduler scans
!task_scheduler_stack = $0326   ; the scheduler's own stack

; Scratch bytes many routines use for their own working values, $000C up
; (offsets from !scratch).
!scratch = $000C

; Joypads (system/joypad.asm): pad 1's word, then pad 2's.
!joy_held = $0377               ; buttons held
!joy_pressed = $037B            ; buttons newly pressed this frame

; Shadows of PPU and CPU registers, written to them in the vertical blank.
!nmitimen_shadow = $03B7        ; NMITIMEN: interrupts and auto joypad read
!inidisp_shadow = $03B8         ; INIDISP: forced blank and brightness
!tmw_shadow = $038B             ; TMW: layers the windows mask on the main screen
!tsw_shadow = $038C             ; TSW: on the sub screen
!hdmaen_shadow = $03B6          ; HDMAEN: the HDMA channels on
!hdma_table_at = $0350          ; per HDMA channel 4-7, its table's address (word)

; The text layer (video/tilemap.asm).
!text_tilemap = $7ED800         ; its 32x32 tilemap, 2 KB
!text_tilemap_upload = $0835    ; nonzero: the NMI copies the tilemap to VRAM

; Sprites (video/oam.asm): the OAM image the NMI copies to the PPU.
!oam_buffer = $7EFA80           ; 128 sprites of 4 bytes
!oam_high = $7EFC80             ; their size and x bit 8, 2 bits each
!oam_free = $084D               ; sprites not yet used this frame (word)
!oam_next = $0852               ; the next sprite's byte offset in oam_buffer (word)
!oam_high_next = $0854          ; the next byte offset in oam_high (word)
!oam_0857 = $0857               ; cleared with them (word); purpose not yet established

; Colour math (video/colour_fade.asm): shadows the NMI writes to the PPU.
!wobjsel_shadow = $03AC         ; WOBJSEL: sprite and colour windows
!cgwsel_shadow = $03AF          ; CGWSEL: where colour math applies
!cgadsub_shadow = $03B0         ; CGADSUB: add or subtract, and on which layers
!coldata_red = $03B2            ; the fixed colour's red (low 5 bits)
!coldata_green = $03B3          ; its green
!coldata_blue = $03B4           ; its blue

; Layers: each layer's offset from the camera, a word per layer.
!layer_offset_y = $1A1C         ; vertical

; Palettes (video/palette.asm).
!palette_buffer = $7EFD00       ; CGRAM's image: 16 palettes of 16 colours
!palette_upload = $0844         ; nonzero: the NMI copies palette_buffer to CGRAM
!palette_saved = $0406          ; colours kept to fade from, laid out as palette_buffer
!palette_saved_at = $0606       ; byte offset of the first colour kept (word)

; Actor animation sets (actor/anim_sets.asm).
!anim_set_of_type = $7FF000     ; per actor type, its animation set; $FF: none
!anim_sets_used = $7FF100       ; the level's sets: (set, 0) pairs, $FF after the last

; The random number (system/random.asm).
!random = $032B                 ; word, stepped by RandomNext

; Per-level values from tables (level/level_tables.asm).
!level_035c = $035C             ; two bytes from the level's entry at $81:B933; purpose not yet established
!level_spot_count = $0821       ; how many trigger spots the level has
!level_spots = $0822            ; their list's offset from $81:EC1E (word)
!v_033f = $033F                 ; the stage number per-stage tables are indexed by
!v_0343 = $0343                 ; a countdown (word) the play loop steps down each frame

; Which BG positions follow the scroll layers (video/bg_follow.asm).
!bg1_held = $1A14               ; nonzero: BG1's position is not taken from the play field
!bg2_follows = $1A15            ; 0: BG2 follows the backdrop layer; $40: its y only; $80: the play field's x; else neither

; Per actor type, its set's place in the level's list (actor/anim_sets.asm).
!anim_set_place_of_type = $7FE000 ; per actor type, its set's place in anim_sets_used; 4: not listed
