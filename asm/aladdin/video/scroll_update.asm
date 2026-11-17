; scroll_update.asm: the play field's scroll each frame.
;
; The level picks one of a table of scroll handlers (SCROLL_KIND): the
; camera that follows the player inside a window, scrolls that run on
; their own at a set speed, or none. Every handler but the last two ends
; in the same tail: the scroll kept inside the level, empty blocks
; stopping it (camera.asm), then the backdrop's scroll (parallax.asm).
; C: src/aladdin/video/scroll_update.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/wram_player.inc"
incsrc "../include/actor.inc"

arch 65816

!su_scroll_x = !layer0+!LAYER_SCROLL_X
!su_scroll_y = !layer0+!LAYER_SCROLL_Y
!scroll_kind = $083D             ; the level's scroll handler, times 2
!v_1a09 = $1A09                  ; added to !v_1a08 for the camera's window (word)
!camera_aim_x = $03F1            ; added to the scroll X for the point the camera keeps the player from (word)
!camera_push = $1A0A             ; how the camera moved across this frame: 0 not, 1 right, 2 left (word)
!scroll_x_held = $1A10           ; nonzero: no follow across and no stop at empty blocks across (word)
!scroll_y_held = $1A12           ; the same up and down (word)
!scroll_x_min = $1A40            ; the scroll X's least (word)
!scroll_x_max = $1A42            ; and its most (word)
!v_1427 = $1427                  ; read here only for its low 4 bits (word)
!target_x = !scratch             ; the camera's point: x (word)
!target_y = !scratch+2           ; and y (word)
; In bank $80 (DB): the camera's windows by !v_1a08 + !v_1a09: x bias, x
; width, y bias, y height, y offset of the camera's point (words); the two
; set speeds of handlers 10 and 14 (24-bit, from the fraction byte).
!camera_windows = $80C672
!scroll_speeds = $80C6EA

; @routine 81:C5A4 ScrollUpdate
; WHAT IT DOES: Moves the play field's scroll for this frame the way the
; level asks: the camera that follows the player (0), a scroll that runs
; on right, slowly, keeping the player on screen (2), at 3 a frame (4),
; fast (6), or down and left (8), at a set speed with the camera
; following up and down (10, 14), or none (12); then keeps it in the
; level and sets the backdrop's.
; HOW: JMP through its table by SCROLL_KIND. 0: the scroll before the
; move kept; with V_1A08 not negative, the window V_1A08 + V_1A09 of
; CAMERA_WINDOWS: the player's offset from the camera's point (the
; scroll plus CAMERA_AIM_X, and the window's y offset), plus the
; window's bias, past its width or height moves the scroll by the
; excess, at most 3 across and 6 up or down, unless held. 2: the scroll
; X plus $A0 (24 bits); with V_1427's low 4 bits 0, the player's x at
; most the scroll X + $F0. 4: plus 3 a frame. 6: plus $300. 8: less 8 across,
; plus 8 down. 10 and 14: plus SCROLL_SPEEDS 0 or 1; up and down as 0.
; Then (not 12, nor 0 with V_1A08 negative): the scroll X not below 0,
; at most SCROLL_LIMIT_X, else within SCROLL_X_MIN and SCROLL_X_MAX; the
; scroll Y not below 0 nor past SCROLL_LIMIT_Y; CameraStopAcross and
; CameraStopUpDown unless held; then ParallaxUpdate. Kind 16's entry is a
; bare RTS no script runs.
; Entry: M 8-bit, X 8-bit; DB = $80. Exit: as ParallaxUpdate leaves the
; registers; for kind 0 with V_1A08 negative, M 8-bit, X 8-bit, A = the
; scroll Y, X = V_1A08, N set.
%at($81C5A4)
ScrollUpdate:
    ldx.w !scroll_kind
    jmp.w (.kinds,x)
.kinds:
    dw .follow, .slow, .three, .fast, .diagonal, .speed0, .none, .speed1, .bare
.bare:
    rts                             ; kind 16: no script runs it
.none:
    rep.b #$30
    jmp.w ParallaxUpdate
.slow:
    rep.b #$30
    clc
    lda.w !su_scroll_x-1
    adc.w #$00A0
    sta.w !su_scroll_x-1
    lda.w !su_scroll_x+1
    adc.w #0
    sta.w !su_scroll_x+1
    lda.w !v_1427
    and.w #$000F
    bne .slowed
    lda.w !su_scroll_x              ; the player kept on screen
    clc
    adc.w #$00F0
    cmp.w !player+!actor_x+1
    bcs .slowed
    sta.w !player+!actor_x+1
.slowed:
    jmp.w .tail
.fast:
    rep.b #$30
    clc
    lda.w !su_scroll_x-1
    adc.w #$0300
    sta.w !su_scroll_x-1
    lda.w !su_scroll_x+1
    adc.w #0
    sta.w !su_scroll_x+1
    jmp.w .tail
.speed1:
    ldx.b #4
    bra .speed
.speed0:
    ldx.b #0
.speed:
    rep.b #$30
    clc
    lda.w !su_scroll_x-1
    adc.w !scroll_speeds,x
    sta.w !su_scroll_x-1
    lda.w !su_scroll_x+1
    adc.w !scroll_speeds+2,x
    sta.w !su_scroll_x+1
    stz.w !camera_push
    ldx.w #0
    clc
    lda.w !su_scroll_y
    adc.w !camera_windows+8,x
    sta.w !target_y
    jmp.w .down
.three:
    clc                             ; with 8-bit A: only the flags
    lda.w !su_scroll_x-1
    adc.b #0
    sta.w !su_scroll_x-1
    rep.b #$30
    lda.w !su_scroll_x
    sta.w !scroll_x_before
    lda.w !su_scroll_x
    adc.w #3
    sta.w !su_scroll_x
    jmp.w .tail
.diagonal:
    rep.b #$30
    lda.w !su_scroll_x
    sta.w !scroll_x_before
    lda.w !su_scroll_y
    sta.w !scroll_y_before
    sec
    lda.w !su_scroll_x
    sbc.w #8
    sta.w !su_scroll_x
    clc
    lda.w !su_scroll_y
    adc.w #8
    sta.w !su_scroll_y
    jmp.w .tail
.held:
    sep.b #$20
    rts
.follow:
    rep.b #$20
    lda.w !su_scroll_x
    sta.w !scroll_x_before
    lda.w !su_scroll_y
    sta.w !scroll_y_before
    ldx.w !v_1a08
    bmi .held
    txa                             ; the window
    clc
    adc.w !v_1a09
    tax
    stz.w !camera_push
    rep.b #$30
    clc
    lda.w !su_scroll_x
    adc.w !camera_aim_x
    sta.w !target_x
    clc
    lda.w !su_scroll_y
    adc.w !camera_windows+8,x
    sta.w !target_y
    lda.w !scroll_x_held
    bne .down
    sec                             ; across: the player against the window
    lda.w !player+!actor_x+1
    sbc.w !target_x
    clc
    adc.w !camera_windows,x
    cmp.w !camera_windows+2,x
    bcc .down
    inc.w !camera_push
    tay
    bpl .right
    inc.w !camera_push
    cmp.w #-3
    bcs .across
    lda.w #-3
    bra .across
.right:
    sec
    sbc.w !camera_windows+2,x
    cmp.w #4
    bcc .across
    lda.w #3
.across:
    clc
    adc.w !su_scroll_x
    sta.w !su_scroll_x
.down:
    lda.w !scroll_y_held
    bne .tail
    sec                             ; up and down
    lda.w !player+!actor_y+1
    sbc.w !target_y
    clc
    adc.w !camera_windows+4,x
    cmp.w !camera_windows+6,x
    bcc .tail
    tay
    bpl .lower
    cmp.w #-6
    bcs .vertical
    lda.w #-6
    bra .vertical
.lower:
    sec
    sbc.w !camera_windows+6,x
    cmp.w #7
    bcc .vertical
    lda.w #6
.vertical:
    clc
    adc.w !su_scroll_y
    sta.w !su_scroll_y
.tail:
    lda.w !su_scroll_x              ; kept in the level
    bpl .not_left
    stz.w !su_scroll_x
    bra .x_kept
.not_left:
    cmp.w !scroll_limit_x
    bcc .not_right
    lda.w !scroll_limit_x
    sta.w !su_scroll_x
    bra .x_kept
.not_right:
    cmp.w !scroll_x_min
    bcs .not_min
    lda.w !scroll_x_min
    sta.w !su_scroll_x
    bra .x_kept
.not_min:
    cmp.w !scroll_x_max
    bcc .x_kept
    lda.w !scroll_x_max
    sta.w !su_scroll_x
.x_kept:
    lda.w !su_scroll_y
    bpl .not_up
    stz.w !su_scroll_y
    bra .y_kept
.not_up:
    cmp.w !scroll_limit_y
    bcc .y_kept
    lda.w !scroll_limit_y
    sta.w !su_scroll_y
.y_kept:
    lda.w !scroll_x_held
    bne .x_stopped
    jsr.w CameraStopAcross
.x_stopped:
    lda.w !scroll_y_held
    bne ParallaxUpdate
    jsr.w CameraStopUpDown
                                    ; on into ParallaxUpdate (parallax.asm)
