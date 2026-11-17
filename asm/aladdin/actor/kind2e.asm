; kind2e.asm: actor kind $2E, the figures of level 5's closing scene (the
; scene's task places them, $80:A2A8): by its $07, one that walks a few
; steps on a cue, starts the window waves and sinks away; one that stands;
; one that keeps beside Aladdin on a cue; one that turns up at its place on
; another.
; C: src/aladdin/actor/kind2e.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!K2E_16 = $16                   ; word: $D14E
!K2E_FACE = $40                 ; part 0: its facing, flipped each step; then frames left
!K2E_STEPS = $41                ; part 0: steps left
!CUE_146B = $146B               ; part 0 goes once this is set (cleared as it starts)
!CUE_BE = $00BE                 ; part 2 goes on this cue
!CUE_C0 = $00C0                 ; part 3 goes on this cue (set by the scene's task)
!PLAYER_2F = !player+$2F        ; Aladdin's byte $2F: the offset pair part 2 keeps by
!BESIDE_X = $D152               ; (data bank) by PLAYER_2F * 2: the x offset from Aladdin (word)
!BESIDE_Y = $D160               ; the y offset (word)

; @routine 83:AADC Kind2EStart
; @actor
; WHAT IT DOES: Actor kind $2E, a figure of level 5's closing scene, by
; its $07: 0 faces right and, on CUE_146B, takes six steps across turning
; each step, starts window effect 6 and sinks away for 144 frames; 1
; faces left and stays; 2 faces left and, on CUE_BE, keeps beside Aladdin
; by the offsets his byte $2F picks; 3 faces left in pose 1 and, on
; CUE_C0, turns up at ($0DA0, $0238) in pose 2 with sound 5.
; HOW: ACTOR_08 bit 1 and ACTOR_09 bits $90 set; K2E_16 $D14E; JMP through
; its four parts by $07. Part 0: CUE_146B cleared and awaited; speed x
; $0200, six frames of BRK, ActorMoveX and the facing flipped; TaskCreate
; (slot 6, parameter $0C, WindowEffectTask); COP $2A; speed y $0100, 144
; frames of BRK and ActorMoveY. Part 2: from ($0E80, $0148), each frame x
; = BESIDE_X + Aladdin's x, y = BESIDE_Y + his y, by PLAYER_2F * 2.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB a bank whose
; $0000-$1FFF is work RAM and that holds the offsets (bank $83); reached
; by the actor loop's jump for kind $2E in state $0C (the table at
; $83:8CAB). Exit: never returns.
%at($83AADC)
Kind2EStart:
    lda.b !actor_08
    ora.b #$02
    sta.b !actor_08
    lda.b !actor_09
    ora.b #$90
    sta.b !actor_09
    rep.b #$20
    lda.w #$D14E
    sta.b !K2E_16
    sep.b #$20
    lda.b !actor_07
    asl a
    tax
    jmp.w (.parts,x)
.parts:
    dw .part0, .part1, .part2, .part3
.part0:
    lda.b #$01
    sta.b !actor_facing
    sta.b !actor_flip
    stz.b !K2E_FACE
    lda.b #$00
    jsl.l AnimStart
    stz.w !CUE_146B
.until_146b:
    brk.b #$00
    lda.w !CUE_146B
    beq .until_146b
    stz.b !actor_speed_x
    lda.b #$02
    sta.b !actor_speed_x+1
    stz.b !actor_speed_x+2
    lda.b #$06
    sta.b !K2E_STEPS
.step:
    brk.b #$00
    jsl.l ActorMoveX
    lda.b !K2E_FACE
    eor.b #$01
    sta.b !actor_facing
    sta.b !K2E_FACE
    dec.b !K2E_STEPS
    bne .step
    jsl.l TaskCreate
    db 6*24, $0C                ; slot 6, parameter $0C
    dl WindowEffectTask
    lda.b #$2A
    cop.b #$00
    lda.b #$90
    sta.b !K2E_FACE
    stz.b !actor_speed_y
    lda.b #$01
    sta.b !actor_speed_y+1
    stz.b !actor_speed_y+2
.sink:
    brk.b #$00
    jsl.l ActorMoveY
    dec.b !K2E_FACE
    bne .sink
.stay0:
    brk.b #$00
    bra .stay0
.part1:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.stay1:
    brk.b #$00
    bra .stay1
.part2:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$00
    jsl.l AnimStart
.until_be:
    brk.b #$00
    lda.w !CUE_BE
    beq .until_be
    rep.b #$20
    lda.w #$0E80
    sta.b !actor_x+1
    lda.w #$0148
    sta.b !actor_y+1
    sep.b #$20
.beside:
    brk.b #$00
    lda.w !PLAYER_2F
    asl a
    tax
    rep.b #$20
    clc
    lda.w !BESIDE_X,x
    adc.w !player+!actor_x+1
    sta.b !actor_x+1
    clc
    lda.w !BESIDE_Y,x
    adc.w !player+!actor_y+1
    sta.b !actor_y+1
    sep.b #$20
    bra .beside
.part3:
    stz.b !actor_facing
    stz.b !actor_flip
    lda.b #$01
    jsl.l AnimStart
.until_c0:
    brk.b #$00
    lda.w !CUE_C0
    beq .until_c0
    rep.b #$20
    lda.w #$0DA0
    sta.b !actor_x+1
    lda.w #$0238
    sta.b !actor_y+1
    sep.b #$20
    lda.b #$02
    jsl.l AnimStart
    lda.b #$05
    jsl.l SoundQueueAdd
.stay3:
    brk.b #$00
    bra .stay3
