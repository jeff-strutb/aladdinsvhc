; kind33.asm: actor kind $33, a platform swinging under the placed one:
; the placed one ($07 0) queues its kind's graphics when it holds its tile
; slot alone, makes the platform ($07 3) and then shows the frame that
; matches the platform's direction; the platform swings to and fro about
; the placed one's position and carries the player standing on it.
; C: src/aladdin/actor/kind33.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_video.inc"

arch 65816

!K33_PARTNER = $40              ; the pair's other record (word): the placed one's platform, the platform's placed one
!K33_PHASE = $42                ; the platform: 0 swinging one way, then 1 the other
!K33_PAST = $43                 ; the platform: nonzero once this swing is slowing
!K33_ACCEL = $44                ; the platform: taken from or added to the speed each frame (word)
!K33_SPEED = $46                ; the platform: its angle's change a frame (word)
!K33_ANGLE = $48                ; the platform: its angle (word); the high byte its direction
!K33_HUB_X = $4A                ; the platform: the placed one's x pixels, where it swings from (word)
!K33_HUB_Y = $4C                ; and its y pixels (word)
!K33_RIDDEN = $4E               ; the platform: nonzero while the player stands on it
!K33_SHOWN = $4F                ; the placed one: the frame it shows
!K33_HOLD_AT = $13              ; ActorHoldPlayer's offset for the player
!K33_OLD_X = $000E              ; the platform's x pixels before its move (word)
!K33_OLD_Y = $0010              ; and its y pixels
!K33_FRAMES = $D199             ; bank $80: the placed one's frame by the platform's direction / 8
!K33_UPLOADS = $D1AA            ; bank $80: its graphics' two uploads, 6 bytes each: tile, source offset, bytes (words)
!K33_CELL_ATTRS = $7FE766       ; kind $33's cell: its attribute word (bits 0-8 its first tile)
!K33_CELL_GFX = $7FE566         ; kind $33's cell: its graphics' address in bank $7F (word)

; @routine 83:B966 Kind33Start
; @actor
; WHAT IT DOES: Actor kind $33's start: the placed one ($07 0) goes to
; Kind33Graphics, its platform ($07 nonzero) to Kind33Swing.
; HOW: $07 zero: JMP Kind33Graphics; else JMP Kind33Swing.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $33 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83B966)
Kind33Start:
    lda.b !actor_07
    beq .placed
    jmp.w Kind33Swing
.placed:
    jmp.w Kind33Graphics

; @routine 83:B970 Kind33Hub
; @actor
; WHAT IT DOES: The placed one of kind $33 after its graphics: makes its
; platform ($07 3, swinging from its position) and then each frame shows
; the frame of animation 0 that matches the platform's direction.
; HOW: $21 = 0; $07 = 3; ActorSpawn with its own hit box ($06); made (N
; clear): K33_PARTNER = the platform, the platform's K33_PARTNER = it,
; K33_HUB_X/Y its x and y pixels, its look ours. 8-bit; Kind33Look; BRK.
; Then each frame (BRK first): the platform's direction ($14) / 8 picks
; from K33_FRAMES into ACTOR_FRAME; a frame other than K33_SHOWN: kept
; there, AnimResume 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
; jump from Kind33Graphics. Exit: never returns.
%at($83B970)
Kind33Hub:
    stz.b !actor_spawn_at
    lda.b #$03
    sta.b !actor_07
    lda.b !actor_06
    jsl.l ActorSpawn
    bmi .made
    rep.b #$20
    txa
    sta.b !K33_PARTNER
    tdc
    sta.w !K33_PARTNER,x
    lda.b !actor_x+1
    sta.w !K33_HUB_X,x
    lda.b !actor_y+1
    sta.w !K33_HUB_Y,x
    lda.b !actor_look
    sta.w !actor_look,x
    sep.b #$20
.made:
    sep.b #$30
    jsr.w Kind33Look
    brk.b #$00
.frame:
    brk.b #$00
    rep.b #$10
    ldx.b !K33_PARTNER
    lda.w !actor_facing,x
    sep.b #$10
    lsr a
    lsr a
    lsr a
    tax
    lda.w !K33_FRAMES,x
    sta.b !ACTOR_FRAME
    cmp.b !K33_SHOWN
    beq .frame
    sta.b !K33_SHOWN
    lda.b #$00
    jsl.l AnimResume
    bra .frame

; @routine 83:B9C1 Kind33Swing
; @actor
; WHAT IT DOES: Kind $33's platform: swings about the placed one, faster
; towards the bottom of each swing and slower past it, one way and back,
; then stops and starts over; it carries the player standing on it and
; may take hold of him; it ends when the placed one's record is free.
; HOW: $08 bit 0 and $09 bit 4 set; AnimStart 1; K33_RIDDEN, K33_SHOWN,
; $21 and K33_HOLD_AT cleared; y pixels $8080; K33_PHASE 0, K33_PAST 1,
; K33_ACCEL 9, K33_SPEED $020A, K33_ANGLE $4AFD. Each frame (BRK first):
; Kind33PartnerState zero: JMP ActorEndPlaced. Kind33SwingMove;
; ActorHoldPlayer. Phase 0: K33_ANGLE += K33_SPEED (16-bit); phase 1: -=.
; K33_PAST clear and the direction below $44 (phase 0) or at least $48
; (phase 1): K33_SPEED += K33_ACCEL, next frame. K33_PAST clear
; otherwise: K33_PAST up. Then K33_SPEED -= K33_ACCEL; not negative, next
; frame; negative in phase 0: phase 1, K33_PAST 0; in phase 1: phase,
; past, speed and angle 0, K33_ACCEL 9.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
; jump from Kind33Start. Exit: never returns.
; (The JMP back to the stop at $83:BA76-BA78, after the JMP at $83:BA73,
; is reached by nothing: no branch or jump targets it. It is not filed.)
%at($83B9C1)
Kind33Swing:
    lda.b #$01
    tsb.b !actor_08
    lda.b #$10
    tsb.b !actor_09
    lda.b #$01
    jsl.l AnimStart
    stz.b !K33_RIDDEN
    stz.b !K33_SHOWN
    stz.b !actor_spawn_at
    stz.b !K33_HOLD_AT
    lda.b #$80
    sta.b !actor_y+1
    sta.b !actor_y+2
    stz.b !K33_PHASE
    lda.b #$01
    sta.b !K33_PAST
    lda.b #$09
    sta.b !K33_ACCEL
    stz.b !K33_ACCEL+1
    lda.b #$0A
    sta.b !K33_SPEED
    lda.b #$02
    sta.b !K33_SPEED+1
    lda.b #$FD
    sta.b !K33_ANGLE
    lda.b #$4A
    sta.b !K33_ANGLE+1
    bra .frame
.stop:
    stz.b !K33_PHASE
    stz.b !K33_PAST
    stz.b !K33_SPEED
    stz.b !K33_SPEED+1
    lda.b #$09
    sta.b !K33_ACCEL
    stz.b !K33_ACCEL+1
    stz.b !K33_ANGLE
    stz.b !K33_ANGLE+1
.frame:
    brk.b #$00
    jsr.w Kind33PartnerState
    bne .live
    jmp.w ActorEndPlaced
.live:
    jsr.w Kind33SwingMove
    jsr.w ActorHoldPlayer
    lda.b !K33_PHASE
    bne .back
    rep.b #$21
    lda.b !K33_ANGLE
    adc.b !K33_SPEED
    sta.b !K33_ANGLE
    sep.b #$20
    lda.b !K33_PAST
    bne .slower
    lda.b !K33_ANGLE+1
    cmp.b #$44
    bcc .faster
    inc.b !K33_PAST
    bra .slower
.back:
    rep.b #$20
    lda.b !K33_ANGLE
    sec
    sbc.b !K33_SPEED
    sta.b !K33_ANGLE
    sep.b #$20
    lda.b !K33_PAST
    bne .slower
    lda.b !K33_ANGLE+1
    cmp.b #$48
    bcs .faster
    inc.b !K33_PAST
    bra .slower
.faster:
    rep.b #$20
    lda.b !K33_SPEED
    adc.b !K33_ACCEL
    sta.b !K33_SPEED
    sep.b #$20
    bra .frame
.slower:
    rep.b #$20
    lda.b !K33_SPEED
    sec
    sbc.b !K33_ACCEL
    sta.b !K33_SPEED
    sep.b #$20
    bpl .frame
    lda.b !K33_PHASE
    bne .stop
    inc.b !K33_PHASE
    stz.b !K33_PAST
    jmp.w .frame
%ends_at($83BA76)

; @routine 83:BA79 Kind33PartnerState
; WHAT IT DOES: Reads the state of the pair's other record (K33_PARTNER).
; HOW: 16-bit X = K33_PARTNER; A low = its state byte; X 8-bit again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: A low = the state; N and Z from it; X = K33_PARTNER's low
; byte; M 8-bit, X 8-bit; the rest unchanged.
%at($83BA79)
Kind33PartnerState:
    rep.b #$10
    ldx.b !K33_PARTNER
    lda.w !actor_state,x
    sep.b #$10
    rts

; @routine 83:BA83 Kind33SwingMove
; WHAT IT DOES: Puts the platform where its angle sets it about the placed
; one, and the player with it: lands him on it when he falls onto it,
; carries him by its move while he stands on it, and marks when he is off.
; HOW: Its x and y pixels kept in K33_OLD_X/Y; x and y pixels =
; K33_HUB_X/Y, their fractions 0; the facing = the angle's high byte;
; ActorMoveByVector with set 4. K33_RIDDEN clear: PlayerLandOnActor with
; box 0; landed (C clear): K33_RIDDEN up. Set: the player's x and y
; pixels moved by its new place less the old; PlayerLandOnActor with box
; 0; off (C set): K33_RIDDEN = 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit; C as PlayerLandOnActor leaves it.
%at($83BA83)
Kind33SwingMove:
    rep.b #$20
    lda.b !actor_x+1
    sta.w !K33_OLD_X
    lda.b !actor_y+1
    sta.w !K33_OLD_Y
    lda.b !K33_HUB_X
    sta.b !actor_x+1
    lda.b !K33_HUB_Y
    sta.b !actor_y+1
    sep.b #$20
    stz.b !actor_x
    stz.b !actor_y
    lda.b !K33_ANGLE+1
    sta.b !actor_facing
    ldx.b #$04
    jsl.l ActorMoveByVector
    lda.b !K33_RIDDEN
    bne .ridden
    ldy.b #$00
    jsl.l PlayerLandOnActor
    bcs .done
    inc.b !K33_RIDDEN
    bra .done
.ridden:
    rep.b #$20
    lda.b !actor_x+1
    sec
    sbc.w !K33_OLD_X
    clc
    adc.w !player+!actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    sec
    sbc.w !K33_OLD_Y
    clc
    adc.w !player+!actor_y+1
    sta.w !player+!actor_y+1
    sep.b #$20
    ldy.b #$00
    jsl.l PlayerLandOnActor
    bcc .done
    stz.b !K33_RIDDEN
.done:
    rts

; @routine 83:BAE0 Kind33Look
; WHAT IT DOES: The placed one of kind $33's look: $08 bit 1 and $09 bits
; 7 and 4 set, animation 0 from its start.
; HOW: TSB $02 on $08, TSB $90 on $09; AnimStart with A = 0.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: as AnimStart.
%at($83BAE0)
Kind33Look:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
    rts

; @routine 83:BB2F Kind33Graphics
; @actor
; WHAT IT DOES: The placed one of kind $33 first: when it holds its tile
; slot alone it queues its kind's graphics (two uploads, a frame apart);
; then Kind33Hub.
; HOW: The slot's holders ($15E8 by $39) less one: zero: X = 0,
; Kind33GraphicsQueue, BRK, X = 6, Kind33GraphicsQueue, JMP Kind33Hub;
; else BRK, JMP Kind33Hub.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by a
; jump from Kind33Start. Exit: never returns.
%at($83BB2F)
Kind33Graphics:
    ldx.b !actor_tile_slot
    lda.w !actor_tile_slot_users,x
    dec a
    bne .shared
    ldx.b #$00
    jsr.w Kind33GraphicsQueue
    brk.b #$00
    ldx.b #$06
    jsr.w Kind33GraphicsQueue
    jmp.w Kind33Hub
.shared:
    brk.b #$00
    jmp.w Kind33Hub

; @routine 83:BB4B Kind33GraphicsQueue
; WHAT IT DOES: Queues one of kind $33's graphics uploads (X = 0 or 6) for
; the vertical blank: a VRAM queue record.
; HOW: 16-bit. Y = the queue's count (its low byte) times 8. The VRAM
; address: (K33_CELL_ATTRS AND $01FF) plus the upload's tile word, times
; 16, plus $6000; the source: K33_CELL_GFX plus its offset word; bank
; $7F; the byte count its third word (written over the bank's high
; byte); VMAIN $80. The count up.
; Entry: M 8-bit, X 8-bit; X = the upload's offset in K33_UPLOADS; DB =
; $80; called by JSR. Exit: M 8-bit, X 8-bit; A low = $80; Y = the
; record's offset; N and Z from the count's INC.
%at($83BB4B)
Kind33GraphicsQueue:
    rep.b #$30
    lda.w !vram_queue_count
    and.w #$00FF
    asl a
    asl a
    asl a
    tay
    lda.l !K33_CELL_ATTRS
    and.w #$01FF
    clc
    adc.w !K33_UPLOADS,x
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w #$6000
    sta.w !vram_queue,y
    clc
    lda.l !K33_CELL_GFX
    adc.w !K33_UPLOADS+2,x
    sta.w !vram_queue+2,y
    lda.w #$007F
    sta.w !vram_queue+4,y
    lda.w !K33_UPLOADS+4,x
    sta.w !vram_queue+5,y
    sep.b #$20
    lda.b #$80
    sta.w !vram_queue+7,y
    sep.b #$10
    inc.w !vram_queue_count
    rts
%ends_at($83BB91)
