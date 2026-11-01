; anim.asm: actors' animations.
;
; Each actor kind has an animation set: a table of animations, each a
; list of frames (a time, then a picture), ended by a time of 0 with how
; many frames to step back. An actor's record holds the animation, the
; frame, the time it has left and the picture's graphics.
; C: src/aladdin/actor/anim.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

; @routine 81:90BE AnimResume
; WHAT IT DOES: Switches an actor to animation A at the frame it is on.
; HOW: The animation's number and offset go in the record and it is
; marked animated; the actor's kind gives its animation set (its
; graphics base and its table); the animation's first frame word gives
; the time and the frames to step back at its end (its time 0 entry);
; then AnimFrameLookup, and the picture is marked changed.
; Entry: M 8-bit, X 8-bit; A = the animation; D = the actor's record;
; called by JSL. Exit: M and X 8-bit; A = $FF; X, Y as AnimFrameLookup
; leaves them (8-bit); DB unchanged, N and Z from it; C from
; AnimFrameLookup's last add.
%at($8190BE)
AnimResume:
    ldx.b !actor_06
    bra AnimStart_set

; @routine 81:90C2 AnimStart
; WHAT IT DOES: Starts an actor's animation A from its first frame.
; HOW: AnimResume, with the frame set to 0 first.
; Entry: M 8-bit, X 8-bit; A = the animation; D = the actor's record;
; called by JSL. Exit: as AnimResume.
AnimStart:
    ldx.b !actor_06
    stz.b !ACTOR_FRAME
.set:
    sep.b #$30
    sta.b !ACTOR_PICTURE
    asl a
    sta.b !ACTOR_ANIM
    lda.b !actor_08
    ora.b #$08
    sta.b !actor_08
    phb
    lda.b #!anim_data>>16
    pha
    plb
    rep.b #$30
    txa
    and.w #$00FF
    tax
    lda.l !anim_set_of_kind,x
    and.w #$00FF
    asl a
    tax
    lda.l !anim_set_gfx,x
    sta.b !ACTOR_GFX_BASE
    lda.l !anim_set_table,x
    sta.b !ACTOR_ANIM_TABLE
    clc
    adc.b !ACTOR_ANIM
    tay
    lda.w !anim_data,y
    clc
    adc.b !ACTOR_ANIM_TABLE
    tay
    lda.w !anim_data,y
    tax
    xba
    and.w #$00FF
    sep.b #$30
    sta.b !ACTOR_LOOP_BACK
    txa
    sta.b !ACTOR_FRAME_TIME
    lda.b !ACTOR_PICTURE
    jsr.w AnimFrameLookup
    sep.b #$30
    lda.b #$FF
    sta.b !ACTOR_REDRAW
    plb
    rtl

; @routine 81:912B AnimStep
; WHAT IT DOES: Plays an actor's animation on by one frame of time: when
; the frame's time runs out, on to the next frame, or back the
; animation's step at its end.
; HOW: The time steps down; at 0 the frame steps on and AnimFrameLookup
; gives its time; a time of 0 is the end: the frame steps back by the
; end's count and is looked up again. The new time goes in the record.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL.
; Exit: time left: M and X 8-bit, N and Z from it, the rest unchanged.
; Else M and X 8-bit; A = the new time; X, Y as AnimFrameLookup leaves
; them (8-bit); DB unchanged, N and Z from it.
%at($81912B)
AnimStep:
    sep.b #$30
    dec.b !ACTOR_FRAME_TIME
    bne .done
    phb
    lda.b #!anim_data>>16
    pha
    plb
    inc.b !ACTOR_FRAME
    jsr.w AnimFrameLookup
    and.w #$00FF
    bne .time
    txa
    xba
    and.w #$00FF
    sep.b #$20
    sec
    sbc.b !ACTOR_LOOP_BACK
    sta.b !ACTOR_FRAME
    jsr.w AnimFrameLookup
    and.w #$00FF
.time:
    sep.b #$30
    sta.b !ACTOR_FRAME_TIME
    plb
.done:
    rtl

; @routine 81:9158 AnimFrameLookup
; WHAT IT DOES: Looks up an actor's current frame: its time and its
; picture, and the picture's graphics.
; HOW: The animation's frame list from the set's table, the frame's word
; in it; its high byte is the picture, whose graphics come from the
; picture list, offset by the set's graphics base.
; Entry: any widths; D = the actor's record; DB = $7E. Exit: M 16-bit,
; X 16-bit; A = X = the frame's word (the time low, the picture high); Y =
; its offset in the picture list; N and Z from it; C from the last add;
; $000C = the frame list's offset.
AnimFrameLookup:
    rep.b #$30
    lda.b !ACTOR_ANIM
    and.w #$00FF
    clc
    adc.b !ACTOR_ANIM_TABLE
    tay
    lda.w !anim_data,y
    sta.w !scratch
    lda.b !ACTOR_FRAME
    and.w #$00FF
    asl a
    clc
    adc.w !scratch
    clc
    adc.b !ACTOR_ANIM_TABLE
    tay
    lda.w !anim_data,y
    tax
    xba
    and.w #$00FF
    sep.b #$20
    sta.b !ACTOR_PICTURE
    rep.b #$20
    asl a
    clc
    adc.b !ACTOR_GFX_BASE
    tay
    lda.w !picture_list,y
    clc
    adc.w #$2000
    clc
    adc.b !ACTOR_GFX_BASE
    sta.b !ACTOR_PICTURE_GFX
    txa
    rts
