; hit_marks.asm: the small actors (kinds $11 and $13) that mark a hit, each
; started in a record of the second pool at another record's place.
; C: src/aladdin/actor/hit_marks.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; @routine 83:827D Spawn11AtY
; WHAT IT DOES: Starts an actor of kind $11 at record Y's place, when the
; second pool has a free record.
; HOW: Keeps X and Y; ActorAllocBLong; with a record: kind $11 in its $06,
; state $0C (new), $07 cleared, $08 = $80, and record Y's x and y pixels
; copied into it; X and Y put back. The tail from the kind on is shared
; with Spawn13AtY.
; Entry: M 8-bit, X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M 8-bit, X 16-bit; X, Y as on entry; N, Z
; from X. Started: A = record Y's y pixels (16 bits, M 8-bit). None free:
; A unchanged. C, V unchanged.
%at($83827D)
Spawn11AtY:
    phx
    phy
    sep.b #$10
    jsl.l ActorAllocBLong
    bmi .none
    lda.b #$11
.start:
    sta.w !actor_06,x
    lda.b #$0C
    sta.w !actor_state,x
    stz.w !actor_07,x
    lda.b #$80
    sta.w !actor_08,x
    ply
    rep.b #$20
    lda.w !actor_x+1,y
    sta.w !actor_x+1,x
    lda.w !actor_y+1,y
    sta.w !actor_y+1,x
    sep.b #$20
    plx
    rts
.none:
    rep.b #$10
    ply
    plx
    rts

; @routine 83:82B5 Spawn13AtY
; WHAT IT DOES: Starts an actor of kind $13 at record Y's place, when the
; second pool has a free record.
; HOW: As Spawn11AtY with kind $13: keeps X and Y, ActorAllocBLong, then
; Spawn11AtY's own tail.
; Entry: M 8-bit, X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: as Spawn11AtY.
%at($8382B5)
Spawn13AtY:
    phx
    phy
    sep.b #$10
    jsl.l ActorAllocBLong
    bmi Spawn11AtY_none
    lda.b #$13
    bra Spawn11AtY_start

; @routine 83:82C3 LoneReturn82C3
; WHAT IT DOES: Nothing: a lone RTS after Spawn13AtY's BRA. Nothing in the
; game reaches it (no branch, JSR, JSL, JMP, pointer or word names it):
; unit_bank83_pools runs it.
; HOW: RTS.
; Entry: any; called by JSR. Exit: all unchanged.
%at($8382C3)
LoneReturn82C3:
    rts

; @routine 83:82C4 Spawn11At1Long
; WHAT IT DOES: Spawn11At1 for callers in other banks. Nothing in the game
; calls it (no JSL, JMP, pointer or word names it): unit_bank83_pools runs
; it.
; HOW: JSR Spawn11At1, then RTL.
; Entry: as Spawn11At1; called by JSL. Exit: as Spawn11At1.
Spawn11At1Long:
    jsr.w Spawn11At1
    rtl

; @routine 83:82C8 Spawn11At1
; WHAT IT DOES: Starts an actor of kind $11 with $07 1 at record Y's place,
; when the second pool has a free record. Nothing in the game calls it but
; its long entry, which nothing calls.
; HOW: Keeps X and Y; ActorAllocBLong; none free: Spawn11AtY's way out.
; Else state $0C, kind $11, $07 1, record Y's x and y pixels copied into
; it; X and Y put back.
; Entry: M 8-bit, X 16-bit; Y = the record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: as Spawn11AtY.
Spawn11At1:
    phx
    phy
    sep.b #$10
    jsl.l ActorAllocBLong
    bmi Spawn11AtY_none
    lda.b #$0C
    sta.w !actor_state,x
    lda.b #$11
    sta.w !actor_06,x
    lda.b #$01
    sta.w !actor_07,x
    ply
    rep.b #$20
    lda.w !actor_x+1,y
    sta.w !actor_x+1,x
    lda.w !actor_y+1,y
    sta.w !actor_y+1,x
    sep.b #$20
    plx
    rts
%ends_at($8382F4)
