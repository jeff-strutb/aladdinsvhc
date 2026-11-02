; small_actor_helpers.asm: small actor routines: kind $89, which sets
; Aladdin's sprite priority by where he is; a waiting actor that sends out
; a kind $8E child every 8 frames; and a child of kind $3D started at a
; spawn point and kept in its parent's list. Drafted by cdraft.py; named
; and described by hand.
; C: src/aladdin/actor/small_actor_helpers.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"
incsrc "../include/actor_anim.inc"

arch 65816

!PLAYER_09 = $8E4  ; the player's byte at $09 (bits 4-5: his sprite priority)
!PRIORITY_BY_SIDE = $D7C8  ; his priority bits: left of x $0934, then right of it (in the bank DB names)
!V_032D = $32D  ; the high byte of the word at $032C
!CHILD_COUNT = $44  ; children started so far; the list of their records is at $40

; @routine 84:8F9C Kind89PlayerPriority
; @actor
; WHAT IT DOES: Actor kind $89: each frame sets Aladdin's sprite priority
; (bits 4-5 of his $09) from PRIORITY_BY_SIDE by whether he is left of x
; $0934 or not, for good.
; HOW: Each frame (BRK first): X = 0, or 1 when his x pixels are $0934 or
; more; PLAYER_09 and $CF, OR PRIORITY_BY_SIDE at X.
; Entry: M 8-bit, X 8-bit; D = the actor's record; DB = $84; reached by
; the actor loop's jump for kind $89 in state $0C (through the stub at
; $83:9326). Exit: never returns.
%at($848F9C)
Kind89PlayerPriority:
.l8F9C:
    brk.b #$00
    ldx.b #$00
    rep.b #$20
    lda.w !player+!actor_x+1
    cmp.w #$0934
    sep.b #$20
    bcc .l8FAD
    inx
.l8FAD:
    lda.w !PLAYER_09
    and.b #$CF
    ora.w !PRIORITY_BY_SIDE,x
    sta.w !PLAYER_09
    bra .l8F9C


; @routine 84:9B4D SenderWait8E
; WHAT IT DOES: What a sending actor runs each frame it waits: its
; animation on and its picture sent; when the low three bits of V_032D are
; 0, a child of kind $8E ($07 = 1) at its spawn point $80.
; HOW: AnimStep, ActorPictureSend; V_032D and 7 zero: $21 = $80, $07 = 1,
; ActorCreateChild8 with kind $8E.
; Entry: M 8-bit, X 8-bit; D = the actor's record; called by JSL from the
; actor loop's stub at $83:9348. Exit: as ActorCreateChild8 or
; ActorPictureSend.
%at($849B4D)
SenderWait8E:
    jsl.l AnimStep
    jsl.l ActorPictureSend
    lda.w !V_032D
    and.b #$07
    bne .l9B6A
    lda.b #$80
    sta.b !actor_spawn_at
    lda.b #$01
    sta.b !actor_07
    lda.b #$8E
    jsl.l ActorCreateChild8
.l9B6A:
    rtl


; @routine 83:C4EC ChildStartListed
; WHAT IT DOES: Starts a child of kind $3D at spawn point X with $07 = Y,
; and keeps its record in the actor's list of children ($40 + 2 * $44),
; counting $44 up.
; HOW: $21 = X, $07 = Y; ActorCreateChild $3D; the record (X) to Y; at $40
; + $44 * 2 (direct page, X-indexed) the record (16-bit); M and X 8-bit;
; $44 up.
; Entry: M 8-bit, X 8-bit; X = the spawn point; Y = the child's $07; D =
; the actor's record; DB = $83; called by JSR (83:C32E, 83:C358, 83:C35F).
; Exit: M and X 8-bit; X = $44 * 2 before the count (low byte); Y = the
; record's low byte.
%at($83C4EC)
ChildStartListed:
    stx.b !actor_spawn_at
    sty.b !actor_07
    lda.b #$3D
    jsl.l ActorCreateChild
    txy
    rep.b #$20
    lda.b !CHILD_COUNT
    and.w #$00FF
    asl a
    tax
    sty.b !actor_40,x
    sep.b #$30
    inc.b !CHILD_COUNT
    rts
