; chain_piece.asm: what the pieces of the nine-piece chain (kinds $74 and
; $75, behind its head of kind $73) do each frame they wait.
; C: src/aladdin/actor/chain_piece.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/actor.inc"

arch 65816

!CHAIN_HEAD = $1B14             ; the head's record (word; child_chain.asm)
!CHAIN_UNTOUCHABLE = $1B2C      ; nonzero: no touch tests (the head's K73_UNTOUCHABLE)
!V_032D = $032D                 ; the high byte of the word at $032C
!STATE_GONE = $8C               ; the head's state once it is going (and so the pieces')
!PIECE_RESUME = $35             ; word: where the piece resumes, kept from actor_03

; @routine 84:8A78 ChainPieceWork
; WHAT IT DOES: What the chain's pieces (kinds $74 and $75) do each frame
; they wait: animate, follow their path and the pieces after them, and,
; unless the chain is untouchable, hurt Aladdin on touch on odd counts of
; V_032D or turn his shots back on even ones; when the head goes into
; state $8C the piece goes with it; its $08 bits 4 and 7 follow the
; head's.
; HOW: AnimStep, ActorPathStep, ActorsFollowIf8; CHAIN_UNTOUCHABLE zero:
; V_032D bit 0 set ActorHurtsPlayerOnTouchLong, clear
; ActorTurnsShotBackLong. With X 16-bit = CHAIN_HEAD: the head's state
; $8C: the piece's state $8C, PIECE_RESUME = its $03 word, $09 bit 7
; cleared (ASL, LSR). SCRATCH = the head's $08 AND $90; the piece's $08 =
; its $08 AND $6F OR SCRATCH.
; Entry: M 8-bit, X 8-bit; D = the piece's record; DB a bank whose
; $0000-$1FFF is work RAM; called by JSL from the kinds' stubs ($83:926B,
; $83:9278) through the actor loop's wait table. Exit: M 8-bit, X 8-bit;
; A = the new $08, N and Z from it; X = the low byte of CHAIN_HEAD's
; record; Y as the touch tests left it, or the low byte of $03 when the
; head went; C clear when the head went, else from comparing $8C with the
; head's state.
%at($848A78)
ChainPieceWork:
    jsl.l AnimStep
    jsr.w ActorPathStep
    jsr.w ActorsFollowIf8
    lda.w !CHAIN_UNTOUCHABLE
    bne .head
    lda.w !V_032D
    and.b #$01
    beq .shots
    jsl.l ActorHurtsPlayerOnTouchLong
    bra .head
.shots:
    jsl.l ActorTurnsShotBackLong
.head:
    rep.b #$10
    ldx.w !CHAIN_HEAD
    lda.b #!STATE_GONE
    cmp.w !actor_state,x
    bne .bits
    sta.b !actor_state
    ldy.b !actor_03
    sty.b !PIECE_RESUME
    asl.b !actor_09
    lsr.b !actor_09
.bits:
    lda.w !actor_08,x
    and.b #$90
    sta.w !scratch
    lda.b !actor_08
    and.b #$6F
    ora.w !scratch
    sta.b !actor_08
    sep.b #$10
    rtl
