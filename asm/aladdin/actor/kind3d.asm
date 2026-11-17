; kind3d.asm: actor kind $3D, a thing the player can stand on and ride:
; its contact with him each frame it waits, a child it starts and its bob. Written from the bytes where
; no script runs them; named and described by hand.
; C: src/aladdin/actor/kind3d.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/wram_player.inc"

arch 65816

!K3D_X_WAS = $41                 ; K3D_X_COUNT as it was the frame before (word)
!K3D_Y_WAS = $43                 ; K3D_Y_COUNT as it was (word)
!K3D_X_COUNT = $45               ; nonzero: the player within its box in x (word)
!K3D_Y_COUNT = $47               ; in y (word)
!K3D_RIDDEN = $49                ; nonzero: the player rides it
!K3D_4B = $4B                    ; nonzero: touching it from below does not hurt
!K3D_BELOW = $003D               ; his y below it, pushed under
!K3D_ON = $003B                  ; his y above it, standing on it
!K3D_LEFT = $0010                ; his x left of it, pushed off
!K3D_RIGHT = $0011               ; right of it
!K3D_RECORDS_NEEDED = $D296      ; in bank $80: by $07, the free list's top it needs at least
!K3D_CHILD_SETS = $C324          ; in bank $83: by $07, the code that starts its children (words)

; @routine 83:C3E4 Kind3DPlayerContact
; WHAT IT DOES: Actor kind $3D's contact with the player: once he comes
; within its box from above he stands on it and rides it (it carries him
; with its moves); from below he is pushed under it, and hurt when its $07
; is nonzero and its $4B zero; touching it from the side he is pushed
; 16 pixels off to its left or 17 to its right.
; HOW: K3D_X_COUNT and K3D_Y_COUNT (counted by ActorPlayerBoxCount) moved
; to K3D_X_WAS and K3D_Y_WAS and cleared. Not ridden (K3D_RIDDEN 0):
; ActorPlayerBoxCount; within on both axes: unless within in y the frame
; before, its y pixels below his: his y = its y + $3D, PlayerEnter8CFacing
; as above, then (V_141C nonzero) K3D_Y_COUNT cleared, or on to the side
; push; not below: his y = its y - $3B, V_1428 and K3D_RIDDEN 1, V_141C 0.
; The side push: his x = its x - 16 when its x is not below his, else its
; x + 17; K3D_Y_COUNT up. Ridden: PlayerMoveWithActor,
; PlayerEnter8CFacing, ActorPlayerBoxCount; V_141C zero and within on both
; axes: V_1428 up; else K3D_RIDDEN 0. No script runs the push, the landing
; or the ride: written from their bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR. Exit: M 8-bit, X 8-bit; as the
; last step leaves A and the flags.
%at($83C3E4)
Kind3DPlayerContact:
    rep.b #$20
    lda.b !K3D_X_COUNT
    sta.b !K3D_X_WAS
    lda.b !K3D_Y_COUNT
    sta.b !K3D_Y_WAS
    stz.b !K3D_X_COUNT
    stz.b !K3D_Y_COUNT
    ldx.b !K3D_RIDDEN
    bne .ridden
    jsr.w ActorPlayerBoxCount
    lda.b !K3D_X_COUNT
    beq .out
    lda.b !K3D_Y_COUNT
    beq .out
    ldx.b !K3D_Y_WAS
    bne .side
    rep.b #$20
    lda.b !actor_y+1
    cmp.w !player+!actor_y+1
    bcs .on
    adc.w #!K3D_BELOW
    sta.w !player+!actor_y+1
    sep.b #$20
    lda.b !actor_07
    beq .hurtDone
    lda.b !K3D_4B
    bne .hurtDone
    jsr.w PlayerEnter8CFacing
.hurtDone:
    rep.b #$20
    ldx.w !v_141c
    beq .side
    stz.b !K3D_Y_COUNT
    bra .out
.on:
    sec
    sbc.w #!K3D_ON
    sta.w !player+!actor_y+1
    ldx.b #$01
    stx.w !v_1428
    stx.b !K3D_RIDDEN
    ldx.b #$00
    stx.w !v_141c
    bra .out
.side:
    lda.b !actor_x+1
    cmp.w !player+!actor_x+1
    bcc .right
    sbc.w #!K3D_LEFT
    bra .pushed
.right:
    adc.w #!K3D_RIGHT
.pushed:
    sta.w !player+!actor_x+1
    inc.b !K3D_Y_COUNT
.out:
    sep.b #$20
    rts
.ridden:
    sep.b #$20
    jsl.l PlayerMoveWithActor
    jsr.w PlayerEnter8CFacing
    rep.b #$20
    jsr.w ActorPlayerBoxCount
    ldy.w !v_141c
    bne .off
    lda.b !K3D_X_COUNT
    beq .off
    lda.b !K3D_Y_COUNT
    beq .off
    sep.b #$20
    inc.w !v_1428
    rts
.off:
    sep.b #$20
    stz.b !K3D_RIDDEN
    rts

; @routine 83:C4A4 Kind3DWait
; WHAT IT DOES: What actor kind $3D runs each frame it waits: its contact
; with the player, then a shot that hits it turned back.
; HOW: Kind3DPlayerContact, ActorTurnsShotBack.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSR through the actor loop's wait
; table by kind ($83:8F4F). Exit: as ActorTurnsShotBack.
%at($83C4A4)
Kind3DWait:
    jsr.w Kind3DPlayerContact
    jsr.w ActorTurnsShotBack
    rts

; @routine 83:C332 ActorStartChild1E5
; WHAT IT DOES: Starts the actor's one child (set 1 of its table at
; 83:C324): at spawn point $1E, its $07 = 5.
; HOW: ChildStartListed with X = $1E, Y = 5. No script runs it: written
; from its bytes.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSR through
; the actor's table at 83:C324 (83:C317). Exit: as ChildStartListed.
%at($83C332)
ActorStartChild1E5:
    ldx.b #$1E
    ldy.b #$05
    jsr.w ChildStartListed
    rts

; @routine 83:C363 Kind3DBob
; @actor
; WHAT IT DOES: Actor kind $3D with $07 4 and up: a thing the player can
; ride (Kind3DWait) that bobs: 40 frames still, 14 frames up (its $4B set
; at the top, with sound $4E), 27 frames still, 56 frames back down,
; again, keeping its place each frame for the ride.
; HOW: $07 = its bit 0; ACTOR_08 = 2; K3D_X_WAS, $42, K3D_Y_WAS, $44 and
; K3D_RIDDEN 0; AnimStart with $07; ACTOR_09 = $90. Again and again: K3D_4B
; 0; BRK; ActorKeepSpot; COP $28; 14 frames (BRK first) of ActorKeepSpot
; and ActorBobStep; K3D_4B up; SoundQueueAdd $4E; BRK; ActorKeepSpot;
; COP $1B; K3D_4B 0; 56 frames (BRK first) of ActorKeepSpot and
; ActorBobReturnStep.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; a branch from kind $3D's start ($83:C306). Exit: never returns.
%at($83C363)
Kind3DBob:
    lda.b !actor_07
    and.b #$01
    sta.b !actor_07
    lda.b #$02
    sta.b !actor_08
    stz.b !K3D_X_WAS
    stz.b !K3D_X_WAS+1
    stz.b !K3D_Y_WAS
    stz.b !K3D_Y_WAS+1
    stz.b !K3D_RIDDEN
    lda.b !actor_07
    jsl.l AnimStart
    lda.b #$90
    sta.b !actor_09
.again:
    stz.b !K3D_4B
    brk.b #$00
    jsl.l ActorKeepSpot
    lda.b #$28
    cop.b #$00
    lda.b #$0E
    sta.b !actor_40
.up:
    brk.b #$00
    jsl.l ActorKeepSpot
    jsr.w ActorBobStep
    dec.b !actor_40
    bne .up
    inc.b !K3D_4B
    lda.b #$4E
    jsl.l SoundQueueAdd
    brk.b #$00
    jsl.l ActorKeepSpot
    lda.b #$1B
    cop.b #$00
    stz.b !K3D_4B
    lda.b #$38
    sta.b !actor_40
.down:
    brk.b #$00
    jsl.l ActorKeepSpot
    jsr.w ActorBobReturnStep
    dec.b !actor_40
    bne .down
    bra .again

; @routine 83:C302 Kind3DStart
; @actor
; WHAT IT DOES: Actor kind $3D: with $07 4 and up the bobbing thing the
; player rides (Kind3DBob); else, when the free list holds records enough
; for the set, it starts its children by set $07 (0: ActorStartChild1E,
; 1: ActorStartChild1E5, 2: ActorStartTwoChildren) and stays, ended with
; them once it is far from the range point; with too few records it ends.
; HOW: $07 at least 4: on at Kind3DBob. Else $44 0; ACTOR_FREE_TOP
; negative or below K3D_RECORDS_NEEDED by $07: JMP ActorEndPlaced (no
; script runs this). JSR through K3D_CHILD_SETS by $07 times 2; then each
; frame (BRK first) ActorEndFarWithChildren.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80 (the table
; K3D_RECORDS_NEEDED's bank); reached by the actor loop's jump for kind
; $3D in state $0C (the table at $83:8CAB). Exit: never returns.
%at($83C302)
Kind3DStart:
    lda.b !actor_07
    cmp.b #$04
    bcs Kind3DBob
    stz.b !actor_44
    tay
    asl a
    tax
    lda.w !actor_free_top
    bmi .end
    cmp.w !K3D_RECORDS_NEEDED,y
    bcc .end
    jsr.w (!K3D_CHILD_SETS,x)
.frame:
    brk.b #$00
    jsr.w ActorEndFarWithChildren
    bra .frame
.end:
    jmp.w ActorEndPlaced
