; kind61.asm: actor kinds $61 and $64, pads that bounce Aladdin: when his
; test box meets one he is sent on in his bounce code ($D77C) from its top,
; kind $61 also setting his x to its own; while he is hurt it pushes him
; aside instead.
; C: src/aladdin/actor/kind61.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!PAD_KEEP_X = $41               ; nonzero (kind $64): Aladdin's x is left as it is
!PAD_WAIT = $40                 ; frames before it bounces him again
!PAD_145A = $145A               ; negative: no bouncing
!PAD_LEVEL = $035E              ; the level (in level $0A it bounces only while hittable and he is not taken)
!PAD_TAKEN = $1427              ; nonzero while Aladdin is taken
!PAD_141C = $141C               ; set when it meets him
!PAD_HURT = $141B               ; nonzero while he is hurt (state $8C)
!PAD_BOUNCE_CODE = $D77C        ; his code once bounced
!PAD_SOUND = $12                ; the bounce's sound
!PAD_144E = $144E               ; given the byte at $0001
!PAD_V_0001 = $0001

; @routine 83:E4EC Kind61Start
; @actor
; WHAT IT DOES: Actor kind $61: the pad that bounces Aladdin and puts him
; at its x (PAD_KEEP_X 0), then on as Kind64Start.
; HOW: PAD_KEEP_X 0; on into Kind64Start past its own setting
; ($83:E4F4).
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $61 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83E4EC)
Kind61Start:
    stz.b !PAD_KEEP_X
    bra Kind64Pad

; @routine 83:E4F0 Kind64Start
; @actor
; WHAT IT DOES: Actor kind $64 (and $61 from $83:E4F4): a pad. Each frame,
; unless bouncing is off ($145A negative), or in level $0A while Aladdin
; is taken or it cannot be hit, or it is still waiting: when his test box
; meets it, $141C set; hurt, he is pushed aside (PlayerPushAside); else
; $14 frames of wait, sound $12, his code $D77C, and next frame his y its
; y less 8 (his x its x for kind $61), $144E the byte at $0001.
; HOW: PAD_KEEP_X 1. ACTOR_09 bit 7 set, PAD_WAIT 0. Each frame (BRK):
; PAD_145A negative, again; level $0A: PAD_TAKEN nonzero or ACTOR_09 bit
; 6 clear, again; PAD_WAIT nonzero: down, again; ActorHitsTestBoxIfActive
; C set, again. PAD_141C 1; PAD_HURT nonzero: PlayerPushAside, again.
; PAD_WAIT $14, SoundQueueAdd $12; 16-bit with C clear: his code
; $D77C; BRK; his y = its y - 7 - 1; PAD_KEEP_X 0: his x = its x; 8-bit:
; PAD_144E = the byte at $0001; again.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $80; reached by
; the actor loop's jump for kind $64 in state $0C (the table at $83:8CAB).
; Exit: never returns.
%at($83E4F0)
Kind64Start:
    lda.b #$01
    sta.b !PAD_KEEP_X
Kind64Pad:
    lda.b #$80
    tsb.b !actor_09
    stz.b !PAD_WAIT
.frame:
    brk.b #$00
    lda.w !PAD_145A
    bmi .frame
    lda.w !PAD_LEVEL
    cmp.b #$0A
    bne .free
    lda.w !PAD_TAKEN
    bne .frame
    lda.b !actor_09
    and.b #$40
    beq .frame
.free:
    lda.b !PAD_WAIT
    beq .test
    dec.b !PAD_WAIT
    bra .frame
.test:
    jsr.w ActorHitsTestBoxIfActive
    bcs .frame
    lda.b #$01
    sta.w !PAD_141C
    lda.w !PAD_HURT
    bne .aside
    lda.b #$14
    sta.b !PAD_WAIT
    lda.b #!PAD_SOUND
    jsl.l SoundQueueAdd
    rep.b #$21
    lda.w #!PAD_BOUNCE_CODE
    sta.w !player+!actor_03
    brk.b #$00
    lda.b !actor_y+1
    sbc.w #$0007
    sta.w !player+!actor_y+1
    ldx.b !PAD_KEEP_X
    bne .kept
    lda.b !actor_x+1
    sta.w !player+!actor_x+1
.kept:
    sep.b #$20
    lda.w !PAD_V_0001
    sta.w !PAD_144E
    bra .frame
.aside:
    jsr.w PlayerPushAside
    bra .frame
%ends_at($83E55E)
