; tile_slots.asm: four slots of animated tile graphics that map cells can
; claim; a claimed slot's graphics are copied into the slot's place in a
; work-RAM copy of the tiles, and counted for upload.
; C: src/aladdin/video/tile_slots.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!TILE_SLOT_UPLOADS = $0844      ; tiles copied, waiting for upload
!TILE_SLOT_USED = $15E9         ; 4 bytes: nonzero while a slot is claimed
!TILE_SLOT_CELL = $15ED         ; 4 bytes: the low byte of the cell that claimed it
!MAP_CELL = !scratch+16         ; word: the map cell worked on
!MAP_CELL_ATTR = !scratch+22    ; word: its offset in the attribute table
!COPY_COUNT = !scratch+30       ; word: words left to copy
!CELL_TILES = $7FE9FF           ; by cell, the word whose top 13 bits give the source tile
!CELL_ATTRS = $7FE700           ; by cell, attribute words: bits 9-11 a slot place
!CELL_SLOTS = $7FEB00           ; by cell, the slot (1-4) it holds
!SLOT_PLACES = $82ABBE          ; 8 words: where each place's tile goes in !TILE_COPY
!SLOT_WORDS = $AC06             ; in the bank DB names: a word per slot (1-4)
!TILE_SOURCE = $84B000          ; the tiles' graphics, 32 bytes each
!TILE_COPY = $7EFD00            ; the work-RAM copy the slots go into

; @routine 82:AB77 TileSlotFill
; WHAT IT DOES: Copies the cell's tile graphics (32 bytes) into its slot's
; place in the work-RAM tile copy, and counts one more for upload.
; HOW: The cell's tile word, shifted right 3 with its low 5 bits cleared,
; is the source's offset in $84:B000; the place comes from bits 9-11 of
; the cell's attribute word through the table at $82:ABBE. 16 words are
; copied with 16-bit registers, with DB $84 while they are.
; Entry: any widths; DB a bank whose $0000-$1FFF is work RAM. Exit: as the C states.
%at($82AB77)
TileSlotFill:
    rep.b #$30
    ldx.w !MAP_CELL
    lda.l !CELL_TILES,x
    ldx.w !MAP_CELL_ATTR
    lsr a
    lsr a
    lsr a
    and.w #$FFE0
    tay
    lda.w #$0010
    sta.w !COPY_COUNT
    lda.l !CELL_ATTRS,x
    and.w #$0E00
    xba
    tax
    lda.l !SLOT_PLACES,x
    tax
    sep.b #$20
    phb
    lda.b #!TILE_SOURCE>>16
    pha
    plb
    rep.b #$20
.copy:
    lda.w !TILE_SOURCE&65535,y
    sta.l !TILE_COPY,x
    iny
    iny
    inx
    inx
    dec.w !COPY_COUNT
    bne .copy
    sep.b #$30
    inc.w !TILE_SLOT_UPLOADS
    plb
    rts

; @routine 82:ABCE TileSlotClaim
; WHAT IT DOES: Claims a free animated-tile slot for the cell and the
; actor whose record Y points at; the carry is set when all four are
; taken.
; HOW: Slots are tried from 3 down to 0. The claimed one is marked used
; and keeps the cell's low byte; the slot's number (1-4) goes to the
; actor and to the cell, and the slot's word to the actor's word at $2B
; and to the cell's attribute word.
; Entry: M 8-bit, X 16-bit; Y = the actor; DB a bank whose $0000-$1FFF is
; work RAM, the slot words' bank. Exit: as the C states.
%at($82ABCE)
TileSlotClaim:
    ldx.w #3
.try:
    lda.w !TILE_SLOT_USED,x
    beq .claim
    dex
    bpl .try
    sec
    rts
.claim:
    lda.b #$00
    xba
    inc.w !TILE_SLOT_USED,x
    lda.w !MAP_CELL
    sta.w !TILE_SLOT_CELL,x
    inx
    txa
    sta.w !actor_tile_slot,y
    ldx.w !MAP_CELL
    sta.l !CELL_SLOTS,x
    rep.b #$20
    asl a
    tax
    lda.w !SLOT_WORDS,x
    sta.w !actor_2b,y
    ldx.w !MAP_CELL_ATTR
    sta.l !CELL_ATTRS,x
    sep.b #$20
    clc
    rts
