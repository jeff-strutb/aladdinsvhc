; spawn.asm: an actor spawning another (a projectile, a piece, a child).
;
; The spawning actor is the record the direct page is; the child's record
; comes from the free list (ActorAlloc) and is reached at X. The child
; starts new ($0C), placed at the spawner's spawn offset, facing the same
; way, sharing its animated-tile slot.
; C: src/aladdin/actor/spawn.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"

arch 65816

!ACTOR_NEW = $0C
; Spawn offsets (words, Y the spawner's actor_spawn_at): across, taken
; away when bit 0 of the spawner's flip is set; down.
!spawn_offset_x = $81DBBB
!spawn_offset_y = $81DC51

; @routine 81:A9BF ActorSpawn
; WHAT IT DOES: Spawns a child with hit box A at the actor's spawn point,
; sharing its tile slot, if a record is free.
; HOW: ActorAlloc; with a record, ActorSpawnTyped and ActorTileSlotShare.
; Entry: M 8-bit, X 8-bit; D = the spawner; A low byte = the hit box; DB =
; $81. Exit: none free: N set, as ActorAlloc leaves it. Spawned: X 16-bit
; = the child; A low byte = 0; Z set, N clear; Y as ActorTileSlotShare leaves
; it.
%at($81A9BF)
ActorSpawn:
    jsl.l ActorAlloc
    bmi .done
    jsr.w ActorSpawnTyped
    jsr.w ActorTileSlotShare
    lda.b #0
.done:
    rtl

; @routine 81:A9CE ActorSpawnSameLook
; WHAT IT DOES: Spawns a child with hit box A that looks like its spawner,
; at the actor's spawn point, sharing its tile slot, if a record is free.
; HOW: As ActorSpawn, with the spawner's sprite attributes copied to the
; child before it takes the slot; returns with 8-bit registers.
; Entry: as ActorSpawn. Exit: none free: as ActorSpawn. Spawned: M and X
; 8-bit; X = the child's address's low byte; A low byte = 0; Z set, N
; clear; Y as ActorTileSlotShare leaves it (low byte).
ActorSpawnSameLook:
    jsl.l ActorAlloc
    bmi .done
    jsr.w ActorSpawnTyped
    lda.b !actor_look
    sta.w !actor_look,x
    lda.b !actor_look+1
    sta.w !actor_look+1,x
    jsr.w ActorTileSlotShare
    sep.b #$30
    lda.b #0
.done:
    rtl

; @routine 81:A9FA ActorTileSlotShare
; WHAT IT DOES: Gives a child its spawner's animated-tile slot, and counts
; it among the slot's holders.
; HOW: With a slot (nonzero), the child's slot = it and the slot's count
; of holders steps up.
; Entry: M 8-bit, X 16-bit; D = the spawner; X = the child; DB a bank
; whose $0000-$1FFF is work RAM; called by JSR. Exit: no slot: A low
; byte = 0, Z set. Else: A = the new count (high byte 0), Y = the slot,
; N, Z from the count. X, C, V unchanged.
%at($81A9FA)
ActorTileSlotShare:
    lda.b !actor_tile_slot
    beq .done
    sta.w !actor_tile_slot,x
    xba
    lda.b #0
    xba
    tay
    lda.w !actor_tile_slot_users,y
    inc a
    sta.w !actor_tile_slot_users,y
.done:
    rts

; @routine 81:AA0E ActorSpawnTyped
; WHAT IT DOES: Starts a child with hit box A at its spawner's spawn point.
; HOW: Sets the child's $06 (its hit box), then goes on into ActorSpawnInit.
; Entry: as ActorSpawnInit; A low byte = the hit box. Exit: as
; ActorSpawnInit.
ActorSpawnTyped:
    sta.w !actor_06,x
; @routine 81:AA11 ActorSpawnInit
; WHAT IT DOES: Starts a child as a new actor at its spawner's spawn
; point, facing the spawner's way.
; HOW: The child's state = new; ActorSpawnPlace; the spawner's flip
; becomes both the child's facing and flip; $34 = $FF; $07 is the
; spawner's.
; Entry: M 8-bit, X 16-bit; D = the spawner; X = the child; DB = $81;
; called by JSR. Exit: A low byte = the spawner's $07; N, Z from it;
; Y = the spawn offset index; X, C, V as ActorSpawnPlace leaves them.
ActorSpawnInit:
    lda.b #!ACTOR_NEW
    sta.w !actor_state,x
    jsl.l ActorSpawnPlace
    lda.b !actor_flip
    sta.w !actor_facing,x
    sta.w !actor_flip,x
    lda.b #$FF
    sta.w !actor_34,x
    lda.b !actor_07
    sta.w !actor_07,x
    rts

; @routine 81:AA2D ActorSpawnFacing
; WHAT IT DOES: Spawns a child with hit box A at the actor's spawn point,
; facing X, with $07 = Y, if a record is free.
; HOW: Keeps X and Y in scratch $0D and $0E; ActorAlloc; with a record,
; the child's hit box and state (new), ActorSpawnPlace, then its facing and
; flip = the kept X and its $07 = the kept Y.
; Entry: M 8-bit, X 8-bit; D = the spawner; A low byte = the hit box; X =
; the facing; Y = the $07 value; DB = $81. Exit: none free: N set, as
; ActorAlloc leaves it. Spawned: M and X 8-bit; X = the child's address's
; low byte; A low byte = the kept Y; Z set; N from it; Y = the spawn
; offset index (low byte); C, V from ActorSpawnPlace.
ActorSpawnFacing:
    stx.w !scratch+1
    sty.w !scratch+2
    jsl.l ActorAlloc
    bmi .done
    sta.w !actor_06,x
    lda.b #!ACTOR_NEW
    sta.w !actor_state,x
    jsl.l ActorSpawnPlace
    lda.w !scratch+1
    sta.w !actor_facing,x
    sta.w !actor_flip,x
    lda.w !scratch+2
    sta.w !actor_07,x
    sep.b #$32
.done:
    rtl

; @routine 81:AA81 ActorSpawnPlace
; WHAT IT DOES: Places a child at its spawner's spawn point.
; HOW: The spawner's spawn offset index picks an offset across and one
; down; across is added to the spawner's x, or taken away when bit 0 of
; its flip is set; down is added to its y.
; Entry: M 8-bit, X 16-bit; D = the spawner; X = the child; DB = $81.
; Exit: M 8-bit; A = the child's y (16 bits); Y = the index; N, V, Z, C
; from the y ADC; X unchanged.
%at($81AA81)
ActorSpawnPlace:
    lda.b #0
    xba
    lda.b !actor_spawn_at
    tay
    rep.b #$20
    lda.b !actor_flip
    lsr a
    bcs .left
    clc
    lda.b !actor_x+1
    adc.w !spawn_offset_x,y
    sta.w !actor_x+1,x
    bra .down
.left:
    sec
    lda.b !actor_x+1
    sbc.w !spawn_offset_x,y
    sta.w !actor_x+1,x
.down:
    clc
    lda.b !actor_y+1
    adc.w !spawn_offset_y,y
    sta.w !actor_y+1,x
    sep.b #$20
    rtl
