; map_probe.asm: an actor looks at the level map around itself: the block
; a few pixels ahead of it along its facing and below it, and whether the
; ground between it and the player is clear to see along.
; C: src/aladdin/actor/map_probe.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

; The map test: the class of the level map's block at the point !scratch
; (x, word), !scratch+2 (y, word); A = X = it, 8-bit registers.
!MAP_BLOCK_AT = $81B82B
; Whether the player is within the actor's vertical reach (box X): C set
; when not.
!PLAYER_IN_REACH_Y = $81AEFD
; In bank $80 (DB), x offsets ahead of the actor: a word for facing
; right, then one for facing left, an entry each 4 bytes.
!AHEAD_OFFSETS = $D062
; In bank $80 (DB): 5 pixels ahead, facing right then left (words).
!AHEAD_5 = $D176
!steps_left = !actor_stream     ; $4A: here, the 16-pixel steps left to test

; @routine 83:9A01 ActorMapAhead7Down28
; WHAT IT DOES: Gives the class of the map's block 7 pixels ahead of the
; actor and 28 below it.
; HOW: Entry 4 of the ahead table and 28 below, then on into
; ActorMapAtOffset.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($839A01)
ActorMapAhead7Down28:
    lda.b #$1C
    sta.w !scratch+2
    lda.b #$04
    sta.w !scratch
    stz.w !scratch+3
    jmp.w ActorMapAtOffset

; @routine 83:9A15 ActorMapAtOffset
; WHAT IT DOES: Gives the class of the level map's block at a point ahead
; of the actor (along its facing) and below it.
; HOW: The x offset is the word of the ahead table (bank $80, $D062) by
; the entry in SCRATCH and the facing; it and the y offset in SCRATCH + 2
; are added to the actor's pixels, and the map test at $81:B82B looks the
; point up.
; Entry: M 8-bit, X 8-bit; SCRATCH (byte) = the entry's offset, SCRATCH + 2
; (word) = pixels below the actor; D = the actor; DB = $80. Exit: as the C states.
%at($839A15)
ActorMapAtOffset:
    lda.b !actor_facing
    asl a
    adc.w !scratch
    tax
    rep.b #$21
    lda.w !AHEAD_OFFSETS,x
    adc.b !actor_x+1
    sta.w !scratch
    lda.b !actor_y+1
    clc
    adc.w !scratch+2
    sta.w !scratch+2
    jsl.l !MAP_BLOCK_AT
    rts

; @routine 83:9A46 ActorSeesPlayer
; WHAT IT DOES: Tells whether the actor sees the player: C clear when the
; player is within its vertical reach and the map between them is clear,
; and then it turns to face the player.
; HOW: The reach test at $81:AEFD (box 0) first. Unless bit 4 of $07 is
; set, the map is tested every 16 pixels from the actor toward the
; player, at the actor's height (12 lower when bit 3 of $07 is clear),
; for as many steps as 16-pixel spans between them; a block there (or no
; span at all) and it does not see. Seen: ActorSideOfPlayer, as both its
; facing and its flip.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($839A46)
ActorSeesPlayer:
    ldx.b #$00
    jsl.l !PLAYER_IN_REACH_Y
    bcs .not_seen
    lda.b !actor_07
    and.b #$10
    bne .seen
    rep.b #$20
    ldx.b #$00
    lda.b !actor_x+1
    sec
    sbc.w !player+!actor_x+1
    bpl .distance
    eor.w #$FFFF
    inc a
    inx
.distance:
    lsr a
    lsr a
    lsr a
    lsr a
    beq .not_seen
    sta.b !steps_left
    lda.w #$FFF0
    cpx.b #$00
    beq .step_set
    lda.w #$0010
.step_set:
    sta.b !actor_48
    stz.b !actor_4c
.step:
    rep.b #$21
    lda.b !actor_4c
    adc.b !actor_48
    sta.b !actor_4c
    lda.b !actor_x+1
    clc
    adc.b !actor_4c
    sta.w !scratch
    lda.b !actor_y+1
    sta.w !scratch+2
    lda.b !actor_07
    and.w #$0008
    bne .test
    lda.b !actor_y+1
    clc
    adc.w #$000C
    sta.w !scratch+2
.test:
    jsl.l !MAP_BLOCK_AT
    bne .not_seen
    dec.b !steps_left
    bne .step
.seen:
    jsl.l ActorSideOfPlayer
    sta.b !actor_facing
    sta.b !actor_flip
    clc
    bra .out
.not_seen:
    sec
.out:
    sep.b #$20
    rts

; @routine 83:B5F6 ActorMapAhead5Down12
; WHAT IT DOES: Gives the class of the map's block 5 pixels ahead of the
; actor and 12 below it.
; HOW: As ActorMapAtOffset, with its own table of offsets (bank $80,
; $D176) and 12 below.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80. Exit: as the C states.
%at($83B5F6)
ActorMapAhead5Down12:
    lda.b #$0C
    sta.w !scratch+2
    stz.w !scratch+3
    lda.b !actor_facing
    asl a
    tax
    rep.b #$21
    lda.w !AHEAD_5,x
    adc.b !actor_x+1
    sta.w !scratch
    lda.b !actor_y+1
    clc
    adc.w !scratch+2
    sta.w !scratch+2
    jsl.l !MAP_BLOCK_AT
    rts
