; anim_sets.asm: which animation sets a level's actors need.
;
; Each actor type uses one animation set. Loading a level marks the sets
; of the types it has (the always-present types and the level's own) and
; lists each set once, so each is loaded once.
; C: src/aladdin/actor/anim_sets.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!NO_SET = $FF
; Each type's animation set, 256 bytes; the first 32 types are present in
; every level.
!type_anim_sets = $81EF6F
!ALWAYS_TYPES = 32
; Per level, the offset (from level_actors) of its actor list: a count,
; then 6-byte entries whose first byte is the type.
!level_actors = $828A7A
!LEVEL_ACTOR_SIZE = 6
!LEVEL_DATA_BANK = $82
!WORK_BANK_HIGH = $7F

; @routine 81:902C AnimSetsList
; WHAT IT DOES: Lists each animation set the level's types use, once.
; HOW: With the data bank at $7F: for each type with a set, looks for the
; set in the list; if it is not there, it takes the end's place, with a 0
; after it and a new end mark.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = 0; A low byte and Y
; as the last type's search leaves them; C from its last compare; DB as at
; entry, N and Z from it (PLB); V unchanged.
%at($81902C)
AnimSetsList:
    phb
    lda.b #!WORK_BANK_HIGH
    pha
    plb
    lda.b #!NO_SET
    sta.w !anim_sets_used
    ldx.b #0
.type:
    lda.w !anim_set_of_type,x
    cmp.b #!NO_SET
    beq .next
    ldy.b #0
.search:
    lda.w !anim_set_of_type,x
    cmp.w !anim_sets_used,y
    beq .next
    lda.w !anim_sets_used,y
    iny
    iny
    cmp.b #!NO_SET
    bne .search
    lda.w !anim_set_of_type,x
    dey
    dey
    sta.w !anim_sets_used,y
    iny
    lda.b #0
    sta.w !anim_sets_used,y
    iny
    lda.b #!NO_SET
    sta.w !anim_sets_used,y
.next:
    inx
    bne .type
    plb
    rts

; @routine 81:906B AnimSetsAlways
; WHAT IT DOES: Marks the animation sets of the 32 types every level has.
; HOW: Copies the first 32 bytes of the type table, last first.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: A low byte = type 0's set;
; X = $FF; N set, Z clear; Y, C, V unchanged.
AnimSetsAlways:
    ldx.b #!ALWAYS_TYPES-1
.type:
    lda.l !type_anim_sets,x
    sta.l !anim_set_of_type,x
    dex
    bpl .type
    rts

; @routine 81:9079 AnimSetsListEnd
; WHAT IT DOES: Finds the end of the level's list of animation sets.
; HOW: Steps through the list a pair at a time until the $FF end mark.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: X = the end mark's offset
; + 2 (twice the sets listed, + 2); A low byte = $FF; Z set, C set, N
; clear; Y, V unchanged.
AnimSetsListEnd:
    ldx.b #0
.pair:
    lda.l !anim_sets_used,x
    inx
    inx
    cmp.b #!NO_SET
    bne .pair
    rts

; @routine 81:9086 AnimSetsPlaces
; WHAT IT DOES: For each actor type, finds where its animation set is in
; the level's list of sets.
; HOW: With the data bank at $7F: AnimSetsListEnd gives the sets listed;
; for each of the 256 types the place is 4 (not listed) unless its set is
; found in the list, when it is the set's place (offset / 2); the count
; of sets left to look at is kept in scratch $0E.
; Entry: M 8-bit, X 8-bit; called by JSR. Exit: Y = 0; X as the last
; type's search left it; $0C = the sets listed; DB as at entry, N and Z
; from it (PLB).
AnimSetsPlaces:
    phb
    lda.b #!WORK_BANK_HIGH
    pha
    plb
    jsr.w AnimSetsListEnd
    txa
    lsr a
    dec a
    sta.w !scratch
    ldy.b #0
.type:
    lda.w !scratch
    sta.w !scratch+2
    lda.b #4
    sta.w !anim_set_place_of_type,y
    ldx.b #0
.search:
    lda.w !anim_set_of_type,y
    cmp.w !anim_sets_used,x
    bne .next
    txa
    lsr a
    sta.w !anim_set_place_of_type,y
    bra .found
.next:
    inx
    inx
    dec.w !scratch+2
    bne .search
.found:
    iny
    bne .type
    plb
    rts

; @routine 81:9D0C AnimSetsOfLevel
; WHAT IT DOES: Marks the animation sets of the actor types the level has.
; HOW: With the data bank at $82: the level's actor list; for each of its
; entries, the type's set is copied from the type table, counting in
; scratch $0C.
; Entry: M 8-bit, X 8-bit. Exit: M and X 8-bit; DB as at entry, N and Z
; from it (PLB); with entries, A = the entry offset past the last (low
; byte; C, V from its ADC), X = the last type, Y = the low byte of that
; offset; with none, A = 0, Y = the list's offset, X, C, V unchanged.
%at($819D0C)
AnimSetsOfLevel:
    phb
    lda.b #!LEVEL_DATA_BANK
    pha
    plb
    rep.b #$30
    lda.w !level
    asl a
    tay
    lda.w !level_actors,y
    tay
    lda.w !level_actors,y
    and.w #$00FF
    beq .done
    sta.w !scratch
    iny
.actor:
    lda.w !level_actors,y
    and.w #$00FF
    tax
    sep.b #$20
    lda.l !type_anim_sets,x
    sta.l !anim_set_of_type,x
    rep.b #$20
    clc
    tya
    adc.w #!LEVEL_ACTOR_SIZE
    tay
    dec.w !scratch
    bne .actor
.done:
    sep.b #$30
    plb
    rtl

; @routine 81:9D4A AnimSetsClear
; WHAT IT DOES: Marks every actor type as needing no animation set.
; HOW: AnimSetsFill with $FF.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = $FF; X = 0; Z set, N clear;
; Y, C, V unchanged.
AnimSetsClear:
    lda.b #!NO_SET
; @routine 81:9D4C AnimSetsFill
; WHAT IT DOES: Sets every actor type's animation set to A.
; HOW: Stores A in all 256 bytes, the first, then the last down.
; Entry: M 8-bit, X 8-bit; A low byte = the value. Exit: X = 0; Z set,
; N clear; A, Y, C, V unchanged.
AnimSetsFill:
    ldx.b #0
.type:
    sta.l !anim_set_of_type,x
    dex
    bne .type
    rtl
