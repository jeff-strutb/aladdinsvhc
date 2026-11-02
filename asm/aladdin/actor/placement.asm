; placement.asm: creating a level's placed actors as the camera comes near
; them.
;
; The level's placements are indexed in 32-pixel bands across and down
; (actor/placement_index.asm). Each time the camera has moved 32 pixels
; from the last band it placed from, the actors of the band now coming into
; reach are created, those within 384 pixels of the camera and not already
; placed or gone. A created actor of a kind gets its record from the free
; list and its kind's set-up (graphics: a tile slot, or the attributes of a
; slot already loaded).
; C: src/aladdin/actor/placement.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!band_count = $7EC100             ; per band down: placements in it
!band_first = $7EC200             ; per band down: the first placement's number
!band_runs = $7EC300              ; per band across: placements in it
!band_starts = $7EC400            ; per band across: where its run starts in band_order
!band_order = $7EC500             ; placement numbers, band by band
!placement_flags = $7EC600        ; per placement: bit 0 placed, bit 7 gone
!placements = $8A7A               ; placement entries (data bank $82): kind, x, y (words), from list_at
!list_at = $6D                    ; direct page: the level's placements (word)
!placed_band_x = $1BE7            ; the band across placed last (and the next, $1BE9)
!placed_band_y = $1BEB            ; the band down placed last (and the next, $1BED)
!placed_x = $1BEF                 ; the camera x the last band across was placed at (word)
!placed_y = $1BF1                 ; the camera y the last band down was placed at (word)
!band_steps = $82A794             ; per direction: the step (word): +32, then -32
!band_moves = $82A798             ; per direction: the band step (byte)
!dp_left = $0C                    ; placements left in the band
!dp_index = $0D                   ; the band run's index
!dp_entry = $0E                   ; the placement entry's offset (word)
!dp_entry_r = $10                 ; the row's entry offset (word)
!dp_number = $12                  ; the placement's number (word)
!dp_dir = $2A                     ; direction flag (word)
!kind_setups = $82AA81            ; per kind's set-up index, its set-up routine
!kind_setup_of = $82B000          ; per kind, its set-up index
!create_kind = $001C              ; the kind being created (word)
!create_record = $0020            ; its record (word)
!create_kind2 = $0022             ; the kind * 2 (word)
!cell_flags = $7FE701             ; per cell (kind * 2): bit 7: graphics loaded
!cell_slot = $7FEB00              ; per cell: the tile slot it holds
!cell_gfx = $7FE500               ; per cell: its graphics (word)
!slot_vram = $82AC18              ; per slot: VRAM address
!slot_sizes = $82AC58             ; per slot: transfer size
!slot_attrs = $82AC06             ; per slot: sprite attributes (word)
!uploads_this_frame = $0403       ; uploads queued this frame (the NMI clears it)

; @routine 82:A79C PlacementsScroll
; WHAT IT DOES: Creates the placed actors the camera's move brings into
; reach: a band of placements across, or down, each time the camera is 32
; pixels or more from where the last band was placed.
; HOW: For x, then y: the camera less the last placing point; 32 or more
; either way moves the point by 32 and the band numbers by one, and places
; the band ahead (PlacementsAcross, PlacementsDown), the one on the side it
; moved to.
; Entry: D = 0; DB any; called by JSL. Exit: M and X 8-bit; DB as at
; entry.
%at($82A79C)
PlacementsScroll:
    sep.b #$20
    phb
    lda.b #$82
    pha
    plb
    rep.b #$30
    ldx.w #$0000
    stz.b !dp_dir
    lda.w !layer0+!LAYER_SCROLL_X
    sec
    sbc.w !placed_x
    bpl .lA7BB
    inc.b !dp_dir
    inx
    inx
    eor.w #$FFFF
    inc a
.lA7BB:
    cmp.w #$0020
    bcc .lA7EE
    lda.w !placed_x
    sbc.w !band_steps,x
    sta.w !placed_x
    sep.b #$30
    lda.w !band_moves,x
    tay
    clc
    adc.w !placed_band_y
    sta.w !placed_band_y
    tya
    clc
    adc.w !placed_band_y+2
    sta.w !placed_band_y+2
    tax
    lda.b !dp_dir
    beq .lA7E8
    jsr.w PlacementsDown
    bra .lA7EE
.lA7E8:
    ldx.w !placed_band_y
    jsr.w PlacementsDown
.lA7EE:
    rep.b #$30
    ldx.w #$0000
    stz.b !dp_dir
    lda.w !layer0+!LAYER_SCROLL_Y
    sec
    sbc.w !placed_y
    bpl .lA806
    inc.b !dp_dir
    inx
    inx
    eor.w #$FFFF
    inc a
.lA806:
    cmp.w #$0020
    bcc .lA839
    lda.w !placed_y
    sbc.w !band_steps,x
    sta.w !placed_y
    sep.b #$30
    lda.w !band_moves,x
    tay
    clc
    adc.w !placed_band_x+2
    sta.w !placed_band_x+2
    tya
    clc
    adc.w !placed_band_x
    sta.w !placed_band_x
    tax
    lda.b !dp_dir
    beq .lA833
    jsr.w PlacementsAcross
    bra .lA839
.lA833:
    ldx.w !placed_band_x+2
    jsr.w PlacementsAcross
.lA839:
    sep.b #$30
    plb
    rtl
; The band empty (PlacementsAcross, PlacementsDown).
.none:
    rts


; @routine 82:A83E PlacementsAcross
; WHAT IT DOES: Creates the actors of band X across that are near the
; camera and not yet placed.
; HOW: The band's run in the placement order; each placement not placed
; or gone, whose x is within 384 pixels round the camera's band, is created
; (ActorCreate with its kind) and placed (ActorPlace).
; Entry: M 8-bit, X 8-bit; D = 0; DB = $82; X = the band; called by JSR.
; Exit: M and X 8-bit.
%at($82A83E)
PlacementsAcross:
    lda.l !band_runs,x
    beq PlacementsScroll_none
    sta.b !scratch
    lda.l !band_starts,x
    sta.b !dp_index
.lA84C:
    rep.b #$20
    ldx.b !dp_index
    lda.l !band_order,x
    and.w #$00FF
    sta.b !dp_number
    asl a
    adc.b !dp_number
    asl a
    adc.b !list_at
    sta.b !dp_entry
    inc.b !dp_number
    sep.b #$20
    ldx.b !dp_number
    lda.l !placement_flags,x
    and.b #$81
    bne .lA892
    rep.b #$31
    ldy.b !dp_entry
    lda.w !placements+2,y
    adc.w #$0040
    sec
    sbc.w !placed_x
    cmp.w #$0180
    bcs .lA892
    lda.w !placements,y
    sep.b #$10
    jsl.l ActorCreate
    bcs .lA892
    ldy.b !dp_entry
    jsr.w ActorPlace
.lA892:
    sep.b #$30
    inc.b !dp_index
    dec.b !scratch
    bne .lA84C
    sep.b #$30
    rts


; @routine 82:A89D PlacementsDown
; WHAT IT DOES: Creates the actors of band X down that are near the camera
; and not yet placed.
; HOW: The band's placements, from its first; each not placed or gone,
; whose y is within 384 pixels round the camera's band, is created
; (ActorCreate with its kind) and placed (ActorPlace).
; Entry: M 8-bit, X 8-bit; D = 0; DB = $82; X = the band; called by JSR.
; Exit: M and X 8-bit.
%at($82A89D)
PlacementsDown:
    lda.l !band_count,x
    beq PlacementsScroll_none
    sta.b !scratch
    stz.b !dp_index
    rep.b #$20
    lda.l !band_first,x
    and.w #$00FF
    sta.b !dp_number
    asl a
    adc.b !dp_number
    asl a
    adc.b !list_at
    sta.b !dp_entry_r
    inc.b !dp_number
.lA8BC:
    sep.b #$30
    ldx.b !dp_number
    lda.l !placement_flags,x
    and.b #$81
    bne .lA8EF
    rep.b #$31
    ldy.b !dp_entry_r
    lda.w !placements+4,y
    adc.w #$0040
    sec
    sbc.w !placed_y
    cmp.w #$0180
    bcs .lA8EF
    sep.b #$20
    ldy.b !dp_entry_r
    lda.w !placements,y
    sep.b #$10
    jsl.l ActorCreate
    bcs .lA8EF
    ldy.b !dp_entry_r
    jsr.w ActorPlace
.lA8EF:
    rep.b #$21
    lda.b !dp_entry_r
    adc.w #!actor_06
    sta.b !dp_entry_r
    inc.b !dp_number
    dec.b !scratch
    beq .lA903
    bmi .lA903
    jmp.w .lA8BC
.lA903:
    sep.b #$30
    rts


; @routine 82:AA2B ActorCreate
; WHAT IT DOES: Creates an actor of kind A, if a record is free and its
; kind's set-up succeeds: C clear.
; HOW: The free list's top; none free: C set, A = 1. Else ActorCreateIn.
; Entry: M 8-bit, X 8-bit; A = the kind; DB any; called by JSL. Exit: as
; ActorCreateIn; none free: M and X 8-bit, C set, A low byte = 1.
%at($82AA2B)
ActorCreate:
    ldy.w !actor_free_top
    bmi ActorCreateIn_lAA5C


; @routine 82:AA30 ActorCreateIn
; WHAT IT DOES: Creates an actor of kind A in the free list's top record.
; HOW: Keeps the kind; the record's $06 = the kind; the kind's set-up
; (ActorKindSetup); on success the record leaves the free list and the
; kind's count of live actors steps up.
; Entry: A = the kind; Y = the free list's top offset; DB = $82; falls
; in from ActorCreate. Exit: created: C clear, X 16-bit = the record.
; Failed: M and X 8-bit, C set, A low byte = 1.
%at($82AA30)
ActorCreateIn:
    sta.w !create_kind
    stz.w !create_kind+1
    rep.b #$10
    ldx.w !actor_free_list,y
    sta.w !actor_06,x
    stx.w !create_record
    jsr.w ActorKindSetup
    bcs .lAA5C
    dec.w !actor_free_top
    dec.w !actor_free_top
    ldx.w !create_kind
    lda.l !kind_count,x
    inc a
    sta.l !kind_count,x
    ldx.w !create_record
    rtl
.lAA5C:
    sep.b #$31
    lda.b #$01
    rtl


; @routine 82:AA61 ActorKindSetup
; WHAT IT DOES: Runs the set-up of actor kind A for its new record.
; HOW: Keeps the kind * 2; with the data bank at $82, the kind's set-up
; index picks its routine from the set-up table.
; Entry: A = the kind; called by JSR. Exit: M 8-bit, X 16-bit; C and the
; rest as the set-up leaves them (C set: it failed).
%at($82AA61)
ActorKindSetup:
    sep.b #$30
    tay
    sta.w !create_kind2
    stz.w !create_kind2+1
    asl.w !create_kind2
    rol.w !create_kind2+1
    phb
    lda.b #$82
    pha
    plb
    ldx.w !kind_setup_of,y
    jsr.w (!kind_setups,x)
    plb
    sep.b #$20
    rep.b #$10
    rts


; @routine 82:AA89 ActorKindTilesShared
; WHAT IT DOES: Gives a new actor its kind's graphics: the slot already
; loaded for the kind, or a new tile slot (one, two or three slots).
; HOW: Graphics loaded (the cell's flag): the record takes the cell's
; sprite attributes and slot, whose holders step up. Else claims one slot
; (TileSlotClaim), two (TileSlotsClaim2) or three (TileSlotsClaim3) as the
; cell needs, then queues their graphics (ActorKindTilesQueue).
; Entry: M 8-bit; scratch $1C = the kind, $20 = the record, $22 = the
; kind * 2; called by JSR. Exit: C clear when given, C set when no slot was
; free.
%at($82AA89)
ActorKindTilesShared:
    rep.b #$10
    ldy.w !create_record
    ldx.w !create_kind2
    lda.l !cell_flags,x
    bmi .lAABC
    ldx.w !create_kind
    lda.l !cell_extra_slots,x
    beq .lAAB1
    dec a
    beq .lAAAA
    jsr.w TileSlotsClaim3
    bcs .lAABB
    bra .lAAB6
.lAAAA:
    jsr.w TileSlotsClaim2
    bcs .lAABB
    bra .lAAB6
.lAAB1:
    jsr.w TileSlotClaim
    bcs .lAABB
.lAAB6:
    jsr.w ActorKindTilesQueue
.given:
    clc
    rts
.lAABB:
    rts
.lAABC:
    lda.l !cell_attrs,x
    sta.w !actor_look,y
    lda.l !cell_flags,x
    sta.w !actor_look+1,y
    lda.b #$00
    xba
    ldx.w !create_kind
    lda.l !cell_slot,x
    sta.w !actor_tile_slot,y
    tax
    inc.w !actor_tile_slot_users,x
    clc
    rts


; @routine 82:AAE7 ActorKindTilesQueue
; WHAT IT DOES: Queues a kind's graphics for the NMI to upload to the
; actor's tile slot, and fills the slot's record (TileSlotFill).
; HOW: A VRAM queue record: the slot's VRAM address from the cell's
; attributes, the graphics' source and bank $7F, the slot's size; a zero
; size is not queued. Kind $33 (X = $66) only fills.
; Entry: M 8-bit, X 16-bit; X = the kind * 2; called by JSR. Exit: as
; TileSlotFill; C clear.
%at($82AAE7)
ActorKindTilesQueue:
    rep.b #$20
    cpx.w #$0066
    beq .fill_only
    lda.w !vram_queue_count
    and.w #$00FF
    asl a
    asl a
    asl a
    tay
    lda.l !cell_attrs,x
    and.w #$01FF
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w #$6000
    sta.w !vram_queue,y
    lda.l !cell_gfx,x
    sta.w !vram_queue+2,y
    lda.w #$007F
    sta.w !vram_queue+4,y
    lda.l !slot_vram,x
    tax
    lda.l !slot_sizes,x
    and.w #$3FFF
    sta.w !vram_queue+5,y
    sep.b #$20
    beq ActorKindTilesShared_given
    lda.b #$80
    sta.w !vram_queue+7,y
    sep.b #$10
    inc.w !vram_queue_count
    inc.w !uploads_this_frame
.fill:
    jsr.w TileSlotFill
    rts
.fill_only:
    sep.b #$30
    bra .fill

; @routine 82:AB3F ActorKindTiles
; WHAT IT DOES: Gives a new actor a new tile slot (one, two or three) for
; its kind's graphics.
; HOW: As the cell needs: TileSlotClaim, TileSlotsClaim2 or
; TileSlotsClaim3; then TileSlotFill.
; Entry: scratch $1C = the kind, $20 = the record; called by JSR. Exit: C
; clear when given; C set when no slot was free.
%at($82AB3F)
ActorKindTiles:
    rep.b #$10
    ldy.w !create_record
    ldx.w !create_kind
    lda.l !cell_extra_slots,x
    beq .one
    dec a
    beq .two
    jsr.w TileSlotsClaim3
    bcs .none
    jsr.w TileSlotFill
    clc
    rts
.two:
    jsr.w TileSlotsClaim2
    bcs .none
    jsr.w TileSlotFill
    clc
    rts
.one:
    jsr.w TileSlotClaim
    bcs .none
    jsr.w TileSlotFill
    clc
    rts
.none:
    rts

; @routine 82:AC10 TileSlotsClaim2
; WHAT IT DOES: Claims two neighbouring tile slots for a kind's graphics,
; if two are free: C clear.
; HOW: The first pair of free neighbours among slots 1-4; its second
; slot's holders step up, then TileSlotClaim's steps with the first.
; Entry: M 8-bit, X 16-bit; scratch $1C = the kind, $20 = the record;
; called by JSR. Exit: C set when no pair was free; else as TileSlotClaim.
%at($82AC10)
TileSlotsClaim2:
    ldx.w #$0000
    lda.w !actor_tile_slot_users+1,x
    ora.w !actor_tile_slot_users+2,x
    beq .free
    inx
    lda.w !actor_tile_slot_users+1,x
    ora.w !actor_tile_slot_users+2,x
    beq .free
    inx
    lda.w !actor_tile_slot_users+1,x
    ora.w !actor_tile_slot_users+2,x
    beq .free
    sec
    rts
.free:
    inc.w !actor_tile_slot_users+2,x
    bra TileSlotClaim_claim

; @routine 82:AC34 TileSlotsClaim3
; WHAT IT DOES: Claims three neighbouring tile slots for a kind's
; graphics, if three are free: C clear.
; HOW: Slots 1-3 or 2-4, the first free run; its second and third slots'
; holders step up, then TileSlotClaim's steps with the first.
; Entry: M 8-bit, X 16-bit; scratch $1C = the kind, $20 = the record;
; called by JSR. Exit: C set when no run was free; else as TileSlotClaim.
TileSlotsClaim3:
    ldx.w #$0000
    lda.w !actor_tile_slot_users+1
    ora.w !actor_tile_slot_users+2
    ora.w !actor_tile_slot_users+3
    beq .free
    inx
    lda.w !actor_tile_slot_users+2
    ora.w !actor_tile_slot_users+3
    ora.w !actor_tile_slot_users+4
    beq .free
    sec
    rts
.free:
    inc.w !actor_tile_slot_users+2,x
    inc.w !actor_tile_slot_users+3,x
    bra TileSlotClaim_claim

; @routine 81:A9A6 ActorCreateChild
; WHAT IT DOES: Creates an actor of kind A as a child of the actor,
; placed at its spawn point, if the kind can be created.
; HOW: ActorCreate; on success ActorSpawnInit for the new record.
; Entry: M 8-bit, X 8-bit; D = the spawner; A = the kind; DB = $81;
; called by JSL. Exit: C clear when created (X 16-bit = the record); else
; as ActorCreate.
%at($81A9A6)
ActorCreateChild:
    jsl.l ActorCreate
    bcs .lA9B0
    jsr.w ActorSpawnInit
    clc
.lA9B0:
    rtl


; @routine 81:A9B1 ActorCreateChild8
; WHAT IT DOES: ActorCreateChild, returning with 8-bit registers and A =
; 0 when created.
; HOW: ActorCreate; on success ActorSpawnInit, then M and X 8-bit and A
; = 0.
; Entry: as ActorCreateChild. Exit: created: M and X 8-bit, A low byte =
; 0, Z set; else as ActorCreate.
%at($81A9B1)
ActorCreateChild8:
    jsl.l ActorCreate
    bcs .lA9BE
    jsr.w ActorSpawnInit
    sep.b #$30
    lda.b #$00
.lA9BE:
    rtl


; @routine 82:A72C PlacementsStart
; WHAT IT DOES: Indexes the level's placements as it starts, sets the
; placing points from the camera, and creates the actors of the 12 bands
; down from 64 pixels left of the camera.
; HOW: With D = 0 and DB = $82: PlacementsIndex; the last placement's
; flags byte ($7E:C6FF) set to $FF; from the scroll y, placed_y and the
; bands (y - 64) / 32 and (y + 288) / 32 at placed_band_x; from the scroll
; x, placed_x and the bands (x + 288) / 32 and (x - 64) / 32 at
; placed_band_y; PlacementsDown for 12 bands from (x - 64) / 32.
; Entry: DB, D any; called by JSL. Exit: M 8-bit, X 8-bit; D and DB as at
; entry; Y = 0.
%at($82A72C)
PlacementsStart:
    sep.b #$30
    phd
    lda.b #$00
    pha
    pha
    pld
    phb
    lda.b #$82
    pha
    plb
    jsr.w PlacementsIndex
    lda.b #$FF
    sta.l !placement_flags+255
    rep.b #$21
    lda.w !layer0+!LAYER_SCROLL_Y
    pha
    sta.w !placed_y
    adc.w #$0120
    jsr.w ShiftRight5
    sta.w !placed_band_x+2
    pla
    sec
    sbc.w #$0040
    jsr.w ShiftRight5
    sta.w !placed_band_x
    lda.w !layer0+!LAYER_SCROLL_X
    pha
    sta.w !placed_x
    clc
    adc.w #$0120
    jsr.w ShiftRight5
    sta.w !placed_band_y
    pla
    sec
    sbc.w #$0040
    jsr.w ShiftRight5
    sta.w !placed_band_y+2
    sep.b #$30
    tax
    ldy.b #$0C
.band:
    phx
    phy
    jsr.w PlacementsDown
    ply
    plx
    inx
    dey
    bne .band
    plb
    pld
    rtl
