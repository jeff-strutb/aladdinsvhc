; frame_step.asm: one frame of play: the actors' turns (the actor loop),
; then the scroll, the map's tile changes, the play time, the palette and
; sequence steps, and the sprites, with the frame's queues marked ready.
; C: src/aladdin/game/frame_step.c.

incsrc "../include/macros.inc"

arch 65816

!PLAYER_X = $08E6               ; the player's x: fraction, then pixels (3 bytes)
!PLAYER_Y = $08E9               ; and its y
!FRAME_START_X = $143A          ; the player's x as the frame began (3 bytes)
!FRAME_START_Y = $143D          ; and its y
!TILE_BLOCKS_LENGTH = $1BE4     ; the tile block queue's length in bytes (word)
!TILE_BLOCKS_PENDING = $1BE6    ; nonzero: the block queue is ready
!OAM_0857 = $0857               ; cleared, then counted up, each frame; purpose not yet established
!VRAM_QUEUE_PENDING = $0858     ; nonzero: the VRAM queue is ready
!VIEW_X = $1773                 ; the view's left edge (pixels, word)
!VIEW_Y = $1777                 ; the view's top edge (pixels, word)
!RANGE_X = $1A44                ; the point actors are kept in range of (word)
!RANGE_Y = $1A46
!PAUSE_BLOCK = $0404            ; nonzero: no pause; with PAUSED, no sequence or palette steps
!PAUSED = $0405                 ; counted up while paused
!VIEW_HALF = $0080              ; the range point: the view's corner + this

; @routine 81:830D FrameStepLong
; WHAT IT DOES: One frame of play (FrameStep), for callers in other banks.
; HOW: FrameStep, then RTL.
; Entry: as FrameStep; called by JSL. Exit: as FrameStep.
%at($81830D)
FrameStepLong:
    jsr.w FrameStep
    rtl

; @routine 81:8311 FrameStep
; WHAT IT DOES: One frame of play: the player's turn and every other
; actor's (the actor loop), the scroll and the map, the play time, the
; sequence and palette cycle steps (not while paused or with PAUSE_BLOCK
; set), and the sprites; the frame's tile block and VRAM queues are then
; marked ready.
; HOW: The player's x and y (3 bytes each) kept in FRAME_START_X/Y;
; TILE_BLOCKS_LENGTH and OAM_0857 cleared. PlayerLoop, OamBuildBegin,
; ActorLoop, ScrollUpdate, TileChangesApply, LayersScroll,
; PlacementsScroll, PlayTimeTick. RANGE_X/Y = VIEW_X/Y + $80. PAUSED or
; PAUSE_BLOCK clear: SequencesStep, PaletteCyclesStep. ActorsInView,
; OamDrawActors; OAM_0857, TILE_BLOCKS_PENDING and VRAM_QUEUE_PENDING
; counted up; BgFollowLayers.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSR. Exit: M 8-bit; registers as BgFollowLayers leaves them.
FrameStep:
    rep.b #$20
    lda.w !PLAYER_X
    sta.w !FRAME_START_X
    lda.w !PLAYER_Y
    sta.w !FRAME_START_Y
    ldx.w !PLAYER_X+2
    stx.w !FRAME_START_X+2
    ldx.w !PLAYER_Y+2
    stx.w !FRAME_START_Y+2
    sep.b #$20
    stz.w !TILE_BLOCKS_LENGTH
    stz.w !TILE_BLOCKS_LENGTH+1
    stz.w !OAM_0857
    jsl.l PlayerLoop
    jsl.l OamBuildBegin
    jsl.l ActorLoop
    jsr.w ScrollUpdate
    jsl.l TileChangesApply
    jsr.w LayersScroll
    jsl.l PlacementsScroll
    jsr.w PlayTimeTick
    rep.b #$20
    clc
    lda.w !VIEW_X
    adc.w #!VIEW_HALF
    sta.w !RANGE_X
    clc
    lda.w !VIEW_Y
    adc.w #!VIEW_HALF
    sta.w !RANGE_Y
    sep.b #$20
    lda.w !PAUSED
    ora.w !PAUSE_BLOCK
    bne .in_view
    jsr.w SequencesStep
    jsr.w PaletteCyclesStep
.in_view:
    jsl.l ActorsInView
    jsl.l OamDrawActors
    inc.w !OAM_0857
    inc.w !TILE_BLOCKS_PENDING
    inc.w !VRAM_QUEUE_PENDING
    jsr.w BgFollowLayers
    rts
