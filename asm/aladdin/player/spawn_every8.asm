; spawn_every8.asm: a child the player starts every eighth frame. Drafted by cdraft.py;
; named and described by hand.
; C: src/aladdin/player/spawn_every8.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!v_032d = $032D                 ; its low three bits 0: a child is started

; @routine 81:DD90 PlayerSpawnEvery8
; WHAT IT DOES: Every eighth frame (low three bits of $032D 0) starts a
; child of hit box $34 at the player's spawn point 2, facing as the player
; faces.
; HOW: Spawn point 2; the flip kept on the stack and set to the facing;
; ActorSpawnFacing with hit box $34 and Y = 1; the flip put back. No
; record free: X 8-bit and the kept flip pulled, not put back.
; Entry: M 8-bit, X 8-bit; D = the player's record; DB = $80; called by
; JSR from the player's code (81:D28D). Exit: M 8-bit; started: as
; ActorSpawnFacing, the flip put back; none free: X 8-bit, the player's
; flip left as its facing; not this frame: A low nonzero.
%at($81DD90)
PlayerSpawnEvery8:
    lda.w !v_032d
    and.b #$07
    bne .lDDAF
    lda.b #$02
    sta.b !actor_spawn_at
    lda.b !actor_flip
    pha
    ldx.b !actor_facing
    stx.b !actor_flip
    lda.b #$34
    ldy.b #$01
    jsl.l ActorSpawnFacing
    bmi .lDDB0
    pla
    sta.b !actor_flip
.lDDAF:
    rts
.lDDB0:
    sep.b #$10
    pla
    rts
