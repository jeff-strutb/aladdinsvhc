; kind_8f_94.asm: actor kinds $8F (a figure that crosses the screen and
; back, a song playing, its x kept for other code) and $94 (a spawner that
; starts one more of its kind at each step of a count). Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/actor/kind_8f_94.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!V_00A5 = $A5  ; the walker's x pixels (word), kept for other code
!V_00AD = $AD  ; a count the scene steps; each new value, one more spark
!WALK_TIME = $40  ; frames left of its crossing
!SPARK_STEP = $40  ; the spawner: the count it last acted on
!SPARK_X = $41  ; the spawner: the next one's x pixels
!SPARK_Y = $42  ; the spawner: the next one's y pixels
!KIND8F_29 = $29  ; word: $890B
!SPARK_PLACES = $D87C  ; by count, x and y pixels (bytes), in the bank DB names

; @routine 83:FA4F Kind8FWalk
; @actor
; WHAT IT DOES: Actor kind $8F: starts off the screen's left edge, waits,
; plays song $27, then walks right across the screen for $56 frames, comes
; back left $80 lower for as long, and ends; its x is kept in V_00A5 all
; the while.
; HOW: $09 bits $90 set, $29 = $890B, x = V_00A5 = -32; COP $3A; SongPlay
; $27; COP 6; animation 0, ActorPictureSend, ActorSpeedXSet $5D. $56
; frames: ActorMoveX, its x pixels to V_00A5. Facing right then: y pixels'
; low byte = $80, facing and flip 1, again; else ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $8F in state $0C (through the stub at
; $83:9355). Exit: never returns; ends in ActorEndPlaced.
%at($83FA4F)
Kind8FWalk:
    lda.b #$00
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    rep.b #$20
    lda.w #$890B
    sta.b !KIND8F_29
    lda.w #$FFE0
    sta.b !actor_x+1
    sta.w !V_00A5
    sep.b #$20
    lda.b #$3A
    cop.b #$00
    lda.b #$27
    jsl.l SongPlay
    lda.b #$06
    cop.b #$00
    lda.b #$00
    jsl.l AnimStart
    jsl.l ActorPictureSend
    ldy.b #$5D
    jsl.l ActorSpeedXSet
.lFA86:
    lda.b #$56
    sta.b !actor_40
.lFA8A:
    brk.b #$00
    jsl.l ActorMoveX
    lda.b !actor_x+1
    sta.w !V_00A5
    lda.b !actor_x+2
    sta.w !V_00A5+1
    dec.b !actor_40
    bne .lFA8A
    lda.b !actor_facing
    bne .lFAB7
    lda.b #$80
    sta.b !actor_y+1
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    bra .lFA86


; @routine 83:FABB Kind94Spark
; @actor
; WHAT IT DOES: Actor kind $94. With $07 zero, one spark: shown $20 frames
; with animation 0, then ended. Else the spawner: each frame V_00AD has a
; new value, it starts a spark (its own kind, $07 0) at the place
; SPARK_PLACES gives for it, with its own look.
; HOW: Spark: $08 bit 0 and $09 bits $90 set, the high bytes of x and y
; cleared, animation 0, COP $20, ActorEndPlaced. Spawner: $40 cleared;
; each frame (BRK first) V_00AD compared with $40; new: kept, SPARK_PLACES
; at twice it to $41 and $42; ActorSpawn of its kind; a record (N clear):
; its $2B and $2C, x and y pixels' low bytes from $41 and $42, $07 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $94 in state $0C (through the stub at
; $83:9396). Exit: never returns; a spark ends in ActorEndPlaced.
%at($83FABB)
Kind94Spark:
    lda.b !actor_07
    bne .lFAD9
    lda.b #$01
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    stz.b !actor_x+2
    stz.b !actor_y+2
    lda.b #$00
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    jml.l ActorEndPlaced
.lFAD9:
    stz.b !actor_40
.lFADB:
    brk.b #$00
    lda.w !V_00AD
    cmp.b !actor_40
    beq .lFADB
    sta.b !actor_40
    asl a
    tax
    lda.w !SPARK_PLACES,x
    sta.b !actor_41
    lda.w !SPARK_PLACES+1,x
    sta.b !actor_42
    lda.b !actor_06
    jsl.l ActorSpawn
    bmi .lFB11
    lda.b !actor_look
    sta.w !actor_look,x
    lda.b !actor_2c
    sta.w !actor_2c,x
    lda.b !actor_41
    sta.w !actor_x+1,x
    lda.b !actor_42
    sta.w !actor_y+1,x
    stz.w !actor_07,x
.lFB11:
    sep.b #$10
    bra .lFADB
