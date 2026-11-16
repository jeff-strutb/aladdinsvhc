; story_actor_9b.asm: actor kind $9B, a figure of the story's scenes: by
; its $07 one of four parts, each moving and posing on the story's cues and
; starting the story's scene task with the next scene's number.
; C: src/aladdin/actor/story_actor_9b.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!STORY_76 = $0076               ; set to $40 (and $0077 to 0) as part 2 flies off
!STORY_77 = $0077
!STORY_78 = $0078               ; counted up by part 2 once in place
!STORY_79 = $0079               ; counted up by part 3 at its end; part 1 waits for it
!STORY_7A = $007A               ; counted up by part 1, cleared at its end; negative: parts 0 and 2 go on
!STORY_7B = $007B               ; counted up by part 2; part 3 waits for it
!CUE_CF = $00CF                 ; nonzero while the story's text is busy
!STORY_SCENE_TASK = $81F344     ; the task the story's scenes start in slot 4

; @routine 84:A509 Kind9BStory
; @actor
; WHAT IT DOES: Actor kind $9B, a figure of the story's scenes: part 0
; waits for STORY_7A to go negative, poses (8) and ends; part 1 turns,
; walks, turns back and starts scenes 7, 16 and 17 between the story's
; cues, counting STORY_7A up and clearing it at the end; part 2 flies in
; and out on set motions, starts scenes 8, 12 and 13, counts STORY_78
; and STORY_7B up, and flies on for good; part 3 waits for STORY_7B,
; walks across, starts scene 15 and counts STORY_79 up.
; HOW: actor_09 bit 4 set, actor_2c bits 1-3 cleared, actor_42 and 44
; 0; then by its $07 through the table .roles. Each part sets its
; actor_2c bits, actor_08 bit 1 and actor_09 bit 7, and runs its steps:
; AnimStart poses, COP waits, BRK waits on STORY_7A, 79, 7B and CUE_CF,
; motion presets and speeds, frame counts in actor_40, TaskCreate of
; STORY_SCENE_TASK in slot 4 with the scene's number. Part 0 ends in
; ActorEndPlaced; the others yield for good.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM; reached by the actor loop's jump for kind $9B
; in state $0C (the table at $83:8DAB, through the stub at $83:93E5).
; Exit: never returns.
%at($84A509)
Kind9BStory:
    lda.b #$10
    tsb.b !actor_09
    lda.b #$0E
    trb.b !actor_2c
    stz.b !actor_42
    stz.b !actor_44
    lda.b !actor_07
    asl a
    tax
    jmp.w (.roles,x)
.roles:
    dw .role0, .role1, .role2, .role3
.role0:
    lda.b #$0C
    tsb.b !actor_2c
.lA528:
    brk.b #$00
    lda.w !STORY_7A
    bpl .lA528
    lda.b #$80
    tsb.b !actor_09
    lda.b #$08
    jsl.l AnimStart
    lda.b #$30
    cop.b #$00
    jml.l ActorEndPlaced
.role1:
    lda.b #$08
    tsb.b !actor_2c
    lda.b #$02
    tsb.b !actor_08
    lda.b #$80
    tsb.b !actor_09
    lda.b #$01
    jsl.l AnimStart
.lA553:
    brk.b #$00
    lda.w !CUE_CF
    bne .lA553
    lda.b #$02
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    inc.b !actor_facing
    inc.b !actor_flip
    lda.b #$20
    sta.b !actor_40
    ldy.b #$66
    jsl.l ActorSpeedXSet
.lA578:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !actor_40
    bne .lA578
    lda.b #$20
    cop.b #$00
    lda.b #$02
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    stz.b !actor_facing
    stz.b !actor_flip
    jsl.l TaskCreate
    db 4*24, 7                 ; slot 4, parameter 7
    dl !STORY_SCENE_TASK
.lA5A3:
    brk.b #$00
    lda.w !CUE_CF
    bne .lA5A3
    lda.b #$30
    cop.b #$00
    inc.w !STORY_7A
.lA5B1:
    brk.b #$00
    lda.w !STORY_79
    beq .lA5B1
    lda.b #$02
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    inc.b !actor_flip
.lA5CA:
    brk.b #$00
    lda.w !CUE_CF
    bne .lA5CA
    jsl.l TaskCreate
    db 4*24, 16                 ; slot 4, parameter 16
    dl !STORY_SCENE_TASK
.lA5DA:
    brk.b #$00
    lda.w !CUE_CF
    bne .lA5DA
    lda.b #$20
    cop.b #$00
    lda.b #$02
    jsl.l AnimStart
    lda.b #$18
    cop.b #$00
    lda.b #$01
    jsl.l AnimStart
    stz.b !actor_flip
    jsl.l TaskCreate
    db 4*24, 17                 ; slot 4, parameter 17
    dl !STORY_SCENE_TASK
    stz.w !STORY_7A
.lA603:
    brk.b #$00
    bra .lA603
.role2:
    lda.b #$0C
    tsb.b !actor_2c
    lda.b #$02
    tsb.b !actor_08
    lda.b #$80
    tsb.b !actor_09
    inc.b !actor_facing
    inc.b !actor_flip
    lda.b #$04
    jsl.l AnimStart
.lA61D:
    brk.b #$00
    lda.w !STORY_7A
    beq .lA61D
    ldy.b #$6E
    jsl.l ActorMotionPreset
    lda.b #$40
    sta.b !actor_40
.lA62E:
    brk.b #$00
    jsr.w ActorFlyAgainstNoRise
    dec.b !actor_40
    bne .lA62E
    lda.b #$20
    cop.b #$00
    jsl.l TaskCreate
    db 4*24, 8                 ; slot 4, parameter 8
    dl !STORY_SCENE_TASK
    inc.w !STORY_78
.lA647:
    brk.b #$00
    lda.w !STORY_7A
    bpl .lA647
    lda.b #$06
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    lda.b #$07
    jsl.l AnimStart
    lda.b #$20
    cop.b #$00
    jsl.l TaskCreate
    db 4*24, 12                 ; slot 4, parameter 12
    dl !STORY_SCENE_TASK
.lA66B:
    brk.b #$00
    lda.w !CUE_CF
    bne .lA66B
    lda.b #$05
    jsl.l AnimStart
    lda.b #$10
    cop.b #$00
    ldy.b #$87
    jsl.l ActorSpeedYSet
    lda.b #$50
    sta.b !actor_40
.lA686:
    brk.b #$00
    jsl.l ActorMoveY
    dec.b !actor_40
    bne .lA686
    lda.b #$09
    jsl.l AnimStart
    rep.b #$20
    lda.w #$00E0
    sta.b !actor_x+1
    lda.w #$0030
    sta.b !actor_y+1
    sep.b #$20
    ldy.b #$6F
    jsl.l ActorMotionPreset
    lda.b #$60
    sta.b !actor_40
.lA6AE:
    brk.b #$00
    jsr.w ActorFlyAgainstNoRise
    dec.b !actor_40
    bne .lA6AE
    lda.b #$50
    cop.b #$00
    rep.b #$20
    lda.w #$0020
    sta.b !actor_x+1
    lda.w #$0030
    sta.b !actor_y+1
    sep.b #$20
    stz.b !actor_facing
    stz.b !actor_flip
    ldy.b #$6F
    jsl.l ActorMotionPreset
    lda.b #$60
    sta.b !actor_40
.lA6D7:
    brk.b #$00
    jsr.w ActorFlyAgainstNoRise
    dec.b !actor_40
    bne .lA6D7
    lda.b #$30
    cop.b #$00
    rep.b #$20
    lda.w #$00E8
    sta.b !actor_x+1
    lda.w #$0050
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$05
    jsl.l AnimStart
    lda.b #$40
    ldy.b #$69
    jsl.l ActorSpeedXSetAccel
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    lda.b #$20
    sta.b !actor_40
.lA70A:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !actor_40
    bne .lA70A
.lA714:
    brk.b #$00
    jsl.l ActorAccelerateXBack
    jsl.l ActorMoveX
    lda.b !actor_speed_x+2
    bpl .lA714
    lda.b #$20
    cop.b #$00
    jsl.l TaskCreate
    db 4*24, 13                 ; slot 4, parameter 13
    dl !STORY_SCENE_TASK
    inc.w !STORY_7B
.lA732:
    brk.b #$00
    lda.w !STORY_7A
    bne .lA732
    lda.w !CUE_CF
    bne .lA732
    lda.b #$40
    sta.w !STORY_76
    stz.w !STORY_77
    ldy.b #$71
    jsl.l ActorMotionPreset
.lA74C:
    brk.b #$00
    jsr.w ActorFlyAgainstNoRise
    bra .lA74C
.role3:
    lda.b #$0A
    tsb.b !actor_2c
    lda.b #$02
    tsb.b !actor_08
    lda.b #$80
    tsb.b !actor_09
    lda.b #$03
    jsl.l AnimStart
.lA765:
    brk.b #$00
    lda.w !STORY_7B
    beq .lA765
    lda.w !CUE_CF
    bne .lA765
    ldy.b #$66
    jsl.l ActorSpeedXSet
    lda.b #$FB
    sta.b !actor_44
    inc.b !actor_42
    lda.b #$60
    sta.b !actor_40
.lA781:
    brk.b #$00
    jsl.l ActorMoveX
    dec.b !actor_40
    bne .lA781
    stz.b !actor_42
    lda.b #$20
    cop.b #$00
    jsl.l TaskCreate
    db 4*24, 15                 ; slot 4, parameter 15
    dl !STORY_SCENE_TASK
    inc.w !STORY_79
.lA79D:
    brk.b #$00
    bra .lA79D
