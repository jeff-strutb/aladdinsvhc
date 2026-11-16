; cue_task_actors.asm: actor kinds $24 and $9C, which wait for a cue and
; then start a task (and show a pose, a sound, a palette cycle).
; C: src/aladdin/actor/cue_task_actors.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!CUE_D7 = $00D7                 ; kind $24 starts its task once this reaches $18
!CUE_B7 = $00B7                 ; then shows its second pose once this is 0
!V_0399 = $0399                 ; set to $10 as kind $24 shows its second pose
!CUE_CF = $00CF                 ; kind $9C starts its task once this is 0
!KIND24_TASK = $80B263          ; the task kind $24 starts: slot 3, parameter 1
!KIND9C_TASK = $81F344          ; the task kind $9C starts: slot 4, parameter 5

; @routine 84:8000 Kind24Start
; @actor
; WHAT IT DOES: Actor kind $24: shown facing right in its first pose
; until CUE_D7 reaches $18; then it starts its task, and once CUE_B7 is 0
; shows its second pose with sound $1C, and stays.
; HOW: actor_08 bit 1 and actor_09 bits $B0 set; facing and flip 0;
; AnimStart 0. Each frame (BRK first) until CUE_D7 is $18 or more;
; TaskCreate (slot 3, parameter 1, KIND24_TASK); each frame until CUE_B7
; is 0; V_0399 = $10, AnimStart 1, SoundQueueAdd $1C; then BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $24
; in state $0C (the table at $83:8CAB, through the stub at $83:9233).
; Exit: never returns.
%at($848000)
Kind24Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$B0
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.cue:
    brk.b #$00
    lda.w !CUE_D7
    cmp.b #$18
    bcc .cue
    jsl.l TaskCreate
    db 3*24, 1                  ; slot 3, parameter 1
    dl !KIND24_TASK
.second:
    brk.b #$00
    lda.w !CUE_B7
    bne .second
    lda.b #$10
    sta.w !V_0399
    lda.b #$01
    jsl.l AnimStart
    lda.b #$1C
    jsl.l SoundQueueAdd
.stay:
    brk.b #$00
    bra .stay

; @routine 84:A7E5 Kind9CStart
; @actor
; WHAT IT DOES: Actor kind $9C: after 16 frames, waits until CUE_CF is
; 0, then 128 frames more; it starts its task, palette cycle $24 and its
; pose, and stays.
; HOW: actor_08 bit 1 and actor_09 bit 4 set (TSB); COP $10; each frame
; (BRK first) until CUE_CF is 0; COP $80; TaskCreate (slot 4, parameter
; 5, KIND9C_TASK); PaletteCycleStartLong with X = 0, A = $24; actor_09
; bit 7 set; AnimStart 0; then BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $9C
; in state $0C (the table at $83:8DAB, through the stub at $83:93EE).
; Exit: never returns.
%at($84A7E5)
Kind9CStart:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$10
    tsb.b !actor_09
    lda.b #$10
    cop.b #$00
.cue:
    brk.b #$00
    lda.w !CUE_CF
    bne .cue
    lda.b #$80
    cop.b #$00
    jsl.l TaskCreate
    db 4*24, 5                  ; slot 4, parameter 5
    dl !KIND9C_TASK
    ldx.b #$00
    lda.b #$24
    jsl.l PaletteCycleStartLong
    lda.b #$80
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
.stay:
    brk.b #$00
    bra .stay
