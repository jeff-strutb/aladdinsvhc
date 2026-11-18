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
!KIND_GFX = $7FE500             ; per kind: where its graphics went in bank $7F (word)

; @routine 81:949E ActorPictureSend
; WHAT IT DOES: When the actor shows its own picture (bit 6 of $09) and
; the picture has changed, queues its graphics for VRAM.
; HOW: Bit 6 clear: nothing. Else ActorPictureQueue.
; Entry: M 8-bit; D = the actor; DB = $81; called by JSL. Exit: as
; ActorPictureQueue; nothing to send: A low byte = 0, Z set.
; @routine 81:946F Compare2FStep26
; WHAT IT DOES: With A equal to the direct page's $2F, X = its $26 less
; one. Nothing in the game calls it.
; HOW: CMP $0A:00C1 (long: work RAM's $00C1; its result is not used), CMP
; $2F: not equal, out; else LDX $26, DEX. Bytes or words as M and X are
; set.
; Entry: any widths; D, A as above; called by JSL. Exit: not equal: N, Z,
; C from CMP $2F; else X = $26 less one, N and Z from it, C from the CMP.
%at($81946F)
Compare2FStep26:
    cmp.l $0A00C1
    cmp.b $2F
    bne .out
    ldx.b $26
    dex
.out:
    rtl

; @routine 81:947B ActorPictureQueueAltOnce
; WHAT IT DOES: Once for the actor (while its byte at $0843+X is 0), when
; it shows its own picture (bit 6 of $09) and the picture in its $2F is not
; the one last sent, queues that picture's parts. Nothing in the game calls
; it.
; HOW: $0843+X nonzero, bit 6 of $09 clear, or $2F = ACTOR_REDRAW: out
; (ActorPictureNone's RTL). Else $0843+X counted up and, with A = $2F, on
; at ActorPictureQueueNew.
; Entry: M 8-bit, X 8-bit; D = the actor; X = its index into $0843; DB =
; $81; called by JSL. Exit: out: A = the byte that ended it, N, Z from it
; (CMP for $2F); queued: as ActorPictureQueueParts.
%at($81947B)
ActorPictureQueueAltOnce:
    lda.w $0843,x
    bne ActorPictureNone
    lda.b !actor_09
    and.b #$40
    beq ActorPictureNone
    lda.b $2F
    cmp.b !ACTOR_REDRAW
    beq ActorPictureNone
    inc.w $0843,x
    bra ActorPictureQueueNew

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
; @routine 81:94AA ActorPictureQueueNew
; WHAT IT DOES: The rest of ActorPictureQueue once the picture is new:
; picture A kept as the one sent, and its parts queued.
; HOW: A to ACTOR_REDRAW; A * 2 to scratch $0C (word); on into
; ActorPictureQueueParts.
; Entry: M 8-bit, X 8-bit; A low byte = the picture; D = the actor; DB =
; $81; reached from ActorPictureQueue and by a branch from
; ActorPictureQueueAltOnce. Exit: as ActorPictureQueueParts.
ActorPictureQueueNew:
    sta.b !ACTOR_REDRAW
    asl a
    sta.w !scratch
    stz.w !scratch+1
; @routine 81:94B3 ActorPictureQueueParts
; WHAT IT DOES: Queues the parts of the picture whose number * 2 is in
; scratch $0C, from the actor's picture list, as VRAM queue records.
; HOW: As ActorPictureQueue's HOW, from the queue's end (vram_queue_count
; * 8): the first part to the slot's VRAM address from the sprite
; attributes, each after it to the last one's address plus its step, until
; the $FF that ends the list.
; Entry: M 8-bit, X 8-bit; scratch $0C = the picture * 2 (word); D = the
; actor; DB = $81; reached from ActorPictureQueueNew and by a jump from
; ActorPictureQueueAltParts. Exit: M 8-bit, X 8-bit; N, Z from the last
; compare (Z set).
ActorPictureQueueParts:
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
; ActorPictureSendFromKind's way out after queuing, and with nothing to
; send.
ActorPictureFromKindDone:
    sep.b #$30
    rtl
ActorPictureFromKindNone:
    rtl

; @routine 81:9527 ActorPictureSendFromKind
; WHAT IT DOES: As ActorPictureSend, for an actor whose graphics were
; unpacked to bank $7F for its kind: when it shows its own picture (bit 6
; of $09) and the picture has changed, queues the picture's graphics for
; VRAM, their source the part's offset from where its kind's graphics
; went (KIND_GFX).
; HOW: Bit 6 clear, or the picture the one last sent: nothing (the RTL
; before its entry). Else the picture kept and, from the actor's picture
; list, its first part as a VRAM queue record: the slot's VRAM address
; from the sprite attributes, the source KIND_GFX by its kind plus the
; part's offset, then the part's bank and size, VMAIN $80; the queue
; counted up. The $FF that ends the list: M and X 8-bit and out. Else
; ($81:959A) the next record, its address the last one's plus the part's
; step, and on from the next part's source until the $FF.
; Entry: M 8-bit, X 8-bit; D = the actor; DB = $81; called by JSL. Exit:
; nothing to send: A low byte = 0 or the picture, Z set. Queued: M
; 8-bit, X 8-bit; N, Z from the last compare (Z set).
%at($819527)
ActorPictureSendFromKind:
    lda.b !actor_09
    and.b #$40
    beq ActorPictureFromKindNone
    lda.b !ACTOR_PICTURE
    cmp.b !ACTOR_REDRAW
    beq ActorPictureFromKindNone
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
.part:
    phx
    lda.b !actor_06
    and.w #$00FF
    asl a
    tax
    lda.l !KIND_GFX,x
    plx
    clc
    adc.w 0,y
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
    beq ActorPictureFromKindDone
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
    bra .part

; @routine 81:95B0 ActorPictureQueueAltParts
; WHAT IT DOES: Queues the parts of the picture whose number * 2 is in
; scratch $0C, as ActorPictureQueueParts. Nothing in the game calls it.
; HOW: LDA $2F (its value is not used: the next step loads A), then on at
; ActorPictureQueueParts.
; Entry: M 8-bit, X 8-bit; as ActorPictureQueueParts; called by JSL. Exit:
; as ActorPictureQueueParts.
%at($8195B0)
ActorPictureQueueAltParts:
    lda.b $2F
    jmp.w ActorPictureQueueParts
