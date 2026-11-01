; rise_fall.asm: actor helpers of bank $A5: an animation by whether the
; actor rises or falls, and a placing from a table.
; C: src/aladdin/actor/rise_fall.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!actor_spots = $D02C            ; spots an actor is put at: X, Y (pixels, words), 4 bytes each (in the data bank)

; @routine A5:EA20 ActorAnimRiseFall
; WHAT IT DOES: Shows an actor rising or falling: animation $0C while it
; rises, $0E while it falls.
; HOW: By the sign of its vertical speed's top byte, then
; ActorAnimChange.
; Entry: M 8-bit, X 8-bit; D = the actor's record. Exit: as
; ActorAnimChange; X = the speed's top byte.
%at($A5EA20)
ActorAnimRiseFall:
    ldx.b !actor_speed_y+2
    bpl .falls
    lda.b #$0C
    bra ActorAnimChange
.falls:
    lda.b #$0E
    bra ActorAnimChange

; @routine A5:EB1E ActorPlaceAtSpot
; WHAT IT DOES: Puts an actor at spot X of the spot table, on whole
; pixels.
; HOW: Its X and Y pixels from the spot's two words, their fractions 0.
; Entry: M 8-bit, X 8-bit; X = the spot's offset; D = the actor's
; record; DB the table's bank. Exit: M 8-bit; A = the spot's Y; N, Z from
; it; X, Y unchanged.
%at($A5EB1E)
ActorPlaceAtSpot:
    rep.b #$20
    lda.w !actor_spots,x
    sta.b !actor_x+1
    lda.w !actor_spots+2,x
    sta.b !actor_y+1
    sep.b #$20
    stz.b !actor_x
    stz.b !actor_y
    rts
