; kind2a.asm: actor kind $2A's second state, downed: knocked away, a short
; flight up and a fall. Its start ($83:A1E5) is not filed yet; its wait
; is Kind2AWait (actor_waits_small.asm).
; C: src/aladdin/actor/kind2a.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND2A_DOWNS = $47             ; counted each time it is downed
!KIND2A_HITS_2 = $48            ; counted when downed by a shot (Kind2AWait)
!KIND2A_RIGHT_2 = $4C           ; counted when downed facing right (Kind2AWait)
!KIND2A_FRAMES = $43            ; frames left of its flight
!COPY_TEST = $701331            ; save RAM, which the retail board lacks: a write there reads back on a copier

; @routine 83:A37B Kind2ASecond
; @actor
; WHAT IT DOES: Actor kind $2A in its second state (downed): it is knocked
; away from Aladdin, flies up a little, then falls for good (its wait
; ends it out of range). On a cartridge whose save RAM can be written
; (a copy, not the retail board) each frame of the fall also pushes the
; player 16 pixels right.
; HOW: AnimStart 4; ActorMotionPreset with Y = $28; ACTOR_09 bit 7 set;
; KIND2A_DOWNS up. Facing negative (no script has it): facing and flip =
; its bit 0, speed x high byte up, on at the fly. Else KIND2A_RIGHT zero:
; SoundQueueAdd $0F, y plus 9. KIND2A_HITS zero: facing away from the
; player's flip byte, COP 9. ActorMotionPreset with Y = $28; 16 frames
; (BRK) of ActorFly; AnimStart 3; ActorMotionReset with A = $10; then each
; frame (BRK) AnimStep, ActorAccelerateY unless the speed y high byte is
; 3, ActorStepY, and the byte at COPY_TEST counted up: read back equal
; (save RAM), the player's x plus 16.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $2A in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($83A37B)
Kind2ASecond:
    lda.b #$04
    jsl.l AnimStart
    ldy.b #$28
    jsl.l ActorMotionPreset
    lda.b #$80
    tsb.b !actor_09
    inc.b !KIND2A_DOWNS
    lda.b !actor_facing
    bpl .right
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    inc.b !actor_speed_x+1
    bra .fly
.right:
    lda.b !KIND2A_RIGHT_2
    bne .hits
    lda.b #$0F
    jsl.l SoundQueueAdd
    rep.b #$21
    lda.b !actor_y+1
    adc.w #$0009
    sta.b !actor_y+1
    sep.b #$20
.hits:
    lda.b !KIND2A_HITS_2
    bne .preset
    lda.w !player+!actor_flip
    sta.b !actor_flip
    eor.b #$01
    sta.b !actor_facing
    lda.b #$09
    cop.b #$00
.preset:
    ldy.b #$28
    jsl.l ActorMotionPreset
.fly:
    lda.b #$10
    sta.b !KIND2A_FRAMES
.up:
    brk.b #$00
    jsl.l ActorFly
    dec.b !KIND2A_FRAMES
    bne .up
    lda.b #$03
    jsl.l AnimStart
    lda.b #$10
    jsl.l ActorMotionReset
.fall:
    brk.b #$00
    jsl.l AnimStep
    lda.b !actor_speed_y+1
    cmp.b #$03
    beq .step
    jsl.l ActorAccelerateY
.step:
    jsl.l ActorStepY
    lda.l !COPY_TEST
    inc a
    sta.l !COPY_TEST
    cmp.l !COPY_TEST
    bne .on
    rep.b #$21
    lda.w !player+!actor_x+1
    adc.w #$0010
    sta.w !player+!actor_x+1
    sep.b #$20
.on:
    bra .fall
