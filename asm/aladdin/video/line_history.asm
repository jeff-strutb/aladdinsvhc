; line_history.asm: a 32-word list at $7E:DFC0 of an actor's past heights,
; newest first, sent to VRAM as upload 11.
; C: src/aladdin/video/line_history.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/wram_video.inc"
incsrc "../include/actor.inc"

arch 65816

!LINE_HISTORY = $7EDFC0
!LINE_LAST = $18CD              ; the newest height, less 256 (word)
!UPLOAD_LINE_HISTORY = $0B

; @routine 84:8EBD ActorYHistoryPush
; WHAT IT DOES: Pushes the actor's height into the list: the older 31
; entries move down one, and the new head is its y pixel less 256, in 9
; bits, with bit 14 set; the value less 256 is kept whole too; the list's
; upload is asked for.
; HOW: Word moves from the end down, with DB $7E and 8-bit X; then the new
; entry with 16-bit A.
; Entry: M 8-bit, X 8-bit; D = the actor; DB a bank whose $0000-$1FFF is
; work RAM. Exit: as the C states.
%at($848EBD)
ActorYHistoryPush:
    phb
    lda.b #!LINE_HISTORY>>16
    pha
    plb
    rep.b #$20
    ldx.b #62
.move:
    lda.w !LINE_HISTORY-2&65535,x
    sta.w !LINE_HISTORY&65535,x
    dex
    dex
    bne .move
    sep.b #$20
    plb
    rep.b #$20
    sec
    lda.b !actor_y+1
    sbc.w #$0100
    sta.w !LINE_LAST
    and.w #$01FF
    ora.w #$4000
    sta.l !LINE_HISTORY
    sep.b #$20
    lda.b #!UPLOAD_LINE_HISTORY
    sta.w !vram_upload_number
    rts
