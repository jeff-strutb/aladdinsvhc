; tile_behaviour.asm: the behaviour of each map tile (solid, slope ...),
; read by the collision code from tile_behaviour.
; C: src/aladdin/map/tile_behaviour.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

; Two 4 KB behaviour tables: the first for map set $9C, the second for the
; others.
!tile_behaviour_sets = $82E000
!map_set = $005C                ; which map set the level uses
!MAP_SET_FIRST = $9C
!WORK_BANK_LOW = $7E

; @routine 81:BEC5 TileBehaviourLoad
; WHAT IT DOES: Loads the tile behaviour table of the level's map set.
; HOW: With the data bank at $7E, copies 4 KB from bank $82's tables into
; TILE_BEHAVIOUR, last byte first: from $82:E000 for map set $9C, from
; $82:F000 for the others.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: X and Y 8-bit = $FF; A
; low byte = the first byte copied; DB as at entry, N and Z from it (PLB);
; C from the map-set compare; V unchanged.
%at($81BEC5)
TileBehaviourLoad:
    rep.b #$10
    phb
    lda.b #!WORK_BANK_LOW
    pha
    plb
    ldy.w #4095
    ldx.w #4095
    lda.w !map_set
    cmp.b #!MAP_SET_FIRST
    beq .copy
    ldx.w #8191
.copy:
    lda.l !tile_behaviour_sets,x
    sta.w !tile_behaviour,y
    dex
    dey
    bpl .copy
    sep.b #$10
    plb
    rts
