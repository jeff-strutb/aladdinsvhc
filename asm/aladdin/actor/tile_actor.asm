; tile_actor.asm: an actor drawn as tiles of the level's map, changed
; through the tile change queue as it moves. Drafted by cdraft.py; named
; and described by hand.
; C: src/aladdin/actor/tile_actor.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

; In the data bank: per kind ($47) the offset of its entries; 4 bytes an
; entry: the tile, and the tile 16 pixels below (0: none).
!TILE_ACTOR_TILES = $D633
!EMPTY_TILE = $7F80             ; the tile put while $19 is 0
!TILE_ACTOR_FRAME = $4A         ; its frame (0 or 1), flipped as it moves
!TILE_ACTOR_SHOWN = $19         ; zero: the empty tile is put in its place (word)

; @routine 83:E7F0 TileActorDraw
; WHAT IT DOES: Puts the actor into the level's map as tiles at its tile
; spot ($42, $44): the tile for its kind, frame and flip, and the tile 16
; pixels below when it has one; the empty tile in place of each while its
; $19 is 0.
; HOW: The entry: ($4A xor $19) times 2, or its flip ($15), times 4, plus
; the kind's offset from the table at its $47. TileChangeAdd at ($42, $44)
; with the first tile; the second nonzero: TileChangeAdd at ($42, $44 +
; 16).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; called by
; JSR. Exit: M 8-bit, X 8-bit; as TileChangeAdd, or A low = 0 and Z set
; when there is no second tile.
%at($83E7F0)
TileActorDraw:
    ldx.b !actor_47
    lda.b !TILE_ACTOR_FRAME
    eor.b !TILE_ACTOR_SHOWN
    asl a
    ora.b !actor_flip
    asl a
    asl a
    clc
    adc.w !TILE_ACTOR_TILES,x
    rep.b #$30
    and.w #$00FF
    tax
    phx
    lda.w !TILE_ACTOR_TILES,x
    ldx.b !TILE_ACTOR_SHOWN
    bne .lE810
    lda.w #!EMPTY_TILE
.lE810:
    ldx.b !actor_42
    ldy.b !actor_44
    jsl.l TileChangeAdd
    plx
    lda.w !TILE_ACTOR_TILES+2,x
    beq .lE834
    ldx.b !TILE_ACTOR_SHOWN
    bne .lE825
    lda.w #!EMPTY_TILE
.lE825:
    pha
    ldx.b !actor_42
    clc
    lda.b !actor_44
    adc.w #$0010
    tay
    pla
    jsl.l TileChangeAdd
.lE834:
    sep.b #$30
    rts
