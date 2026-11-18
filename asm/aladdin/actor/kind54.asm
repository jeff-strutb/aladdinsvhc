; kind54.asm: actor kinds $54 and $55, things that leap from a spot (kind
; $54 straight up, kind $55 in an arc), trailing copies of themselves,
; and come back down to leap again; with $07 bit 3 set, one that only
; drops and ends. Kind $54 notes a copier's cartridge RAM, and then holds
; Aladdin's y at its own as it turns to fall.
; C: src/aladdin/actor/kind54.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K54_CHECK = $703000            ; the copier check's bytes: work RAM on a copier, none on the cartridge
!K54_COPIER = $4F               ; 1: the check found a copier
!K54_PROBES = $16               ; its probe list (word)
!K54_HOME_X = $43               ; kind $55: the x it leaps from (word)
!K54_HOME_Y = $43               ; kind $54: the y it leaps from (word)
!K55_HOME_Y = $45               ; kind $55: the y it leaps from (word)
!K54_TRAIL = $42                ; frames left to the next copy it trails
!K54_LAST_RISE = $41            ; its vertical speed's high byte last frame
!K54_FRAMES = $40               ; frames left of its drop
!K54_PROBE_LIST = $D3A4         ; (data bank) kind $54's probe list
!K54_RISES = $D3A8              ; (data bank) kind $54's rise speed by $07 bits 0-1
!K55_PROBE_LIST = $D3AC         ; (data bank) kind $55's probe list
!K55_ACROSS = $D3B0             ; (data bank) kind $55's speed across by $07 bits 0-1
!K54_PLAYER_Y = !player+!actor_y+1 ; Aladdin's y pixels (word)

; @routine 83:D6D7 Kind54Start
; @actor
; WHAT IT DOES: Actor kind $54: leaps straight up from where it is placed
; (speed by $07 bits 0-1, sound $25), trailing a copy every 8 frames, and
; once back down at its start, sound $26, a pause of 64 frames, and up
; again; with $07 bit 3 set it drops for 14 frames and ends. A copier's
; cartridge RAM (the check at K54_CHECK) makes it hold Aladdin's y at its
; own each time it turns to fall.
; HOW: The byte at K54_CHECK + X up and compared: equal, K54_COPIER 1,
; else 0. ACTOR_08 bit 1, ACTOR_09 bits $90 set. $07 bit 3: the drop
; (below). Else K54_PROBES $D3A4, K54_HOME_Y its y, K54_TRAIL $18, ACTOR_08
; bit 7 set, bit 3 cleared. Each leap: $07 bit 2 set, ActorSpawn5E, COP 6,
; ACTOR_08 bit 3 set and bit 7 cleared, AnimStart 2, ActorSpeedYSet by
; K54_RISES, gravity $20, SoundQueueAdd $25; each frame (BRK) ActorFall;
; turning to fall: AnimStart 0 and (K54_COPIER) the player's y = its y;
; K54_TRAIL down, at 0 $08 again and ActorSpawnCopy; until its y is at or
; below K54_HOME_Y. Its y = that (A); $07 bit 2 cleared, ActorSpawn5E,
; SoundQueueAdd $26, BRK, ACTOR_08 bit 7 set and bit 3 cleared, COP $40,
; again. The drop: ACTOR_08 bits 0-1 set, ActorMotionReset 8, K54_FRAMES
; $0E, COP 7, AnimStart 1, each frame (BRK) ActorFall K54_FRAMES times,
; then ActorEndPlaced (JML at $83:D7F7).
; Entry: M 8-bit, X 8-bit (twice its kind); D = the actor's record; DB =
; $83; reached by the actor loop's jump for kind $54 in state $0C (the
; table at $83:8CAB). Exit: never returns.
%at($83D6D7)
Kind54Start:
    lda.l !K54_CHECK,x
    inc a
    sta.l !K54_CHECK,x
    cmp.l !K54_CHECK,x
    bne .no_copier
    lda.b #$01
    sta.b !K54_COPIER
    bra .set
.no_copier:
    stz.b !K54_COPIER
.set:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    and.b #$08
    beq .leaper
    jmp.w Kind54Drop
.leaper:
    rep.b #$20
    lda.w #!K54_PROBE_LIST
    sta.b !K54_PROBES
    lda.b !actor_y+1
    sta.b !K54_HOME_Y
    sep.b #$20
    lda.b #$18
    sta.b !K54_TRAIL
    lda.b #$80
    tsb.b !actor_08
    lda.b #$08
    trb.b !actor_08
.leap:
    lda.b #$04
    tsb.b !actor_07
    jsr.w ActorSpawn5E
    lda.b #$06
    cop.b #$00
    lda.b #$08
    tsb.b !actor_08
    lda.b #$80
    trb.b !actor_08
    lda.b #$02
    jsl.l AnimStart
    lda.b !actor_07
    and.b #$03
    tax
    ldy.w !K54_RISES,x
    jsl.l ActorSpeedYSet
    lda.b #$20
    sta.b !actor_accel_y
    lda.b #$25
    jsl.l SoundQueueAdd
.fly:
    brk.b #$00
    jsl.l ActorFall
    lda.b !actor_speed_y+2
    bmi .trail
    lda.b !K54_LAST_RISE
    bpl .trail
    lda.b #$00
    jsl.l AnimStart
    lda.b !K54_COPIER
    beq .trail
    rep.b #$20
    lda.b !actor_y+1
    sta.w !K54_PLAYER_Y
    sep.b #$20
.trail:
    lda.b !actor_speed_y+2
    sta.b !K54_LAST_RISE
    dec.b !K54_TRAIL
    bne .home
    lda.b #$08
    sta.b !K54_TRAIL
    jsr.w ActorSpawnCopy
.home:
    rep.b #$20
    lda.b !K54_HOME_Y
    cmp.b !actor_y+1
    sep.b #$20
    bcs .fly
    rep.b #$20
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$04
    trb.b !actor_07
    jsr.w ActorSpawn5E
    lda.b #$26
    jsl.l SoundQueueAdd
    brk.b #$00
    lda.b #$80
    tsb.b !actor_08
    lda.b #$08
    trb.b !actor_08
    lda.b #$40
    cop.b #$00
    jmp.w .leap

; Kinds $54 and $55 with $07 bit 3 set: they drop 14 frames and end.
Kind54Drop:
    lda.b #$03
    tsb.b !actor_08
    lda.b #$08
    jsl.l ActorMotionReset
    lda.b #$0E
    sta.b !K54_FRAMES
    lda.b #$07
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
.fall:
    brk.b #$00
    jsl.l ActorFall
    dec.b !K54_FRAMES
    bne .fall
    bra Kind54End
assert pc() == $83D7C9

; The drop's end, after Kind54Wait.
%at($83D7F7)
Kind54End:
    jml.l ActorEndPlaced

; @routine 83:D7FB Kind55Start
; @actor
; WHAT IT DOES: Actor kind $55: (unless $07 bits 0, 1 and 3 are all
; clear, after 16 frames) leaps in an arc from where it is placed (across
; by $07 bits 0-1, facing turned for bit 4; up $96 for variant 2, $3F
; otherwise; sound $25), trailing a copy every 8 frames; once back down
; at its start's height, sound $26, a pause of 64 frames, put back at its
; start, and again. With $07 bit 3 set it drops and ends as kind $54's
; does.
; HOW: $07 AND $0B zero: COP $10. ACTOR_08 bit 1, ACTOR_09 bits $90 set.
; $07 bit 3: Kind54Drop. Else K54_PROBES $D3AC, K54_HOME_X its x,
; K55_HOME_Y its y, K54_TRAIL $18, ACTOR_08 bit 7 set, bit 3 cleared;
; $07 bit 4: facing up. Each leap: $07 bit 2 set, ActorSpawn5E, COP 6,
; ACTOR_08 bit 3 set and bit 7 cleared, AnimStart 2, ActorSpeedXSet by
; K55_ACROSS, gravity $16, ActorSpeedYSet $96 (variant 2) or $3F,
; SoundQueueAdd $25; each frame (BRK) ActorFly; turning to fall:
; AnimStart 0; K54_TRAIL down, at 0 $08 again and ActorSpawnCopy; until
; its y is at or below K55_HOME_Y. $07 bit 2 cleared, ActorSpawn5E,
; SoundQueueAdd $26, BRK, ACTOR_08 bit 7 set and bit 3 cleared, COP $40,
; its x and y back to K54_HOME_X and K55_HOME_Y, again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $55 in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83D7FB)
Kind55Start:
    lda.b !actor_07
    and.b #$0B
    bne .go
    lda.b #$10
    cop.b #$00
.go:
    lda.b #$02
    tsb.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    and.b #$08
    bne Kind54Drop
    rep.b #$20
    lda.w #!K55_PROBE_LIST
    sta.b !K54_PROBES
    lda.b !actor_x+1
    sta.b !K54_HOME_X
    lda.b !actor_y+1
    sta.b !K55_HOME_Y
    sep.b #$20
    lda.b #$18
    sta.b !K54_TRAIL
    lda.b #$80
    tsb.b !actor_08
    lda.b #$08
    trb.b !actor_08
    lda.b !actor_07
    and.b #$10
    beq .leap
    inc.b !actor_facing
.leap:
    lda.b #$04
    tsb.b !actor_07
    jsr.w ActorSpawn5E
    lda.b #$06
    cop.b #$00
    lda.b #$08
    tsb.b !actor_08
    lda.b #$80
    trb.b !actor_08
    lda.b #$02
    jsl.l AnimStart
    lda.b !actor_07
    and.b #$03
    tax
    ldy.w !K55_ACROSS,x
    jsl.l ActorSpeedXSet
    lda.b #$16
    sta.b !actor_accel_y
    ldy.b #$96
    lda.b !actor_07
    and.b #$03
    cmp.b #$02
    beq .rise
    ldy.b #$3F
.rise:
    jsl.l ActorSpeedYSet
    lda.b #$25
    jsl.l SoundQueueAdd
.fly:
    brk.b #$00
    jsl.l ActorFly
    lda.b !actor_speed_y+2
    bmi .trail
    lda.b !K54_LAST_RISE
    bpl .trail
    lda.b #$00
    jsl.l AnimStart
.trail:
    lda.b !actor_speed_y+2
    sta.b !K54_LAST_RISE
    dec.b !K54_TRAIL
    bne .home
    lda.b #$08
    sta.b !K54_TRAIL
    jsr.w ActorSpawnCopy
.home:
    rep.b #$20
    lda.b !K55_HOME_Y
    cmp.b !actor_y+1
    sep.b #$20
    bcs .fly
    lda.b #$04
    trb.b !actor_07
    jsr.w ActorSpawn5E
    lda.b #$26
    jsl.l SoundQueueAdd
    brk.b #$00
    lda.b #$80
    tsb.b !actor_08
    lda.b #$08
    trb.b !actor_08
    lda.b #$40
    cop.b #$00
    rep.b #$20
    lda.b !K54_HOME_X
    sta.b !actor_x+1
    lda.b !K55_HOME_Y
    sta.b !actor_y+1
    sep.b #$20
    jmp.w .leap
assert pc() == $83D8CE
