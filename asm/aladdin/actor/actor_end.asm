; actor_end.asm: an actor's end: its record cleared and given back, its
; animated-tile slot let go, and the actor loop goes on without it.
; C: src/aladdin/actor/actor_end.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

; Where the actor loop takes the next record (docs/aladdin/ACTORS.md).
!actor_loop_next = $8394BF

; @routine 83:95B8 ActorEnd
; @actor
; WHAT IT DOES: Ends an actor of the second pool for good: the actor
; loop goes on to the next record.
; HOW: ActorFree, ActorTileSlotRelease, then into the actor loop where it
; takes the next record; the actor's code is left there.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by a jump from the actor's code, at
; the actor loop's stack level. Exit: never returns; in the actor loop
; with the registers ActorTileSlotRelease leaves.
%at($8395B8)
ActorEnd:
    jsr.w ActorFree
    jsr.w ActorTileSlotRelease
    jmp.w !actor_loop_next

; @routine 83:95E5 ActorFree
; WHAT IT DOES: Ends an actor of the second pool: clears its record and
; puts it back on the pool's free stack.
; HOW: ActorClear, then the record's address goes on top of the stack.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM. Exit: A = the record's address, bytes
; swapped; Y = the stack's new top; X = the kind; N and Z from the
; address's high byte; the rest unchanged.
%at($8395E5)
ActorFree:
    jsr.w ActorClear
    inc.w !actor_pool2_top
    inc.w !actor_pool2_top
    ldy.w !actor_pool2_top
    tdc
    sta.w !actor_pool2,y
    xba
    sta.w !actor_pool2+1,y
    rts

; @routine 83:9706 ActorTileSlotRelease
; WHAT IT DOES: An ending actor lets go of its animated-tile slot; when it
; was the slot's last user, the slot and the map cell that claimed it
; are freed.
; HOW: The slot's user count steps down; at 0 the cell's attribute word
; is cleared, and the cell's further slots with it: the next one, or the
; next two (by the cell's count of further slots). The actor's slot
; becomes 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM. Exit: M and X 8-bit; A, X and the flags as
; the last instructions leave them; Y unchanged.
%at($839706)
ActorTileSlotRelease:
    ldx.b !actor_tile_slot
    beq .done
    dec.w !actor_tile_slot_users,x
    bne .done
    lda.w !tile_slot_cell-1,x
    pha
    rep.b #$30
    asl a
    and.w #$01FF
    tax
    lda.w #0
    sta.l !cell_attrs,x
    sep.b #$30
    plx
    lda.l !cell_extra_slots,x
    beq .done
    ldx.b !actor_tile_slot
    dec a
    beq .one
    stz.w !actor_tile_slot_users+2,x
.one:
    stz.w !actor_tile_slot_users+1,x
.done:
    stz.b !actor_tile_slot
    rts

; @routine 83:975B ActorClear
; WHAT IT DOES: Clears an actor's record: it is free, still, drawn no
; more, and one fewer of its kind is live.
; HOW: Its kind's live count steps down; its state, flags, facing and
; the other fields the actors use stay set are zeroed.
; Entry: M either, X 8-bit; D = the actor's record. Exit: M 8-bit; A low
; = the kind's new count; X = the kind; N and Z from the count; the
; rest unchanged.
%at($83975B)
ActorClear:
    sep.b #$20
    ldx.b !actor_06
    lda.l !kind_count,x
    dec a
    sta.l !kind_count,x
    stz.b !actor_34
    stz.b !actor_state
    stz.b !actor_08
    stz.b !actor_09
    stz.b !actor_0a
    stz.b !actor_flip
    stz.b !actor_facing
    stz.b !actor_12
    stz.b !actor_look
    stz.b !actor_2c
    stz.b !actor_next
    stz.b !actor_38
    stz.b !actor_13
    stz.b !ACTOR_REDRAW
    stz.b !actor_33
    rts
