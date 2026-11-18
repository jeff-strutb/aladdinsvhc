; kind30.asm: actor kind $30, the pickups: by type, one with only its
; sound, a once-only item, a counted item, a health mark, health filled
; or one more, and the tens of a BCD count. Its wait is Kind30Wait
; ($83:B94A).
; C: src/aladdin/actor/kind30.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_state.inc"

arch 65816

!K30_16 = $16                   ; word: $D17A
!K30_SHOT = $43                 ; nonzero once a shot has hit it (type 7's wait sets it)
!K30_COUNT = $0364              ; counted up by each type 2 taken, to at most 100
!K30_TENS = $0369               ; BCD; types 6 and 7 add $10, to at most $99
!K30_ONCE = $036E               ; set by a type 1 taken; then type 1 starts as type 5
!K30_LEVEL_MARKS = $0358        ; a bit per level whose health mark (type 3) is taken
!K30_LEVEL_BITS = $D182         ; the level's bit in K30_LEVEL_MARKS, by LEVEL_035C (bytes, bank DB)
!K30_FALL_MOTIONS = $D188       ; types 6 and 7: the first drop's motion preset, by ACTOR_SPAWN_AT (bank DB)
!K30_PLACED_FLAGS = $7EC600     ; a byte per placement: bit 7 set, never placed again

; @routine 83:B684 Kind30Start
; @actor
; WHAT IT DOES: Actor kind $30, the pickups. Its type (bits 0-2 of $07)
; picks its picture and what taking it does; bits 3-4 how it appears:
; none set, it pops up and drops twice (ActorMotionPreset $27, then $29;
; types 6 and 7 the first by ACTOR_SPAWN_AT); $08, it turns round and
; drops once ($2C); $10, it is there at once. Then it waits until
; Aladdin touches it with health left, plays its sound and ends; types
; 0-5 are not placed again. Type 0: nothing more (sound $0B). Type 1:
; K30_ONCE set (sound 9); once that is set, a type 1 starts as type 5.
; Type 2: AnimStart 0 and palette 1 (look bits 1-3); taken, K30_COUNT
; up, to at most 100; below 100 also $07 = $10, ACTOR_SPAWN_AT 0 and
; ActorCreateChild8 with A = $85 (sound 9; sound $0B at 100). Type 3: a
; health mark, once a level (the level's bit in K30_LEVEL_MARKS):
; HEALTH_MARKS up, to at most 10, and HEALTH filled to them (sound 9);
; the level's bit already set, it starts as type 4. Type 4: HEALTH
; filled to HEALTH_MARKS (sound $0B). Type 5: HEALTH up by one, to at
; most HEALTH_MARKS (sound $0B). Types 6 and 7: K30_TENS up by $10 (BCD
; tens), to at most $99 (sound $0B), and they may be placed again. Type
; 6 that has dropped drops again; type 7 waits (type 6 too with $10),
; and once a shot has hit it (K30_SHOT, set by type 7's wait) it hops
; ($2C, then $29), turning at a wall.
; HOW: K30_16 = $D17A, ACTOR_LOOK = $0400 (words); ACTOR_08 = 3;
; ACTOR_09 bits $90 set; AnimStart with the type; ACTOR_FLIP 0; JMP
; (.starts,X) by type * 2. Appearing: bits $18 clear: ActorMotionPreset
; Y = $27 (types 6, 7: K30_SHOT cleared, Y from K30_FALL_MOTIONS by
; ACTOR_SPAWN_AT), each frame (BRK) ActorFly, ActorOutOfWallProbe4,
; ActorOutOfGround until on the ground; then Y = $29 the same; bit 3
; only: ACTOR_FACING up by one, Y = $2C the same; then JMP (.waits,X) by
; type * 2. Waiting: each frame (BRK) ActorHitsTestBoxLong until C clear
; with HEALTH nonzero; then the type's effect, SoundQueueAdd,
; PLACED_FLAGS by ACTOR_34 = $80 (not for types 6, 7), and JML
; ActorEndPlaced. Waiting at .tens (type 7, or 6 with $10) not touched
; (C set) with K30_SHOT nonzero: ACTOR_FLIP 0; Y = $2C, each frame
; ActorFly, ActorOutOfWallProbe4 (C set: ACTOR_FACING bit 0 turned),
; ActorOutOfGround until on the ground; then Y = $29 the same (no turn);
; K30_SHOT cleared. Kind $30's second state is the JML at .second.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $30 in its start state (the table at
; $83:8CAB). Exit: never returns.
%at($83B684)
Kind30Start:
    rep.b #$20
    lda.w #$D17A
    sta.b !K30_16
    lda.w #$0400
    sta.b !actor_look
    sep.b #$20
    lda.b #$03
    sta.b !actor_08
    lda.b #$90
    tsb.b !actor_09
    lda.b !actor_07
    and.b #$07
    jsl.l AnimStart
    stz.b !actor_flip
    lda.b !actor_07
    and.b #$07
    asl a
    tax
    jmp.w (.starts,x)
.starts:
    dw .type0, .type1, .type2, .type3, .type4, .type5, .tens_drop, .tens_drop
.type0:
    lda.b !actor_07
    and.b #$18
    bne .lB6C6
    jmp.w .drop
.lB6C6:
    and.b #$10
    bne .wait0
    jmp.w .turn_drop
.wait0:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcs .wait0
    lda.w !health
    beq .wait0
    lda.b #$0B
    jsl.l SoundQueueAdd
    ldx.b !actor_34
    lda.b #$80
    sta.l !K30_PLACED_FLAGS,x
    jmp.w .second
.type1:
    lda.w !K30_ONCE
    beq .lB6FF
    lda.b !actor_07
    ora.b #$04
    sta.b !actor_07
    and.b #$07
    jsl.l AnimStart
    jmp.w .type5
.lB6FF:
    lda.b !actor_07
    and.b #$18
    bne .lB708
    jmp.w .drop
.lB708:
    and.b #$10
    bne .wait1
    jmp.w .turn_drop
.wait1:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcs .wait1
    lda.w !health
    beq .wait1
    lda.b #$01
    sta.w !K30_ONCE
    lda.b #$09
    jsl.l SoundQueueAdd
    ldx.b !actor_34
    lda.b #$80
    sta.l !K30_PLACED_FLAGS,x
    jmp.w .second
.type2:
    lda.b !actor_look+1
    and.b #$F1
    ora.b #$02
    sta.b !actor_look+1
    lda.b #$00
    jsl.l AnimStart
    lda.b !actor_07
    and.b #$18
    bne .lB749
    jmp.w .drop
.lB749:
    and.b #$10
    bne .wait2
    jmp.w .turn_drop
.wait2:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcs .wait2
    lda.w !health
    beq .wait2
    lda.w !K30_COUNT
    cmp.b #$64
    beq .lB777
    inc.w !K30_COUNT
    lda.b #$10
    sta.b !actor_07
    stz.b !actor_spawn_at
    lda.b #$85
    jsl.l ActorCreateChild8
    lda.b #$09
    bra .lB779
.lB777:
    lda.b #$0B
.lB779:
    jsl.l SoundQueueAdd
    ldx.b !actor_34
    lda.b #$80
    sta.l !K30_PLACED_FLAGS,x
    jmp.w .second
.type3:
    ldx.w !level_035c
    lda.w !K30_LEVEL_BITS,x
    and.w !K30_LEVEL_MARKS
    beq .lB7A5
    lda.b #$04
    jsl.l AnimStart
    lda.b !actor_07
    and.b #$F8
    clc
    adc.b #$04
    sta.b !actor_07
    jmp.w .type4
.lB7A5:
    lda.b !actor_07
    and.b #$18
    bne .lB7AE
    jmp.w .drop
.lB7AE:
    and.b #$10
    bne .wait3
    jmp.w .turn_drop
.wait3:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcs .wait3
    lda.w !health
    beq .wait3
    lda.w !health_marks
    cmp.b #$0A
    beq .lB7CC
    inc.w !health_marks
.lB7CC:
    ldx.w !level_035c
    lda.w !K30_LEVEL_BITS,x
    tsb.w !K30_LEVEL_MARKS
    lda.w !health_marks
    sta.w !health
    lda.b #$09
    jsl.l SoundQueueAdd
    ldx.b !actor_34
    lda.b #$80
    sta.l !K30_PLACED_FLAGS,x
    jmp.w .second
.type4:
    lda.b !actor_07
    and.b #$18
    bne .lB7F5
    jmp.w .drop
.lB7F5:
    and.b #$10
    bne .wait4
    jmp.w .turn_drop
.wait4:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcs .wait4
    lda.w !health
    beq .wait4
    lda.w !health_marks
    sta.w !health
    lda.b #$0B
    jsl.l SoundQueueAdd
    ldx.b !actor_34
    lda.b #$80
    sta.l !K30_PLACED_FLAGS,x
    jmp.w .second
.type5:
    lda.b !actor_07
    and.b #$18
    bne .lB829
    jmp.w .drop
.lB829:
    and.b #$10
    bne .wait5
    jmp.w .turn_drop
.wait5:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcs .wait5
    lda.w !health
    beq .wait5
    lda.w !health_marks
    cmp.w !health
    beq .lB848
    inc.w !health
.lB848:
    lda.b #$0B
    jsl.l SoundQueueAdd
    ldx.b !actor_34
    lda.b #$80
    sta.l !K30_PLACED_FLAGS,x
    jmp.w .second
.tens_drop:
    stz.b !K30_SHOT
    lda.b !actor_07
    and.b #$18
    bne .lB86D
    ldx.b !actor_spawn_at
    ldy.w !K30_FALL_MOTIONS,x
    jsl.l ActorMotionPreset
    jmp.w .drop_y
.lB86D:
    and.b #$10
    bne .tens
    jmp.w .turn_drop
.tens:
    brk.b #$00
    jsl.l ActorHitsTestBoxLong
    bcc .lB87F
    jmp.w .hop
.lB87F:
    lda.w !health
    beq .tens
    lda.w !K30_TENS
    cmp.b #$99
    beq .lB89F
    clc
    lda.w !K30_TENS
    adc.b #$10
    sta.w !K30_TENS
    and.b #$F0
    cmp.b #$A0
    bne .lB89F
    lda.b #$99
    sta.w !K30_TENS
.lB89F:
    lda.b #$0B
    jsl.l SoundQueueAdd
    jmp.w .second
.drop:
    ldy.b #$27
.drop_y:
    jsl.l ActorMotionPreset
.lB8AE:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorOutOfGround
    beq .lB8AE
    ldy.b #$29
    jsl.l ActorMotionPreset
.lB8C4:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorOutOfGround
    beq .lB8C4
    bra .landed
.turn_drop:
    inc.b !actor_facing
    ldy.b #$2C
    jsl.l ActorMotionPreset
.lB8DE:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorOutOfGround
    beq .lB8DE
.landed:
    lda.b !actor_07
    and.b #$07
    asl a
    tax
    jmp.w (.waits,x)
.waits:
    dw .wait0, .wait1, .wait2, .wait3, .wait4, .wait5, .tens_drop, .tens
.hop:
    lda.b !K30_SHOT
    beq .lB943
    stz.b !actor_flip
    ldy.b #$2C
    jsl.l ActorMotionPreset
.lB913:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    bcc .lB925
    lda.b !actor_facing
    eor.b #$01
    sta.b !actor_facing
.lB925:
    jsl.l ActorOutOfGround
    beq .lB913
    ldy.b #$29
    jsl.l ActorMotionPreset
.lB931:
    brk.b #$00
    jsl.l ActorFly
    jsl.l ActorOutOfWallProbe4
    jsl.l ActorOutOfGround
    beq .lB931
    stz.b !K30_SHOT
.lB943:
    jmp.w .tens
.second:
    jml.l ActorEndPlaced
