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
