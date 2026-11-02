; collision.asm: what map tile is at a point, and how high its surface is.
;
; An actor tests the map at probe points: offsets from its position, in a
; list its record points to. A point's tile comes from the screen map (the
; screen under the point) and the screen's tiles; the tile's behaviour byte
; gives its kind (empty, solid, slope ...) and, for a slope, the surface's
; height at the point's x. The point and the results are kept in scratch.
; C: src/aladdin/map/collision.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; The probe list (its address, word) and the probe's offset in it.
!actor_probes = $16
!actor_probe_at = $18

!point_x = !scratch
!point_y = !scratch+2
!surface_y = !scratch+4
!tile_behaviour_byte = !scratch+6
!tile_kind = !scratch+7
!screen_place = !scratch+8
!tile_place = !scratch+10
!screen_tiles = !scratch+12
!tile_mirrored = !scratch+14
!tile_word = !scratch+16
!slope_drop = !scratch+18

!screen_map = $7FF000           ; per screen place, its tiles' offset in level_tiles (word)
!level_tiles = $7F0000          ; the screens' tiles, a word each
!tile_kinds = $81BF22           ; per behaviour's low 3 bits, the tile's kind
!KIND_SOLID = 1
!SLOPE_FLAT = $70
!VIEW_HEIGHT = 240
!LEVEL_TALL_A = $000A
!LEVEL_TALL_B = $000F

; @routine 81:B4AC ProbeAtActor
; WHAT IT DOES: Makes the probe point at the actor's x and its probe's y.
; HOW: Point x = the actor's x; point y = the probe's y offset plus the
; actor's y (the probe at $18 in the list at $16).
; Entry: X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
; and that holds the probe list; called by JSR. Exit: M 16-bit, X 16-bit;
; A = the point's y; Y = the probe's offset + 2; N, V, Z, C from its ADC;
; X unchanged.
%at($81B4AC)
ProbeAtActor:
    rep.b #$20
    sep.b #$10
    ldy.b !actor_probe_at
    lda.b !actor_x+1
    sta.w !point_x
    bra ProbeRight_y

; @routine 81:B4B9 ProbeLeft
; WHAT IT DOES: Makes the probe point to the actor's left: the probe's x
; offset back from it, and its y offset down.
; HOW: Point x = the actor's x less the probe's x offset; point y = its y
; offset plus the actor's y.
; Entry: X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
; and that holds the probe list; called by JSR. Exit: as ProbeAtActor.
ProbeLeft:
    rep.b #$20
    sep.b #$10
    ldy.b !actor_probe_at
    sec
    lda.b !actor_x+1
    sbc.b (!actor_probes),y
    sta.w !point_x
    bra ProbeRight_y

; @routine 81:B4C9 ProbeRight
; WHAT IT DOES: Makes the probe point to the actor's right: the probe's x
; offset on from it, and its y offset down.
; HOW: Point x = the probe's x offset plus the actor's x; point y = its y
; offset plus the actor's y.
; Entry: X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is work RAM
; and that holds the probe list; called by JSR. Exit: as ProbeAtActor.
ProbeRight:
    rep.b #$20
    sep.b #$10
    ldy.b !actor_probe_at
    clc
    lda.b (!actor_probes),y
    adc.b !actor_x+1
    sta.w !point_x
; The point's y: the probe's next word plus the actor's y.
ProbeRight_y:
    iny
    iny
    clc
    lda.b (!actor_probes),y
    adc.b !actor_y+1
    sta.w !point_y
    rep.b #$10
    rts

; @routine 81:B79F GroundInFront
; WHAT IT DOES: Says whether the ground is at the probe point in front of
; the actor, the way it faces.
; HOW: Facing right, GroundRight; facing left, GroundLeft.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
; PointInGround.
%at($81B79F)
GroundInFront:
    lda.b !actor_facing
    bne GroundLeft
; @routine 81:B7A3 GroundRight
; WHAT IT DOES: Says whether the ground is at the probe point to the
; actor's right.
; HOW: ProbeRight, then PointInGround.
; Entry: X 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
; PointInGround.
GroundRight:
    jsr.w ProbeRight
    bra PointInGround

; @routine 81:B7A8 GroundLeft
; WHAT IT DOES: Says whether the ground is at the probe point to the
; actor's left.
; HOW: ProbeLeft, then PointInGround.
; Entry: X 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
; PointInGround.
GroundLeft:
    jsr.w ProbeLeft
    bra PointInGround

; @routine 81:B7AD GroundBehind
; WHAT IT DOES: Says whether the ground is at the probe point behind the
; actor.
; HOW: Its flip clear (facing left), GroundRight; set (facing right),
; GroundLeft.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSR. Exit: as
; PointInGround.
GroundBehind:
    lda.b !actor_flip
    bne .left
    jsr.w ProbeRight
    bra PointInGround
.left:
    jsr.w ProbeLeft
    bra PointInGround

; @routine 81:B7BB GroundRightLong
; WHAT IT DOES: GroundRight for callers in other banks.
; HOW: GroundRight.
; Entry: as GroundRight; called by JSL. Exit: as PointInGround.
GroundRightLong:
    jsr.w GroundRight
    rtl

; @routine 81:B7BF GroundLeftLong
; WHAT IT DOES: GroundLeft for callers in other banks.
; HOW: GroundLeft.
; Entry: as GroundLeft; called by JSL. Exit: as PointInGround.
GroundLeftLong:
    jsr.w GroundLeft
    rtl

; @routine 81:B7C3 ActorInGround
; WHAT IT DOES: Says whether the actor's own position is in the ground.
; HOW: The point = the actor's x and y; then PointInGroundLong.
; Entry: D = the actor; DB = $81; called by JSL. Exit: as PointInGround.
ActorInGround:
    rep.b #$30
    lda.b !actor_x+1
    sta.w !point_x
    lda.b !actor_y+1
    sta.w !point_y
; @routine 81:B7CF PointInGroundLong
; WHAT IT DOES: PointInGround for callers in other banks.
; HOW: 16-bit registers, then PointInGround.
; Entry: D = the actor; DB = $81; called by JSL. Exit: as PointInGround.
PointInGroundLong:
    rep.b #$30
    jsr.w PointInGround
    rtl

; @routine 81:B7D5 PointInGround
; WHAT IT DOES: Says whether the probe point is in the ground: C set when
; it is.
; HOW: TileAt; an empty tile (kind 0) or kind with bit 7 is not ground; a
; solid tile (kind 1) is; otherwise the point is in it when at or below
; the surface (SurfaceHeight).
; Entry: M 16-bit, X 16-bit; D = the actor; DB = $81; called by JSR.
; Exit: M and X 8-bit; C as above; not ground or solid: A and N, Z as
; TileAt leaves them (solid: Z set); else A = the point's y (16 bits), N,
; Z from its compare with the surface.
PointInGround:
    jsr.w TileAt
    beq .not
    bmi .not
    cmp.b #!KIND_SOLID
    beq .in
    jsr.w SurfaceHeight
    rep.b #$30
    lda.w !point_y
    cmp.w !surface_y
    sep.b #$30
    rts
.not:
    clc
    rts
.in:
    sec
    rts

; @routine 81:B7F2 ProbeOnView
; WHAT IT DOES: For the player, makes a point above or below the view
; read as an empty tile, except in levels $0A and $0F once the camera has
; moved down (their maps are tested whole).
; HOW: When D is the player's record and the point's y is at or above the
; camera's, or more than 240 pixels below it, pulls its own return
; address and returns from the routine that called it (the tile lookup)
; with behaviour and kind 0.
; Entry: M 16-bit, X 16-bit; D = the actor; DB a bank whose $0000-$1FFF
; is work RAM; called by JSR from the tile lookup. Exit: on view: M and X
; 16-bit, A and the flags from the last compare. Off: to the lookup's
; caller; M and X 8-bit; A low byte = X = 0; Z set, N clear; C as the
; compare left it.
ProbeOnView:
    tdc
    cmp.w #!player
    bne .on
    lda.w !layer0+!LAYER_SCROLL_Y
    beq .test
    lda.w !level
    cmp.w #!LEVEL_TALL_A
    beq .on
    cmp.w #!LEVEL_TALL_B
    beq .on
.test:
    sec
    lda.w !layer0+!LAYER_SCROLL_Y
    cmp.w !point_y
    bcs .off
    adc.w #!VIEW_HEIGHT
    sec
    cmp.w !point_y
    bcc .off
.on:
    rts
.off:
    sep.b #$30
    pla
    pla
    stz.w !tile_behaviour_byte
    stz.w !tile_kind
    lda.b #0
    tax
    rts

; @routine 81:B82B TileAtProbeLong
; WHAT IT DOES: TileAtProbe for callers in other banks.
; HOW: TileAtProbe.
; Entry: as TileAtProbe; called by JSL. Exit: as TileAt.
TileAtProbeLong:
    jsr.w TileAtProbe
    rtl

; @routine 81:B82F TileAtProbe
; WHAT IT DOES: The map tile at the probe point, from 8-bit code.
; HOW: 16-bit registers, then TileAt.
; Entry: D = the actor; DB = $81; called by JSR. Exit: as TileAt.
TileAtProbe:
    rep.b #$30
; @routine 81:B831 TileAt
; WHAT IT DOES: The map tile at the probe point: its word, behaviour and
; kind (for the player, an empty tile off the view).
; HOW: ProbeOnView; the point's screen (x / 256 and y / 256, each plus
; one, place it in the 32-wide screen map) gives the screen's tiles
; offset, a negative one taken as 0; the point's 16-pixel tile place in the
; screen; the tile's top y kept as the surface; then TileAtPlace.
; Entry: M 16-bit, X 16-bit; D = the actor; DB = $81; called by JSR.
; Exit: as TileAtPlace, or as ProbeOnView off the view.
TileAt:
    jsr.w ProbeOnView
    lda.w !point_x
    xba
    inc a
    asl a
    and.w #$003E
    sta.w !screen_place
    lda.w !point_y
    and.w #$FFF0
    sta.w !surface_y
    xba
    inc a
    asl a
    asl a
    asl a
    asl a
    asl a
    asl a
    and.w #$03C0
    ora.w !screen_place
    sta.w !screen_place
    tax
    lda.l !screen_map,x
    bpl .screen
    lda.w #0
.screen:
    sta.w !screen_tiles
    lda.w !point_x
    lsr a
    lsr a
    lsr a
    and.w #$001E
    sta.w !tile_place
    lda.w !point_y
    asl a
    and.w #$01E0
    ora.w !tile_place
    sta.w !tile_place
; @routine 81:B880 TileAtPlace
; WHAT IT DOES: The tile at a place in a screen: its word, behaviour and
; kind.
; HOW: A (the tile's place) plus the screen's tiles offset indexes the
; level's tiles; the tile number (word / 8) indexes the behaviour table;
; the behaviour's bit 7 is kept as mirrored, its low 3 bits pick the kind.
; Entry: M 16-bit, X 16-bit; A = the tile's place (byte offset in the
; screen); DB = $81; called by JSR. Exit: M and X 8-bit; A low byte = X =
; the kind; N, Z from it; C = bit 2 of the tile word; Y, V unchanged.
TileAtPlace:
    clc
    adc.w !screen_tiles
    tax
    lda.l !level_tiles,x
    sta.w !tile_word
    lsr a
    lsr a
    lsr a
    tax
    sep.b #$20
    lda.l !tile_behaviour,x
    sta.w !tile_behaviour_byte
    pha
    and.b #$80
    sta.w !tile_mirrored
    sep.b #$10
    pla
    and.b #$07
    tax
    lda.w !tile_kinds,x
    sta.w !tile_kind
    tax
    rts

; @routine 81:B8AD SurfaceHeightSolid
; WHAT IT DOES: The surface's y for a tile of kind A, a solid tile (kind
; 1) taken as flat at its top.
; HOW: Kind 1 becomes $70 (flat, height 0); then SurfaceHeight.
; Entry: as SurfaceHeight. Exit: as SurfaceHeight.
SurfaceHeightSolid:
    cmp.b #!KIND_SOLID
    bne SurfaceHeight
    lda.b #!SLOPE_FLAT
    tax
    bra SurfaceHeight

; @routine 81:B8B6 SurfaceHeightOver
; WHAT IT DOES: The surface's y for a tile of kind A; for a solid tile
; (kind 1), the surface of the tile above it, if any.
; HOW: Kind 1: the surface moves up a tile and the tile place a row; a
; place past the screen's top takes the screen above's tiles; then
; TileAtPlace: an empty tile above returns Z set; else SurfaceHeight with
; its kind.
; Entry: M 8-bit, X 8-bit; A = X = the kind; DB = $81; called by JSR.
; Exit: as SurfaceHeight; or, the tile above empty, A low byte = X = 0,
; Z set, M and X 8-bit.
SurfaceHeightOver:
    cmp.b #!KIND_SOLID
    bne SurfaceHeight
    rep.b #$30
    sec
    lda.w !surface_y
    sbc.w #16
    sta.w !surface_y
    sec
    lda.w !tile_place
    sbc.w #32
    and.w #$01FE
    cmp.w #$01E0
    bcc .tile
    pha
    sec
    lda.w !screen_place
    sbc.w #64
    tax
    lda.l !screen_map,x
    sta.w !screen_tiles
    pla
.tile:
    jsr.w TileAtPlace
    beq SurfaceHeight_done

; @routine 81:B8EB SurfaceHeight
; WHAT IT DOES: The y of a tile's surface at the point's x, for a tile of
; kind A.
; HOW: A flat kind ($70 in bits 4-6) puts the surface its low nibble
; below the tile's top. A slope's surface drops its low nibble less the
; point's x in the tile (mirrored for a mirrored tile) shifted right by
; bits 4-6. The result is ORed into the surface y (the tile's top).
; Entry: M 8-bit, X 8-bit; A = X = the kind; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: A low byte = 1; N, Z clear; X unchanged;
; flat: Y unchanged, C set; slope: Y = 0, C from the SBC; mirrored byte
; shifted left.
%at($81B8EB)
SurfaceHeight:
    and.b #$70
    cmp.b #!SLOPE_FLAT
    bne .slope
    txa
    and.b #$0F
    ora.w !surface_y
    sta.w !surface_y
    lda.b #1
.done:
    rts
.slope:
    txa
    lsr a
    lsr a
    lsr a
    lsr a
    tay
    lda.w !point_x
    and.b #$0F
    asl.w !tile_mirrored
    bcc .shift
    eor.b #$0F
.shift:
    cpy.b #0
    beq .drop
.halve:
    lsr a
    dey
    bne .halve
.drop:
    sta.w !slope_drop
    txa
    and.b #$0F
    sec
    sbc.w !slope_drop
    ora.w !surface_y
    sta.w !surface_y
    lda.b #1
    rts

; Per behaviour's low 3 bits: nonzero, a wall the actor is pushed out of
; (wall_kinds); nonzero, a floor it settles on (floor_kinds).
!wall_kinds = $81BF0A
!floor_kinds = $81BF12
!BEHAVIOUR_THROUGH = $40

; @routine 81:B59F ActorOutOfWalls
; WHAT IT DOES: Moves the actor out of a wall in front of it, testing
; probes $0C, 8 and 4 until one moves it.
; HOW: ActorOutOfWall at probes $0C and 8 while it returns C clear, then
; ActorOutOfWallProbe4.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as the
; last ActorOutOfWall.
%at($81B59F)
ActorOutOfWalls:
    lda.b #$0C
    jsr.w ActorOutOfWall
    bcs ActorOutOfWallProbe4_done
    lda.b #8
    jsr.w ActorOutOfWall
    bcs ActorOutOfWallProbe4_done
; @routine 81:B5AD ActorOutOfWallProbe4
; WHAT IT DOES: Moves the actor out of a wall at probe 4 in front of it.
; HOW: ActorOutOfWall at probe 4.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
; ActorOutOfWall.
ActorOutOfWallProbe4:
    lda.b #4
    jsr.w ActorOutOfWall
.done:
    rtl

; @routine 81:B5B3 GroundBehindBit3
; WHAT IT DOES: Tests the ground behind the actor at probes $10 and then
; $14, until a tile with behaviour bit 3.
; HOW: GroundBehind at probe $10; without bit 3, again at probe $14.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
; GroundBehind, but A low byte = the last tile's behaviour bit 3, N, Z
; from it.
GroundBehindBit3:
    lda.b #$10
    sta.b !actor_probe_at
    jsr.w GroundBehind
    lda.w !tile_behaviour_byte
    and.b #$08
    bne .done
    lda.b #$14
    sta.b !actor_probe_at
    jsr.w GroundBehind
    lda.w !tile_behaviour_byte
    and.b #$08
    bne .done
.done:
    rtl

; @routine 81:B5F6 GroundBehindBit4
; WHAT IT DOES: Tests the ground behind the actor, and moves the tile
; behaviour's bit 4 into bit 5 when it is set.
; HOW: GroundBehind; then the behaviour byte, when bit 4 is set, shifts
; left.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
; GroundBehind, but A low byte = the behaviour's bit 4 and N, Z from it;
; C its bit 7 when shifted.
%at($81B5F6)
GroundBehindBit4:
    jsr.w GroundBehind
    lda.w !tile_behaviour_byte
    and.b #$10
    beq .done
    asl.w !tile_behaviour_byte
.done:
    rtl

; @routine 81:B608 ActorOutOfWallLong
; WHAT IT DOES: ActorOutOfWall for callers in other banks.
; HOW: ActorOutOfWall.
; Entry: as ActorOutOfWall; called by JSL. Exit: as ActorOutOfWall.
%at($81B608)
ActorOutOfWallLong:
    jsr.w ActorOutOfWall
    rtl

; @routine 81:B60C ActorOutOfWall
; WHAT IT DOES: When the probe in front of the actor is in a wall, moves
; the actor out of it: C set when it was moved.
; HOW: GroundInFront at probe A; a tile whose kind is a wall and that the
; actor cannot pass through moves the actor's x by the point's distance
; past the tile's near edge: back by (x in tile) + 1 in its left half,
; on by 16 - (x in tile) in its right half.
; Entry: M 8-bit; D = the actor; A low byte = the probe's offset; DB =
; $81; called by JSR. Exit: moved: M 8-bit, C set; A = the new x (16
; bits); N, Z from its ADC. Not: C clear; A, N, Z as the tests left them.
ActorOutOfWall:
    sta.b !actor_probe_at
    jsr.w GroundInFront
    bcc .none
    lda.w !tile_behaviour_byte
    and.b #$07
    tax
    lda.w !wall_kinds,x
    beq .none
    lda.b #!BEHAVIOUR_THROUGH
    bit.w !tile_behaviour_byte
    bne .none
    rep.b #$20
    lda.w !point_x
    and.w #$000F
    bit.w #$0008
    bne .right
    inc a
    bra .move
.right:
    sec
    sbc.w #16
.move:
    eor.w #$FFFF
    inc a
    clc
    adc.b !actor_x+1
    sta.b !actor_x+1
    sep.b #$21
    rts
.none:
    clc
    rts

; @routine 81:B673 ActorSettleOnFloor
; WHAT IT DOES: When a floor tile is under the actor's left or right
; probe at offset 8, and the ground is under the actor itself, stops its
; fall and stands it on that ground's tile top.
; HOW: ProbeLeft at probe 8, then TileAtProbe: a floor kind (and not
; passed through) goes on; else the same at ProbeRight. A point above the
; map's top (negative y) gives up with A = 0. Then the tile under the
; actor's own x at that y: when it is ground, the vertical speed is
; cleared and y moves down to the next tile boundary (16 less the
; point's y in the tile).
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSL.
; Exit: M 8-bit; X as the last test leaves it (8-bit, or 16-bit when the
; point was above the top). Above the top or a pass-through or kind with
; bit 7: A low byte = 0, Z set. Settled: A = the new y (16 bits); N, V, Z,
; C from its ADC. Else: A, flags from the last test.
%at($81B673)
ActorSettleOnFloor:
    lda.b #8
    sta.b !actor_probe_at
    jsr.w ProbeLeft
    sep.b #$20
    ldx.w !point_y
    bmi .out
    jsr.w TileAtProbe
    beq .right
    bmi .right
    lda.w !tile_behaviour_byte
    bit.b #!BEHAVIOUR_THROUGH
    bne .right
    and.b #$07
    tax
    lda.w !floor_kinds,x
    bne .under
.right:
    jsr.w ProbeRight
    sep.b #$20
    ldx.w !point_y
    bmi .out
    jsr.w TileAtProbe
    beq .done
    bmi .out
    lda.w !tile_behaviour_byte
    bit.b #!BEHAVIOUR_THROUGH
    bne .out
    and.b #$07
    tax
    lda.w !floor_kinds,x
    beq .done
.under:
    lda.b !actor_x+1
    sta.w !point_x
    lda.b !actor_x+2
    sta.w !point_x+1
    jsr.w TileAtProbe
    beq .done
    bmi .out
    stz.b !actor_speed_y+2
    rep.b #$20
    stz.b !actor_speed_y
    lda.w !point_y
    and.w #$000F
    eor.w #$FFFF
    inc a
    clc
    adc.w #16
    clc
    adc.b !actor_y+1
    sta.b !actor_y+1
    sep.b #$20
.done:
    rtl
.out:
    lda.b #0
    rtl

; @routine 81:B795 GroundInFrontLong
; WHAT IT DOES: GroundInFront for callers in other banks.
; HOW: GroundInFront.
; Entry: as GroundInFront; called by JSL. Exit: as PointInGround.
%at($81B795)
GroundInFrontLong:
    jsr.w GroundInFront
    rtl

; Per behaviour's low 3 bits: nonzero, ground the player does not stand
; firm on.
!soft_kinds = $81BF1A
!y_saved = !scratch+30          ; scratch $2A: the actor's y kept

; @routine 81:B44F ActorSnapToGround
; WHAT IT DOES: When there is ground just under the actor (within 3
; pixels), stands it on that ground: C set. Else leaves it where it was.
; HOW: Keeps the actor's y and moves it 3 down; tests probe 0 at the
; actor's x (the tile above for a solid tile), then to its left and right
; (a solid tile flat at its top); the first ground found moves the y by
; the surface less the point's y. With none, the kept y comes back.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSL.
; Exit: M and X 8-bit; on ground: C set, A = the new y (16 bits), N, V, Z
; from its ADC; none: Z set, A = the kept y (16 bits), N as it, C as the
; last test left it.
%at($81B44F)
ActorSnapToGround:
    rep.b #$20
    clc
    lda.b !actor_y+1
    sta.w !y_saved
    adc.w #3
    sta.b !actor_y+1
    sep.b #$20
    stz.b !actor_probe_at
    jsr.w ProbeAtActor
    jsr.w TileAt
    beq .left
    bmi .left
    jsr.w SurfaceHeightOver
    bne .ground
.left:
    jsr.w ProbeLeft
    jsr.w TileAt
    beq .right
    bmi .right
    jsr.w SurfaceHeightSolid
    bne .ground
.right:
    jsr.w ProbeRight
    jsr.w TileAt
    beq .none
    bmi .none
    jsr.w SurfaceHeightSolid
    beq .none
.ground:
    rep.b #$20
    sec
    lda.w !surface_y
    sbc.w !point_y
    clc
    adc.b !actor_y+1
    sta.b !actor_y+1
    sep.b #$31
    rtl
.none:
    rep.b #$20
    lda.w !y_saved
    sta.b !actor_y+1
    sep.b #$32
    rtl

; @routine 81:B4F3 ActorOutOfGround
; WHAT IT DOES: When the actor is in the ground at its probes, raises it
; onto the surface, unless that would leave it in the ground 15 pixels
; higher too, where it goes as far as that allows.
; HOW: Probe 0 at the actor's x (the tile above for a solid tile), then to
; its left and right (a solid tile flat at its top), stopping at the
; first tile; with none, Z set. When the point is at or below the surface
; the raised y is kept in scratch $2A, and the actor tested at 15 pixels
; above it: ground there with its surface above the point lowers the kept
; y by the difference. The actor takes the kept y.
; Entry: X 8-bit; D = the actor; DB = $81; called by JSL. Exit: M and X
; 8-bit; no tile, or the point above the surface: Z set; raised: A = the
; new y (16 bits), N, Z from it, C as the last test left it.
%at($81B4F3)
ActorOutOfGround:
    stz.b !actor_probe_at
    jsr.w ProbeAtActor
    jsr.w TileAtProbe
    beq .left
    bmi .left
    jsr.w SurfaceHeightOver
    bra .tile
.left:
    jsr.w ProbeLeft
    jsr.w TileAtProbe
    beq .right
    bmi .right
    jsr.w SurfaceHeightSolid
    bra .tile
.right:
    jsr.w ProbeRight
    jsr.w TileAtProbe
    beq .done
    bmi .none
    jsr.w SurfaceHeightSolid
.tile:
    rep.b #$30
    lda.w !point_y
    cmp.w !surface_y
    bcc .none
    sec
    lda.w !surface_y
    sbc.w !point_y
    clc
    adc.b !actor_y+1
    sta.w !y_saved
    clc
    adc.w #-15
    sta.b !actor_y+1
    sep.b #$30
    jsr.w ProbeAtActor
    jsr.w TileAtProbe
    beq .back
    bmi .back
    rep.b #$30
    sec
    lda.w !surface_y
    sbc.w !point_y
    bcs .back
    adc.b !actor_y+1
    sta.w !y_saved
.back:
    rep.b #$30
    lda.w !y_saved
    sta.b !actor_y+1
    sep.b #$30
.done:
    rtl
.none:
    sep.b #$32
    rtl

; @routine 81:B74E PlayerOnFirmGroundProbe8
; WHAT IT DOES: Says whether the player stands on firm ground at probe 8:
; C set when the ground to its left or right is solid and not soft.
; HOW: PlayerOnFirmGround's test with the probe offset 8.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSL. Exit: as
; PlayerOnFirmGround.
%at($81B74E)
PlayerOnFirmGroundProbe8:
    ldx.b #8
    bra PlayerOnFirmGroundAtX

; @routine 81:B756 PlayerOnFirmGround
; WHAT IT DOES: Says whether the player stands on firm ground at probe 0:
; C set when the ground to its left or right is solid and not soft.
; HOW: With the player's record as the direct page (its own kept on the
; stack): GroundLeft at probe 0, its behaviour not $20 or $40 and its
; kind not soft; else the same at GroundRight (behaviour not $40).
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSL. Exit: D as at entry,
; N and Z from it (PLD); C as above; M and X 8-bit; A, X, Y as the last
; test left them.
%at($81B756)
PlayerOnFirmGround:
    ldx.b #0
    bra PlayerOnFirmGroundAtX
; @routine 81:B75A PlayerOnFirmGroundProbe4
; WHAT IT DOES: Says whether the player stands on firm ground at probe 4:
; C set when the ground to its left or right is solid and not soft.
; HOW: PlayerOnFirmGround's test with the probe offset 4.
; Entry: M 8-bit, X 8-bit; DB = $81; called by JSL. Exit: as
; PlayerOnFirmGround.
PlayerOnFirmGroundProbe4:
    ldx.b #4
; The test the entries above share, at probe offset X.
PlayerOnFirmGroundAtX:
    phd
    rep.b #$20
    lda.w #!player
    tcd
    sep.b #$20
    stx.b !actor_probe_at
    jsr.w GroundLeft
    bcc .right
    lda.w !tile_behaviour_byte
    bit.b #$60
    bne .right
    and.b #$07
    tax
    lda.w !soft_kinds,x
    beq .firm
.right:
    jsr.w GroundRight
    bcc .soft
    lda.w !tile_behaviour_byte
    bit.b #!BEHAVIOUR_THROUGH
    bne .soft
    and.b #$07
    tax
    lda.w !soft_kinds,x
    bne .soft
.firm:
    pld
    sec
    rtl
.soft:
    pld
    clc
    rtl
