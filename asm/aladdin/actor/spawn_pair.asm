; spawn_pair.asm: two children of hit box $A3 started at the spawner's
; spawn point and moved to the point it keeps at $48.
; C: src/aladdin/actor/spawn_pair.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!PAIR_HIT_BOX = $A3
!pair_busy = $43               ; direct page (the spawner): set while the pair starts
!pair_x = $48                  ; the point the children go to: x (word)
!pair_y = $4A                  ; its y (word)

; @routine 84:8173 ActorSpawnPairA3
; WHAT IT DOES: Starts two children with hit box $A3, their $07 0 and 1,
; each placed at the spawner's point in its $48 (x) and $4A (y); while it
; runs the spawner's $43 is set.
; HOW: $43 up; for each child the spawn point 0 ($21), ActorSpawn, its
; $07, and the whole pixels of its x and y from the two words; a spawn
; with no record free ends it; then $43 cleared.
; Entry: M 8-bit, X 8-bit; D = the spawner; DB = $81; called by JSR.
; Exit: M 8-bit, X 8-bit (spawned) or as ActorSpawn left them; $43 = 0.
%at($848173)
ActorSpawnPairA3:
    inc.b !pair_busy
    stz.b !actor_spawn_at
    lda.b #!PAIR_HIT_BOX
    jsl.l ActorSpawn
    bmi .done
    lda.b #$00
    sta.w !actor_07,x
    rep.b #$20
    lda.b !pair_x
    sta.w !actor_x+1,x
    lda.b !pair_y
    sta.w !actor_y+1,x
    sep.b #$30
    stz.b !actor_spawn_at
    lda.b #!PAIR_HIT_BOX
    jsl.l ActorSpawn
    bmi .done
    lda.b #$01
    sta.w !actor_07,x
    rep.b #$20
    lda.b !pair_x
    sta.w !actor_x+1,x
    lda.b !pair_y
    sta.w !actor_y+1,x
    sep.b #$30
.done:
    stz.b !pair_busy
    rts
