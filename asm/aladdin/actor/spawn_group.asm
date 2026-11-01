; spawn_group.asm: actors that spawn children around themselves: a ring
; of five, a pair, a copy of themselves, a child of a set kind.
; C: src/aladdin/actor/spawn_group.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; Spawns a child of kind A at the actor's spawn point when a record is
; free: A = 0 (Z set) when it did, C set and A nonzero when none was.
!SPAWN_KIND = $81A9B1
; In bank $80 (DB): five x and y offsets (words, 4 bytes an entry) the
; ring's children are spawned at, each from the one before.
!RING_OFFSETS = $D36E
!RING_KIND = $34
!ring_entry = $43               ; the actor's $43: here the ring entry's offset

; @routine 83:D0A3 ActorSpawnRing
; WHAT IT DOES: Spawns up to five children of kind $34 in a ring of
; points around the actor, each with $07 = 1, at spawn point 0 of its
; point; stops at the first that finds no record free.
; HOW: The actor's x, y and $07 (words) are kept on the stack; for each
; entry of the ring table its offsets are added to x and y, $07 = 1 and
; spawn point 0, and the spawn at $81:A9B1 made; then they are put back.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($83D0A3)
ActorSpawnRing:
    lda.b #$05
    sta.b !actor_40
    ldx.b #$00
    rep.b #$20
    lda.b !actor_x+1
    pha
    lda.b !actor_y+1
    pha
    lda.b !actor_07
    pha
.next:
    rep.b #$21
    lda.w !RING_OFFSETS,x
    adc.b !actor_x+1
    sta.b !actor_x+1
    clc
    lda.w !RING_OFFSETS+2,x
    adc.b !actor_y+1
    sta.b !actor_y+1
    sep.b #$20
    stx.b !ring_entry
    lda.b #$01
    sta.b !actor_07
    stz.b !actor_spawn_at
    lda.b #!RING_KIND
    jsl.l !SPAWN_KIND
    bne .done
    ldx.b !ring_entry
    inx
    inx
    inx
    inx
    dec.b !actor_40
    bne .next
.done:
    rep.b #$20
    pla
    sta.b !actor_07
    pla
    sta.b !actor_y+1
    pla
    sta.b !actor_x+1
    sep.b #$20
    rts

; @routine 83:D332 ActorSpawnPair
; WHAT IT DOES: Spawns two children that look like the actor, of its own
; hit box: at spawn point $46, then (if the first found a record) at $48
; with bit 2 of $07 set; the children take $07 + 2 as theirs. The actor's
; $07 keeps only bit 0 after.
; HOW: $07 up by 2 for the children; ActorSpawnSameLook twice; then $07
; masked to bit 0.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($83D332)
ActorSpawnPair:
    inc.b !actor_07
    inc.b !actor_07
    lda.b #$46
    sta.b !actor_spawn_at
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .done
    lda.b #$04
    tsb.b !actor_07
    lda.b #$48
    sta.b !actor_spawn_at
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    bmi .done
.done:
    lda.b !actor_07
    and.b #$01
    sta.b !actor_07
    rts

; @routine 83:D7C9 ActorSpawn5E
; WHAT IT DOES: Spawns a child of kind $5E at spawn point $4E.
; HOW: The spawn at $81:A9B1.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($83D7C9)
ActorSpawn5E:
    lda.b #$4E
    sta.b !actor_spawn_at
    lda.b #$5E
    jsl.l !SPAWN_KIND
    rts

; @routine 83:D7D4 ActorSpawnCopy
; WHAT IT DOES: Spawns a child that looks like the actor, of its own hit
; box, at spawn point 0, with bit 3 of $07 set for it.
; HOW: Bit 3 of $07 is set around ActorSpawnSameLook.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
ActorSpawnCopy:
    lda.b #$08
    tsb.b !actor_07
    lda.b #$00
    sta.b !actor_spawn_at
    lda.b !actor_06
    jsl.l ActorSpawnSameLook
    lda.b #$08
    trb.b !actor_07
    rts
