; grabber.asm: actor kinds $46 and $62, a thing that catches Aladdin,
; holds and shakes him, lets him fall, breaks into pieces and falls
; itself, then bounces him. Drafted by cdraft.py; named and described by
; hand.
; C: src/aladdin/actor/grabber.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!GRAB_FRAMES = $40               ; frames, or shake steps, left
!GRAB_KIND = $42                 ; 0: kind $46; 1: kind $62
!GRAB_DROPPED = $43              ; nonzero: straight to the shake, the player not moved
!GRAB_HELD = $44                 ; nonzero: it holds the player
!GRAB_SHAKE_AT = $D2CA           ; in bank $80: by step (1-8), the offset in GRAB_SHAKES
!GRAB_SHAKES = $D2D3             ; in bank $80: x steps of the shake (words)
!GRAB_V_141B = $141B             ; nonzero: it does not catch
!GRAB_V_145A = $145A             ; negative: it does not catch
!GRAB_V_1427 = $1427             ; zero: it lets go
!GRAB_HOLD_CODE = $D6F8          ; where it sends the player's code while it holds him
!GRAB_FALL_CODE = $CDD2          ; PlayerFall: where it sends it as it lets go
!GRAB_ZONE = $6C                 ; ActorZoneHasTestPointLong's zone
!GRAB_LAND_Y = $A8               ; kind $62 falls on from these y pixels down

; @routine 83:C761 Kind46Start
; @actor
; WHAT IT DOES: Actor kind $46: GrabberStart with GRAB_KIND 0.
; HOW: GRAB_KIND cleared, on into GrabberStart.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $46 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83C761)
Kind46Start:
    stz.b !GRAB_KIND

; @routine 83:C763 GrabberStart
; @actor
; WHAT IT DOES: The code of actor kinds $46 and $62 (GRAB_KIND: 0 for
; $46, 1 for $62), a thing that catches Aladdin: it waits until he comes
; into its zone, holds him (his code sent to $D6F8, his place kept at
; it) for 48 frames, shakes for 8 steps of 2 frames, lets him fall (his
; code sent to PlayerFall) if he is still held, breaks into two pieces
; and falls; when it lands it breaks into three more, settles on the
; tile below, and from then on bounces him when he comes down on it,
; with a sound. Kind $62's stays hittable, makes no landing sound, and
; after landing keeps falling and animating; with $07 bit 7 set it flies
; off at once (ActorFlyOffEnd).
; HOW: GRAB_DROPPED and GRAB_HELD 0; ACTOR_08 bit 1 and ACTOR_09 bits
; $90 set (TSB). The facing ($15) from $07 bit 0; AnimStart 0. Each frame
; (BRK first) until caught: GRAB_DROPPED nonzero goes straight to the
; shake; on when V_141B is 0, V_145A not negative and
; ActorZoneHasTestPointLong (Y = $6C) says C clear. Caught: its $14 = 1
; when the player's $15 is 0, else 0; the player's resume address =
; $D6F8; GRAB_HELD up; Point145DToActor; BRK; PlayerToActor; 48 frames
; (BRK each), GRAB_HELD cleared when V_1427 is 0. The shake, GRAB_FRAMES
; 8 down to 1: its x pixels plus the word of GRAB_SHAKES at the offset
; GRAB_SHAKE_AT gives (16-bit, C clear); unless GRAB_DROPPED: V_1427 0
; clears GRAB_HELD, else with GRAB_HELD the player's x = the same; COP 2.
; Then with V_1427 and GRAB_HELD the player's resume address = $CDD2
; (PlayerFall). Its $15 = its $14; kind $46: ACTOR_09 bit 7 cleared;
; AnimStart 2; ActorMotionReset with gravity $38; $07 = $81, ACTOR_SPAWN_AT
; $40: ActorSpawnSameLook with its hit box ($06), each child counted in
; KIND_COUNT by $06, $07 down, while $07 is negative. Each frame (BRK
; first): AnimStep, ActorFall, (kind $62: on at y pixels $A8 and below)
; ActorFallTileTest until Z (the tile below of kind 2). Landed: sound $18
; for kind $46 when ACTOR_09 bit 6 is set. $07 = $84, ACTOR_SPAWN_AT $42:
; children as before, its $15 flipped while $07 is at least $84, $07 down
; until $81. Its y pixels set to (y + 10) AND $FFF0 less 9. Kind $62:
; each frame (BRK first) AnimStep, ActorFall, for good. Kind $46:
; AnimStart 1, ACTOR_08 low bits 2; each frame (BRK first)
; ActorBouncesPlayer, and on a bounce sound $12. No script runs kind
; $62, GRAB_DROPPED set or the hold's GRAB_HELD clear: written from their
; bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the shake
; tables' bank); GRAB_KIND set; reached from Kind46Start (falling in) or
; Kind62Start (a jump). Exit: never returns.
GrabberStart:
    stz.b !GRAB_DROPPED
    stz.b !GRAB_HELD
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    bpl .start
    jmp.w ActorFlyOffEnd
.start:
    lda.b !actor_07
    and.b #$01
    sta.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.wait:
    brk.b #$00
    lda.b !GRAB_DROPPED
    beq .test
    bra .shake
.test:
    lda.w !GRAB_V_141B
    bne .wait
    lda.w !GRAB_V_145A
    bmi .wait
    ldy.b #!GRAB_ZONE
    jsl.l ActorZoneHasTestPointLong
    bcs .wait
    stz.b !actor_facing
    lda.w !player+!actor_flip
    bne .faced
    inc.b !actor_facing
.faced:
    lda.b #!GRAB_HOLD_CODE
    sta.w !player+!actor_03
    lda.b #!GRAB_HOLD_CODE>>8
    sta.w !player+!actor_03+1
    inc.b !GRAB_HELD
    jsr.w Point145DToActor
    brk.b #$00
    jsr.w PlayerToActor
    lda.b #$30
    sta.b !GRAB_FRAMES
.hold:
    lda.w !GRAB_V_1427
    bne .held
    stz.b !GRAB_HELD
.held:
    brk.b #$00
    dec.b !GRAB_FRAMES
    bne .hold
.shake:
    lda.b #$08
    sta.b !GRAB_FRAMES
.step:
    ldx.b !GRAB_FRAMES
    ldy.w !GRAB_SHAKE_AT,x
    rep.b #$21
    lda.b !actor_x+1
    adc.w !GRAB_SHAKES,y
    sta.b !actor_x+1
    ldx.b !GRAB_DROPPED
    bne .stepped
    ldx.w !GRAB_V_1427
    beq .letGo
    ldx.b !GRAB_HELD
    beq .stepped
    sta.w !player+!actor_x+1
    bra .stepped
.letGo:
    ldx.b #$00
    stx.b !GRAB_HELD
.stepped:
    sep.b #$20
    lda.b #$02
    cop.b #$00
    dec.b !GRAB_FRAMES
    bne .step
    ldx.w !GRAB_V_1427
    beq .break
    ldx.b !GRAB_HELD
    beq .break
    lda.b #!GRAB_FALL_CODE
    sta.w !player+!actor_03
    lda.b #!GRAB_FALL_CODE>>8
    sta.w !player+!actor_03+1
.break:
    lda.b !actor_facing
    sta.b !actor_flip
    lda.b !GRAB_KIND
    bne .hittable
    lda.b #$80
    trb.b !actor_09
.hittable:
    lda.b #$02
    jsl.l AnimStart
    lda.b #$38
    jsl.l ActorMotionReset
    lda.b #$81
    sta.b !actor_07
    lda.b #$40
    sta.b !actor_spawn_at
.spawn:
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .fall
    ldx.b !actor_06
    lda.l !kind_count,x
    inc a
    sta.l !kind_count,x
    dec.b !actor_07
    bmi .spawn
.fall:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFall
    lda.b !GRAB_KIND
    beq .tile
    lda.b !actor_y+1
    cmp.b #!GRAB_LAND_Y
    bcs .landed
.tile:
    jsr.w ActorFallTileTest
    bne .fall
    lda.b !actor_09
    and.b #$40
    beq .landed
    lda.b !GRAB_KIND
    bne .landed
    lda.b #$18
    jsl.l SoundQueueAdd
.landed:
    lda.b #$84
    sta.b !actor_07
    lda.b #$42
    sta.b !actor_spawn_at
.pieces:
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .settle
    ldx.b !actor_06
    lda.l !kind_count,x
    inc a
    sta.l !kind_count,x
    lda.b !actor_07
    cmp.b #$84
    bcc .same
    lda.b !actor_flip
    eor.b #$01
    sta.b !actor_flip
.same:
    dec.b !actor_07
    lda.b !actor_07
    cmp.b #$81
    bne .pieces
.settle:
    rep.b #$21
    lda.b !actor_y+1
    adc.w #$000A
    and.w #$FFF0
    sec
    sbc.w #$0009
    sta.b !actor_y+1
    sep.b #$20
    lda.b !GRAB_KIND
    bne .fallOn
    lda.b #$01
    jsl.l AnimStart
    lda.b !actor_08
    and.b #$FC
    ora.b #$02
    sta.b !actor_08
.bounce:
    brk.b #$00
    jsr.w ActorBouncesPlayer
    beq .bounce
    lda.b #$12
    jsl.l SoundQueueAdd
    bra .bounce
.fallOn:
    brk.b #$00
    jsl.l AnimStep
    jsl.l ActorFall
    bra .fallOn

; @routine 83:C95F Kind62Start
; @actor
; WHAT IT DOES: Actor kind $62: GrabberStart with GRAB_KIND 1.
; HOW: GRAB_KIND = 1, JMP GrabberStart. No script runs it: written from
; its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $62 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83C95F)
Kind62Start:
    lda.b #$01
    sta.b !GRAB_KIND
    jmp.w GrabberStart
