; falling_actor.asm: an actor falling down the level: the map tile under it, and its end when
; it is above the level's top. Drafted by cdraft.py; named and described
; by hand.
; C: src/aladdin/actor/falling_actor.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_player.inc"

arch 65816

; @routine 83:C903 ActorFallTileTest
; WHAT IT DOES: For a falling actor: the map tile 10 pixels below its
; position, Z set when its kind is 2; an actor above the level's top (y
; negative) is ended instead.
; HOW: Its x pixels to $000C and its y pixels plus 10 to $000E,
; TileAtProbeLong, the kind ($0012) compared with 2. Its y negative: its
; return address dropped (PLA PLA, 8-bit), then into ActorEndOutOfRange
; after its pulls: ActorFreePlaced and the actor loop.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR
; from the actor's code at its own level (83:C85A). Exit: M 16-bit (as
; TileAtProbeLong left it); A low = the kind; Z set when it is 2, C from
; the comparison. Above the top: does not return.
%at($83C903)
ActorFallTileTest:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !scratch
    lda.b !actor_y+1
    bmi .lC91E
    adc.w #$000A
    sta.w !scratch+2
    jsl.l TileAtProbeLong
    lda.w !v_0012
    cmp.b #$02
    rts
.lC91E:
    sep.b #$20
    pla
    pla
    jmp.w ActorEndPlaced
