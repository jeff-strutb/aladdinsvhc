; iris_close_task.asm: a task that closes the screen down to a circle
; round a point, by the windows: started by actor kind $24 and by the scene
; at $80:B04E.
; C: src/aladdin/video/iris_close_task.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

!IRIS_WHICH = $00BD             ; which iris: an index into the tables below
!IRIS_RADIUS = $00BE            ; the circle's radius, one down a frame to 0
!IRIS_X = $00BF                 ; its centre on the screen (words)
!IRIS_Y = $00C1
!IRIS_RADII = $DB87             ; by IRIS_WHICH: the starting radius (bytes)
!IRIS_CENTRES = $DB89           ; by IRIS_WHICH * 4: x, then y (words)
!WINDOW_03A6 = $03A6            ; the windows' settings, $03A6-$03AE
!SHAPE_BANK = $7F               ; where CircleWipeStep draws the window table ($7F:5000)
!HDMA_BANK = $7E                ; where the HDMA reads it ($7E:F500)

; @routine 80:B263 IrisCloseTask
; WHAT IT DOES: Closes the screen down to nothing round a point: starts
; the window effect, masks layers 1 and 2 by the windows, and draws a
; circle round the point of the iris IRIS_WHICH names whose radius shrinks
; by one each frame from its start until it is 0, then ends.
; HOW: TaskCreate (slot 6, parameter 4, WindowEffectTask); TMW and TSW 3,
; $03A6 0, $03A7 $FF, $03AA $33, $03AB and WOBJSEL 0. IRIS_RADIUS =
; IRIS_RADII by IRIS_WHICH; IRIS_X and IRIS_Y from IRIS_CENTRES by
; IRIS_WHICH * 4. Each frame: CircleWipeStep with A = IRIS_RADIUS, X =
; IRIS_X, Y = IRIS_Y (16-bit); MVN of the $200-byte window table from
; $7F:5000 to $7E:F500, DB kept; M and X 8-bit; TaskWait 1; IRIS_RADIUS
; down by one; at 0, TaskExit.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM and
; whose $DB87 holds the tables (bank $80); a task's entry (TaskCreate slot
; 3 at $84:8025, and at $80:B04E). Exit: never returns (TaskExit).
%at($80B263)
IrisCloseTask:
    jsl.l TaskCreate
    db 6*24, 4                  ; slot 6, parameter 4
    dl WindowEffectTask
    lda.b #$03
    sta.w !tmw_shadow
    sta.w !tsw_shadow
    stz.w !WINDOW_03A6
    lda.b #$FF
    sta.w !WINDOW_03A6+1
    lda.b #$33
    sta.w !WINDOW_03A6+4
    stz.w !WINDOW_03A6+5
    stz.w !wobjsel_shadow
    ldx.w !IRIS_WHICH
    lda.w !IRIS_RADII,x
    sta.w !IRIS_RADIUS
    rep.b #$20
    txa
    asl a
    asl a
    tax
    lda.w !IRIS_CENTRES,x
    sta.w !IRIS_X
    lda.w !IRIS_CENTRES+2,x
    sta.w !IRIS_Y
.step:
    rep.b #$30
    lda.w !IRIS_RADIUS
    and.w #$00FF
    ldx.w !IRIS_X
    ldy.w !IRIS_Y
    jsl.l CircleWipeStep
    phb
    ldx.w #$5000
    ldy.w #$F500
    lda.w #$01FF
    mvn !HDMA_BANK,!SHAPE_BANK
    plb
    sep.b #$30
    lda.b #$01
    jsl.l TaskWait
    dec.w !IRIS_RADIUS
    bne .step
    jml.l TaskExit
