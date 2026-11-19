; kind79.asm: actor kind $79, which starts below the point actors are
; kept in range of and rises until it is taken up (its $42 set), then
; carries the player while he holds on.
; C: src/aladdin/actor/kind79.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!RANGE_Y = $1A46                ; the y of the point actors are kept in range of (word)
!KIND79_FIRST_Y = $45           ; the first of a pair: its y as it was placed (word)
!KIND79_TAKEN = $42             ; nonzero: it is taken up, and goes on
!KIND79_43 = $43                ; counted up each frame a shot reaches it
!KIND79_SLOT = $44              ; its offset in SLOT_WORDS
!KIND79_WAIT = $47              ; frames left before it can be taken up (counted down)
!KIND79_LOW = $49               ; nonzero: it came up past y $0032 (hit state each frame)
!RANGE_X = $1A44                ; the x of the point actors are kept in range of (word)
!SLOT_WORDS = $1C77             ; a word per kind $79 slot, cleared as it leaves
!PLAYER_STATE_8C_COUNT = $141B   ; counted up on entering state $8C
!V_141C = $141C                 ; zero: no taking hold
!HEALTH = $0367                 ; the player's health
!PLAYER_RESUME = $08DE          ; the player's resume address (its record's $03, word)
!KIND79_ZONE = $80              ; the zone it is taken up by
!KIND79_RISE = $6F              ; its vertical speed's entry in the speed table
!KIND79_HOLD_CODE = $D6F8       ; the player's code while an actor holds him
!KIND79_HOLD_AT = $145D         ; where the player is held: x, y (words)
!KIND79_BOX_WANTED = $1448      ; the player's box wanted: $07-$0F while he holds on
!KIND79_HOLDER = $0912          ; the player's record's $37: the record of what holds him (word)
!KIND79_DIP = $6C               ; its vertical speed's entry as it dips with him
!KIND79_DIP_GRAVITY = $04       ; its gravity as it dips
!KIND79_LET_GO_WAIT = $10       ; frames before it can be taken up again

; @routine 83:F541 Kind79Rise
; @actor
; WHAT IT DOES: Actor kind $79: starts 32 pixels (an even $07: the first
; of a pair, which marks its $07 odd) or 112 pixels (an odd $07) below
; the point actors are kept in range of, and rises each frame until it
; is taken up.
; HOW: Even $07: $07 up, its y kept in KIND79_FIRST_Y, its y RANGE_Y plus
; $20, actor_09 = $90, actor_12 = 8. Odd: its y RANGE_Y plus $70. Then
; actor_08 = 2, animation 0, ActorSpeedYSet $6F, $41, KIND79_TAKEN,
; KIND79_43 and actor_49 cleared; each frame (BRK first)
; ActorStepYBack, until KIND79_TAKEN is set. Taken: the player's code
; KIND79_HOLD_CODE, KIND79_HOLD_AT its x and its y + $2B; BRK; animation
; 2, Kind79PlayerToIt, ActorSpeedYSetGravity (A 4, Y $6C). Each frame
; (BRK) ActorFallAgainst and Kind79Held (C set: Kind79PlayerToIt) while
; it falls or rises faster than $FFA0; ActorSpeedYSet $6F; KIND79_TAKEN
; cleared (let go): the rise again. Else each frame (BRK) ActorStepYBack
; and Kind79Held: C set, Kind79PlayerToIt; clear (let go), the rise again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $83; reached by
; the actor loop's jump for kind $79 in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83F541)
Kind79Rise:
    lda.b !actor_07
    and.b #$01
    bne .second
    inc.b !actor_07
    rep.b #$20
    lda.b !actor_y+1
    sta.b !KIND79_FIRST_Y
    clc
    lda.w !RANGE_Y
    adc.w #$0020
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$90
    sta.b !actor_09
    lda.b #$08
    sta.b !actor_12
    bra .placed
.second:
    rep.b #$20
    clc
    lda.w !RANGE_Y
    adc.w #$0070
    sta.b !actor_y+1
    sep.b #$20
.placed:
    lda.b #$02
    sta.b !actor_08
    lda.b #$00
    jsl.l AnimStart
    ldy.b #!KIND79_RISE
    jsl.l ActorSpeedYSet
    stz.b !actor_41
    stz.b !KIND79_TAKEN
    stz.b !KIND79_43
    stz.b !actor_49
.rise:
    brk.b #$00
    jsl.l ActorStepYBack
    lda.b !KIND79_TAKEN
    beq .rise
    lda.b #!KIND79_HOLD_CODE&255
    sta.w !PLAYER_RESUME
    lda.b #!KIND79_HOLD_CODE>>8
    sta.w !PLAYER_RESUME+1
    rep.b #$21
    lda.b !actor_x+1
    sta.w !KIND79_HOLD_AT
    lda.b !actor_y+1
    clc
    adc.w #$002B
    sta.w !KIND79_HOLD_AT+2
    sep.b #$20
    brk.b #$00
    lda.b #$02
    jsl.l AnimStart
    jsr.w Kind79PlayerToIt
    lda.b #!KIND79_DIP_GRAVITY
    ldy.b #!KIND79_DIP
    jsl.l ActorSpeedYSetGravity
.dip:
    sep.b #$20
.dipping:
    brk.b #$00
    jsl.l ActorFallAgainst
    jsr.w Kind79Held
    bcc .dipped
    jsr.w Kind79PlayerToIt
.dipped:
    lda.b !actor_speed_y+2
    bpl .dip
    rep.b #$20
    lda.b !actor_speed_y+1
    cmp.w #$FFA0
    sep.b #$20
    bcc .dipping
    ldy.b #!KIND79_RISE
    jsl.l ActorSpeedYSet
    lda.b !KIND79_TAKEN
    beq .rise
.carry:
    brk.b #$00
    jsl.l ActorStepYBack
    jsr.w Kind79Held
    bcc .let_go
    jsr.w Kind79PlayerToIt
    bra .carry
.let_go:
    bra .rise

; @routine 83:F5FD Kind79PlayerToIt
; WHAT IT DOES: Puts Aladdin at actor kind $79's place: its x, and its y
; plus $2B.
; HOW: 16-bit (C clear): the actor's x pixels to the player's; its y
; pixels + $2B to the player's.
; Entry: M 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: M 8-bit; A = the player's new y (16
; bits); N, V, Z, C from the addition.
%at($83F5FD)
Kind79PlayerToIt:
    rep.b #$21
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.b !actor_y+1
    clc
    adc.w #$002B
    sta.w !player+!actor_y+1
    sep.b #$20
    rts

; @routine 83:F610 Kind79Held
; WHAT IT DOES: Tells whether Aladdin still holds on to actor kind $79
; (his box wanted $07-$0F): if so it is marked as what holds him; if not
; it lets him go: animation 0, no longer taken, nothing holds him, and 16
; frames before it can be taken again.
; HOW: KIND79_BOX_WANTED from $07 to $0F: KIND79_HOLDER = D, C set. Else
; AnimStart 0, KIND79_TAKEN 0, KIND79_HOLDER 0 (byte by byte),
; KIND79_WAIT $10, C clear.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: M 8-bit, X 8-bit; C set
; while he holds on.
%at($83F610)
Kind79Held:
    ldx.w !KIND79_BOX_WANTED
    cpx.b #$07
    bcc .let_go
    cpx.b #$10
    bcs .let_go
    rep.b #$20
    tdc
    sta.w !KIND79_HOLDER
    sep.b #$20
    sec
    rts
.let_go:
    lda.b #$00
    jsl.l AnimStart
    stz.b !KIND79_TAKEN
    stz.w !KIND79_HOLDER
    stz.w !KIND79_HOLDER+1
    lda.b #!KIND79_LET_GO_WAIT
    sta.b !KIND79_WAIT
    clc
    rts

; @routine 83:F639 Kind79InRange
; WHAT IT DOES: Keeps actor kind $79 near the point actors are kept in
; range of: more than $D0 pixels to either side, its slot is cleared and
; it ends; from $98 pixels below to $B0 above, nothing; out of that
; height it starts again (state $0C), and if it had been taken up while
; Aladdin has health left, his code goes on at $CDD2.
; HOW: 16-bit: RANGE_X less its x, plus $D0, $01A0 or more:
; Kind79SlotClear, then ActorEndDropReturn (JMP): the actor loop's call
; dropped, the actor ended. Else RANGE_Y less its y, plus $98, below
; $0148: return. Else KIND79_TAKEN and HEALTH nonzero: PLAYER_RESUME =
; $CDD2; state = its low nibble | $0C.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by a jump from Kind79Wait, one JSR
; (the actor loop's) above the actor loop's level. Exit: M 8-bit; A =
; the new state, or the height test's difference (16 bits) with N, Z and
; C (clear) from its compare; or as ActorEndDropReturn.
%at($83F639)
Kind79InRange:
    rep.b #$20
    sec
    lda.w !RANGE_X
    sbc.b !actor_x+1
    clc
    adc.w #$00D0
    cmp.w #$01A0
    sep.b #$20
    bcc .in_x
    jsr.w Kind79SlotClear
    jmp.w ActorEndDropReturn
.in_x:
    rep.b #$20
    sec
    lda.w !RANGE_Y
    sbc.b !actor_y+1
    clc
    adc.w #$0098
    cmp.w #$0148
    sep.b #$20
    bcc .done
    lda.b !KIND79_TAKEN
    beq .restart
    lda.w !HEALTH
    beq .restart
    lda.b #$D2
    sta.w !PLAYER_RESUME
    lda.b #$CD
    sta.w !PLAYER_RESUME+1
.restart:
    lda.b !actor_state
    and.b #$0F
    ora.b #$0C
    sta.b !actor_state
.done:
    sep.b #$20
    rts

; @routine 83:F683 Kind79SlotClear
; WHAT IT DOES: Clears actor kind $79's word in SLOT_WORDS as it leaves.
; HOW: X = KIND79_SLOT; SLOT_WORDS + X and the byte after it 0.
; Entry: X 8-bit; D = the actor's record; DB a bank whose $0000-$1FFF is
; work RAM; called by JSR. Exit: X = KIND79_SLOT, N and Z from it.
%at($83F683)
Kind79SlotClear:
    ldx.b !KIND79_SLOT
    stz.w !SLOT_WORDS,x
    stz.w !SLOT_WORDS+1,x
    rts

; @routine 83:F68C Kind79Wait
; WHAT IT DOES: What actor kind $79 runs each frame it waits: its wait
; counted down; unless its $41 is set, it is taken up (KIND79_TAKEN up)
; when Aladdin's test box is in its zone $80 and he can take hold (not in
; state $8C, V_141C set, health left, its wait over); it counts the shots
; that reach it; it goes to state $8C when he lands on it, once it has
; come up past y $0032 (its $07 bit 1), and every frame after that; then
; its animation steps, and on into Kind79InRange.
; HOW: KIND79_WAIT nonzero: down. actor_41 zero: KIND79_LOW set, state
; |= $8C; Y = $80, ActorZoneHasTestPointLong, C clear and
; PLAYER_STATE_8C_COUNT 0, V_141C, HEALTH nonzero, KIND79_WAIT 0:
; KIND79_TAKEN up; ActorDownedByShot C clear: KIND79_43 up;
; PlayerLandsOnActor Z clear: state |= $8C; $07 bit 1 with its y
; (16-bit) below $0032: state |= $8C, KIND79_LOW up. Then AnimStep and on
; into Kind79InRange (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM, the zones' and hit boxes' bank; called by JSR
; through the actor loop's wait table ($83:8F4F, kind $79). Exit: as
; Kind79InRange.
%at($83F68C)
Kind79Wait:
    lda.b !KIND79_WAIT
    beq .waited
    dec.b !KIND79_WAIT
.waited:
    lda.b !actor_41
    bne .step
    lda.b !KIND79_LOW
    beq .zone
    lda.b #$8C
    tsb.b !actor_state
.zone:
    ldy.b #!KIND79_ZONE
    jsl.l ActorZoneHasTestPointLong
    bcs .shot
    lda.w !PLAYER_STATE_8C_COUNT
    bne .shot
    lda.w !V_141C
    beq .shot
    lda.w !HEALTH
    beq .shot
    lda.b !KIND79_WAIT
    bne .shot
    inc.b !KIND79_TAKEN
.shot:
    jsr.w ActorDownedByShot
    bcs .landed
    inc.b !KIND79_43
.landed:
    jsr.w PlayerLandsOnActor
    beq .low
    lda.b #$8C
    tsb.b !actor_state
.low:
    lda.b !actor_07
    and.b #$02
    beq .step
    rep.b #$20
    lda.b !actor_y+1
    cmp.w #$0032
    sep.b #$20
    bcs .step
    lda.b #$8C
    tsb.b !actor_state
    inc.b !KIND79_LOW
.step:
    jsl.l AnimStep
    jmp.w Kind79InRange

; @routine 83:F6E9 Kind79EndIfCarry
; WHAT IT DOES: Ends actor kind $79 for good when C is set: its slot
; cleared, its caller's return dropped, then ActorEndPlaced. C clear, it
; returns. Nothing in the game calls it (no JSR, JSL, JMP, pointer or
; word in the ROM names it): unit_bank83_keep_paths runs it with C clear.
; HOW: BCC to an RTS. Else Kind79SlotClear, then ActorEndDropReturn
; (JMP).
; Entry: M 8-bit, X 8-bit; C set to end it; D = the actor's record; DB a bank
; whose $0000-$1FFF is work RAM; called by JSR from the actor's code at
; its own level. Exit: C clear: everything unchanged; else never
; returns.
%at($83F6E9)
Kind79EndIfCarry:
    bcc .keep
    jsr.w Kind79SlotClear
    jmp.w ActorEndDropReturn
.keep:
    rts

; @routine 83:F6F2 Kind79Second
; @actor
; WHAT IT DOES: Actor kind $79 in a state with bit 7 set (landed on, or
; hit): no longer taken up; landed on (not shot, not up past $0032) it
; sinks a little with a sound; shot while it carried Aladdin with health
; left, his code goes on at $CDD2; then it is hit (ACTOR_09 bit 7), poses,
; sounds $2D, and after 6 frames falls away for good.
; HOW: $41 up. KIND79_LOW and KIND79_43 both zero: SoundQueueAdd $0F,
; animation 1, ActorSpeedYSetGravity (A 4, Y $6C), 8 frames (BRK) of
; ActorFallAgainst. Else KIND79_TAKEN and HEALTH nonzero:
; PLAYER_RESUME $CDD2, KIND79_TAKEN 0; ACTOR_09 bit 7 set. Then animation
; 3, SoundQueueAdd $2D, COP 6, ActorMotionReset $30, ActorFall each frame
; (BRK) for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $79 in a state with bit 7 set (the table
; at $83:8DFD). Exit: never returns.
%at($83F6F2)
Kind79Second:
    inc.b !actor_41
    lda.b !KIND79_LOW
    bne .shot
    lda.b !KIND79_43
    beq .landed_on
.shot:
    lda.b !KIND79_TAKEN
    beq .hit
    lda.w !HEALTH
    beq .hit
    lda.b #$D2
    sta.w !PLAYER_RESUME
    lda.b #$CD
    sta.w !PLAYER_RESUME+1
    stz.b !KIND79_TAKEN
.hit:
    lda.b #$80
    tsb.b !actor_09
    bra .pose
.landed_on:
    lda.b #$0F
    jsl.l SoundQueueAdd
    lda.b #$01
    jsl.l AnimStart
    lda.b #!KIND79_DIP_GRAVITY
    ldy.b #!KIND79_DIP
    jsl.l ActorSpeedYSetGravity
    lda.b #$08
    sta.b !actor_40
.sink:
    brk.b #$00
    jsl.l ActorFallAgainst
    dec.b !actor_40
    bne .sink
.pose:
    lda.b #$03
    jsl.l AnimStart
    lda.b #$2D
    jsl.l SoundQueueAdd
    lda.b #$06
    cop.b #$00
    lda.b #$30
    jsl.l ActorMotionReset
.fall:
    brk.b #$00
    jsl.l ActorFall
    bra .fall
%ends_at($83F757)
