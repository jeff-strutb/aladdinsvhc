; tile_ahead.asm: an actor's look at the map tile ahead of it and below,
; and an actor's end from code in another bank.
; C: src/aladdin/actor/tile_ahead.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; Not yet filed: the class of the map tile at the actor's position offset
; by the word at $D062 + facing x 2 + the byte at $000C (in the bank DB
; names) and by the word at $000E, through $81:B82B.
!TILE_AHEAD = $839A15
!TILE_BELOW = 12                ; pixels below the actor's position
!CLASS_70 = $70                 ; the class's bits 4-6

; @routine 83:96D0 ActorFreePlacedAtYLong
; WHAT IT DOES: ActorFreePlacedAtY for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ActorFreePlacedAtY; called by JSL. Exit: as the C states.
%at($8396D0)
ActorFreePlacedAtYLong:
    jsr.w ActorFreePlacedAtY
    rtl

; @routine 83:99EE ActorTileAheadIs70
; WHAT IT DOES: Tells whether the map tile ahead of the actor, 12 pixels
; below its position, is of the class whose bits 4-6 are all set.
; HOW: Offsets 0 (in the facing table) and 12 for $83:9A15, which returns
; the tile's class (a byte by its behaviour's low 3 bits, through
; $81:B82B); then AND #$70 and CMP #$70.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the offsets' bank. Exit: as the C states.
%at($8399EE)
ActorTileAheadIs70:
    stz.w !scratch
    lda.b #!TILE_BELOW
    sta.w !scratch+2
    stz.w !scratch+3
    jsr.w !TILE_AHEAD
    and.b #!CLASS_70
    cmp.b #!CLASS_70
    rts
