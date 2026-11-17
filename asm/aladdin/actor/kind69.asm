; kind69.asm: actor kind $69 (also kind $68's second state): a figure
; that, on a cue, opens a circle wipe round itself and hands its place to
; the level's escort, then follows the escort's facing and motion with its
; poses; and kind $54's per-frame work.
; C: src/aladdin/actor/kind69.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!CUE_BE = $00BE                 ; the scene's cue: nonzero, the wipe starts
!WIPE_STEPS = $40               ; steps of the wipe left
!K69_HANDED = $42               ; set once the escort has its place
!WIPE_RADIUS = $45              ; the wipe's radius (word; ActorCircleWipeStep's)
!K69_16 = $16                   ; word: $D657
!SUB_LAYERS = $039A             ; layers on the sub screen (as kind76.asm names it)
!WINDOW_03A6 = $03A6            ; the windows' settings, $03A6-$03AE
!ESCORT_AT_PLACE = $E554        ; the escort's code once it is given a place

; @routine 83:E9F7 Kind69Start
; @actor
; WHAT IT DOES: Actor kind $69 (and $68 in its second state): faces
; left and waits for cue $BE; then a circle wipe opens round it over 32
; steps of 3 frames (window effect 2 started, colour math adding to the
; backdrop), and the level's escort takes its place ($07 3, its code at
; $E554, at its x and y); from then on each frame it faces as the escort
; does and shows pose 1 while the escort moves, pose 0 while it stands.
; HOW: ACTOR_09 bits $30 set; K69_16 $D657, WIPE_RADIUS 0; facing and
; flip 1; K69_HANDED 0. Each frame (BRK) until CUE_BE. WIPE_STEPS $20;
; SUB_LAYERS 3, CGWSEL 2, CGADSUB $20, TMW 3, $03AA $22, $03AB and WOBJSEL
; 0; ActorCircleWipeStep; TaskCreate (slot 6, parameter 4,
; WindowEffectTask); COP 2; ACTOR_09 bit 7 set, AnimStart 0; COP 3 and
; ActorCircleWipeStep until WIPE_STEPS counts down. K69_HANDED up,
; ACTOR_09 bits 4-5 = 1; HDMAEN, SUB_LAYERS, TMW and $03AA 0; the escort's
; $07 3, code $E554, x and y its own. Each frame: facing and flip the
; escort's flip; the pose (1 if the escort's speed x is nonzero) doubled
; against ACTOR_ANIM: changed, AnimStart.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $69
; in state $0C (the table at $83:8CAB) and for kind $68 with state bit 7
; set (the table at $83:8DFD). Exit: never returns.
%at($83E9F7)
Kind69Start:
    lda.b #$30
    tsb.b !actor_09
    rep.b #$20
    lda.w #$D657
    sta.b !K69_16
    stz.b !WIPE_RADIUS
    sep.b #$20
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    stz.b !K69_HANDED
.until_cue:
    brk.b #$00
    lda.w !CUE_BE
    beq .until_cue
    lda.b #$20
    sta.b !WIPE_STEPS
    lda.b #$03
    sta.w !SUB_LAYERS
    lda.b #$02
    sta.w !cgwsel_shadow
    lda.b #$20
    sta.w !cgadsub_shadow
    lda.b #$03
    sta.w !tmw_shadow
    lda.b #$22
    sta.w !WINDOW_03A6+4
    stz.w !WINDOW_03A6+5
    stz.w !wobjsel_shadow
    jsr.w ActorCircleWipeStep
    jsl.l TaskCreate
    db 6*24, 4                  ; slot 6, parameter 4
    dl WindowEffectTask
    lda.b #$02
    cop.b #$00
    lda.b #$80
    tsb.b !actor_09
    lda.b #$00
    jsl.l AnimStart
.wipe:
    lda.b #$03
    cop.b #$00
    jsr.w ActorCircleWipeStep
    dec.b !WIPE_STEPS
    bne .wipe
    inc.b !K69_HANDED
    lda.b !actor_09
    and.b #$CF
    ora.b #$10
    sta.b !actor_09
    stz.w !hdmaen_shadow
    stz.w !SUB_LAYERS
    stz.w !tmw_shadow
    stz.w !WINDOW_03A6+4
    lda.b #$03
    sta.w !actor_escort+!actor_07
    rep.b #$20
    lda.w #!ESCORT_AT_PLACE
    sta.w !actor_escort+!actor_03
    lda.b !actor_x+1
    sta.w !actor_escort+!actor_x+1
    lda.b !actor_y+1
    sta.w !actor_escort+!actor_y+1
    sep.b #$20
.follow:
    brk.b #$00
    lda.w !actor_escort+!actor_flip
    sta.b !actor_facing
    sta.b !actor_flip
    lda.w !actor_escort+!actor_speed_x
    ora.w !actor_escort+!actor_speed_x+1
    bne .moving
    lda.b #$00
    bra .pose
.moving:
    lda.b #$01
.pose:
    asl a
    cmp.b !ACTOR_ANIM
    beq .follow
    lsr a
    jsl.l AnimStart
    bra .follow

; @routine 83:D7E7 Kind54Wait
; WHAT IT DOES: What actors of kind $54 do each frame they wait: animate,
; hurt Aladdin on touch unless its $07 bit 3 is set, and end once left
; behind.
; HOW: AnimStep; $07 AND 8 zero: ActorHurtsPlayerOnTouch; then on into
; ActorEndIfLeftBehind (JMP).
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $80; called by JSR from
; the actor loop (83:94D0) by its kind (the wait table at $83:8F4F).
; Exit: as ActorEndIfLeftBehind.
%at($83D7E7)
Kind54Wait:
    jsl.l AnimStep
    lda.b !actor_07
    and.b #$08
    bne .end
    jsr.w ActorHurtsPlayerOnTouch
.end:
    jmp.w ActorEndIfLeftBehind
