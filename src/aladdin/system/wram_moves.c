/*
 * wram_moves.c: block moves within bank $7E queued for the NMI to make
 * (a screen effect's HDMA table copied into place in the vertical blank).
 */
#include "machine.h"
#include "ops65.h"
void abi_WramMovesRun(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/*
 * @routine 80:8721 WramMovesRun
 * WHAT IT DOES: Makes the block moves queued at $1705, last first, each a
 * copy within bank $7E, and empties the queue.
 * HOW: While the length less 6 stays non-negative: the move's count less
 * 1 into A's low byte, its source into X and destination into Y, MVN
 * $7E,$7E (DB kept); then the length cleared.
 * Entry: M 8-bit, X 8-bit; DB a bank whose $0000-$1FFF is work RAM; called
 * by JSR from the NMI. Exit: M 8-bit, X 8-bit; queue empty.
 */
void abi_WramMovesRun(void)
{
    lda8(ea_abs(0x1705));  /* 808721 lda */
    if (IF_Z) goto L_80874E;  /* 808724 beq */
    op_rep(0x10);  /* 808726 rep */
L_808728:
    sec();  /* 808728 sec */
    lda8(ea_abs(0x1705));  /* 808729 lda */
    sbc8_v(0x06);  /* 80872C sbc */
    sta8(ea_abs(0x1705));  /* 80872E sta */
    if (IF_N) goto L_808749;  /* 808731 bmi */
    phb();  /* 808733 phb */
    ldy16(ea_abs(0x1705));  /* 808734 ldy */
    ldx16(ea_absy(0x170B));  /* 808737 ldx */
    txa8();  /* 80873A txa */
    ldx16(ea_absy(0x1709));  /* 80873B ldx */
    phx16();  /* 80873E phx */
    ldx16(ea_absy(0x1707));  /* 80873F ldx */
    ply16();  /* 808742 ply */
    mvn(0x7E, 0x7E);  /* 808743 mvn */
    plb();  /* 808746 plb */
    goto L_808728;  /* 808747 bra */
L_808749:
    op_sep(0x10);  /* 808749 sep */
    stz8(ea_abs(0x1705));  /* 80874B stz */
L_80874E:
    return;  /* 80874E rts */
}
