; kind41.asm: actor kind $41's start. With $07 bits 1-2 clear it is the
; walker: it rises out of the ground when Aladdin is in reach, starts the
; layer 1 wobble, hops twice and walks, hopping again where its ground
; ends. With them set it is a maker of walkers: one each time Aladdin is
; in reach (bits 3-4 clear), or one every so often at its own place.
; Its wait is Kind41Wait (kind_waits_rest.asm).
; C: src/aladdin/actor/kind41.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K41_LOOKS = $D29F              ; its look table
!K41_LOOK_AT = $16              ; where its look table is kept (word)
!K41_ACTIVE = $42               ; nonzero: the walker (Kind41Wait: it turns shots back, Aladdin can stand on it)
!K41_FRAMES = $43               ; Kind41Wait: frames before he can stand on it again
!K41_PAUSE = $41                ; the maker: frames between its walkers
!K41_PAUSES = $D2A7             ; bank $80: the maker's pause by $07 bits 1-2
!K41_SPAWN_ATS = $D2AE          ; bank $80: the timed maker's spawn point by $07 bits 3-4
!K41_SPAWN_LOOK = $4D           ; ActorSpawnLookWait's look: its own kind
!K41_X_LIMIT = $48              ; a walker the timed maker makes: the x (word) its side is judged by
!K41_Y_LIMIT = $4A              ; and the y (word) below which it may end
!K41_REACH_SPAWN = $2E          ; the maker in reach: its spawn point
!K41_SRAM = $701200             ; cartridge RAM (none on a retail cartridge): the animation it picks
!K41_HOP_SOUND = $1B            ; the sound of each hop
!K41_WOBBLE_SLOT = 4*24         ; TaskCreate's slot for the wobble
!K41_WOBBLE_PARAM = 4           ; and the wobble's parameter

; @routine 83:B201 Kind41Start
; @actor
; WHAT IT DOES: Actor kind $41. The walker ($07 bits 1-2 clear): faces by
; $07 bit 0, rises out of the ground when Aladdin is in reach, starts the
; layer 1 wobble (Bg1WobbleTask), hops twice with sound $1B, then walks,
; and where it finds no ground under it hops again. The maker (bits 1-2
; set): with bits 3-4 clear, each time Aladdin is in reach (entry 5) it
; makes one more of its kind ($07 AND $19, so a walker), then pauses; with
; them set it makes one every pause at its spawn point and gives it its
; own x and y as that walker's limits.
; HOW: $08 bit 1, $09 bits 7 and 4 set; $16 K41_LOOKS; K41_ACTIVE and
; K41_FRAMES 0. The walker: facing and flip from $07 bit 0, K41_ACTIVE 1;
; AnimStart with (K41_SRAM less itself after two DECs: 0 with no cartridge
; RAM); ActorRiseWhenInReach entry 0; TaskCreate (slot 4, parameter 4,
; Bg1WobbleTask); SoundQueueAdd $1B, ActorHopFromTable 0, the sound, hop
; 5, the sound; AnimStart 1; ActorSpeedYSet $1E. Each frame (BRK first):
; ActorMoveX, ActorOutOfWallProbe4 (C set: LDA $14, EOR 1, LDA $14, LDA
; $15, which changes nothing but the flags), ActorSnapToGround; C clear:
; ActorHopFromTable $1E and the sound. The maker: facing and flip 0;
; K41_PAUSE from K41_PAUSES by $07 bits 1-2; $07 AND $19; $08 bit 3
; cleared; K41_SPAWN_LOOK its kind. Bits 3-4 clear: $21 $2E; each frame
; (BRK first) PlayerInReach entry 5; in reach: ActorSpawnLookWait, COP
; K41_PAUSE. Set: $21 from K41_SPAWN_ATS; each turn (BRK first)
; ActorSpawnLookWait, the record it took (the free list's entry above its
; top) given K41_X_LIMIT and K41_Y_LIMIT its x and y, COP K41_PAUSE.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $41 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83B201)
Kind41Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #!K41_LOOKS
    sta.b !K41_LOOK_AT
    sep.b #$20
    stz.b !K41_ACTIVE
    stz.b !K41_FRAMES
    lda.b !actor_07
    bit.b #$06
    bne .maker
    lda.b !actor_07
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    inc.b !K41_ACTIVE
    jmp.w .walker
.maker:
    stz.b !actor_facing
    stz.b !actor_flip
    tay
    lsr a
    and.b #$03
    tax
    lda.w !K41_PAUSES,x
    sta.b !K41_PAUSE
    tya
    and.b #$19
    sta.b !actor_07
    lda.b !actor_08
    and.b #$F7
    sta.b !actor_08
    lda.b !actor_06
    sta.b !K41_SPAWN_LOOK
    tya
    and.b #$18
    bne .timed
    lda.b #!K41_REACH_SPAWN
    sta.b !actor_spawn_at
.reach:
    brk.b #$00
    ldx.b #$05
    jsr.w PlayerInReach
    sep.b #$20
    bcs .reach
    jsr.w ActorSpawnLookWait
    lda.b !K41_PAUSE
    cop.b #$00
    bra .reach
.timed:
    lsr a
    lsr a
    lsr a
    tax
    lda.w !K41_SPAWN_ATS,x
    sta.b !actor_spawn_at
.turn:
    brk.b #$00
    jsr.w ActorSpawnLookWait
    ldy.w !actor_free_top
    iny
    iny
    rep.b #$30
    ldx.w !actor_free_list,y
    lda.b !actor_x+1
    sta.w !K41_X_LIMIT,x
    lda.b !actor_y+1
    sta.w !K41_Y_LIMIT,x
    sep.b #$30
    lda.b !K41_PAUSE
    cop.b #$00
    bra .turn
.walker:
    lda.l !K41_SRAM
    pha
    dec a
    dec a
    sta.l !K41_SRAM
    pla
    sec
    sbc.l !K41_SRAM
    jsl.l AnimStart
    ldx.b #$00
    jsr.w ActorRiseWhenInReach
    jsl.l TaskCreate
    db !K41_WOBBLE_SLOT, !K41_WOBBLE_PARAM
    dl Bg1WobbleTask
    lda.b #!K41_HOP_SOUND
    jsl.l SoundQueueAdd
    ldx.b #$00
    jsr.w ActorHopFromTable
    lda.b #!K41_HOP_SOUND
    jsl.l SoundQueueAdd
    ldx.b #$05
    jsr.w ActorHopFromTable
    lda.b #!K41_HOP_SOUND
    jsl.l SoundQueueAdd
    lda.b #$01
    jsl.l AnimStart
    ldy.b #$1E
    jsl.l ActorSpeedYSet
.walk:
    brk.b #$00
    jsl.l ActorMoveX
    jsl.l ActorOutOfWallProbe4
    bcc .floor
    lda.b !actor_facing
    eor.b #$01
    lda.b !actor_facing
    lda.b !actor_flip
.floor:
    jsl.l ActorSnapToGround
    bcs .walk
    ldx.b #$1E
    jsr.w ActorHopFromTable
    lda.b #!K41_HOP_SOUND
    jsl.l SoundQueueAdd
    bra .walk
%ends_at($83B303)
