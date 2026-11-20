; kind3a.asm: actor kind $3A's start, a hold Aladdin hangs from: once he
; comes into its zone it takes him (his code sent to $D6F8) and keeps him
; at the place its picture sets, with his frame matching; when he is in
; the air or hurt again it lets go and, 13 frames on, can take him again.
; Its wait is Kind3AWait (kind_waits_rest.asm).
; C: src/aladdin/actor/kind3a.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K3A_SHOWN = $40                ; the picture it last gave Aladdin a frame for ($FF: none)
!K3A_42 = $42                   ; set to 1 at its start; purpose not yet established
!K3A_LET_GO = $43               ; frames left of its pause after letting him go
!K3A_ZONES = $0A                ; its zones: ActorZoneHasTestPoint's zone is ($27 + this) * 4
!K3A_HELD_CODE = $D6F8          ; where it sends Aladdin's code while it holds him
!K3A_PAUSE = $0D                ; frames of its pause after letting him go
!K3A_STATE_8C_COUNT = $141B     ; counted up as Aladdin enters state $8C
!K3A_V_145A = $145A             ; negative: no hold
!K3A_V_0380 = $000380           ; bit 2 set: no hold
!K3A_HOLD_X = $D1FE             ; bank $80: by picture, the hold's offset from its x (words)
!K3A_HOLD_Y = $D210             ; and from its y
!K3A_FRAMES = $D1DE             ; bank $80: by its $2F, Aladdin's frame while held (facing right)
!K3A_FRAMES_LEFT = $D1EE        ; and facing left
!K3A_HOLD_SPOT = $145D          ; the place kept for Aladdin while held (words: x, y)
!K3A_BOX_RIGHT = $14            ; V_1A08 while he is held facing right
!K3A_BOX_LEFT = $1E             ; and facing left

; @routine 83:BEB3 Kind3AStart
; @actor
; WHAT IT DOES: Actor kind $3A: a hold Aladdin hangs from. When he comes
; into its zone (not hurt, $145A not negative, $0380 bit 2 clear) it takes
; him: his code goes to $D6F8 and it keeps him at the place its picture
; sets, his frame by its $2F as he faces. When he is in the air ($141C) or
; hurt ($141B) it lets go, waits 13 frames and can take him again.
; HOW: Kind33Look; K3A_42 1, K3A_SHOWN $FF. Each frame (BRK first)
; AnimStep; the tests above, then ActorZoneHasTestPointLong with zone
; ($27 + 10) * 4: C clear, taken. Taken: his resume address K3A_HELD_CODE;
; Kind3AHoldSpot; each frame (BRK first) AnimStep, Kind3AHoldBox; $141C
; or $141B nonzero: K3A_LET_GO 13 frames (BRK first) of AnimStep, V_1A08
; 0, and the zone test again. Else his x and y = its own plus the hold's
; offsets by its picture, his frame time $20; its $2F other than
; K3A_SHOWN: kept, his frame from K3A_FRAMES (K3A_FRAMES_LEFT when his
; flip is set), his picture marked changed, his frame time 1.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $3A in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83BEB3)
Kind3AStart:
    jsr.w Kind33Look
    lda.b #$01
    sta.b !K3A_42
    lda.b #$FF
    sta.b !K3A_SHOWN
.free:
    brk.b #$00
    jsl.l AnimStep
    lda.w !K3A_STATE_8C_COUNT
    bne .free
    lda.w !K3A_V_145A
    bmi .free
    lda.l !K3A_V_0380
    bit.b #$04
    bne .free
    lda.b !ACTOR_PICTURE
    clc
    adc.b #!K3A_ZONES
    asl a
    asl a
    tay
    jsl.l ActorZoneHasTestPointLong
    bcs .free
    lda.b #!K3A_HELD_CODE&255
    sta.w !player+!actor_03
    lda.b #!K3A_HELD_CODE>>8
    sta.w !player+!actor_03+1
    jsr.w Kind3AHoldSpot
.held:
    brk.b #$00
    jsl.l AnimStep
    jsr.w Kind3AHoldBox
    lda.w !v_141c
    bne .letGo
    lda.w !K3A_STATE_8C_COUNT
    beq .keep
.letGo:
    lda.b #!K3A_PAUSE
    sta.b !K3A_LET_GO
.pause:
    brk.b #$00
    jsl.l AnimStep
    dec.b !K3A_LET_GO
    bne .pause
    stz.w !v_1a08
    bra .free
.keep:
    lda.b !ACTOR_PICTURE
    asl a
    tax
    rep.b #$21
    lda.w !K3A_HOLD_X,x
    adc.b !actor_x+1
    sta.w !player+!actor_x+1
    lda.w !K3A_HOLD_Y,x
    clc
    adc.b !actor_y+1
    sta.w !player+!actor_y+1
    sep.b #$20
    lda.b #$20
    sta.w !player+!ACTOR_FRAME_TIME
    ldx.b !ACTOR_FRAME
    cpx.b !K3A_SHOWN
    beq .held
    stx.b !K3A_SHOWN
    lda.w !K3A_FRAMES,x
    ldy.w !player+!actor_flip
    beq .right
    lda.w !K3A_FRAMES_LEFT,x
.right:
    sta.w !player+!ACTOR_FRAME
    lda.b #$FF
    sta.w !player+!ACTOR_REDRAW
    lda.b #$01
    sta.w !player+!ACTOR_FRAME_TIME
    bra .held

; @routine 83:BF57 Kind3AHoldBox
; WHAT IT DOES: Sets V_1A08 for Aladdin held: $14 facing right, $1E
; facing left (his flip set).
; HOW: Y = K3A_BOX_RIGHT, or K3A_BOX_LEFT with his flip nonzero; STY.
; Entry: M 8-bit, X 8-bit; DB = $80; called by JSR. Exit: Y = the value;
; A low = his flip; N and Z from it.
%at($83BF57)
Kind3AHoldBox:
    ldy.b #!K3A_BOX_RIGHT
    lda.w !player+!actor_flip
    beq .right
    ldy.b #!K3A_BOX_LEFT
.right:
    sty.w !v_1a08
    rts

; @routine 83:BF64 Kind3AHoldSpot
; WHAT IT DOES: Keeps the place Aladdin is held at: its x and y plus the
; hold's offsets by its picture, in K3A_HOLD_SPOT.
; HOW: X = its picture times 2; 16-bit, carry clear: K3A_HOLD_X,x plus
; its x pixels, K3A_HOLD_Y,x plus its y pixels.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by
; JSR. Exit: M 8-bit, X 8-bit; A = the y (16 bits); X = the picture
; times 2; C from the addition.
%at($83BF64)
Kind3AHoldSpot:
    lda.b !ACTOR_PICTURE
    asl a
    tax
    rep.b #$21
    lda.w !K3A_HOLD_X,x
    adc.b !actor_x+1
    sta.w !K3A_HOLD_SPOT
    lda.w !K3A_HOLD_Y,x
    clc
    adc.b !actor_y+1
    sta.w !K3A_HOLD_SPOT+2
    sep.b #$20
    rts
%ends_at($83BF7E)
