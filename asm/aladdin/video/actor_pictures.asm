; actor_pictures.asm: sending an actor's new picture to its tile slot in
; VRAM when its animation changes it.
; C: src/aladdin/video/actor_pictures.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"

arch 65816

!ACTOR_PICTUREs_at = $29        ; the actor's picture list (direct-page pointer, word)

; @routine 81:949E ActorPictureSend
; WHAT IT DOES: When the actor shows its own picture (bit 6 of $09) and
; the picture has changed, queues its graphics for VRAM.
; HOW: Bit 6 clear: nothing. Else ActorPictureQueue.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
; ActorPictureQueue; nothing to send: A low byte = 0, Z set.
%at($819491)
; ActorPictureQueue's way out after queuing, and both routines' way out
; with nothing to send.
ActorPictureDone:
    sep.b #$10
ActorPictureNone:
    rtl
; ActorPictureSend for kind 4's set-up index only (an entry no script
; takes).
ActorPictureSendKind4:
    ldx.b !actor_06
    lda.l !kind_setup_of,x
    cmp.b #4
    bne ActorPictureNone
ActorPictureSend:
    lda.b !actor_09
    and.b #$40
    beq ActorPictureNone


; @routine 81:94A4 ActorPictureQueue
; WHAT IT DOES: Queues the actor's picture, when it has changed, for the
; NMI to write to its tile slot in VRAM.
; HOW: The picture against the one last sent; a new one is kept and, from
; the actor's picture list, its parts: each part a VRAM queue record (the
; slot's VRAM address from the sprite attributes for the first, the last
; part's address plus the part's step after; source, bank and size from
; the part list), until the $FF that ends the list.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSL. Exit:
; unchanged picture: Z set, A low byte = the picture. Queued: M 8-bit,
; X 8-bit; N, Z from the last compare (Z set).
%at($8194A4)
ActorPictureQueue:
    lda.b !ACTOR_PICTURE
    cmp.b !ACTOR_REDRAW
    beq ActorPictureNone
    sta.b !ACTOR_REDRAW
    asl a
    sta.w !scratch
    stz.w !scratch+1
    lda.w !vram_queue_count
    asl a
    asl a
    asl a
    tax
    rep.b #$30
    lda.w #$0000
    tay
    lda.b (!ACTOR_PICTUREs_at),y
    clc
    adc.w !scratch
    tay
    lda.w 0,y
    tay
    lda.b !actor_look
    and.w #$01FF
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w #$6000
    sta.w !vram_queue,x
.l94DB:
    phx
    lda.b !actor_06
    and.w #$00FF
    asl a
    tax
    plx
    clc
    lda.w 0,y
    sta.w !vram_queue+2,x
    iny
    iny
    lda.w 0,y
    sta.w !vram_queue+4,x
    iny
    iny
    sep.b #$20
    lda.w 0,y
    sta.w !vram_queue+6,x
    iny
    lda.b #$80
    sta.w !vram_queue+7,x
    inc.w !vram_queue_count
    lda.w 0,y
    cmp.b #$FF
    beq ActorPictureDone
    rep.b #$20
    clc
    txa
    adc.w #$0008
    tax
    clc
    lda.w 0,y
    adc.w !oam_next,x
    sta.w !vram_queue,x
    iny
    iny
    bra .l94DB
    sep.b #$30
    rtl
    rtl
