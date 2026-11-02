; kind_brief.asm: actors that only play an animation for a few frames and
; end.
; C: src/aladdin/actor/kind_brief.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

; @routine 81:EB90 ActorBriefAnim8
; @actor
; WHAT IT DOES: An actor (kind 17) that plays animation 8 for 8 frames,
; then ends.
; HOW: Sets bit 4 of its flags at $09 and $2C to 4, starts animation 8
; (AnimStart), then each frame yields and steps the animation
; (AnimStep), 8 times; then ActorEnd.
; Entry: M 8-bit, X 8-bit; D = the actor's record; S the actor loop's;
; reached by the actor loop's jump. Exit: never returns: yields at each
; BRK, and ends in ActorEnd.
%at($81EB90)
ActorBriefAnim8:
    lda.b !actor_09
    ora.b #$10
    sta.b !actor_09
    lda.b #8
    jsl.l AnimStart
    lda.b #4
    sta.b !actor_2c
    lda.b #8
    sta.b !actor_40
.frame:
    brk.b #0
    jsl.l AnimStep
    dec.b !actor_40
    bne .frame
    jml.l ActorEnd

; @routine 81:EBB2 ActorBriefAnimB
; @actor
; WHAT IT DOES: Actor kind $13: plays sound 8 and animation $0B for 6
; frames, then ends.
; HOW: SoundQueueAdd 8; bit 0 of its $08 set; $09 = $10; AnimStart $0B;
; $2C = 4; then each frame yields and steps the animation (AnimStep), 6
; times; then ActorEnd.
; Entry: M 8-bit, X 8-bit; D = the actor's record; S the actor loop's;
; reached by the actor loop's jump for kind $13 in state $0C (the table
; at $83:8CAB, through the stub at $83:9222). Exit: never returns: yields
; at each BRK, and ends in ActorEnd.
%at($81EBB2)
ActorBriefAnimB:
    lda.b #8
    jsl.l SoundQueueAdd
    lda.b !actor_08
    ora.b #$01
    sta.b !actor_08
    lda.b #$10
    sta.b !actor_09
    lda.b #$0B
    jsl.l AnimStart
    lda.b #4
    sta.b !actor_2c
    lda.b #6
    sta.b !actor_40
.frame:
    brk.b #0
    jsl.l AnimStep
    dec.b !actor_40
    bne .frame
    jml.l ActorEnd
