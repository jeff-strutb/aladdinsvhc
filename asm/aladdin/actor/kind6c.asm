; kind6c.asm: actor kind $6C, a spot that takes Aladdin in: when he comes
; within reach it gives him the code at $D6F8 of his bank and holds him at
; its place until he is done, again and again.
; C: src/aladdin/actor/kind6c.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

!V_145A = $145A                 ; negative: the spot waits
!PLAYER_STATE_8C_COUNT = $141B  ; counted up on entering state $8C (nonzero: it waits)
!HELD_X = $145D                 ; where the player is held: x, y (words)
!HELD_Y = $145F
!V_1427 = $1427                 ; nonzero while the player is still taken
!PLAYER_TAKEN_CODE = $D6F8      ; the player's code while a spot has him
!LEVEL_10 = $0A                 ; the level where the spot must be hittable ($09 bit 6) to act

; @routine 83:E55E Kind6CStart
; @actor
; WHAT IT DOES: Actor kind $6C: each frame, unless V_145A is negative, a
; state $8C is counted, or (in level $0A) it is not hittable, it checks
; whether Aladdin is in reach; when he is, his code goes to $D6F8, the
; spot's place is kept for him, and the next frame he is put there; once
; V_1427 is 0 again it waits 16 frames and starts over.
; HOW: ACTOR_09 bit 7 set. Each frame (BRK): V_145A negative, again;
; LEVEL $0A with ACTOR_09 bit 6 clear, again; PLAYER_STATE_8C_COUNT
; nonzero, again; ActorInReachOfPlayer C set, again. The player's code
; word $D6F8; HELD_X, HELD_Y = its x, y; BRK; the player's x, y = its x,
; y; BRK until V_1427 is 0; COP 16; over.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $6C
; in state $0C (the table at $83:8CAB). Exit: never returns.
%at($83E55E)
Kind6CStart:
    lda.b #$80
    tsb.b !actor_09
.wait:
    brk.b #$00
    lda.w !V_145A
    bmi .wait
    lda.w !level
    cmp.b #!LEVEL_10
    bne .free
    lda.b !actor_09
    and.b #$40
    beq .wait
.free:
    lda.w !PLAYER_STATE_8C_COUNT
    bne .wait
    jsl.l ActorInReachOfPlayer
    bcs .wait
    lda.b #!PLAYER_TAKEN_CODE&255
    sta.w !player+!actor_03
    lda.b #!PLAYER_TAKEN_CODE>>8
    sta.w !player+!actor_03+1
    rep.b #$20
    lda.b !actor_x+1
    sta.w !HELD_X
    lda.b !actor_y+1
    sta.w !HELD_Y
    sep.b #$20
    brk.b #$00
    rep.b #$20
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    sta.w !player+!actor_y+1
    sep.b #$20
.taken:
    brk.b #$00
    ldx.w !V_1427
    bne .taken
    lda.b #$10
    cop.b #$00
    bra Kind6CStart
