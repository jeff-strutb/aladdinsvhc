; level_actor_graphics.asm: the graphics of the actor kinds a level places:
; marked from its placement list, then unpacked into bank $7F. Drafted by
; cdraft.py; named and described by hand.
; C: src/aladdin/video/level_actor_graphics.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!PLACEMENTS = $8A7A  ; bank $82: by level, its placement list's offset (word); there a count, then 6-byte entries (+1 the kind, +2 bits 5-6 its variant)
!GFX_ENTRIES = $AC18  ; bank $82: per kind, its graphics entry's offset from GFX_PARTS (word)
!GFX_PARTS = $AC58  ; bank $82: per entry: length (low 14 bits; bits 14-15 the extra tile slots), ..., +5 by variant a byte
!KIND_GFX = $7FE500  ; per kind: where its graphics went (word), then three more tables to $7F:EAFF
!KIND_VARIANT_BYTE = $7FEA00  ; per kind: its variant's byte from its graphics entry, 0 until set
!VARIANT = $0E  ; the placement's variant (word)
!UNPACK_NEXT = $6B  ; where the next unpack goes in bank $7F (word)

; @routine 81:B9F4 LevelActorGraphicsLoad
; WHAT IT DOES: Loads the graphics of every actor kind the level places:
; the kinds are marked from the level's placement list, with each kind's
; extra tile slots and its variant's byte, then each marked kind's
; graphics are unpacked into bank $7F from $7000 on.
; HOW: With DB = $82: AnimSetsFill 0; $7F:E500-$7F:EAFF cleared. The
; level's list (PLACEMENTS by LEVEL): for each of its count's entries, the
; kind's flag in ANIM_SET_OF_TYPE = 1, its CELL_EXTRA_SLOTS = bits 6-7 of
; byte 1 of its GFX_PARTS entry, and its KIND_VARIANT_BYTE, while 0, = the
; entry's byte 5 + the variant (bits 5-6 of the placement's byte 2). Then
; UNPACK_NEXT = $7000 and AnimSetGraphicsUnpack for each kind 0-255
; marked.
; Entry: M 8-bit, X 8-bit; DB any; called by JSL (81:86BA). Exit: M and X
; 8-bit; X = 0; DB as at entry, N and Z from it (PLB); A, Y as the last
; unpack left them.
%at($81B9F4)
LevelActorGraphicsLoad:
    phb
    lda.b #$82
    pha
    plb
    lda.b #$00
    jsl.l AnimSetsFill
    rep.b #$10
    ldx.w #$05FF
    lda.b #$00
.lBA06:
    sta.l !KIND_GFX,x
    dex
    bpl .lBA06
    lda.b #$00
    xba
    lda.w !level
    asl a
    tax
    ldy.w !PLACEMENTS,x
    lda.w !PLACEMENTS,y
    beq .lBA85
    sta.w !scratch
.lBA20:
    lda.b #$00
    xba
    lda.w !PLACEMENTS+1,y
    tax
    lda.b #$01
    sta.l !anim_set_of_type,x
    lda.w !PLACEMENTS+2,y
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    and.b #$03
    sta.w !VARIANT
    stz.w !VARIANT+1
    phy
    phb
    lda.b #$82
    pha
    plb
    rep.b #$20
    txa
    and.w #$00FF
    asl a
    tay
    lda.w !GFX_ENTRIES,y
    tay
    sep.b #$20
    lda.w !GFX_PARTS+1,y
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta.l !cell_extra_slots,x
    lda.l !KIND_VARIANT_BYTE,x
    bne .lBA75
    rep.b #$20
    clc
    tya
    adc.w !VARIANT
    tay
    sep.b #$20
    lda.w !GFX_PARTS+5,y
    sta.l !KIND_VARIANT_BYTE,x
.lBA75:
    rep.b #$20
    plb
    pla
    clc
    adc.w #$0006
    tay
    sep.b #$20
    dec.w !scratch
    bne .lBA20
.lBA85:
    ldx.w #$7000
    stx.w !UNPACK_NEXT
    sep.b #$30
    ldx.b #$00
.lBA8F:
    lda.l !anim_set_of_type,x
    beq .lBA99
    txa
    jsr.w AnimSetGraphicsUnpack
.lBA99:
    inx
    bne .lBA8F
    plb
    rtl
