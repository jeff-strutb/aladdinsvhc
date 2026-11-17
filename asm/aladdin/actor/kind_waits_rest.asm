; kind_waits_rest.asm: the wait routines of the actor kinds whose waits
; had no C (the actor loop's wait table, 83:8F4F, reaches them by JSR each
; frame an actor of the kind waits): most animate, then end the actor when
; it is out of range (ActorEndOutOfRange, by its box); two do nothing.
; Written from their bytes.
; C: src/aladdin/actor/kind_waits_rest.c.

incsrc "../include/macros.inc"

arch 65816

!KWR_ACTOR_07 = $07            ; passed from a spawner to its child
!KWR_K2B_4B = $4B              ; cleared while bit 4 of ACTOR_07 is set
!KWR_K2B_STAYS = $4E           ; nonzero: it is never ended out of range
!KWR_K2C_FRAMES = $43          ; frames to its next sound, from 6
!KWR_K2C_SOUNDS = $44          ; nonzero: it plays sound $40 every 6 frames
!KWR_CARRYING = $42            ; nonzero: it carries (ActorTestCarries)
!KWR_K41_ACTIVE = $42          ; nonzero: it turns shots back and Aladdin can stand on it
!KWR_K41_FRAMES = $43          ; frames before he can stand on it again
!KWR_K41_Y = $0F               ; its y (word)
!KWR_K41_Y_LIMIT = $4A         ; below this y (word) it may end
!KWR_K41_X_LIMIT = $48         ; the x (word) its side is judged by
!KWR_K41_SIDE = $14            ; 0: it ends left of K41_X_LIMIT; 1: right
!KWR_X = $0C                   ; its x (word)
!KWR_HEALTH = $0367            ; the player's health
!KWR_K5A_SHAPE = $27           ; 0, 7, 8 or 9: its sprites get priority 3
!KWR_ACTOR_09 = $09            ; its flags
!KWR_K50_FACING = $14          ; negative: it faces left
!KWR_K50_DOWNED = $45          ; counted up each time a shot downs it facing right
!KWR_K42_PARENT = $40          ; the record (word) whose $4F it sets
!KWR_V_03F5 = $03F5            ; nonzero: no mark on the parent
!KWR_K57_FIRST_X = $0937       ; the first record's x (word)
!KWR_K57_RIGHT_OF = $146C      ; 1: the first record is at or right of it
!KWR_K57_POSE = $2D            ; its pose
!KWR_K57_FRAME = $2F           ; its animation's frame
!KWR_K57_MODE = $43            ; 0: in play; 1: still; 2 up: posed by facing
!KWR_K57_FACES = $44           ; nonzero: it turns to face Aladdin
!KWR_K57_TOUCH = $42           ; 0: hurts him; positive: stomp or hurt
!KWR_K57_FACING_SRC = $1F      ; its sign picks pose 8 or $0B
!KWR_FACING = $14
!KWR_FLIP = $15
!KWR_K71_SIZE = $27            ; its size: its hurt zone is ($0F + it) * 4
!KWR_BOB_STATE = $13           ; 0 still; 1-2 rising; 3-4 falling back
!KWR_BOB_Y = $0F               ; its y (word)
!KWR_BOB_LINE = $40            ; the y (word) it turns at
!KWR_ACTOR_01 = $01            ; frames left to wait
!KWR_ACTOR_03 = $03            ; resume address (low, high at $04)
!KWR_ACTOR_04 = $04
!KWR_K5D_STATE = $13           ; 0 waiting; 1-4 the timed steps
!KWR_K5D_COUNT = $41           ; frames left in the step
!KWR_K5D_GO = $4A              ; nonzero: it starts
!KWR_K5D_48 = $48
!KWR_K5D_49 = $49
!KWR_ACTOR_08 = $08
!KWR_RANGE_X = $1A44           ; the point actors are kept in range of (x, word)

; @routine 83:91F3 ActorWaitNothing
; WHAT IT DOES: The wait of the kinds that do nothing while they wait
; (kinds 4-6, 9-$0B and others).
; HOW: RTS.
; Entry: any; called by JSR through the actor loop's wait table. Exit:
; nothing changed.
%at($8391F3)
ActorWaitNothing:
    rts

; @routine 83:BBF1 Kind34Wait
; WHAT IT DOES: Kind $34's wait: nothing.
; HOW: RTS.
; Entry: any; called by JSR through the actor loop's wait table. Exit:
; nothing changed.
%at($83BBF1)
Kind34Wait:
    rts

; @routine 83:9D46 Kind23Wait
; WHAT IT DOES: Kind $23's wait: its animation.
; HOW: AnimStep, RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as AnimStep.
%at($839D46)
Kind23Wait:
    jsl.l AnimStep
    rts

; @routine 83:CE25 Kind48Wait
; WHAT IT DOES: Kind $48's wait: its animation.
; HOW: AnimStep, RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as AnimStep.
%at($83CE25)
Kind48Wait:
    jsl.l AnimStep
    rts

; @routine 83:D9B5 Kind56Wait
; WHAT IT DOES: Kind $56's wait: its animation.
; HOW: AnimStep, RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as AnimStep.
%at($83D9B5)
Kind56Wait:
    jsl.l AnimStep
    rts

; @routine 83:DEB9 Kind59Wait
; WHAT IT DOES: Kind $59's wait: its animation.
; HOW: AnimStep, RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as AnimStep.
%at($83DEB9)
Kind59Wait:
    jsl.l AnimStep
    rts

; @routine 83:E5EC Kind65Wait
; WHAT IT DOES: Kind $65's wait: its animation.
; HOW: AnimStep, RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as AnimStep.
%at($83E5EC)
Kind65Wait:
    jsl.l AnimStep
    rts

; @routine 83:F53C Kind77Wait
; WHAT IT DOES: The wait of kinds $77 and $78: its animation.
; HOW: AnimStep, RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as AnimStep.
%at($83F53C)
Kind77Wait:
    jsl.l AnimStep
    rts

; @routine 83:B488 Kind44Wait
; WHAT IT DOES: Kind $44's wait: its animation, then its end when out of
; range by box 0.
; HOW: AnimStep; X = 0; on into ActorEndOutOfRange (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
%at($83B488)
Kind44Wait:
    jsl.l AnimStep
    ldx.b #$00
    jml.l ActorEndOutOfRange

; @routine 83:B190 Kind3EWait
; WHAT IT DOES: Kind $3E's wait: its end when out of range by box $38.
; HOW: X = $38; on into ActorEndOutOfRange (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
%at($83B190)
Kind3EWait:
    ldx.b #$38
    jml.l ActorEndOutOfRange

; @routine 83:BB91 Kind33Wait
; WHAT IT DOES: Kind $33's wait: its end when out of range by box 8.
; HOW: X = 8; on into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
%at($83BB91)
Kind33Wait:
    ldx.b #$08
    jmp.w ActorEndOutOfRange

; @routine 83:BF7E Kind3AWait
; WHAT IT DOES: Kind $3A's wait: its end when out of range by box 8.
; HOW: X = 8; on into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
%at($83BF7E)
Kind3AWait:
    ldx.b #$08
    jmp.w ActorEndOutOfRange

; @routine 83:9CE2 Kind22Wait
; WHAT IT DOES: Kind $22's wait: Aladdin touching it is hurt; its end when
; out of range by box $18.
; HOW: ActorHurtsPlayerOnTouch; X = $18; on into ActorEndOutOfRange (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
%at($839CE2)
Kind22Wait:
    jsr.w ActorHurtsPlayerOnTouch
    ldx.b #$18
    jml.l ActorEndOutOfRange

; @routine 83:A709 Kind2BWait
; WHAT IT DOES: Kind $2B's wait: its animation; Aladdin touching it is
; hurt; a shot can down it; with bit 4 of ACTOR_07 its K2B_4B is cleared;
; unless K2B_STAYS, its end when out of range by box 8.
; HOW: AnimStep, ActorHurtsPlayerOnTouch, ActorDownedByShot; ACTOR_07 bit
; 4: K2B_4B 0. K2B_STAYS 0: X = 8, on into ActorEndOutOfRange; else RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange, or A =
; K2B_STAYS (nonzero), Z clear.
%at($83A709)
Kind2BWait:
    jsl.l AnimStep
    jsr.w ActorHurtsPlayerOnTouch
    jsr.w ActorDownedByShot
    lda.b !KWR_ACTOR_07
    and.b #$10
    beq .stays
    stz.b !KWR_K2B_4B
.stays:
    lda.b !KWR_K2B_STAYS
    bne .done
    ldx.b #$08
    jmp.w ActorEndOutOfRange
.done:
    rts

; @routine 83:A818 Kind2CWait
; WHAT IT DOES: Kind $2C's wait: its animation; while K2C_SOUNDS is set,
; sound $40 every 6 frames.
; HOW: AnimStep; K2C_SOUNDS nonzero: K2C_FRAMES down; at 0, SoundQueueAdd
; $40 and K2C_FRAMES 6. RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: M 8-bit.
%at($83A818)
Kind2CWait:
    jsl.l AnimStep
    lda.b !KWR_K2C_SOUNDS
    beq .done
    dec.b !KWR_K2C_FRAMES
    bne .done
    lda.b #$40
    jsl.l SoundQueueAdd
    lda.b #$06
    sta.b !KWR_K2C_FRAMES
.done:
    rts

; @routine 83:B43B Kind43Wait
; WHAT IT DOES: Kind $43's wait: not carrying, its animation; carrying,
; its shot's end and the carry test; then its end when out of range by
; box 0.
; HOW: CARRYING 0: AnimStep. Else ActorEndsShot, X = 0, ActorTestCarries.
; Then X = 0, on into ActorEndOutOfRange (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
%at($83B43B)
Kind43Wait:
    lda.b !KWR_CARRYING
    bne .carrying
    jsl.l AnimStep
    ldx.b #$00
    jml.l ActorEndOutOfRange
.carrying:
    jsr.w ActorEndsShot
    ldx.b #$00
    jsr.w ActorTestCarries
    ldx.b #$00
    jml.l ActorEndOutOfRange

; @routine 83:B1E8 Kind3FWait
; WHAT IT DOES: Kind $3F's wait: not carrying, its animation; carrying,
; the carry test with box 2; then its end when out of range by box 0.
; HOW: CARRYING 0: AnimStep. Else X = 2, ActorTestCarries. Then X = 0, on
; into ActorEndOutOfRange (JML).
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
%at($83B1E8)
Kind3FWait:
    lda.b !KWR_CARRYING
    bne .carrying
    jsl.l AnimStep
    bra .end
.carrying:
    ldx.b #$02
    jsr.w ActorTestCarries
.end:
    ldx.b #$00
    jml.l ActorEndOutOfRange

; @routine 83:B303 Kind41Wait
; WHAT IT DOES: Kind $41's wait. Active: it turns shots back; every $20
; frames at most, while he has health, Aladdin coming down on it (its
; box 8 pixels higher) stands on it, with sound $12; it animates; with bit 3 or 4 of
; ACTOR_07 it ends for good once below y $2F0 or once past K41_Y_LIMIT
; on its K41_SIDE of K41_X_LIMIT, else when out of range by box $28.
; Inactive: with bit 3 or 4 it ends for good once RANGE_X is $100 or
; more past it, else when out of range by box 0.
; HOW: K41_ACTIVE 0: ACTOR_07 & $18 zero: X = 0, ActorEndOutOfRange (JML);
; else its x - $100 < RANGE_X: RTS, else ActorEndDropReturn. Active:
; ActorTurnsShotBack; K41_FRAMES nonzero: down by 1; else K41_Y - 8 (old
; kept on the stack), HEALTH: ActorBouncesOrHurtsPlayer, A = 1 (stood on):
; SoundQueueAdd $12, K41_FRAMES $20; K41_Y back. AnimStep; ACTOR_07 & $18
; zero: X = $28, ActorEndOutOfRange (JML). K41_Y >= $2F0: ActorEndDropReturn.
; K41_Y >= K41_Y_LIMIT, or C of (x - K41_X_LIMIT) differing from K41_SIDE:
; X = 0, ActorEndOutOfRange (JML); else RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
; table. Exit: as ActorEndOutOfRange or ActorEndDropReturn, or M 8-bit.
;
%at($83B303)
Kind41Wait:
    lda.b !KWR_K41_ACTIVE
    beq .inactive
    jsr.w ActorTurnsShotBack
    lda.b !KWR_K41_FRAMES
    beq .stand
    dec.b !KWR_K41_FRAMES
    bra .anim
.stand:
    rep #$20
    lda.b !KWR_K41_Y
    pha
    clc
    adc.w #$FFF8
    sta.b !KWR_K41_Y
    sep #$20
    lda.w !KWR_HEALTH
    beq .back
    jsr.w ActorBouncesOrHurtsPlayer
    dec
    bne .back
    lda.b #$12
    jsl.l SoundQueueAdd
    lda.b #$20
    sta.b !KWR_K41_FRAMES
.back:
    rep #$20
    pla
    sta.b !KWR_K41_Y
    sep #$20
.anim:
    jsl.l AnimStep
    lda.b !KWR_ACTOR_07
    bit.b #$18
    bne .limits
    ldx.b #$28
    jml.l ActorEndOutOfRange
.limits:
    rep #$20
    lda.b !KWR_K41_Y
    cmp.w #$02F0
    bcs .gone
    cmp.b !KWR_K41_Y_LIMIT
    bcs .range
    lda.b !KWR_X
    cmp.b !KWR_K41_X_LIMIT
    sep #$20
    lda.b #$00
    rol
    eor.b !KWR_K41_SIDE
    beq .done
.range:
    sep #$20
    ldx.b #$00
    jml.l ActorEndOutOfRange
.done:
    rts
.inactive:
    lda.b !KWR_ACTOR_07
    bit.b #$18
    bne .past
    ldx.b #$00
    jml.l ActorEndOutOfRange
.past:
    rep #$20
    clc
    lda.b !KWR_X
    adc.w #$FF00
    cmp.w !KWR_RANGE_X
    sep #$20
    bcc .done
.gone:
    sep #$20
    jml.l ActorEndDropReturn

; @routine 83:DEF3 Kind5AWait
; WHAT IT DOES: Kind $5A's wait: its animation and picture; bit 5 of its
; sprite priority (ACTOR_09) cleared, or bits 4-5 set (priority 3) when
; K5A_SHAPE is 0, 7, 8 or 9.
; HOW: AnimStep, ActorPictureSend; TRB $20 on ACTOR_09; K5A_SHAPE 0, 7, 8
; or 9: TSB $30 on ACTOR_09. RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: M 8-bit.
;
%at($83DEF3)
Kind5AWait:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    lda.b #$20
    trb.b !KWR_ACTOR_09
    lda.b !KWR_K5A_SHAPE
    beq .set
    cmp.b #$07
    beq .set
    cmp.b #$08
    beq .set
    cmp.b #$09
    bne .done
.set:
    lda.b #$30
    tsb.b !KWR_ACTOR_09
.done:
    rts

; @routine 83:EC20 Kind6AWait
; WHAT IT DOES: Kind $6A's wait: its animation; Aladdin touching it is
; hurt; its end when out of range by box $18.
; HOW: AnimStep, ActorHurtsPlayerOnTouch; X = $18; on into
; ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: as ActorEndOutOfRange.
;
%at($83EC20)
Kind6AWait:
    jsl.l AnimStep
    jsr.w ActorHurtsPlayerOnTouch
    ldx.b #$18
    jmp.w ActorEndOutOfRange

; @routine 83:D1B0 Kind50Wait
; WHAT IT DOES: Kind $50's wait: its animation; Aladdin touching it is
; hurt or stomps it (a margin of $10); a shot downing it while it faces
; right counts in K50_DOWNED; its end when out of range by box 0.
; HOW: AnimStep, ActorStompOrHurt16, ActorDownedByShot; C clear (downed)
; and K50_FACING not negative: K50_DOWNED up. X = 0; on into
; ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
; table. Exit: as ActorEndOutOfRange.
;
%at($83D1B0)
Kind50Wait:
    jsl.l AnimStep
    jsr.w ActorStompOrHurt16
    jsr.w ActorDownedByShot
    bcs .end
    lda.b !KWR_K50_FACING
    bmi .end
    inc.b !KWR_K50_DOWNED
.end:
    ldx.b #$00
    jmp.w ActorEndOutOfRange

; @routine 83:C6E3 Kind42Wait
; WHAT IT DOES: Kind $42's wait: a harmless shot downs it, a hitting one
; ends; Aladdin standing on it plays sound $12; hurt by it, with V_03F5
; clear, its parent record's $4F is set to 1 when 0; its animation; its
; end when out of range by box 0.
; HOW: ActorDownedByHarmlessShot; ActorBouncesOrHurtsPlayer: A 1
; (stood on): SoundQueueAdd $12; A 2 (hurt), V_03F5 0: X = K42_PARENT
; (16-bit), its $4F 0: 1. AnimStep; X = 0; on into ActorEndOutOfRange.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
; table. Exit: as ActorEndOutOfRange.
;
%at($83C6E3)
Kind42Wait:
    jsr.w ActorDownedByHarmlessShot
    jsr.w ActorBouncesOrHurtsPlayer
    beq .anim
    dec
    bne .hurt
    lda.b #$12
    jsl.l SoundQueueAdd
    bra .anim
.hurt:
    lda.w !KWR_V_03F5
    bne .anim
    rep #$10
    ldx.b !KWR_K42_PARENT
    lda.w $004F,x
    bne .set
    lda.b #$01
    sta.w $004F,x
.set:
    sep #$10
.anim:
    jsl.l AnimStep
    ldx.b #$00
    jmp.w ActorEndOutOfRange

; @routine 83:DAD6 Kind57Wait
; WHAT IT DOES: Kind $57's wait: K57_RIGHT_OF set when the first record is
; at or right of it. In play (K57_MODE 0): animated and drawn; in pose $18,
; or $0C at its first frame, Aladdin in its zone $38 is hurt; shots hit it
; (state $CC); with K57_FACES it turns to face him; touching it, by
; K57_TOUCH, hurts him, or he stomps it or is hurt, or nothing. Still
; (K57_MODE 1): animated and drawn. Posed (2 up): pose 8 (K57_FACING_SRC
; negative) or $0B started when its frame is not $10 or $16, then
; animated and drawn.
; HOW: K57_FIRST_X compared with its x (16-bit), C rotated into
; K57_RIGHT_OF. K57_MODE 0: AnimStep, ActorPictureSend; K57_POSE $18, or
; $0C with K57_FRAME 0: Y = $38, ActorZoneHasTestBox, C clear:
; PlayerEnter8CFacing. ActorHitByShotCC. K57_FACES: ActorSideOfPlayer, X
; to the facing ($14) and flip ($15). K57_TOUCH 0: ActorHurtsPlayerOnTouch;
; positive: ActorStompOrHurt. Else A = $10 ($16 when K57_FACING_SRC is not
; negative); K57_MODE 1 or K57_FRAME = A: skip; else AnimStart with A / 2.
; Then AnimStep, ActorPictureSend.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
; table. Exit: M 8-bit.
;
%at($83DAD6)
Kind57Wait:
    rep #$20
    lda.w !KWR_K57_FIRST_X
    cmp.b !KWR_X
    sep #$20
    lda.b #$00
    rol
    sta.w !KWR_K57_RIGHT_OF
    lda.b !KWR_K57_MODE
    bne .posed
    jsl.l AnimStep
    jsl.l ActorPictureSend
    lda.b !KWR_K57_POSE
    cmp.b #$18
    beq .zone
    cmp.b #$0C
    bne .shots
    lda.b !KWR_K57_FRAME
    bne .shots
.zone:
    ldy.b #$38
    jsr.w ActorZoneHasTestBox
    bcs .shots
    jsr.w PlayerEnter8CFacing
.shots:
    jsr.w ActorHitByShotCC
    lda.b !KWR_K57_FACES
    beq .touch
    jsl.l ActorSideOfPlayer
    stx.b !KWR_FACING
    stx.b !KWR_FLIP
.touch:
    lda.b !KWR_K57_TOUCH
    beq .hurts
    bmi .done
    jsr.w ActorStompOrHurt
    rts
.hurts:
    jsr.w ActorHurtsPlayerOnTouch
.done:
    rts
.posed:
    dec
    beq .still
    lda.b #$10
    ldx.b !KWR_K57_FACING_SRC
    bmi .pick
    lda.b #$16
.pick:
    cmp.b !KWR_K57_FRAME
    beq .still
    lsr
    jsl.l AnimStart
.still:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    rts

; @routine 83:F3F1 Kind71Wait
; WHAT IT DOES: Kind $71's wait: Aladdin in its hurt zone (by K71_SIZE)
; is hurt; its animation; it ends once ahead of the view (bit 4 of
; ACTOR_07 set) or left behind (clear).
; HOW: Y = (K71_SIZE + $0F) * 4, ActorZoneHurtsPlayer; AnimStep; ACTOR_07
; bit 4: on into ActorEndIfAhead, else ActorEndIfLeftBehind (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR through the actor loop's wait table. Exit: as ActorEndIfAhead or
; ActorEndIfLeftBehind.
;
%at($83F3F1)
Kind71Wait:
    lda.b !KWR_K71_SIZE
    clc
    adc.b #$0F
    asl
    asl
    tay
    jsr.w ActorZoneHurtsPlayer
    jsl.l AnimStep
    lda.b !KWR_ACTOR_07
    and.b #$10
    beq .behind
    jmp.w ActorEndIfAhead
.behind:
    jmp.w ActorEndIfLeftBehind

; @routine 83:E1EE Kind5CWait
; WHAT IT DOES: Kind $5C's wait: it bobs about its line, animated and
; drawn; out of the view (more than $140 pixels to either side of
; RANGE_X), its code goes on at K5C_GONE_AT next; in view with Aladdin
; out of health, at K5C_DONE_AT after one frame, its wait over.
; HOW: ActorBobAboutLine, AnimStep, ActorPictureSend; its x - RANGE_X +
; $140 (16-bit) not below $280: resume address ($03-$04) = K5C_GONE_AT,
; RTS. Else HEALTH 0: resume = K5C_DONE_AT, frames ($01) 1, bit 7 of
; ACTOR_09 cleared (ASL, LSR). RTS.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
; table. Exit: M 8-bit.
;
%at($83E1EE)
Kind5CWait:
    jsr.w ActorBobAboutLine
    jsl.l AnimStep
    jsl.l ActorPictureSend
    rep #$20
    sec
    lda.b !KWR_X
    sbc.w !KWR_RANGE_X
    adc.w #$0140
    cmp.w #$0280
    sep #$20
    bcc .inview
    lda.b #$28
    sta.b !KWR_ACTOR_03
    lda.b #$E1
    sta.b !KWR_ACTOR_04
    rts
.inview:
    lda.w !KWR_HEALTH
    bne .done
    lda.b #$82
    sta.b !KWR_ACTOR_03
    lda.b #$E2
    sta.b !KWR_ACTOR_04
    lda.b #$01
    sta.b !KWR_ACTOR_01
    asl.b !KWR_ACTOR_09
    lsr.b !KWR_ACTOR_09
.done:
    rts

; @routine 83:E239 ActorBobAboutLine
; WHAT IT DOES: Moves the actor by BOB_STATE: 0 still; 1 motion preset
; $47, then (2) flown against its motion until its y is above BOB_LINE;
; 3 preset $48, then (4) flown until its y is back at or below the line,
; and again.
; HOW: JMP (BOB_TABLE,X) by BOB_STATE * 2. 1: Y = $47, ActorMotionPreset,
; BOB_STATE up, on into 2. 2: ActorFlyAgainst; BOB_Y below BOB_LINE: BOB_STATE
; up; else RTS. 3: Y = $48, ActorMotionPreset, BOB_STATE up, on into
; 4. 4: ActorFly; BOB_Y below BOB_LINE: RTS; else BOB_STATE 1.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR. Exit:
; M 8-bit.
;
%at($83E239)
ActorBobAboutLine:
    lda.b !KWR_BOB_STATE
    asl
    tax
    jmp.w (.states,x)
.states:
    dw .still, .rise, .rising, .fall, .falling
.rise:
    ldy.b #$47
    jsl.l ActorMotionPreset
    inc.b !KWR_BOB_STATE
.rising:
    jsl.l ActorFlyAgainst
    rep #$20
    lda.b !KWR_BOB_Y
    cmp.b !KWR_BOB_LINE
    sep #$20
    bcs .still
    inc.b !KWR_BOB_STATE
.still:
    rts
.fall:
    ldy.b #$48
    jsl.l ActorMotionPreset
    inc.b !KWR_BOB_STATE
.falling:
    jsl.l ActorFly
    rep #$20
    lda.b !KWR_BOB_Y
    cmp.b !KWR_BOB_LINE
    sep #$20
    bcc .back
    lda.b #$01
    sta.b !KWR_BOB_STATE
.back:
    rts

; @routine 83:E352 Kind5DWait
; WHAT IT DOES: Kind $5D's wait, a timed sequence: K5D_49 = K5D_48 and its
; animation each frame; once K5D_GO is set, $B0 frames; bit 4 of ACTOR_08
; set; $1F frames, then animation 1, its code to resume at K5D_NEXT_AT;
; $38 frames, then it ends for good.
; HOW: K5D_48 to K5D_49, AnimStep; JMP (K5D_TABLE,X) by K5D_STATE * 2.
; 0: K5D_GO set: K5D_COUNT $B0, state up. 1: K5D_COUNT down, at 0 state
; up. 2: ACTOR_08 |= $10, K5D_COUNT $1F, state up. 3: K5D_COUNT down, at
; 0: AnimStart 1, K5D_COUNT $38, resume address ($03-$04) = K5D_NEXT_AT,
; state up. 4: K5D_COUNT down, at 0: the loop's return address dropped
; (PLA PLA), on into ActorEndPlaced ($83:96CA).
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor loop's wait table. Exit: M 8-bit; or ended: no return.
;
%at($83E352)
Kind5DWait:
    lda.b !KWR_K5D_48
    sta.b !KWR_K5D_49
    jsl.l AnimStep
    lda.b !KWR_K5D_STATE
    asl
    tax
    jmp.w (.steps,x)
.steps:
    dw .wait, .first, .flag, .second, .last
.wait:
    lda.b !KWR_K5D_GO
    beq .done0
    lda.b #$B0
    sta.b !KWR_K5D_COUNT
    inc.b !KWR_K5D_STATE
.done0:
    rts
.first:
    dec.b !KWR_K5D_COUNT
    bne .done1
    inc.b !KWR_K5D_STATE
.done1:
    rts
.flag:
    lda.b !KWR_ACTOR_08
    ora.b #$10
    sta.b !KWR_ACTOR_08
    lda.b #$1F
    sta.b !KWR_K5D_COUNT
    inc.b !KWR_K5D_STATE
    rts
.second:
    dec.b !KWR_K5D_COUNT
    bne .done3
    lda.b #$01
    jsl.l AnimStart
    lda.b #$38
    sta.b !KWR_K5D_COUNT
    lda.b #$0E
    sta.b !KWR_ACTOR_03
    lda.b #$E3
    sta.b !KWR_ACTOR_04
    inc.b !KWR_K5D_STATE
.done3:
    rts
.last:
    dec.b !KWR_K5D_COUNT
    bne .done4
    pla
    pla
    jmp.w ActorEndPlaced
.done4:
    rts
