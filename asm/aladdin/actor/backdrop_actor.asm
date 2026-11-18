; backdrop_actor.asm: the actor of kind $6E (its start, Kind6EStart,
; its wait routine at $83:F1D5 and its pieces), drawn as the picture of the
; backdrop layer, not as sprites: it moves right 3 pixels a frame, its
; picture's frames are uploads cycled from a table, and the backdrop is
; shown, scrolled to put the picture where the actor is, only while it is
; near.
; C: src/aladdin/actor/backdrop_actor.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_video.inc"

arch 65816

; The actor's own fields of its record.
!PICTURE_AT = $41               ; the offset of its picture's frame in PICTURE_FRAMES
!PICTURE_TIME = $42             ; frames left before the next

; In the bank DB names: 2 bytes a frame: the upload's number, then its
; frames (bit 7 set on the last of the cycle).
!PICTURE_FRAMES = $D67B

!RANGE_X = $1A44                ; the point actors are kept in range of (words)
!RANGE_Y = $1A46
!TM_SHADOW = $0399              ; TM's shadow: the main screen's layers

; Kind6EStart's fields and tables.
!K6E_STEP = $40                 ; the spawner's offset in K6E_SPAWNS; else frames left of a stretch
!K6E_COPIER = $4A               ; set when the copier checks found one
!K6E_SOUND_TIME = $48           ; frames to Kind6EWait's next sound
!K6E_QUIET = $49                ; nonzero: Kind6EWait sounds no more
!K6E_RAMP = $43                 ; its gravity ramp (word; the high byte is the gravity)
!K6E_PAIRS_AT = $47             ; its offset in K6E_PAIRS
!K6E_CHILD_SLOT = $21           ; the slot byte its children are made with
!K6E_SPAWNS = $D687             ; (data bank) 4 bytes a child: scroll x it waits for (word), $07, slot
!K6E_MOTIONS = $D69C            ; (data bank) by $07: its motion preset
!K6E_PAIRS = $D69F              ; (data bank) bytes: decelerate's step, then frames of speeding up; 0 ends
!K6E_18CA = $18CA               ; set to 1 by the spawner
!K6E_0834 = $0834               ; set to 1 by the spawner
!K6E_083D = $083D               ; set to $10 when a copier was found
!K6E_CHECK = $700020            ; the copier check's bytes ($70:0020, $70:0040): none on the cartridge
!K6E_CHECK_FIRST = $0000        ; with DB $70 and X $20: K6E_CHECK's first byte ($70:0020)
!K6E_CHECK_SECOND = $0020       ; its second ($70:0040)
!K6E_ROM_PROBE = $A000          ; two ROM bytes read in banks $80 and $00: a copier that maps them apart is found
!K6E_SCROLL_X = $1773           ; the play field's scroll x (word)

; @routine 83:F049 Kind6EStart
; @actor
; WHAT IT DOES: Actor kind $6E, the backdrop picture: by its $07, 0 a
; spawner that makes a kind $6E child each time the view's scroll passes
; the next point of its list; 1 and 2 one that rises and glides right,
; then speeds off right until the main screen's layers are $15 and ends
; (a copier makes it set $083D); 3 one that slows and speeds by a list of
; pairs, turning each time, then a sound and ends.
; HOW: JMP through the table at $83:F050 by $07. 0: facing, K6E_STEP 0,
; K6E_18CA and K6E_0834 1; each frame (BRK) while the scroll x ($1773) is
; at or past the list's word: its x, y = RANGE_X, RANGE_Y, $07 and the
; slot from the list, ActorSpawnShared $6E (X kept), K6E_STEP + 4. 1, 2:
; K6E_COPIER set when $80:A000-$A001 do not read the same at $00, or $23
; reads back at $70:0020 and $70:0040; PICTURE_AT, PICTURE_TIME 0,
; ACTOR_09 $80, SoundQueueAdd $2A, K6E_SOUND_TIME $9D, K6E_QUIET 0,
; ActorMotionPreset by K6E_MOTIONS; each frame (BRK) ActorFly and
; ActorStepRight3 while rising; $30 frames of ActorStepRight3; a copier:
; K6E_083D $10; K6E_QUIET up, SoundQueueAdd $2B, its speed 0, the ramp 0,
; acceleration $10; each frame (BRK) ActorAccelerateX (speed at most
; $0200), the ramp + $80 (at most $3000), gravity its high byte,
; ActorFly, ActorStepRight3, until TM_SHADOW is $15: ActorEndPlaced. 3:
; PICTURE_AT, PICTURE_TIME, K6E_PAIRS_AT 0, SoundQueueAdd $2A,
; K6E_SOUND_TIME $9D, K6E_QUIET 0, ACTOR_09 $80, facing 0,
; ActorSpeedXSet $45; for each pair of K6E_PAIRS: acceleration the first,
; each frame (BRK) ActorDecelerateX, ActorMoveX, ActorStepRight3 until
; its speed is 0; facing turned; the second's frames (BRK) of
; ActorAccelerateX, ActorMoveX, ActorStepRight3; at the list's 0,
; SoundQueueAdd $23 and ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $6E in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83F049)
Kind6EStart:
    lda.b !actor_07
    asl a
    tax
    jmp.w (.ways,x)
.ways:
    dw .spawner, .flyer, .flyer, .pairs
.spawner:
    stz.b !actor_flip
    stz.b !K6E_STEP
    lda.b #$01
    sta.w !K6E_18CA
    lda.b #$01
    sta.w !K6E_0834
    rep.b #$20
.next:
    brk.b #$00
    ldx.b !K6E_STEP
    lda.w !K6E_SCROLL_X
    cmp.w !K6E_SPAWNS,x
    bcc .next
    lda.w !RANGE_X
    sta.b !actor_x+1
    lda.w !RANGE_Y
    sta.b !actor_y+1
    sep.b #$20
    lda.w !K6E_SPAWNS+2,x
    sta.b !actor_07
    lda.w !K6E_SPAWNS+3,x
    sta.b !K6E_CHILD_SLOT
    phx
    lda.b #$6E
    jsl.l ActorSpawnShared
    plx
    inx
    inx
    inx
    inx
    stx.b !K6E_STEP
    rep.b #$20
    bra .next
.flyer:
    stz.b !K6E_COPIER
    phb
    lda.b #$80
    pha
    plb
    ldy.w !K6E_ROM_PROBE
    ldx.w !K6E_ROM_PROBE+1
    lda.b #$00
    pha
    plb
    cpy.w !K6E_ROM_PROBE
    bne .copier
    cpx.w !K6E_ROM_PROBE+1
    bne .copier
    ldx.b #$20
    sec
    lda.b #$90
    sbc.b #$20
    pha
    plb
    lda.b #$23
    sta.w !K6E_CHECK_FIRST,x
    cmp.w !K6E_CHECK_FIRST,x
    bne .cart
    lda.b #$23
    sta.w !K6E_CHECK_SECOND,x
    cmp.w !K6E_CHECK_SECOND,x
    bne .cart
.copier:
    plb
    inc.b !K6E_COPIER
    bra .fly
.cart:
    plb
.fly:
    stz.b !PICTURE_AT
    stz.b !PICTURE_TIME
    lda.b #$80
    sta.b !actor_09
    lda.b #$2A
    jsl.l SoundQueueAdd
    lda.b #$9D
    sta.b !K6E_SOUND_TIME
    stz.b !K6E_QUIET
    ldx.b !actor_07
    ldy.w !K6E_MOTIONS,x
    jsl.l ActorMotionPreset
.rise:
    brk.b #$00
    jsl.l ActorFly
    jsr.w ActorStepRight3
    lda.b !actor_speed_y+2
    bmi .rise
    lda.b #$30
    sta.b !K6E_STEP
.glide:
    brk.b #$00
    jsr.w ActorStepRight3
    dec.b !K6E_STEP
    bne .glide
    lda.b !K6E_COPIER
    beq .off
    lda.b #$10
    sta.w !K6E_083D
.off:
    inc.b !K6E_QUIET
    lda.b #$2B
    jsl.l SoundQueueAdd
    stz.b !actor_speed_x
    stz.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    stz.b !K6E_RAMP
    stz.b !K6E_RAMP+1
    lda.b #$10
    sta.b !actor_accel_x
.away:
    brk.b #$00
    jsl.l ActorAccelerateX
    rep.b #$20
    lda.w #$0200
    cmp.b !actor_speed_x
    bcs .speed
    sta.b !actor_speed_x
.speed:
    clc
    lda.b !K6E_RAMP
    adc.w #$0080
    sta.b !K6E_RAMP
    lda.w #$3000
    cmp.b !K6E_RAMP
    bcs .ramp
    sta.b !K6E_RAMP
.ramp:
    sep.b #$20
    lda.b !K6E_RAMP+1
    sta.b !actor_accel_y
    jsl.l ActorFly
    jsr.w ActorStepRight3
    lda.w !TM_SHADOW
    cmp.b #$15
    bne .away
    jmp.w ActorEndPlaced
.pairs:
    stz.b !PICTURE_AT
    stz.b !PICTURE_TIME
    stz.b !K6E_PAIRS_AT
    lda.b #$2A
    jsl.l SoundQueueAdd
    lda.b #$9D
    sta.b !K6E_SOUND_TIME
    stz.b !K6E_QUIET
    lda.b #$80
    sta.b !actor_09
    stz.b !actor_facing
    ldy.b #$45
    jsl.l ActorSpeedXSet
.pair:
    ldx.b !K6E_PAIRS_AT
    lda.w !K6E_PAIRS,x
    sta.b !actor_accel_x
    inc.b !K6E_PAIRS_AT
.slow:
    brk.b #$00
    jsl.l ActorDecelerateX
    jsl.l ActorMoveX
    jsr.w ActorStepRight3
    lda.b !actor_speed_x
    ora.b !actor_speed_x+1
    ora.b !actor_speed_x+2
    bne .slow
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
    ldx.b !K6E_PAIRS_AT
    lda.w !K6E_PAIRS,x
    sta.b !K6E_STEP
    inc.b !K6E_PAIRS_AT
.fast:
    brk.b #$00
    jsl.l ActorAccelerateX
    jsl.l ActorMoveX
    jsr.w ActorStepRight3
    dec.b !K6E_STEP
    bne .fast
    ldx.b !K6E_PAIRS_AT
    lda.w !K6E_PAIRS+1,x
    bne .pair
    lda.b #$23
    jsl.l SoundQueueAdd
    jmp.w ActorEndPlaced
%ends_at($83F1D5)

; @routine 83:F211 ActorStepRight3
; WHAT IT DOES: Moves the actor 3 pixels to the right.
; HOW: A 24-bit addition of $000300 to its x, a byte at a time.
; Entry: M 8-bit; D = the actor's record. Exit: as the C states.
%at($83F211)
ActorStepRight3:
    clc
    lda.b !actor_x
    adc.b #$00
    sta.b !actor_x
    lda.b !actor_x+1
    adc.b #$03
    sta.b !actor_x+1
    lda.b !actor_x+2
    adc.b #$00
    sta.b !actor_x+2
    rts

; @routine 83:F225 BackdropPictureStep
; WHAT IT DOES: Plays the actor's picture on by a frame of time: when the
; picture's frame has run its time, the next one (after the last, the
; first) is asked for as a fixed VRAM upload.
; HOW: The time steps down; below 0, the offset steps on by 2 (back to 0
; when the frame left was the last) and the new frame's time (low 7 bits)
; and upload number are taken.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB the table's bank,
; whose $0000-$1FFF is work RAM. Exit: as the C states.
BackdropPictureStep:
    dec.b !PICTURE_TIME
    bpl .done
    ldy.b !PICTURE_AT
    iny
    iny
    lda.w !PICTURE_FRAMES-1,y
    bpl .take
    ldy.b #$00
.take:
    lda.w !PICTURE_FRAMES+1,y
    and.b #$7F
    sta.b !PICTURE_TIME
    lda.w !PICTURE_FRAMES,y
    sta.w !vram_upload_number
    sty.b !PICTURE_AT
.done:
    rts

; @routine 83:F244 BackdropActorShow
; WHAT IT DOES: Shows the backdrop, with the actor's picture on it, while
; the actor is within 256 pixels of the range point on both axes: the
; backdrop is scrolled to put its point (128, 128) where the actor is on
; the screen. Farther away the backdrop is not shown.
; HOW: With 16-bit A, each distance plus 256 compared unsigned with 512;
; near: the backdrop's scroll is the play field's plus 128 less the
; actor's pixels, on each axis, and TM's shadow is $17; else $15.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM. Exit: as the C states.
BackdropActorShow:
    rep.b #$20
    sec
    lda.b !actor_x+1
    sbc.w !RANGE_X
    clc
    adc.w #$0100
    cmp.w #$0200
    bcs .far
    sec
    lda.b !actor_y+1
    sbc.w !RANGE_Y
    clc
    adc.w #$0100
    cmp.w #$0200
    bcs .far
    sec
    lda.b !actor_x+1
    sbc.w !layer0+!LAYER_SCROLL_X
    sec
    sbc.w #$0080
    eor.w #$FFFF
    inc a
    sta.w !layer1+!LAYER_SCROLL_X
    sec
    lda.b !actor_y+1
    sbc.w !layer0+!LAYER_SCROLL_Y
    sec
    sbc.w #$0080
    eor.w #$FFFF
    inc a
    sta.w !layer1+!LAYER_SCROLL_Y
    sep.b #$20
    lda.b #$17
    sta.w !TM_SHADOW
    rts
.far:
    sep.b #$20
    lda.b #$15
    sta.w !TM_SHADOW
    rts
