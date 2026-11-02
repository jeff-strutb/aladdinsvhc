; wram_moves.asm: block moves within bank $7E queued for the NMI to make
; (a screen effect's HDMA table copied into place in the vertical blank).
; C: src/aladdin/system/wram_moves.c.

incsrc "../include/macros.inc"

arch 65816

!wram_moves_length = $1705      ; bytes of queued moves (6 per move); 0: none
!wram_move_from = $1707         ; per move: the source (word)
!wram_move_to = $1709           ; the destination (word)
!wram_move_count = $170B        ; the byte count less 1 (its low byte is used)
!WRAM_MOVE_SIZE = 6
!WRAM_BANK = $7E               ; the bank the moves copy within

; @routine 80:8721 WramMovesRun
; WHAT IT DOES: Makes the block moves queued at $1705, last first, each a
; copy within bank $7E, and empties the queue.
; HOW: While the length less 6 stays non-negative: the move's count less
; 1 into A's low byte, its source into X and destination into Y, MVN
; $7E,$7E (DB kept); then the length cleared.
; Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
; by JSR from the NMI. Exit: M 8-bit, X 8-bit; queue empty.
%at($808721)
WramMovesRun:
    lda.w !wram_moves_length
    beq .done
    rep.b #$10
.move:
    sec
    lda.w !wram_moves_length
    sbc.b #!WRAM_MOVE_SIZE
    sta.w !wram_moves_length
    bmi .empty
    phb
    ldy.w !wram_moves_length
    ldx.w !wram_move_count,y
    txa
    ldx.w !wram_move_to,y
    phx
    ldx.w !wram_move_from,y
    ply
    mvn !WRAM_BANK,!WRAM_BANK
    plb
    bra .move
.empty:
    sep.b #$10
    stz.w !wram_moves_length
.done:
    rts
