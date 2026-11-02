; rise_steps.asm: actor kinds $88 (a thing that flies up a course and back
; down each time it is sent) and $96 (one that shows itself after its
; waits). Drafted by cdraft.py; named and described by hand.
; C: src/aladdin/actor/rise_steps.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!V_1B1A = $1B1A  ; nonzero: send the riser
!V_1B1B = $1B1B  ; the course to send it on
!RISE_UP = $D7BE  ; by course, its motion preset up (in the bank DB names)
!RISE_DOWN = $D7C1  ; by course, its motion preset down
!RISE_FLOOR = $40  ; word: the y pixels it rests at ($0100)
!RISE_COURSE = $42  ; the course it was sent on
!STEP_WAITS = $D8FC  ; by $07, the frames its first wait lasts (in the bank DB names)
!STEP_WAIT = $40  ; the frames of its second wait

; @routine 84:8E6A RiserActor
; @actor
; WHAT IT DOES: Actor kind $88: rests at y 256 until it is sent (V_1B1A),
; then flies up its course (V_1B1B) until it starts down, and comes back
; down to y 256; its heights are kept in the y history (ActorYHistoryPush)
; all the while. Sent again as long as V_1B1A stays set.
; HOW: $09 = $80; resting: y and $40 = $0100, BRK, until V_1B1A; the
; course to $42, ActorMotionPreset by RISE_UP; each frame ActorFlyAgainst
; and ActorYHistoryPush while y is at or below $40; then ActorMotionPreset
; by RISE_DOWN, each frame ActorFly and ActorYHistoryPush until y is back
; at $40 or lower; then V_1B1A again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $88 in state $0C (through the stub at
; $83:9319). Exit: never returns.
%at($848E6A)
RiserActor:
    lda.b #$80
    sta.b !actor_09
.l8E6E:
    rep.b #$20
    lda.w #$0100
    sta.b !actor_y+1
    sta.b !actor_40
    sep.b #$20
    brk.b #$00
.l8E7B:
    lda.w !V_1B1A
    beq .l8E6E
    ldx.w !V_1B1B
    stx.b !actor_42
    ldy.w !RISE_UP,x
    jsl.l ActorMotionPreset
.l8E8C:
    brk.b #$00
    jsl.l ActorFlyAgainst
    jsr.w ActorYHistoryPush
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !actor_40
    sep.b #$20
    bcs .l8E8C
    ldx.b !actor_42
    ldy.w !RISE_DOWN,x
    jsl.l ActorMotionPreset
.l8EA8:
    brk.b #$00
    jsl.l ActorFly
    jsr.w ActorYHistoryPush
    rep.b #$20
    lda.b !actor_y+1
    cmp.b !actor_40
    sep.b #$20
    bcc .l8EA8
    bra .l8E7B


; @routine 84:9EF5 SteppedShowActor
; @actor
; WHAT IT DOES: Actor kind $96. With $07 bit 4 clear: waits the frames
; STEP_WAITS gives for $07, then as long again, then shows itself (sound
; $44, animation twice $07) and stays. Bit 4 set: shown at once (animation
; 1) at its priority bits set, and stays.
; HOW: Bits 1-3 of $2C cleared. Clear: $09 bit 4 and $2C bits 2-3 set;
; STEP_WAITS by $07 to $40, COP with it, COP with $40; $08 bits 0-1 and
; $09 bit 7 set; SoundQueueAdd $44; AnimStart $07 * 2; COP for good. Set:
; $2C bits 1-3, $08 bit 1, $09 bits $B0 set; AnimStart 1; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $96 in state $0C (through the stub at
; $83:93AC). Exit: never returns.
%at($849EF5)
SteppedShowActor:
    lda.b #$0E
    trb.b !actor_2c
    lda.b !actor_07
    bit.b #$10
    beq .l9F02
    jmp.w .l9F32
.l9F02:
    lda.b #$10
    tsb.b !actor_09
    lda.b #$0C
    tsb.b !actor_2c
    ldx.b !actor_07
    lda.w !STEP_WAITS,x
    sta.b !actor_40
    cop.b #$00
    lda.b !actor_40
    cop.b #$00
    lda.b !actor_08
    ora.b #$03
    sta.b !actor_08
    lda.b #$80
    tsb.b !actor_09
    lda.b #$44
    jsl.l SoundQueueAdd
    lda.b !actor_07
    asl a
    jsl.l AnimStart
.l9F2E:
    cop.b #$00
    bra .l9F2E
.l9F32:
    lda.b #$0E
    tsb.b !actor_2c
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b #$B0
    tsb.b !actor_09
    lda.b #$01
    jsl.l AnimStart
.l9F46:
    brk.b #$00
    bra .l9F46
