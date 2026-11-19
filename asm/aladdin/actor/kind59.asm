; kind59.asm: actor kind $59, the figures of the Genie's screen (level
; $2F): by its $07 a pose, or a tally that waits for the screen's step
; ($00B5) and then shows the lives, a row of lamps, the gems or the apples
; as they change, each change with a spark (kind $A4) and a sound. Its
; wait is Kind59Wait (kind_waits_rest.asm).
; C: src/aladdin/actor/kind59.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K59_SHOWN = $40                ; the gems' or apples' count shown ($FF: none yet)
!K59_ROW_SHOWN = $41            ; a row lamp's pose shown
!K59_ROW = $43                  ; a row lamp: its number from 1 (its $07's low 4 bits plus 1)
!K59_ROW_AT = $44               ; a row lamp: 4 times its $07's low 4 bits, into K59_ROW_PLACES
!K59_LIVES_SHOWN = $45          ; the lives shown ($FF: none yet)
!K59_STEP = $00B5               ; the screen's step: 0 waits; positive (negative for the apples) goes on
!K59_SCENE_Y = $00B0            ; the screen's y (word)
!K59_APPLES = $035B
!K59_GEMS = $0364
!K59_ROW_LIT = $0365            ; the row's lamps that show
!K59_LIVES = $0367
!K59_ROW_PLACES = $D3D0         ; (data bank $80) per row lamp, its x and its y below the screen (words)
!K59_SPARK = $A4                ; the kind of the spark it spawns at a change
!K59_SPARK_SOUND = $28
!K59_COUNT_SOUND = $0C
!K59_TALLY = $06                ; the $07 it keeps once it goes on (and the way for $07 of $10 up)

; @routine 83:DD10 Kind59Start
; @actor
; WHAT IT DOES: Actor kind $59, a figure of the Genie's screen, by its
; $07: 0 or 1 poses (facing 0, flip its $07, x pixel $80) for good; 4
; poses, waits 24 frames and ends; 2, 6 and $10 up wait for the screen's
; step to be positive (negative: they end), 8 for it to be negative
; (positive: it ends). Then, its $07 6 from there on, each frame at the
; screen's y: 2 shows the gems; 8 the apples; $10 the lives; $11-$1F a
; lamp of a row, its number the $07's low 4 bits plus 1: lit while the
; lives reach it, dark while not, hidden while the row's lamps
; ($0365) fall short of it. A change shows (AnimStart) and, with the
; figure on the screen (its y's top bit clear), spawns a spark (kind $A4)
; and makes a sound.
; HOW: $09 bits 7 and 4 set; K59_SHOWN $FF. $07 1: the pose. Else X = the
; $07 ($06 from $10 up) into the table after the jump. The pose: $08 bit
; 1 set; facing 0; flip = the $07; x pixels $80 (Y, 8-bit); AnimStart 0;
; BRK for good. Way 4: $08 bit 0 left as it is (TSB of 0); AnimStart $0F;
; COP $18; ActorEndPlaced. The waits: BRK until K59_STEP is nonzero; the
; wrong sign: ActorEndPlaced. Then $08 bit 0 set, $07 6, $21 0 (the
; spawn point). The lives (way 6, $07's low bits 0; K59_ROW 1, its
; shown $FF): each frame (BRK first) x $44, y K59_SCENE_Y + $10;
; K59_LIVES unchanged: again; 4 up: AnimStart $0F; else AnimStart lives +
; $0C, the spawn point (lives + $43) times 2, and on the screen
; ActorSpawnSameLook $A4 and SoundQueueAdd $28. A lamp (way 6, low bits
; nonzero; K59_ROW_SHOWN 0): each frame (BRK first) x and y (plus
; K59_SCENE_Y) from K59_ROW_PLACES by K59_ROW_AT; K59_ROW_LIT below
; K59_ROW: $08 bit 3 cleared; else pose $0C, $0B while K59_LIVES reaches
; K59_ROW; unchanged: again; else AnimStart, and on the screen the spark
; and sound $28. The gems (way 2): each frame (BRK first) y K59_SCENE_Y
; + $18, x $24; K59_GEMS unchanged: again; AnimStart it ($0A from $0B
; up); on the screen sound $0C and the spark. The apples (way 8; shown
; $FF): each frame (BRK first) y K59_SCENE_Y + $14, x $4E; K59_APPLES
; unchanged: again; AnimStart it plus 1; on the screen sound $0C and the
; spark.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $59 in state $0C (the table at $83:8CAB).
; Exit: never returns; the ways that end leave by ActorEndPlaced.
%at($83DD10)
Kind59Start:
    lda.b #$90
    tsb.b !actor_09
    lda.b #$FF
    sta.b !K59_SHOWN
    lda.b !actor_07
    tax
    dec a
    beq .pose
    cpx.b #$10
    bcc .way
    ldx.b #!K59_TALLY
.way:
    jmp.w (.ways,x)
.ways:
    dw .pose, .gems_wait, .brief, .tally_wait, .apples_wait
.pose:
    lda.b #$02
    tsb.b !actor_08
    stz.b !actor_facing
    lda.b !actor_07
    sta.b !actor_flip
    ldy.b #$80
    sty.b !actor_x+1
    lda.b #$00
    jsl.l AnimStart
.rest:
    brk.b #$00
    bra .rest
.tally_wait:
    brk.b #$00
    lda.w !K59_STEP
    beq .tally_wait
    bpl .tally
    jmp.w ActorEndPlaced
.tally:
    lda.b #$01
    tsb.b !actor_08
    stz.b !K59_ROW_SHOWN
    lda.b !actor_07
    and.b #$0F
    sta.b !K59_ROW
    asl a
    asl a
    sta.b !K59_ROW_AT
    inc.b !K59_ROW
    lda.b #!K59_TALLY
    sta.b !actor_07
    stz.b !actor_spawn_at
    lda.b !K59_ROW
    dec a
    bne .lamp
    lda.b #$FF
    sta.b !K59_LIVES_SHOWN
.lives:
    brk.b #$00
    rep.b #$21
    lda.w #$0044
    sta.b !actor_x+1
    lda.w !K59_SCENE_Y
    adc.w #$0010
    sta.b !actor_y+1
    sep.b #$20
    lda.w !K59_LIVES
    cmp.b !K59_LIVES_SHOWN
    beq .lives
    sta.b !K59_LIVES_SHOWN
    cmp.b #$04
    bcc .few
    lda.b #$0F
    jsl.l AnimStart
    bra .lives
.few:
    clc
    adc.b #$0C
    jsl.l AnimStart
    lda.b !K59_LIVES_SHOWN
    clc
    adc.b #$43
    asl a
    sta.b !actor_spawn_at
    lda.b !actor_y+2
    bmi .lives
    lda.b #!K59_SPARK
    jsl.l ActorSpawnSameLook
    lda.b #!K59_SPARK_SOUND
    jsl.l SoundQueueAdd
    bra .lives
.lamp:
    brk.b #$00
    ldx.b !K59_ROW_AT
    rep.b #$21
    lda.w !K59_ROW_PLACES,x
    sta.b !actor_x+1
    lda.w !K59_SCENE_Y
    adc.w !K59_ROW_PLACES+2,x
    sta.b !actor_y+1
    sep.b #$20
    ldx.w !K59_ROW_LIT
    cpx.b !K59_ROW
    bcc .dark
    lda.b #$0C
    ldx.w !K59_LIVES
    cpx.b !K59_ROW
    bcc .lit
    dec a
.lit:
    cmp.b !K59_ROW_SHOWN
    beq .lamp
    sta.b !K59_ROW_SHOWN
    jsl.l AnimStart
    lda.b !actor_y+2
    bmi .lamp
    lda.b #!K59_SPARK
    jsl.l ActorSpawnSameLook
    lda.b #!K59_SPARK_SOUND
    jsl.l SoundQueueAdd
    bra .lamp
.dark:
    lda.b #$08
    trb.b !actor_08
    bra .lamp
.gems_wait:
    brk.b #$00
    lda.w !K59_STEP
    beq .gems_wait
    bpl .gems_go
    jmp.w ActorEndPlaced
.gems_go:
    lda.b #$01
    tsb.b !actor_08
    lda.b #!K59_TALLY
    sta.b !actor_07
    stz.b !actor_spawn_at
.gems:
    brk.b #$00
    rep.b #$21
    lda.w !K59_SCENE_Y
    adc.w #$0018
    sta.b !actor_y+1
    lda.w #$0024
    sta.b !actor_x+1
    sep.b #$20
    lda.w !K59_GEMS
    cmp.b !K59_SHOWN
    beq .gems
    sta.b !K59_SHOWN
    cmp.b #$0B
    bcc .gems_few
    lda.b #$0A
    jsl.l AnimStart
    bra .gems_shown
.gems_few:
    jsl.l AnimStart
.gems_shown:
    lda.b !actor_y+2
    bmi .gems
    lda.b #!K59_COUNT_SOUND
    jsl.l SoundQueueAdd
    lda.b #!K59_SPARK
    jsl.l ActorSpawnSameLook
    bra .gems
.brief:
    lda.b #$00
    tsb.b !actor_08
    lda.b #$0F
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    jmp.w ActorEndPlaced
.apples_wait:
    brk.b #$00
    lda.w !K59_STEP
    beq .apples_wait
    bmi .apples_go
    jmp.w ActorEndPlaced
.apples_go:
    lda.b #$01
    tsb.b !actor_08
    lda.b #!K59_TALLY
    sta.b !actor_07
    stz.b !actor_spawn_at
    lda.b #$FF
    sta.b !K59_SHOWN
.apples:
    brk.b #$00
    rep.b #$21
    lda.w !K59_SCENE_Y
    adc.w #$0014
    sta.b !actor_y+1
    lda.w #$004E
    sta.b !actor_x+1
    sep.b #$20
    lda.w !K59_APPLES
    cmp.b !K59_SHOWN
    beq .apples
    sta.b !K59_SHOWN
    inc a
    jsl.l AnimStart
    lda.b !actor_y+2
    bmi .apples
    lda.b #!K59_COUNT_SOUND
    jsl.l SoundQueueAdd
    lda.b #!K59_SPARK
    jsl.l ActorSpawnSameLook
    bra .apples
%ends_at($83DEB9)
