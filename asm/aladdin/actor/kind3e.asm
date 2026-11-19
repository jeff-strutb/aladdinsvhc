; kind3e.asm: actor kind $3E's start, a maker: while it cannot be hit it
; makes an actor of look $31 next to it every 128 frames, as long as fewer
; than three actors hold its tile slot. Its wait is Kind3EWait
; (kind_waits_rest.asm).
; C: src/aladdin/actor/kind3e.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K3E_LEFT = $40                 ; frames left of its pause after a spawn
!K3E_PAUSE = $41                ; its pause's length (128 frames)
!K3E_LOOK = $4D                 ; ActorSpawnLookWait's look: what it makes
!K3E_MADE_LOOK = $31            ; the look of what it makes
!K3E_SLOT_FULL = $03            ; holders of its tile slot at which it makes none

; @routine 83:B154 Kind3EStart
; @actor
; WHAT IT DOES: Actor kind $3E: each frame while it cannot be hit ($09 bit
; 6 clear) and fewer than three actors hold its tile slot, it makes an
; actor of look $31 next to it, then pauses 128 frames.
; HOW: $08 bit 3 cleared, $09 bits 7 and 4 set, facing and flip 0.
; K3E_PAUSE = $80; each frame (BRK first) K3E_LEFT = K3E_PAUSE; $09 bit 6
; set: K3E_PAUSE = $80 again; else the slot's holders ($15E8 by $39) 3 or
; more: the next frame; else $21 = 0, K3E_LOOK = $31,
; ActorSpawnLookWait; K3E_LEFT frames (BRK first); the next frame.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $3E in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83B154)
Kind3EStart:
    lda.b !actor_08
    and.b #$F7
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
.hit:
    lda.b #$80
    sta.b !K3E_PAUSE
.frame:
    brk.b #$00
    lda.b !K3E_PAUSE
    sta.b !K3E_LEFT
    lda.b !actor_09
    bit.b #$40
    bne .hit
    ldx.b !actor_tile_slot
    lda.w !actor_tile_slot_users,x
    cmp.b #!K3E_SLOT_FULL
    bcs .frame
    lda.b #$00
    sta.b !actor_spawn_at
    lda.b #!K3E_MADE_LOOK
    sta.b !K3E_LOOK
    jsr.w ActorSpawnLookWait
.pause:
    brk.b #$00
    dec.b !K3E_LEFT
    bne .pause
    bra .frame
%ends_at($83B190)
