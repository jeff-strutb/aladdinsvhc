; and charges when it sees Aladdin.
;
; It walks its beat ahead of it (one pixel a frame) until the floor
; ends, a wall ($70) is ahead or it has gone its range; then stands,
; looks about (turning every 17 frames for 64) and walks on. Each frame
; it may see Aladdin (ActorSeesPlayer): then it braces and charges,
; speeding up and turning to keep after him, until the floor ahead ends;
; then it halts and looks about again. Its spotting first writes to
; cartridge RAM at $70:0023: a cartridge that keeps the byte (a copier)
; hangs the game there. Drafted by cdraft.py; named, completed and
; described by hand.
; C: src/aladdin/actor/kind20.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND20_16 = $16               ; word: $D05A
!KIND20_29 = $29               ; word: $890F
!K20_PLACE = $07               ; its placement's byte: bit 0 its facing, bit 3 the faster charge, bit 4 no patrol
!K20_BEAT_FROM = $40           ; its beat: where it starts (word) ...
!K20_BEAT_LENGTH = $46         ; ... and how far it walks (word)
!K20_COUNT = $42               ; frames left of a step
!K20_TURNS = $43               ; frames to the next turn while it looks about; while charging, the last surface's sound
!K20_X_PIXELS = $0C            ; its x, whole pixels (word)
!K20_SPED_UP = $4E             ; nonzero once its charge is up to speed
!K20_CHARGE_FRAMES = $4C       ; counts its charge's frames
!K20_SURFACE = $27             ; the class of the ground under it
!V_0013 = $0013
!K20_COPIER_TEST = $700023     ; cartridge RAM, which a retail cartridge has none of
!KIND20_NO_PATROL = $839AC9    ; its code for bit 4 of K20_PLACE, which no script reaches

; @routine 83:97AF Kind20Start
; WHAT IT DOES: Actor kind $20's start: a guard set up to walk its beat
; the way its placement faces.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; KIND20_16 $D05A,
; KIND20_29 $890F; with PLACE $1F, AnimStart 0, a 32-frame wait, then
; PLACE 3; with PLACE bit 4, on at NO_PATROL (neither is run by a script);
; Actor44SetBy07, AnimStart 2, facing and flip PLACE bit 0, ActorXTo40,
; ActorSpeedXSet1; on into Kind20Patrol at its BRK.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $20
; (the table at $83:8CAB). Exit: never returns.
%at($8397AF)
Kind20Start:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    rep.b #$20
    lda.w #$D05A
    sta.b !KIND20_16
    lda.w #$890F
    sta.b !KIND20_29
    sep.b #$20
    lda.b !K20_PLACE
    cmp.b #$1F
    bne .placed
    lda.b #$00                     ; (no script runs this)
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$03
    sta.b !K20_PLACE
.placed:
    lda.b !K20_PLACE
    bit.b #$10
    beq .patrols
    jmp.w !KIND20_NO_PATROL         ; (no script runs this)
.patrols:
    jsr.w Actor44SetBy07
    lda.b #$02
    jsl.l AnimStart
    lda.b !K20_PLACE
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    jsr.w ActorXTo40
    jsr.w ActorSpeedXSet1

; @routine 83:97F9 Kind20Patrol
; @actor
; WHAT IT DOES: The guard walks its beat, a frame at a time, until it
; sees Aladdin (Kind20Spotted) or must stop (Kind20LookAbout): the floor
; ends ahead, a wall ($70) is ahead, or it has walked BEAT_LENGTH from
; BEAT_FROM.
; HOW: BRK each frame; ActorSeesPlayer C clear: Kind20Spotted. Else
; ActorMoveX, ActorSnapToGround; on a ledge (V_0013 $0F, no script) the
; lower probe first; ActorMapAhead7Down28 or ActorTileAheadIs70 Z, or
; BEAT_FROM + ACTOR_44 - its x at least BEAT_LENGTH: Kind20LookAbout.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the JMP from Kind20LookAbout's end
; or from Kind20Start running on. Exit: never returns.
Kind20Patrol:
    brk.b #$00
    jsr.w ActorSeesPlayer
    bcs .walk
    jmp.w Kind20Spotted
.walk:
    jsl.l ActorMoveX
    jsl.l ActorSnapToGround
    lda.w !V_0013
    cmp.b #$0F
    bne .floor
    jsr.w ActorMapAhead7Down36      ; on a ledge (no script runs this)
    beq Kind20LookAbout
    bra .wall
.floor:
    jsr.w ActorMapAhead7Down28
    beq Kind20LookAbout
.wall:
    jsr.w ActorTileAheadIs70
    beq Kind20LookAbout
    rep.b #$21                     ; walked its beat?
    lda.b !K20_BEAT_FROM
    adc.b !actor_44
    sec
    sbc.b !K20_X_PIXELS
    cmp.b !K20_BEAT_LENGTH
    sep.b #$20
    bcc .again

; @routine 83:9832 Kind20LookAbout
; WHAT IT DOES: The guard stands and looks about: 16 frames still, 64
; turning round every 17, 16 still, then walks on the way it last
; looked; seeing Aladdin at any frame, Kind20Spotted.
; HOW: ActorOutOfWallProbe4; AnimStart 0, COUNT 16 frames (BRK, then
; ActorSeesPlayer C clear: Kind20Spotted); AnimStart 1, COUNT $40 and
; TURNS $11, its facing flipped each time TURNS runs out; AnimStart 0,
; 16 frames; AnimStart 2, its flip = its facing; Kind20Patrol.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by Kind20Patrol's jumps and by
; Kind20Halt's. Exit: never returns.
Kind20LookAbout:
    jsl.l ActorOutOfWallProbe4
    lda.b #$00
    jsl.l AnimStart
    lda.b #$10
    sta.b !K20_COUNT
.still:
    brk.b #$00
    jsr.w ActorSeesPlayer
    bcc Kind20Spotted
    dec.b !K20_COUNT
    bne .still
    lda.b #$01
    jsl.l AnimStart
    lda.b #$40
    sta.b !K20_COUNT
    lda.b #$11
    sta.b !K20_TURNS
.looking:
    brk.b #$00
    jsr.w ActorSeesPlayer
    bcc Kind20Spotted
    dec.b !K20_TURNS
    bne .no_turn
    lda.b #$11
    sta.b !K20_TURNS
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
.no_turn:
    dec.b !K20_COUNT
    bne .looking
    lda.b #$00
    jsl.l AnimStart
    lda.b #$10
    sta.b !K20_COUNT
.settle:
    brk.b #$00
    jsr.w ActorSeesPlayer
    bcc Kind20Spotted
    dec.b !K20_COUNT
    bne .settle
    lda.b #$02
    jsl.l AnimStart
    lda.b !actor_facing
    sta.b !actor_flip
Kind20Patrol_again:
    jmp.w Kind20Patrol

; @routine 83:9894 Kind20Spotted
; WHAT IT DOES: The guard has seen Aladdin: unless the floor ahead ends,
; it braces (12 frames, then 4) and charges, at $20 a frame's speed
; ($80 with PLACE bit 3, no script), up to 128 frames. First it writes
; $5A to COPIER_TEST: if that reads back (cartridge RAM, a copier), $43
; too, and if that also reads back the game hangs (no script runs it).
; HOW: The copier test; ActorTileAheadIs70 Z: Kind20Halt (no script);
; ActorMapAhead7Down28 Z and ActorMapAtOffset (0, $2B) Z: Kind20Halt.
; Else AnimStart 0, COP 12, AnimStart 3, COP 4; speed ($19) $20 or $80,
; $1A 1, $1B 0, ACTOR_ACCEL_X 8, COUNT $80, SPED_UP 0; on into
; Kind20Charge past its BRK and move.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the jumps of Kind20Patrol and
; Kind20LookAbout. Exit: never returns.
Kind20Spotted:
    lda.b #$5A                     ; a copier keeps what is written here
    sta.l !K20_COPIER_TEST
    cmp.l !K20_COPIER_TEST
    bne .retail
    lda.b #$43                     ; (no script runs this)
    sta.l !K20_COPIER_TEST
    cmp.l !K20_COPIER_TEST
    bne .retail
.hang:
    bra .hang
.retail:
    jsr.w ActorTileAheadIs70
    bne .floor
    jmp.w Kind20Halt                ; (no script runs this)
.floor:
    jsr.w ActorMapAhead7Down28
    bne .brace
    stz.w !scratch
    lda.b #$2B
    sta.w !scratch+2
    stz.w !scratch+3
    jsr.w ActorMapAtOffset
    bne .brace
    jmp.w Kind20Halt
.brace:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$0C
    cop.b #$00
    lda.b #$03
    jsl.l AnimStart
    lda.b #$04
    cop.b #$00
    ldy.b #$20
    lda.b !K20_PLACE
    and.b #$08
    beq .speed
    ldy.b #$80                     ; (no script runs this)
.speed:
    sty.b !actor_speed_x
    lda.b #$01
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    lda.b #$08
    sta.b !actor_accel_x
    lda.b #$80
    sta.b !K20_COUNT
    stz.b !K20_SPED_UP
    bra Kind20Charge_ground

; @routine 83:9900 Kind20Charge
; WHAT IT DOES: The guard charges: a frame at a time it runs on, slowing
; once COUNT is out, turned again toward Aladdin within 16 frames when he
; gets behind it, with a sound on some surfaces; it halts (Kind20Halt)
; when the floor ahead ends or it has stopped.
; HOW: BRK; ActorMoveX; ActorSnapToGround; probes as Kind20Patrol's
; (ActorMapAhead7Down28 Z and ActorMapAtOffset (0, $2B) Z, or a wall
; ahead without PLACE bit 3: Kind20Halt); on SURFACE 9 or $0C a new one,
; sound $13; COUNT nonzero: counted down, ActorSideOfPlayer not its
; facing with COUNT $10 or more makes it $10; COUNT 0: ActorDecelerateX,
; AnimStart 2 once its speed is under $100, CHARGE_FRAMES counted, and
; at speed 0, Kind20Halt. The paths over a further floor or a wall are
; not run by a script.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by its own and Kind20Spotted's jumps.
; Exit: never returns.
Kind20Charge:
    brk.b #$00
    jsl.l ActorMoveX
.ground:
    jsl.l ActorSnapToGround
    lda.w !V_0013
    cmp.b #$0F
    bne .floor
    jsr.w ActorMapAhead7Down36      ; on a ledge (no script runs this)
    bne .ahead
    bra .beyond
.floor:
    jsr.w ActorMapAhead7Down28
    bne .ahead
.beyond:
    stz.w !scratch                  ; no floor just ahead: one further on?
    lda.b #$2B
    sta.w !scratch+2
    stz.w !scratch+3
    jsr.w ActorMapAtOffset
    beq Kind20Halt
    stz.b !actor_speed_y            ; there is: it runs off, falling (no script runs this)
    stz.b !actor_speed_y+1
    stz.b !actor_speed_y+2
    lda.b #$60
    sta.b !actor_accel_y
    bra .air
.ahead:
    jsr.w ActorTileAheadIs70
    bne .on
    lda.b !K20_PLACE                ; a wall: the faster guard may leap it (no script runs this)
    and.b #$08
    beq Kind20Halt
    lda.b !K20_SPED_UP
    bne Kind20Halt
    stz.w !scratch
    stz.w !scratch+2
    stz.w !scratch+3
    jsr.w ActorMapAtOffset
    beq .leap

; @routine 83:9956 Kind20Halt
; WHAT IT DOES: The guard halts: stands 24 frames, then takes up its beat
; again from here and looks about.
; HOW: AnimStart 0, COP $18; ActorSpeedXSet1, ActorXTo40,
; Actor44Set40And80; Kind20LookAbout.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the jumps of Kind20Spotted and
; Kind20Charge. Exit: never returns.
Kind20Halt:
    lda.b #$00
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    jsr.w ActorSpeedXSet1
    jsr.w ActorXTo40
    jsr.w Actor44Set40And80
    jmp.w Kind20LookAbout

Kind20Charge_leap:
    lda.b #$60                     ; (no script runs this)
    sta.b !actor_speed_y
    lda.b #$FE
    sta.b !actor_speed_y+1
    lda.b #$FF
    sta.b !actor_speed_y+2
    lda.b #$10
    sta.b !actor_accel_y
Kind20Charge_air:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorOutOfGround
    beq Kind20Charge_air
Kind20Charge_on:
    lda.b !K20_SURFACE
    cmp.b #$09
    beq .sound
    cmp.b #$0C
    bne .count
.sound:
    cmp.b !K20_TURNS
    beq .count
    sta.b !K20_TURNS
    lda.b #$13
    jsl.l SoundQueueAdd
.count:
    lda.b !K20_COUNT
    beq .slow
    dec.b !K20_COUNT
    jsl.l ActorSideOfPlayer
    cmp.b !actor_facing
    bne .behind
    jmp.w Kind20Charge
.behind:
    lda.b !K20_COUNT
    cmp.b #$10
    bcs .soon
    jmp.w Kind20Charge
.soon:
    lda.b #$10
    sta.b !K20_COUNT
    jmp.w Kind20Charge
.slow:
    jsl.l ActorDecelerateX
    lda.b !K20_SPED_UP
    bne .slowing
    rep.b #$20
    lda.b !actor_speed_x
    cmp.w #$0100
    sep.b #$20
    bcs .slowing
    inc.b !K20_SPED_UP
    lda.b #$02
    jsl.l AnimStart
.slowing:
    inc.b !K20_CHARGE_FRAMES
    lda.b !actor_speed_x
    bne .running
    lda.b !actor_speed_x+1
    bne .running
    jmp.w Kind20Halt
.running:
    jmp.w Kind20Charge
