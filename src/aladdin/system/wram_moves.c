/*
 * wram_moves.c: block moves within bank $7E queued for the NMI to make
 * (a screen effect's HDMA table copied into place in the vertical blank).
 */
#include "machine.h"
#include "native.h"
#include "cpu_ops.h"
#include "ops65.h"
void abi_WramMovesRun(void);
#include "actor.h"
#include "hw.h"
#include "wram.h"
#include "wram_player.h"
#include "wram_state.h"
#include "wram_video.h"

/* Copies within bank $7E queued for the NMI: the queue's length in bytes,
   then 6 bytes each: source, destination, length less one (words). */
#define WRAM_MOVES_LEN 0x1705u
#define WRAM_MOVE_FROM 0x1707u
#define WRAM_MOVE_TO 0x1709u
#define WRAM_MOVE_SIZE 0x170Bu

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
    uint8_t len = rd8(dba(WRAM_MOVES_LEN));

    a8_load(len);
    if (!len)
        return;
    op_rep(P_X);
    /* from the last entry back to the first */
    for (;;) {
        flag_set(P_C, 1);
        len = op_sbc8(rd8(dba(WRAM_MOVES_LEN)), 6, &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | len);
        wr8(dba(WRAM_MOVES_LEN), len);
        if (len & 0x80)
            break;
        phb();
        cpu.y = rd16(dba(WRAM_MOVES_LEN));
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)rd16(dba(WRAM_MOVE_SIZE) + cpu.y));
        cpu.x = rd16(dba(WRAM_MOVE_TO) + cpu.y);
        phx16();                          /* the destination, by the stack */
        cpu.x = rd16(dba(WRAM_MOVE_FROM) + cpu.y);
        ply16();
        mvn(0x7E, 0x7E);                  /* the copy: A + 1 bytes */
        plb();
    }
    op_sep(P_X);
    wr8(dba(WRAM_MOVES_LEN), 0);
}
