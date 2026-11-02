; still_actors.asm: actors that only stand, drift or fly off: kinds $7D
; (keeps its spot from a table by its height), $7A (flies its course for
; good), $4A (its own palette, an animation, nothing more), and the fly-
; off end of the actor at $83:C763. Drafted by cdraft.py; named and
; described by hand.
; C: src/aladdin/actor/still_actors.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!KIND7D_16 = $16  ; word: $D820
!KIND7D_SPOTS = $D7D2  ; by its y pixels' low six bits, two bytes kept in $40 and $41 (in the bank DB names)
!FLYOFF_LEFT = $41  ; frames left of its flight

; @routine 84:90CE Kind7DKeepSpot
; @actor
; WHAT IT DOES: Actor kind $7D: takes two bytes from KIND7D_SPOTS by the
; low six bits of its y (kept as its $07), then each frame keeps its place
; in $4C and $4E (ActorKeepSpot), for good.
; HOW: $09 = $80; $16 = $D820 (two 8-bit stores); $07 = y pixels and $3F;
; KIND7D_SPOTS at twice it to $40 and $41; each frame (BRK first)
; ActorKeepSpot.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $7D in state $0C (through the stub at
; $83:929B). Exit: never returns.
%at($8490CE)
Kind7DKeepSpot:
    lda.b #$80
    sta.b !actor_09
    lda.b #$20
    sta.b !KIND7D_16
    lda.b #$D8
    sta.b !KIND7D_16+1
    lda.b !actor_y+1
    and.b #$3F
    sta.b !actor_07
    asl a
    tax
    lda.w !KIND7D_SPOTS,x
    sta.b !actor_40
    lda.w !KIND7D_SPOTS+1,x
    sta.b !FLYOFF_LEFT
.l90EC:
    brk.b #$00
    jsl.l ActorKeepSpot
    bra .l90EC


; @routine 83:C925 ActorFlyOffEnd
; @actor
; WHAT IT DOES: The end of the actor at $83:C763: it can no longer be hit,
; flies off on motion $2B for $18 frames with animation 3, and ends.
; HOW: Bit 7 of $09 cleared; A = $80 shifted right, C clear, so Y = $2A +
; 1; ActorMotionPreset; AnimStart 3; $41 = $18; each frame ActorFly (BRK
; first) until $41 is counted out; then ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by a
; jump from that actor's code (83:C773). Exit: never returns; ends in
; ActorEndPlaced.
%at($83C925)
ActorFlyOffEnd:
    lda.b #$80
    trb.b !actor_09
    lsr a
    ldy.b #$2A
    bcs .lC92F
    iny
.lC92F:
    jsl.l ActorMotionPreset
    lda.b #$03
    jsl.l AnimStart
    lda.b #$18
    sta.b !FLYOFF_LEFT
.lC93D:
    brk.b #$00
    jsl.l ActorFly
    dec.b !FLYOFF_LEFT
    bne .lC93D
    jmp.w ActorEndPlaced


; @routine 83:F757 Kind7AFly
; @actor
; WHAT IT DOES: Actor kind $7A: facing right, shown with animation 0, it
; flies its course (motion $53) for good.
; HOW: Facing and flip 0; $08 bit 1 and $09 bit 4 set; AnimStart 0;
; ActorMotionPreset $53; each frame (BRK first) ActorFly.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $7A in state $0C. Exit: never returns.
%at($83F757)
Kind7AFly:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$10
    sta.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    ldy.b #$53
    jsl.l ActorMotionPreset
.lF773:
    brk.b #$00
    jsl.l ActorFly
    bra .lF773


; @routine 83:B496 Kind4AShow
; @actor
; WHAT IT DOES: Actor kind $4A: facing right, with its look $0600, loads
; palette set $0A, shows animation 6 and stays.
; HOW: $09 bits $90 set; facing and flip 0; its look $0600; PaletteLoad
; $0A; AnimStart 6; BRK for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $4A in state $0C. Exit: never returns.
%at($83B496)
Kind4AShow:
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    stz.b !actor_facing
    stz.b !actor_flip
    rep.b #$20
    lda.w #$0600
    sta.b !actor_look
    sep.b #$20
    lda.b #$0A
    jsl.l PaletteLoad
    lda.b #$06
    jsl.l AnimStart
.lB4B5:
    brk.b #$00
    bra .lB4B5
