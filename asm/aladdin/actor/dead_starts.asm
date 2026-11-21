; dead_starts.asm: actor code in banks $84 and $A5 that nothing in the ROM starts (no call, jump, table word or resume address holds
; its address): a hovering spawner and a start of the escort's code. cov_dead_starts starts each by a record's resume address.
; C: src/aladdin/actor/dead_starts.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!player_y = !player+!actor_y+1  ; the player's y, whole pixels (word)
!VIEW_X = $1773                 ; the view's left edge (pixels, word)
!VIEW_Y = $1777                 ; the view's top edge (pixels, word)
!V_1A12 = $1A12                 ; cleared once it hovers
!HOVER_FRAMES = !actor_40       ; frames left of a wait
!HOVER_RANDOM = !actor_42       ; the child's random pick (word)
!HOVER_CHILD = !actor_next      ; the child's record (word)
!HOVER_CHILD_41 = $D86A         ; by the pick: the child's $41
!HOVER_CHILD_FACING = $D85F     ; by the pick: the child's facing

; @routine 84:9A88 HoverSpawnerStart
; @actor
; WHAT IT DOES: An actor nothing starts (no table, call or resume address
; holds $84:9A88): it appears at the view's left edge plus $30 and top
; edge less $20, falls with its speed ($0180 across, $0300 down, gravity 8)
; losing its downward speed (ActorDecelerateY) until it is 0, then hovers,
; following Aladdin's y, and every $60 frames (twice $30) plays sound $28
; and animation 3 and makes a kind $8E child (HoverSpawnChild), then
; animation 2. cov_dead_starts starts it by a record's resume address.
; HOW: x = VIEW_X + $30, y = VIEW_Y - $20 (16-bit); speed $000180 across,
; $000300 down; gravity $08; facing and flip 0; HOVER_FRAMES $80. Each
; frame (BRK first): ActorDecelerateY, ActorMove, until the downward
; speed's two low bytes are 0. V_1A12 and HOVER_RANDOM (low byte) 0. Then
; for ever: $30 frames (BRK, HoverFollowPlayerY); SoundQueueAdd $28;
; AnimStart 3; HoverSpawnChild, $30 frames (BRK, HoverFollowPlayerY);
; AnimStart 2.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by the
; actor loop's RTI at the resume address. Exit: never returns.
%at($849A88)
HoverSpawnerStart:
    rep.b #$20
    lda.w !VIEW_X
    clc
    adc.w #$0030
    sta.b !actor_x+1
    lda.w !VIEW_Y
    sec
    sbc.w #$0020
    sta.b !actor_y+1
    lda.w #$0180
    sta.b !actor_speed_x
    lda.w #$0300
    sta.b !actor_speed_y
    sep.b #$20
    stz.b !actor_speed_x+2
    stz.b !actor_speed_y+2
    lda.b #$08
    sta.b !actor_accel_y
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$80
    sta.b !HOVER_FRAMES
.fall:
    brk.b #$00
    jsl.l ActorDecelerateY
    jsl.l ActorMove
    lda.b !actor_speed_y
    ora.b !actor_speed_y+1
    bne .fall
    stz.w !V_1A12
    stz.b !HOVER_RANDOM
.hover:
    lda.b #$30
    sta.b !HOVER_FRAMES
.first:
    brk.b #$00
    jsr.w HoverFollowPlayerY
    dec.b !HOVER_FRAMES
    bne .first
    lda.b #$28
    jsl.l SoundQueueAdd
    lda.b #$03
    jsl.l AnimStart
    lda.b #$30
    sta.b !HOVER_FRAMES
    jsr.w HoverSpawnChild
.second:
    brk.b #$00
    jsr.w HoverFollowPlayerY
    dec.b !HOVER_FRAMES
    bne .second
    lda.b #$02
    jsl.l AnimStart
    bra .hover

; @routine 84:9AFE HoverSpawnChild
; WHAT IT DOES: Makes a kind $8E child of HoverSpawnerStart's actor, if a
; record is free, with a random facing and $41 from tables. Called only by
; HoverSpawnerStart.
; HOW: ACTOR_SPAWN_AT $7E, $07 2; ActorCreateChild with A = $8E; carry set
; (none free): RTS. Else the child's record kept in HOVER_CHILD (16-bit),
; X 8-bit; RandomFromList with X = $16 gives HOVER_RANDOM (a word, high
; byte 0); X 16-bit, X = the child, Y = the pick: child $41 from
; HOVER_CHILD_41 and child facing from HOVER_CHILD_FACING by Y; X 8-bit.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR.
; Exit: M and X 8-bit; the child's fields set.
%at($849AFE)
HoverSpawnChild:
    lda.b #$7E
    sta.b !actor_spawn_at
    lda.b #$02
    sta.b !actor_07
    lda.b #$8E
    jsl.l ActorCreateChild
    bcs .done
    stx.b !HOVER_CHILD
    sep.b #$10
    ldx.b #$16
    jsl.l RandomFromList
    sta.b !HOVER_RANDOM
    stz.b !HOVER_RANDOM+1
    rep.b #$10
    ldx.b !HOVER_CHILD
    ldy.b !HOVER_RANDOM
    lda.w !HOVER_CHILD_41,y
    sta.w !actor_41,x
    lda.w !HOVER_CHILD_FACING,y
    sta.w !actor_facing,x
    sep.b #$10
.done:
    rts

; @routine 84:9B31 HoverFollowPlayerY
; WHAT IT DOES: Moves the hovering actor across (ActorMoveX) and puts its
; y $10 above Aladdin's. Called only by HoverSpawnerStart.
; HOW: ActorMoveX; 16-bit A: y pixels = Aladdin's y - $10.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; called by JSR.
; Exit: M 8-bit.
%at($849B31)
HoverFollowPlayerY:
    jsl.l ActorMoveX
    rep.b #$20
    lda.w !player_y
    sec
    sbc.w #$0010
    sta.b !actor_y+1
    sep.b #$20
    rts
