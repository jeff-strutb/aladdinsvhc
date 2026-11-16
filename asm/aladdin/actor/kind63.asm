; kind63.asm: actor kind $63's per-frame work while it waits.
; C: src/aladdin/actor/kind63.c.

incsrc "../include/macros.inc"
incsrc "../include/actor.inc"

arch 65816

!KIND63_HIT = $43               ; nonzero while it shows being hit: no shot test then
!KIND63_GONE = $47              ; counted up each frame it is out of range

; @routine 84:81FC Kind63Work
; WHAT IT DOES: What actors of kind $63 do each frame they wait: unless
; already hit, a shot from the second pool turns it to face Aladdin and
; hits it; Aladdin is hurt by it or stomps it; it animates and shows its
; own picture; and out of range by box $28 it ends, its $47 counted up.
; HOW: KIND63_HIT zero: ActorShotHitFace. Then ActorStompOrHurtLong,
; AnimStep, ActorPictureSend, ActorFreeOutOfRangeLong with X = $28; C
; set (out of range): KIND63_GONE up by one.
; Entry: M 8-bit, X 8-bit; D = the actor; called by JSL from the kind's
; stub ($83:924D) through the actor loop's wait table at $83:8F4F. Exit:
; M and X 8-bit; C as ActorFreeOutOfRangeLong left it; in range the
; registers it left, else N and Z from the count.
%at($8481FC)
Kind63Work:
    lda.b !KIND63_HIT
    bne .tested
    jsr.w ActorShotHitFace
.tested:
    jsl.l ActorStompOrHurtLong
    jsl.l AnimStep
    jsl.l ActorPictureSend
    ldx.b #$28
    jsl.l ActorFreeOutOfRangeLong
    bcc .done
    inc.b !KIND63_GONE
.done:
    rtl
