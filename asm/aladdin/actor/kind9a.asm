; kind9a.asm: actor kinds $9A and $99 (stage a5 s2): kind $9A, dropped by
; kind $98, falls to the ground and bursts into 32 kind $99: two that run
; off along the ground, the rest flashing out in a spread and gone. Their
; waits are Kind9AWaitWork and Kind99WaitWork (kind_waits_84.asm).
; C: src/aladdin/actor/kind9a.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!ACTOR_PROBES = $16             ; its probe list's address (word)
!K9A_FRAMES = $40               ; kind $9A: pairs of children left to make
!K9A_BOUNCES = $41              ; kind $9A: nonzero once burst: Aladdin bounces off it (Kind9AWaitWork)
!K9A_PROBES = $D96E             ; (data bank $80) kind $9A's probes
!K9A_CHILD_KIND = $99
!K99_ANIMS = $D90E              ; (data bank $80) kind $99's animation by $07 (bytes)
!K99_SPREAD = $D92E             ; (data bank $80) kind $99's offset across by $07 (words)

; @routine 84:A3B7 Kind99Start
; @actor
; WHAT IT DOES: Actor kind $99, a piece of a kind $9A's burst, by its $07
; (0-31, from its maker): 0 and 1 run off along the ground, right or
; left, until they can no longer be hit; the others show for 8 frames, are
; put across by their offset (right for even $07, left for odd), hide for
; 16 frames and end.
; HOW: $08 bit 1 and $09 bits $90 set; facing and flip = $07 AND 1;
; AnimStart K99_ANIMS by $07. $07 AND $FE zero: COP 8; its speed across
; 8 pixels; each frame (BRK) ActorMoveX while bit 6 of $09 is set; then
; ActorEndPlaced. Else: $08 bit 3 cleared; COP 8; 16-bit, its x plus
; (or, $07 odd, less) K99_SPREAD by $07 AND $1F; $08 bit 3 set (TSB);
; COP $10; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $99 in state $0C (the table at $83:8DAB,
; through the stub at $83:93CB). Exit: never returns.
%at($84A3B7)
Kind99Start:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    lda.b !actor_07
    and.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    ldx.b !actor_07
    lda.w !K99_ANIMS,x
    jsl.l AnimStart
    lda.b !actor_07
    and.b #$FE
    beq .run
    lda.b !actor_08
    and.b #$F7
    sta.b !actor_08
    lda.b #$08
    cop.b #$00
    lda.b !actor_07
    and.b #$01
    bne .left
    rep.b #$20
    lda.b !actor_07
    and.w #$001F
    asl a
    tax
    lda.b !actor_x+1
    clc
    adc.w !K99_SPREAD,x
    sta.b !actor_x+1
    sep.b #$20
    bra .hide
.left:
    rep.b #$20
    lda.b !actor_07
    and.w #$001F
    asl a
    tax
    lda.b !actor_x+1
    sec
    sbc.w !K99_SPREAD,x
    sta.b !actor_x+1
    sep.b #$20
.hide:
    lda.b #$08
    tsb.b !actor_08
    lda.b #$10
    cop.b #$00
    bra .end
.run:
    lda.b #$08
    cop.b #$00
    lda.b #$08
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x
    stz.b !actor_speed_x+2
.running:
    brk.b #$00
    jsl.l ActorMoveX
    lda.b !actor_09
    and.b #$40
    bne .running
.end:
    jml.l ActorEndPlaced
%ends_at($84A438)

; @routine 84:A446 Kind9AStart
; @actor
; WHAT IT DOES: Actor kind $9A, dropped by kind $98: falls until it is in
; the ground, sets down 4 pixels lower with sound $48, and after 8 frames
; with sound $3E bursts: two kind $99 every 2 frames, 32 in all ($07 0 to
; 31). Then Aladdin bounces off it (K9A_BOUNCES); after $80 frames it
; hides ($08 bit 4), after $80 more it ends.
; HOW: K9A_BOUNCES 0; $08 bit 1 and $09 bits $90 set; 16-bit: probes
; K9A_PROBES, the look's palette bits $0600 set, its speed across 0, down
; $0100 (its top bytes 0); gravity $30; AnimStart $17. Each frame (BRK)
; ActorFly and ActorOutOfGround until that raises it (Z clear). Its y
; low byte + 4; AnimStart $18; SoundQueueAdd $48; COP 8; AnimStart $1E;
; SoundQueueAdd $3E; K9A_FRAMES $10, $07 0; each time COP 2, Kind9ASpawn
; twice. K9A_BOUNCES up; AnimStart $1F; COP $80; $08 bit 4 set; COP $80;
; ActorEndPlaced.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $9A in state $0C (the table at $83:8DAB,
; through the stub at $83:93D8). Exit: never returns.
%at($84A446)
Kind9AStart:
    stz.b !K9A_BOUNCES
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #!K9A_PROBES
    sta.b !ACTOR_PROBES
    lda.b !actor_look
    ora.w #$0600
    sta.b !actor_look
    stz.b !actor_speed_x
    lda.w #$0100
    sta.b !actor_speed_y
    sep.b #$20
    stz.b !actor_speed_x+2
    stz.b !actor_speed_y+2
    lda.b #$30
    sta.b !actor_accel_y
    lda.b #$17
    jsl.l AnimStart
.falling:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfGround
    beq .falling
    lda.b !actor_y+1
    clc
    adc.b #$04
    sta.b !actor_y+1
    lda.b #$18
    jsl.l AnimStart
    lda.b #$48
    jsl.l SoundQueueAdd
    lda.b #$08
    cop.b #$00
    lda.b #$1E
    jsl.l AnimStart
    lda.b #$3E
    jsl.l SoundQueueAdd
    lda.b #$10
    sta.b !K9A_FRAMES
    stz.b !actor_07
.burst:
    lda.b #$02
    cop.b #$00
    jsr.w Kind9ASpawn
    jsr.w Kind9ASpawn
    dec.b !K9A_FRAMES
    bne .burst
    inc.b !K9A_BOUNCES
    lda.b #$1F
    jsl.l AnimStart
    lda.b #$80
    cop.b #$00
    lda.b !actor_08
    ora.b #$10
    sta.b !actor_08
    lda.b #$80
    cop.b #$00
    bra Kind9AStartEnd

; @routine 84:A4D4 Kind9ASpawn
; WHAT IT DOES: Makes one kind $99 of a kind $9A's burst, with its $07,
; then counts its $07 up for the next.
; HOW: Spawn place 0; ActorCreateChild8 with A = $99; $07 up.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR from Kind9AStart. Exit: M and X 8-bit; as ActorCreateChild8 leaves
; A and the flags but N and Z from the $07 increment.
Kind9ASpawn:
    stz.b !actor_spawn_at
    lda.b #!K9A_CHILD_KIND
    jsl.l ActorCreateChild8
    inc.b !actor_07
    rts

; Kind9AStart's end, after Kind9ASpawn (its BRA at 84:A4D2).
Kind9AStartEnd:
    jml.l ActorEndPlaced
%ends_at($84A4E3)
